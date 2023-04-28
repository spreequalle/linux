/* linux/drivers/mmc/host/sdhci-xenon-emmc-phy.c
 * eMMC PHY operations for Xenon SDHC when eMMC PHY is used.
 *
 * Author: Hu Ziji <huziji@marvell.com>
 * Date:		2014-12-4
 *
 *  Copyright (C) 2014 Marvell, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>

#include "../core/core.h"

#include "sdhci.h"
#include "sdhci-xenon-core.h"
#include "sdhci-xenon-emmc-phy.h"


int sdhci_xenon_emmc_phy_init(struct sdhci_host *host)
{
	unsigned int reg;
	u32 wait, clock;

	reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
	reg |= PHY_INITIALIZAION;
	sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);

	/* Add duration of FC_SYNC_RST */
	wait = ((reg >> FC_SYNC_RST_DURATION_SHIFT) &
			FC_SYNC_RST_DURATION_MASK);
	/* Add interval between FC_SYNC_EN and FC_SYNC_RST */
	wait += ((reg >> FC_SYNC_RST_EN_DURATION_SHIFT) &
			FC_SYNC_RST_EN_DURATION_MASK);
	/* Add duration of asserting FC_SYNC_EN */
	wait += ((reg >> FC_SYNC_EN_DURATION_SHIFT) &
			FC_SYNC_EN_DURATION_MASK);
	/* Add duration of waiting for PHY */
	wait += ((reg >> WAIT_CYCLE_BEFORE_USING_SHIFT) &
			WAIT_CYCLE_BEFORE_USING_MASK);
	/* According to Moyang, 4 addtional bus clock
	 * and 4 AXI bus clock are required
	 */
	wait += 8;
	/* left shift 20 bits */
	wait <<= 20;

	clock = host->clock;
	if (!clock)
		/* Use the possibly slowest bus frequency value */
		clock = 100000;
	/* get the wait time */
	wait /= clock;
	wait++;
	/* wait for host eMMC PHY init completes */
	udelay(wait);

	reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
	reg &= PHY_INITIALIZAION;
	if (reg) {
		pr_err("%s: eMMC PHY init cannot complete after %d us\n",
			mmc_hostname(host->mmc), wait);
		return -EIO;
	}

	return 0;
}

int xenon_set_sampl_fix_delay(struct sdhci_host *host, unsigned int delay,
		bool invert, bool phase)
{
	unsigned int reg;
	unsigned long flags;
	int ret = 0;
	u8 timeout;

	if (invert)
		invert = 0x1;
	else
		invert = 0x0;

	if (phase)
		phase = 0x1;
	else
		phase = 0x0;

	spin_lock_irqsave(&host->lock, flags);

	/* Setup Sampling fix delay */
	reg = sdhci_readl(host, SDHC_SLOT_OP_STATUS_CTRL);
	reg &= ~TUNING_PROG_FIXED_DELAY_MASK;
	reg |= delay & TUNING_PROG_FIXED_DELAY_MASK;
	sdhci_writel(host, reg, SDHC_SLOT_OP_STATUS_CTRL);

#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
	/* If phase = true, set 90 degree phase */
	reg &= ~DELAY_90_DEGREE_MASK_EMMC5;
	reg |= (phase << DELAY_90_DEGREE_SHIFT_EMMC5);
	sdhci_writel(host, reg, SDHC_SLOT_OP_STATUS_CTRL);
#endif

	/* Disable SDCLK */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~(SDHCI_CLOCK_CARD_EN | SDHCI_CLOCK_INT_EN);
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);

	udelay(200);

#ifndef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
	/* If phase = true, set 90 degree phase */
	reg = sdhci_readl(host, EMMC_PHY_FUNC_CONTROL);
	reg &= ~ASYNC_DDRMODE_MASK;
	reg |= (phase << ASYNC_DDRMODE_SHIFT);
	sdhci_writel(host, reg, EMMC_PHY_FUNC_CONTROL);
#endif

	/* Setup Inversion of Sampling edge */
	reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
	reg &= ~SAMPL_INV_QSP_PHASE_SELECT;
	reg |= (invert << SAMPL_INV_QSP_PHASE_SELECT_SHIFT);
	sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);

	/* Enable SD internal clock */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_INT_EN;
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);
	/* Wait max 20 ms */
	timeout = 20;
	while (!((reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL))
			& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			pr_err("%s: Internal clock never stabilised.\n",
					mmc_hostname(host->mmc));
			return -EIO;
		}
		timeout--;
		mdelay(1);
	}

	/* Enable SDCLK */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);

	udelay(200);

	/* Has to re-initialize eMMC PHY here to active PHY
	 * because later get status cmd will be issued.
	 */
	ret = sdhci_xenon_emmc_phy_init(host);

	spin_unlock_irqrestore(&host->lock, flags);

	return ret;
}

