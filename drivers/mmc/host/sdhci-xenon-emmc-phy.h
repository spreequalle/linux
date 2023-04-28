/* linux/drivers/mmc/host/sdhci-xenon-emmc-phy.h
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
#ifndef SDHCI_XENON_EMMC_PHY_H_
#define SDHCI_XENON_EMMC_PHY_H_

#include "sdhci.h"

#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
#define EMMC_PHY_REG_BASE				0x160
#else
#define EMMC_PHY_REG_BASE				0x170
#endif

#define EMMC_PHY_TIMING_ADJUST			EMMC_PHY_REG_BASE
	#define TIMING_ADJUST_SLOW_MODE			(1 << 29)
	#define TIMING_ADJUST_SDIO_MODE			(1 << 28)
	#define OUTPUT_QSN_PHASE_SELECT			(1 << 17)
	#define SAMPL_INV_QSP_PHASE_SELECT		(1 << 18)
	#define SAMPL_INV_QSP_PHASE_SELECT_SHIFT	18
	#define PHY_INITIALIZAION			(1 << 31)
	#define WAIT_CYCLE_BEFORE_USING_MASK		0xf
	#define WAIT_CYCLE_BEFORE_USING_SHIFT		12
	#define FC_SYNC_EN_DURATION_MASK		0xf
	#define FC_SYNC_EN_DURATION_SHIFT		8
	#define	FC_SYNC_RST_EN_DURATION_MASK		0xf
	#define FC_SYNC_RST_EN_DURATION_SHIFT		4
	#define FC_SYNC_RST_DURATION_MASK		0xf
	#define FC_SYNC_RST_DURATION_SHIFT		0

#define EMMC_PHY_FUNC_CONTROL			(EMMC_PHY_REG_BASE + 0x4)
	#define ASYNC_DDRMODE_MASK			(1 << 23)
	#define ASYNC_DDRMODE_SHIFT			23
	#define CMD_DDR_MODE				(1 << 16)
	#define DQ_DDR_MODE_SHIFT			8
	#define DQ_DDR_MODE_MASK			0xff
	#define DQ_ASYNC_MODE				(1 << 4)

#define EMMC_PHY_PAD_CONTROL			(EMMC_PHY_REG_BASE + 0x8)
	#define REC_EN_SHIFT				24
	#define REC_EN_MASK				0xf
	#define FC_DQ_RECEN				(1 << 24)
	#define FC_CMD_RECEN				(1 << 25)
	#define FC_QSP_RECEN				(1 << 26)
	#define FC_QSN_RECEN				(1 << 27)
	#define OEN_QSN					(1 << 28)
	#define AUTO_RECEN_CTRL				(1 << 30)
	#define FC_ALL_CMOS_RECEIVER			0xf000

	#define EMMC5_FC_QSP_PD				(1 << 18)
	#define EMMC5_FC_QSP_PU				(1 << 22)
	#define EMMC5_FC_CMD_PD				(1 << 17)
	#define EMMC5_FC_CMD_PU				(1 << 21)
	#define EMMC5_FC_DQ_PD				(1 << 16)
	#define EMMC5_FC_DQ_PU				(1 << 20)

#ifndef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
#define EMMC_PHY_PAD_CONTROL1			(EMMC_PHY_REG_BASE + 0xc)
	#define EMMC5_1_FC_QSP_PD			(1 << 9)
	#define EMMC5_1_FC_QSP_PU			(1 << 25)
	#define EMMC5_1_FC_CMD_PD			(1 << 8)
	#define EMMC5_1_FC_CMD_PU			(1 << 24)
	#define EMMC5_1_FC_DQ_PD			0xff
	#define EMMC5_1_FC_DQ_PU			(0xff << 16)

#define EMMC_PHY_PAD_CONTROL2			(EMMC_PHY_REG_BASE + 0x10)
#else
#define EMMC_PHY_PAD_CONTROL2			(EMMC_PHY_REG_BASE + 0xc)
#endif
	#define ZNR_MASK			(0x1f << 8)
	#define ZNR_SHIFT			8
	#define ZPR_MASK			0x1f
	/* Perferred ZNR and ZPR value vary between different boards.
	 * The specific ZNR and ZPR value should be defined here
	 * according to board actual timing.
	 */
	#define ZNR_PREF_VALUE			0xf
	#define ZPR_PREF_VALUE			0xf

#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
#define EMMC_PHY_DLL_CONTROL			(EMMC_PHY_REG_BASE + 0x10)
#else
#define EMMC_PHY_DLL_CONTROL			(EMMC_PHY_REG_BASE + 0x14)
#endif
	#define DLL_ENABLE				(1 << 31)
#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
	#define DLL_UPDATE_STROBE			(1 << 30)
#else
	#define DLL_REFCLK_SEL				(1 << 30)
	#define DLL_UPDATE				(1 << 23)
#endif
	#define DLL_PHSEL1_SHIFT			24
	#define DLL_PHSEL0_SHIFT			16
	#define DLL_PHASE_MASK				0x3f
	#define DLL_PHASE_90_DEGREE			0x1f
	#define DLL_DELAY_TEST_LOWER_SHIFT		8
	#define DLL_DELAY_TEST_LOWER_MASK		0xff
	#define DLL_FAST_LOCK				(1 << 5)
	#define DLL_GAIN2X				(1 << 3)
	#define DLL_BYPASS_EN				(1 << 0)

#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
#define EMMC_LOGIC_TIMING_ADJUST		(EMMC_PHY_REG_BASE + 0x14)
#else
#define EMMC_LOGIC_TIMING_ADJUST		(EMMC_PHY_REG_BASE + 0x18)
#define EMMC_LOGIC_TIMING_ADJUST_LOW		(EMMC_PHY_REG_BASE + 0x1c)
#endif

int sdhci_xenon_emmc_phy_init(struct sdhci_host *host);

int xenon_set_sampl_fix_delay(struct sdhci_host *host, unsigned int delay,
		bool invert, bool phase);

int xenon_set_strobe_fix_delay(struct sdhci_host *host, unsigned int delay);

void sdhci_xenon_emmc_phy_set(struct sdhci_host *host, unsigned char timing,
			u32 ctrl2);

void xenon_emmc_phy_adj_strobe_delay(struct sdhci_host *host);

void xenon_emmc_hw_reset_emmc_phy_prep(struct sdhci_host *host);

#endif
