/* linux/drivers/mmc/host/sdhci-xenon.c
 * Driver for Marvell SOCP Xenon SDHC
 *
 * Author:	Hu Ziji <huziji@marvell.com>
 * Date:		2014-7-17
 *
 *  Copyright (C) 2014 Marvell, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
#include <asm/setup.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/mmc/host.h>
#include <linux/mmc/core.h>
#include "../core/mmc_ops.h"
#include "../core/sdio_ops.h"

#include "sdhci.h"
#include "sdhci-xenon-core.h"
#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY
#include "sdhci-xenon-emmc-phy.h"
#endif

void xenon_set_tuning_count(struct sdhci_host *host, unsigned int count)
{
	if (unlikely(count >= TMR_RETUN_NO_PRESENT)) {
		pr_err("%s: Wrong Re-tuning Count. Set default value %d\n",
			mmc_hostname(host->mmc), DEF_TUNING_COUNT);
		host->tuning_count = DEF_TUNING_COUNT;
		return;
	}

	/* A valid count value */
	host->tuning_count = count;
}

/* Current BSP can only support Tuning Mode 1.
 * Tuning timer is only setup only tuning_mode == Tuning Mode 1.
 * Thus host->tuning_mode has to be forced as Tuning Mode 1.
 */
void xenon_set_tuning_mode(struct sdhci_host *host)
{
	host->tuning_mode = SDHCI_TUNING_MODE_1;
}

/*************************************************************
 *	    Output Delay and Sampling Fixed Delay
 *************************************************************/

void sdhci_xenon_phy_set(struct sdhci_host *host, unsigned char timing,
			u32 ctrl2)
{
#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY
	sdhci_xenon_emmc_phy_set(host, timing, ctrl2);
#endif
}

static int __xenon_emmc_delay_adj_test(struct mmc_card *card)
{
	int err;
	u8 *ext_csd = NULL;

	err = mmc_get_ext_csd(card, &ext_csd);
	kfree(ext_csd);

	return err;
}

static int __xenon_sdio_delay_adj_test(struct mmc_card *card)
{
	u8 reg;

	return mmc_io_rw_direct(card, 0, 0, 0, 0, &reg);
}

static int __xenon_sd_delay_adj_test(struct mmc_card *card)
{
	return mmc_send_status(card, NULL);
}

static int xenon_delay_adj_test(struct mmc_card *card)
{
	if (mmc_card_mmc(card))
		return __xenon_emmc_delay_adj_test(card);
	else if (mmc_card_sd(card))
		return __xenon_sd_delay_adj_test(card);
	else if (mmc_card_sdio(card))
		return __xenon_sdio_delay_adj_test(card);
	else
		return -EINVAL;
}

static int xenon_do_sampl_fix_delay(struct sdhci_host *host,
		struct mmc_card *card, unsigned int delay,
		bool invert, bool phase)
{
	int ret;

	xenon_set_sampl_fix_delay(host, delay, invert, phase);

	ret = xenon_delay_adj_test(card);
	if (ret) {
#ifdef CONFIG_XENON_DEBUG
		pr_debug("Xenon fail when sampling fix delay = %d, inverted = %d, phase = %d\n",
				delay, invert, phase);
#endif
		return -1;
	}

#ifdef CONFIG_XENON_DEBUG
	pr_debug("Xenon secceed when sampling fixed delay = %d, inverted = %d, phase = %d\n",
			delay, invert, phase);
#endif
	return 0;
}

#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
#define COARSE_SAMPL_FIX_DELAY_STEP		50
#define FINE_SAMPL_FIX_DELAY_STEP		20
#else
#define COARSE_SAMPL_FIX_DELAY_STEP		100
#define FINE_SAMPL_FIX_DELAY_STEP		50
#endif

static int xenon_sampl_fix_delay_adj(struct sdhci_host *host,
		struct mmc_card *card)
{
	bool invert, phase;
	int idx, nr_pair;
	int ret;
	unsigned int delay;
	unsigned int min_delay, max_delay;
	u32 reg;
	/* Pairs to set the delay edge
	 * First column is the inversion sequence.
	 * Second column indicates delay 90 degree or not
	 */
	static bool delay_edge_pair[][2] = {
			{ false,	true },
			{ true,		false},
			{ false,	false},
			{ true,		true }
	};

	nr_pair = sizeof(delay_edge_pair) / ARRAY_SIZE(delay_edge_pair);