int xenon_set_strobe_fix_delay(struct sdhci_host *host, unsigned int delay)
{
	u32 reg;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&host->lock, flags);

	/* Setup strobe fix delay */
	reg = sdhci_readl(host, EMMC_PHY_DLL_CONTROL);
	reg &= ~(unsigned int)(DLL_DELAY_TEST_LOWER_MASK <<
			DLL_DELAY_TEST_LOWER_SHIFT);
	reg |= ((delay & DLL_DELAY_TEST_LOWER_MASK) <<
			DLL_DELAY_TEST_LOWER_SHIFT);
	reg |= DLL_BYPASS_EN;
	sdhci_writel(host, reg, EMMC_PHY_DLL_CONTROL);

	/* Set Data Strobe Pull down */
#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL);
	reg |= EMMC5_FC_QSP_PD;
	reg &= ~EMMC5_FC_QSP_PU;
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL);
#else
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL1);
	reg |= EMMC5_1_FC_QSP_PD;
	reg &= ~EMMC5_1_FC_QSP_PU;
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL1);
#endif

	/* Has to re-initialize eMMC PHY here to active PHY
	  * because later get status cmd will be issued.
	  */
	ret = sdhci_xenon_emmc_phy_init(host);

	spin_unlock_irqrestore(&host->lock, flags);

	return ret;
}

void xenon_emmc_phy_adj_strobe_delay(struct sdhci_host *host)
{
	u32 reg;
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);

	/* Enable DLL */
	reg = sdhci_readl(host, EMMC_PHY_DLL_CONTROL);
	reg |= (DLL_ENABLE | DLL_FAST_LOCK);

	/* Set Phase as 90 degree */
	reg &= ~((DLL_PHASE_MASK << DLL_PHSEL0_SHIFT) |
			(DLL_PHASE_MASK << DLL_PHSEL1_SHIFT));
	reg |= ((DLL_PHASE_90_DEGREE << DLL_PHSEL0_SHIFT) |
			(DLL_PHASE_90_DEGREE << DLL_PHSEL1_SHIFT));

	reg &= ~DLL_BYPASS_EN;
#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
	reg |= DLL_UPDATE_STROBE;
#else
	reg |= DLL_UPDATE;
	if (host->clock > 500000000)
		reg &= ~DLL_REFCLK_SEL;
#endif
	sdhci_writel(host, reg, EMMC_PHY_DLL_CONTROL);

	/* Set Data Strobe Pull down */
#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL);
	reg |= EMMC5_FC_QSP_PD;
	reg &= ~EMMC5_FC_QSP_PU;
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL);
#else
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL1);
	reg |= EMMC5_1_FC_QSP_PD;
	reg &= ~EMMC5_1_FC_QSP_PU;
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL1);
#endif

	spin_unlock_irqrestore(&host->lock, flags);
}


#define LOGIC_TIMING_VALUE	0x00aa8977

void sdhci_xenon_emmc_phy_set(struct sdhci_host *host,
		unsigned char timing, u32 ctrl2)
{
	unsigned int reg;
	struct mmc_card *card = host->mmc->card;

#ifdef CONFIG_XENON_DEBUG
	pr_debug("%s: eMMC PHY setting starts\n", mmc_hostname(host->mmc));
#endif

	/* Setup pad, set bit[28] and bits[26:24] */
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL);
	reg |= (FC_DQ_RECEN | FC_CMD_RECEN | FC_QSP_RECEN | OEN_QSN);
	/* According to latest spec, all FC_XX_RECEIVCE should
	  * be set as CMOS Type
	  */
	reg |= FC_ALL_CMOS_RECEIVER;
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL);

	/* Set CMD and DQ Pull Up */
