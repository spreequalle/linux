/* linux/drivers/mmc/host/sdhci-xenon-pltfm.c
 * Driver for Marvell SOCP Xenon SDHC as a platform device
 *
 * Author:	Hu Ziji <huziji@marvell.com>
 * Date:		2015-5-29
 *
 *  Copyright (C) 2014 Marvell, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

/* Inspired by Jisheng Zhang <jszhang@marvell.com>
 *  Special thanks to Video BG4 project team.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/card.h>
#include <linux/module.h>
#include <linux/of.h>

#include "sdhci-pltfm.h"
#include "sdhci-xenon-core.h"

struct sdhci_xenon_priv {
	/* The three fields in below records the current
	 * setting of Xenon SDHC.
	 * Driver will call a Sampling Fixed Delay Adjustment
	 * if any setting is changed.
	 */
	unsigned char	bus_width;
	unsigned char	timing;
	unsigned char	tuning_count;
	unsigned int	clock;
	u32		phy_pad_ctrl2;
	struct clk	*axi_clk;
};

static const struct of_device_id sdhci_xenon_dt_ids[] = {
	{ .compatible = "marvell,xenon-sdhci",},
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_xenon_dt_ids);

static void xenon_platform_init(struct sdhci_host *host)
{
	xenon_set_acg(host, false);
}

static int xenon_pltfm_delay_adj(struct sdhci_host *host,
	struct mmc_ios *ios)
{
	struct mmc_host *mmc = host->mmc;
	struct mmc_card *card;
	int ret;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct card_cntx *cntx;

	if (!host->clock)
		return 0;

	if (ios->timing != priv->timing)
		sdhci_xenon_phy_set(host, ios->timing, priv->phy_pad_ctrl2);

	/* Legacy mode is a special case */
	if (ios->timing == MMC_TIMING_LEGACY) {
		priv->timing = ios->timing;
		return 0;
	}

	/* The timing, frequency or bus width is changed,
	 * better to set eMMC PHY based on current setting
	 * and adjust Xenon SDHC delay.
	 */
	if ((host->clock == priv->clock) &&
		(ios->bus_width == priv->bus_width) &&
		(ios->timing == priv->timing))
		return 0;

	/* Update the record */
	priv->clock = host->clock;
	priv->bus_width = ios->bus_width;
	priv->timing = ios->timing;

	cntx = (struct card_cntx *)mmc->slot.handler_priv;
	card = cntx->delay_adjust_card;
	if (unlikely(card == NULL))
		BUG();

	ret = xenon_hs_delay_adj(mmc, card);
	return ret;
}

static struct sdhci_ops sdhci_xenon_ops = {
	.set_clock		= sdhci_set_clock,
	.set_bus_width		= sdhci_set_bus_width,
	.reset			= sdhci_xenon_reset,
	.set_uhs_signaling	= sdhci_set_uhs_signaling,
	.platform_init		= xenon_platform_init,
	.get_max_clock		= sdhci_pltfm_clk_get_max_clock,
	.delay_adj		= xenon_pltfm_delay_adj,
	.voltage_switch		= sdhci_xenon_emmc_voltage_switch,
};

static struct sdhci_pltfm_data sdhci_xenon_pdata = {
	.ops = &sdhci_xenon_ops,
	.quirks = SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC |
			SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12 |
			SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER,
	.quirks2 = SDHCI_QUIRK2_XENON_HACK,
	/* Add SOC specific quirks in the above .quirks, .quirks2
	 * fields.
	 */
};

static int xenon_probe_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct mmc_host *mmc = host->mmc;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int err;
	u32 val;
	u32 tuning_count;

	/* Standard MMC property */
	err = mmc_of_parse(mmc);
	if (err)
		return err;

	/* Statndard SDHCI property */
	sdhci_get_of_property(pdev);

	/* Xenon Specific property:
	 * emmc: this is a emmc slot.
	 *	Actually, whether current slot is for emmc can be
	 *	got from SDHC_SYS_CFG_INFO register. However, some SOC might
	 *	not implemente the slot type information. In such a case,
	 *	it is necessary to explicity indicate the emmc type.
	 * slotno: the index of slot. Refer to SDHC_SYS_CFG_INFO register
	 */
	if (of_get_property(np, "xenon,emmc", NULL))
		host->quirks2 |= SDHCI_QUIRK2_XENON_EMMC_SLOT;

	if (!of_property_read_u32(np, "xenon,slotno", &val))
		mmc->slotno = val & 0xff;

	if (!of_property_read_u32(np, "xenon,phy-pad-ctrl2", &val))
		priv->phy_pad_ctrl2 = val;

	if (!of_property_read_u32(np, "xenon,tuning-count", &tuning_count))
		priv->tuning_count = tuning_count & 0xf;
	else
		priv->tuning_count = DEF_TUNING_COUNT;

	return 0;
}