	for (idx = 0; idx < nr_pair; idx++) {
		invert = delay_edge_pair[idx][0];
		phase = delay_edge_pair[idx][1];

		/* increase dly value to get fix delay */
		for (min_delay = 0; min_delay <= TUNING_PROG_FIXED_DELAY_MASK;
				min_delay += COARSE_SAMPL_FIX_DELAY_STEP) {
			ret = xenon_do_sampl_fix_delay(host, card,
					min_delay, invert, phase);
			if (!ret)
				break;
		}

		if (ret) {
#ifdef CONFIG_XENON_DEBUG
			pr_debug("Fail to set Sampling Fixed Delay with inversion = %d, phase = %d\n",
					invert, phase);
#endif
			continue;
		}

		for (max_delay = min_delay + 1;
			max_delay < TUNING_PROG_FIXED_DELAY_MASK;
			max_delay += FINE_SAMPL_FIX_DELAY_STEP) {
			ret = xenon_do_sampl_fix_delay(host, card,
					max_delay, invert, phase);
			if (ret) {
				max_delay -= FINE_SAMPL_FIX_DELAY_STEP;
				break;
			}
		}

		if (!ret) {
			ret = xenon_do_sampl_fix_delay(host, card,
					TUNING_PROG_FIXED_DELAY_MASK,
					invert, phase);
			if (!ret)
				max_delay = TUNING_PROG_FIXED_DELAY_MASK;
		}

		/* For eMMC PHY,  Sampling Fixed Delay line window should
		 * be larger enough, thus the sampling point (the middle of
		 * the window) can work when environment varies.
		 * However, there is no clear conclusoin how large the window
		 * should be. Thus we just make an assumption that
		 * the window should be larger 25% of a SDCLK cycle.
		 * Please note that this judgement is only based on experience.
		 *
		 * The field Delay value of Main Delay Line in
		 * register SDHC_SLOT_DLL_CUR represent a half of SDCLK
		 * cycle. Thus the window should be larger than 1/2 of
		 * the field Delay value of Main Delay Line value.
		 */
		reg = sdhci_readl(host, SDHC_SLOT_DLL_CUR_DLY_VAL);
		reg &= TUNING_PROG_FIXED_DELAY_MASK;
		/* get the 1/4 SDCLK cycle */
		reg >>= 1;

		if ((max_delay - min_delay) < reg) {
			pr_info("The window size %d when inversion = %d, phase = %d cannot meet timing requiremnt\n",
				max_delay - min_delay, invert, phase);
			continue;
		}

		delay = (min_delay + max_delay) / 2;
		xenon_set_sampl_fix_delay(host, delay, invert, phase);
#ifdef CONFIG_XENON_DEBUG
		pr_debug("Xenon sampling fix delay = %d with inversion = %d, phase = %d\n",
				delay, invert, phase);
#endif
		return 0;
	}

	return -EIO;
}


/* This the interface to adjust Sampling Dealy
 */
static int xenon_sampl_delay_adj(struct sdhci_host *host, struct mmc_card *card)
{
	/* So far in current ASIC chips, DLL only can help
	 * adjust HS400 Data Strobe delay.
	 * Thus it is still necessary to find a sampling fixed delay
	 * at the beginning before tuning is enabled.
	 * In the latest version, the accurate nr of delay units
	 * in a tuning step can be got according to DLL's help.
	 * After the ASIC chip of the lastes version returns,
	 * the sampling delay adjuestment will be modified
	 * to use tuning.
	 */
	return xenon_sampl_fix_delay_adj(host, card);
}

static int xenon_hs400_strobe_delay_adj(struct sdhci_host *host,
		struct mmc_card *card)
{
	int ret = 0;
	u32 reg;

	BUG_ON(!mmc_card_hs400(card));

	/* Enable SDHC Data Strobe */
	reg = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	reg |= ENABLE_DATA_STROBE;
	sdhci_writel(host, reg, SDHC_SLOT_eMMC_CTRL);

	/* Enable the DLL to automatically adjust HS400 strobe delay.
	 * Of course, xenon_strobe_fix_delay_adj can be used instead.
	 */
	xenon_emmc_phy_adj_strobe_delay(host);
	return ret;
}

/* sdhci_delay_adj should not be called inside IRQ context,
 * either Hard IRQ or Softirq.
 */
int xenon_hs_delay_adj(struct mmc_host *mmc, struct mmc_card *card)
{
	int ret;
	struct sdhci_host *host = mmc_priv(mmc);

	if (!(host->quirks2 & SDHCI_QUIRK2_XENON_HACK))
		return 0;

	if (host->clock <= 4000000)
		/* Currently in a specical stage, for example
		 * during the hardware reset in high speed mode,
		 * but the SDCLK is no more than 400k (legacy mode)
		 * no need to set any delay because will be reset
		 * as legacy mode soon
		 */
		return 0;

	if (mmc_card_hs400(card)) {
#ifdef CONFIG_XENON_DEBUG
		pr_debug("%s: starts HS400 strobe delay adjustment\n",
				mmc_hostname(mmc));
#endif
		ret = xenon_hs400_strobe_delay_adj(host, card);
		if (ret)
			pr_err("%s: fails strobe fixed delay adjustment\n",
					mmc_hostname(mmc));

		return ret;
	}

#ifdef CONFIG_XENON_DEBUG
		pr_debug("%s: starts sampling fixed delay adjustment\n",
			mmc_hostname(mmc));
#endif
	ret = xenon_sampl_delay_adj(host, card);
	if (ret)
		pr_err("%s: fails sampling fixed delay adjustment\n",
				mmc_hostname(mmc));
	return ret;
}