#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL);
	reg |= (EMMC5_FC_CMD_PU | EMMC5_FC_DQ_PU);
	reg &= ~(EMMC5_FC_CMD_PD | EMMC5_FC_DQ_PD);
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL);
#else
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL1);
	reg |= (EMMC5_1_FC_CMD_PU | EMMC5_1_FC_DQ_PU);
	reg &= ~(EMMC5_1_FC_CMD_PD | EMMC5_1_FC_DQ_PD);
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL1);
#endif

	/* If timing belongs to high speed, set bit[17] of
	 * EMMC_PHY_TIMING_ADJUST register
	 */
	if ((timing == MMC_TIMING_MMC_HS400) ||
		(timing == MMC_TIMING_MMC_HS200) ||
		(timing == MMC_TIMING_UHS_SDR50) ||
		(timing == MMC_TIMING_UHS_SDR104) ||
		(timing == MMC_TIMING_UHS_DDR50) ||
		(timing == MMC_TIMING_UHS_SDR25) ||
		(timing == MMC_TIMING_MMC_DDR52)) {
		reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
		reg &= ~OUTPUT_QSN_PHASE_SELECT;
		sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);
	}

	/* If SDIO card, set SDIO Mode
	 * Otherwise, clear SDIO Mode and Slow Mode
	 */
	if (card && (card->type & MMC_TYPE_SDIO)) {
		reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
		reg |= TIMING_ADJUST_SDIO_MODE;

		if ((timing == MMC_TIMING_UHS_SDR25) ||
			(timing == MMC_TIMING_UHS_SDR12) ||
			(timing == MMC_TIMING_SD_HS) ||
			(timing == MMC_TIMING_LEGACY))
			reg |= TIMING_ADJUST_SLOW_MODE;

		sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);
	} else {
		reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
		reg &= ~(TIMING_ADJUST_SDIO_MODE | TIMING_ADJUST_SLOW_MODE);
		sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);
	}

	/* Set preferred ZNR and ZPR value
	 * The ZNR and ZPR value vary between different boards.
	 * Define them both in sdhci-xenon-emmc-phy.h.
	 */
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL2);
	reg &= ~(ZNR_MASK | ZPR_MASK);
	reg |= ctrl2;
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL2);

	/* When setting EMMC_PHY_FUNC_CONTROL register,
	  * SD clock should be disabled */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	if ((timing == MMC_TIMING_UHS_DDR50) ||
		(timing == MMC_TIMING_MMC_HS400) ||
		(timing == MMC_TIMING_MMC_DDR52)) {
		reg = sdhci_readl(host, EMMC_PHY_FUNC_CONTROL);
		reg |= (DQ_DDR_MODE_MASK << DQ_DDR_MODE_SHIFT) | CMD_DDR_MODE;
		sdhci_writel(host, reg, EMMC_PHY_FUNC_CONTROL);
	}

	if (timing == MMC_TIMING_MMC_HS400) {
		reg = sdhci_readl(host, EMMC_PHY_FUNC_CONTROL);
		reg &= ~DQ_ASYNC_MODE;
		sdhci_writel(host, reg, EMMC_PHY_FUNC_CONTROL);
	}

	/* Enable bus clock */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	if (timing == MMC_TIMING_MMC_HS400)
		/* Hardware team recommend a value for HS400 */
		sdhci_writel(host, LOGIC_TIMING_VALUE, EMMC_LOGIC_TIMING_ADJUST);

	sdhci_xenon_emmc_phy_init(host);

#ifdef CONFIG_XENON_DEBUG
	pr_debug("%s: eMMC PHY setting completes\n", mmc_hostname(host->mmc));
#endif
}

void xenon_emmc_hw_reset_emmc_phy_prep(struct sdhci_host *host)
{
	u32 reg;

	/* When setting EMMC_PHY_FUNC_CONTROL register,
	  * SD clock should be disabled */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
	reg &= ~OUTPUT_QSN_PHASE_SELECT;
	sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);

	reg = sdhci_readl(host, EMMC_PHY_FUNC_CONTROL);
	reg &= ~CMD_DDR_MODE;
	reg |= DQ_ASYNC_MODE;
	reg &= ~(DQ_DDR_MODE_MASK << DQ_DDR_MODE_SHIFT);
	sdhci_writel(host, reg, EMMC_PHY_FUNC_CONTROL);

	/* Reset strobe fix delay */
	reg = sdhci_readl(host, EMMC_PHY_DLL_CONTROL);
	reg &= ~(unsigned int)(DLL_DELAY_TEST_LOWER_MASK <<
			DLL_DELAY_TEST_LOWER_SHIFT);
	reg &= ~DLL_BYPASS_EN;
	sdhci_writel(host, reg, EMMC_PHY_DLL_CONTROL);

	sdhci_xenon_emmc_phy_init(host);

	/* Enable bus clock */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);
}