static bool __check_slot_type_emmc(struct sdhci_host *host,
		unsigned char slotno)
{
	u32 reg;
	unsigned int emmc_slot;
	unsigned int sd_slot;

	if (host->quirks2 & SDHCI_QUIRK2_XENON_EMMC_SLOT)
		return true;

	/* Read the eMMC slot type field from SYS_CFG_INFO register
	  * If a bit is set, this slot supports eMMC */
	reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
	emmc_slot = reg >> SLOT_TYPE_EMMC_SHIFT;
	emmc_slot &= SLOT_TYPE_EMMC_MASK;
	emmc_slot &= (1 << slotno);

	/* This slot doesn't support in SDHC IP design */
	if (!emmc_slot)
		return false;

	/* Read the SD/SDIO/MMC slot type field from SYS_CFG_INFO register
	 * if that bit is NOT set, this slot can only support eMMC
	 */
	sd_slot = reg >> SLOT_TYPE_SD_SDIO_MMC_SHIFT;
	sd_slot &= SLOT_TYPE_SD_SDIO_MMC_MASK;
	emmc_slot &= sd_slot;
	/* ONLY support emmc */
	if (!emmc_slot)
		return true;

	return false;
}

static void xenon_add_host_fixup(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

	xenon_set_tuning_mode(host);
	xenon_set_tuning_count(host, priv->tuning_count);
}

static int xenon_slot_probe(struct sdhci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	u8 slotno = mmc->slotno;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct card_cntx *cntx;

	cntx = devm_kzalloc(mmc_dev(mmc), sizeof(*cntx), GFP_KERNEL);
	if (!cntx)
		return -ENOMEM;

	mmc->slot.handler_priv = cntx;

	/* Enable slot */
	xenon_enable_slot(host, slotno);

	/* Enable ACG */
	xenon_set_acg(host, true);

	/* Enable Parallel Transfer Mode */
	xenon_enable_slot_parallel_tran(host, slotno);

	/* Do eMMC setup if it is an eMMC slot */
	if (__check_slot_type_emmc(host, slotno)) {
		/* Mark the flag which requires Xenon eMMC-specific
		 * operations, such as voltage switch
		 */
		mmc->caps |= MMC_CAP_BUS_WIDTH_TEST | MMC_CAP_NONREMOVABLE;
		mmc->caps2 |= MMC_CAP2_HC_ERASE_SZ | MMC_CAP2_PACKED_CMD;
	}

	/* Set tuning functionality of this slot */
	xenon_slot_tuning_setup(host);

	priv->timing = MMC_TIMING_FAKE;

	return 0;
}

static void xenon_slot_remove(struct sdhci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	u8 slotno = mmc->slotno;

	/* disable slot */
	xenon_disable_slot(host, slotno);
}

static int sdhci_xenon_probe(struct platform_device *pdev)
{
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_host *host;
	struct clk *clk, *axi_clk;
	struct sdhci_xenon_priv *priv;
	int err;

	host = sdhci_pltfm_init(pdev, &sdhci_xenon_pdata,
		sizeof(struct sdhci_xenon_priv));
	if (IS_ERR(host))
		return PTR_ERR(host);

	pltfm_host = sdhci_priv(host);
	priv = sdhci_pltfm_priv(pltfm_host);

	clk = devm_clk_get(&pdev->dev, "core");
	if (IS_ERR(clk)) {
		pr_err("%s: Failed to setup input clk.\n",
			mmc_hostname(host->mmc));
		err = PTR_ERR(clk);
		goto free_pltfm;
	}
	clk_prepare_enable(clk);
	pltfm_host->clk = clk;

	/* Some SOCs require additional clock to
	 * manage AXI bus clock.
	 * It is optional.
	 */
	axi_clk = devm_clk_get(&pdev->dev, "axi");
	if (!IS_ERR(axi_clk)) {
		clk_prepare_enable(axi_clk);
		priv->axi_clk = axi_clk;
	}

	err = xenon_probe_dt(pdev);
	if (err)
		goto err_clk;

	err = xenon_slot_probe(host);
	if (err)
		goto err_clk;

	err = sdhci_add_host(host);
	if (err)
		goto remove_slot;

	xenon_add_host_fixup(host);

	return 0;

remove_slot:
	xenon_slot_remove(host);
err_clk:
	clk_disable_unprepare(pltfm_host->clk);
	if (!IS_ERR(axi_clk))
		clk_disable_unprepare(axi_clk);
free_pltfm:
	sdhci_pltfm_free(pdev);
	return err;
}

static int sdhci_xenon_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int dead = (readl(host->ioaddr + SDHCI_INT_STATUS) == 0xffffffff);

	xenon_slot_remove(host);

	sdhci_remove_host(host, dead);

	clk_disable_unprepare(pltfm_host->clk);
	clk_disable_unprepare(priv->axi_clk);

	sdhci_pltfm_free(pdev);

	return 0;
}

static struct platform_driver sdhci_xenon_driver = {
	.driver	= {
		.name	= "mv-xenon-sdhci",
		.owner	= THIS_MODULE,
		.of_match_table = sdhci_xenon_dt_ids,
		.pm = SDHCI_PLTFM_PMOPS,
	},
	.probe	= sdhci_xenon_probe,
	.remove	= sdhci_xenon_remove,
};

module_platform_driver(sdhci_xenon_driver);

MODULE_DESCRIPTION("SDHCI platform driver for Marvell Xenon SDHC");
MODULE_AUTHOR("Hu Ziji <huziji@marvell.com>");