/*************************************************************
 *	   Xenon Specific Initialization Operations
 *************************************************************/
/* Enable/Disable the Auto Clock Gating function */
void xenon_set_acg(struct sdhci_host *host, bool enable)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	if (enable)
		reg &= ~AUTO_CLKGATE_DISABLE_MASK;
	else
		reg |= AUTO_CLKGATE_DISABLE_MASK;
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
}

/* Enable this slot */
void xenon_enable_slot(struct sdhci_host *host, unsigned char slotno)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	reg |= ((0x1 << slotno) << SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);

	/* Manually set the flag which all the slots require,
	 * including SD, eMMC, SDIO
	 */
	host->mmc->caps |= MMC_CAP_WAIT_WHILE_BUSY;
}

/* Disable this slot */
void xenon_disable_slot(struct sdhci_host *host, unsigned char slotno)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	reg &= ~((0x1 << slotno) << SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
}


/* Set SDCLK-off-while-idle */
void xenon_set_sdclk_off_idle(struct sdhci_host *host,
		unsigned char slotno, bool enable)
{
	unsigned int reg;
	u32 mask;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	/* Get the bit shift basing on the slot index */
	mask = (0x1 << (SDCLK_IDLEOFF_ENABLE_SHIFT + slotno));
	if (enable)
		reg |= mask;
	else
		reg &= ~mask;

	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
}

/* Enable Parallel Transfer Mode */
void xenon_enable_slot_parallel_tran(struct sdhci_host *host,
		unsigned char slotno)
{
	unsigned int reg;

	reg = sdhci_readl(host, SDHC_SYS_EXT_OP_CTRL);
	reg |= (0x1 << slotno);
	sdhci_writel(host, reg, SDHC_SYS_EXT_OP_CTRL);
}

void xenon_slot_tuning_setup(struct sdhci_host *host)
{
	unsigned int reg;

	/* Disable the Re-Tuning Request functionality */
	reg = sdhci_readl(host, SDHC_SLOT_RETUNING_REQ_CTRL);
	reg &= ~RETUNING_COMPATIBLE;
	sdhci_writel(host, reg, SDHC_SLOT_RETUNING_REQ_CTRL);

	/* Disbale the Re-tuning Event Signal Enable */
	reg = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);
	reg &= ~SDHCI_RETUNE_EVT_INTSIG;
	sdhci_writel(host, reg, SDHCI_SIGNAL_ENABLE);

	/* Disable Auto-retuning */
	reg = sdhci_readl(host, SDHC_SLOT_AUTO_RETUNING_CTRL);
	reg &= ~ENABLE_AUTO_RETUNING;
	sdhci_writel(host, reg, SDHC_SLOT_AUTO_RETUNING_CTRL);
}


/***************************************************************
 * Operations inside struct sdhci_ops for both platform and PCI
 ***************************************************************/
/* Recover the Register Setting cleared during SOFTWARE_RESET_ALL */
static void sdhci_xenon_reset_exit(struct sdhci_host *host,
		unsigned char slotno, u8 mask)
{
	/* Only SOFTWARE RESET ALL will clear the register setting */
	if (!(mask & SDHCI_RESET_ALL))
		return;

	/* Disable tuning request and auto-retuing again */
	xenon_slot_tuning_setup(host);

	xenon_set_acg(host, true);

	xenon_set_sdclk_off_idle(host, slotno, false);
}

void sdhci_xenon_reset(struct sdhci_host *host, u8 mask)
{
	sdhci_reset(host, mask);

	sdhci_xenon_reset_exit(host, host->mmc->slotno, mask);
}

static int __emmc_signal_voltage_switch(struct sdhci_host *host,
	const unsigned char signal_voltage)
{
	u32 ctrl;
	unsigned char voltage_code;

	if (signal_voltage == MMC_SIGNAL_VOLTAGE_330)
		voltage_code = eMMC_VCCQ_3_3V;
	else if (signal_voltage == MMC_SIGNAL_VOLTAGE_180)
		voltage_code = eMMC_VCCQ_1_8V;
	else
		return -EINVAL;

	/* This host is for eMMC, XENON self-defined
	 * eMMC slot control register should be accessed
	 * instead of Host Control 2
	 */
	ctrl = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	ctrl &= ~eMMC_VCCQ_MASK;
	ctrl |= voltage_code;
	sdhci_writel(host, ctrl, SDHC_SLOT_eMMC_CTRL);

	/* There is no standard to determine this waiting period */
	usleep_range(1000, 2000);

	/* Check whether io voltage switch is done */
	ctrl = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	ctrl &= eMMC_VCCQ_MASK;
	/*
	 * This bit is set only when regulator feedbacks the voltage switch
	 * results.
	 * However, in actaul implementation, regulator might not provide
	 * this feedback.
	 * Thus we shall not rely on this bit status to
	 * determine if switch failed.
	 * If the bit is not set, just throw a warning.
	 * Besides, error code should neither be returned.
	 */
	if (ctrl != voltage_code)
		pr_warn("%s: Xenon fail to detect signal voltage stable\n",
					mmc_hostname(host->mmc));
	return 0;
}

void sdhci_xenon_emmc_voltage_switch(struct sdhci_host *host)
{
	unsigned char voltage = host->mmc->ios.signal_voltage;

	/* Only available for eMMC slot */
	if (!(host->quirks2 & SDHCI_QUIRK2_XENON_EMMC_SLOT))
		return;

	if ((voltage == MMC_SIGNAL_VOLTAGE_330) ||
		(voltage == MMC_SIGNAL_VOLTAGE_180))
		__emmc_signal_voltage_switch(host, voltage);
	else
		pr_err("%s: Xenon Unsupported signal voltage\n",
				mmc_hostname(host->mmc));
}

void sdhci_xenon_post_attach(struct mmc_host *mmc)
{
	struct sdhci_host *host = mmc_priv(mmc);

	if (host->quirks2 & SDHCI_QUIRK2_XENON_HACK)
		xenon_set_sdclk_off_idle(host, mmc->slotno, true);
}

/*
 * Ignore CRC error during eMMC Bus Test (CMD19 and CMD14)
 */
void sdhci_xenon_bus_test_pre(struct mmc_host *mmc)
{
	struct sdhci_host *host = mmc_priv(mmc);
	u32 reg;

	reg = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	reg |= (DISABLE_RD_DATA_CRC | DISABLE_CRC_STAT_TOKEN);
	sdhci_writel(host, reg, SDHC_SLOT_eMMC_CTRL);
}

void sdhci_xenon_bus_test_post(struct mmc_host *mmc)
{
	struct sdhci_host *host = mmc_priv(mmc);
	u32 reg;

	reg = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	reg &= ~(DISABLE_RD_DATA_CRC | DISABLE_CRC_STAT_TOKEN);
	sdhci_writel(host, reg, SDHC_SLOT_eMMC_CTRL);
}

/* After determining the slot is used for SDIO, some addtional task is required.
 */
void sdhci_xenon_init_card(struct mmc_host *mmc, struct mmc_card *card)
{
	struct sdhci_host *host = mmc_priv(mmc);
	u32 reg;
	u8 slot_idx;
	struct card_cntx *cntx;

	if (!(host->quirks2 & SDHCI_QUIRK2_XENON_HACK))
		return;

	cntx = (struct card_cntx *)mmc->slot.handler_priv;
	cntx->delay_adjust_card = card;

	slot_idx = mmc->slotno;

	if (!mmc_card_sdio(card)) {
		/* Re-enable the Auto-CMD12 cap flag. */
		host->quirks |= SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;
		host->flags |= SDHCI_AUTO_CMD12;

		/* Clear SDHC System Config Information Register[31:24] */
		reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
		reg &= ~(1 << (slot_idx + SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHC_SYS_CFG_INFO);
	} else {
		/* Delete the Auto-CMD12 cap flag.
		 * Otherwise, when sending multi-block CMD53,
		 * Driver will set Transfer Mode Register to enable Auto CMD12.
		 * However, SDIO device cannot recognize CMD12.
		 * Thus SDHC will time-out for waiting for CMD12 response.
		 */
		host->quirks &= ~SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;
		host->flags &= ~SDHCI_AUTO_CMD12;

		/* Set SDHC System Configuration Information Register[31:24]
		 * to inform that the current slot is for SDIO */
		reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
		reg |= (1 << (slot_idx + SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHC_SYS_CFG_INFO);
	}
}

/*************************************************************
 *	  Xenon Hardware Reset Preparation Quirks
 *************************************************************/
void sdhci_xenon_emmc_hw_reset_prep(struct sdhci_host *host)
{
	u32 reg;

	/* Disable SDCLK-Off-While-Idle */
	xenon_set_sdclk_off_idle(host, host->mmc->slotno, false);

	/* Disable eMMC Data strobe */
	reg = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	reg &= ~ENABLE_DATA_STROBE;
	sdhci_writel(host, reg, SDHC_SLOT_eMMC_CTRL);

#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY
	xenon_emmc_hw_reset_emmc_phy_prep(host);
#endif
}
