/* linux/drivers/mmc/host/sdhci-xenon-core.h
 *
 * Author: Hu Ziji <huziji@marvell.com>
 * Date:		2014-7-17
 *
 *  Copyright (C) 2014 Marvell, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
#ifndef SDHCI_XENON_H_
#define SDHCI_XENON_H_

#include <linux/mmc/card.h>
#include "sdhci.h"

/* Register Offset of SD Host Controller SOCP self-defined register */
#define SDHC_IPID					0x100
	#define SDHC_IP_REL_TAG_SHIFT			12
	#define SDHC_IP_REL_TAG_MASK			0xfffff
	#define SDHC_IPID_MAIN_MASK			0xfff
	#define SDHC_VER_EMMC_5				0x831
	#define SDHC_VER_EMMC_5_1			0x832

#define SDHC_SYS_CFG_INFO				0x0104
	#define SLOT_TYPE_SDIO_SHIFT			24
	#define SLOT_TYPE_EMMC_MASK			0xff
	#define SLOT_TYPE_EMMC_SHIFT			16
	#define SLOT_TYPE_SD_SDIO_MMC_MASK		0xff
	#define SLOT_TYPE_SD_SDIO_MMC_SHIFT		8

#define SDHC_SYS_OP_CTRL				0x0108
	#define AUTO_CLKGATE_DISABLE_MASK		(0x1<<20)
	#define SDCLK_IDLEOFF_ENABLE_SHIFT		8
	#define SLOT_ENABLE_SHIFT			0

#define SDHC_SYS_EXT_OP_CTRL				0x010c
#define SDHC_TEST_OUT					0x0110
#define SDHC_TESTOUT_MUXSEL				0x0114

#define SDHC_SLOT_EXT_INT_STATUS			0x0120

#define SDHC_SLOT_EXT_ERR_STATUS			0x0122
	#define AXI_SLVIF_RD_SLVERR			(0x1 << 15)
	#define AXI_SLVIF_WR_SLVERR			(0x1 << 14)
	#define TASK_FETCH_ERR				(0x1 << 8)
	#define AXI_MSTIF_RD_DECERR			(0x1 << 5)
	#define AXI_MSTIF_RD_SLVERR			(0x1 << 4)
	#define AXI_MSTIF_RD_ICPRESP			(0x1 << 3)
	#define AXI_MSTIF_WR_DECERR			(0x1 << 2)
	#define AXI_MSTIF_WR_SLVERR			(0x1 << 1)
	#define AXI_MSTIF_WR_ICPRESP			(0x1 << 0)

	#define SDHC_AXI_ERR_MASK	(AXI_SLVIF_RD_SLVERR | AXI_SLVIF_WR_SLVERR | \
					AXI_MSTIF_RD_DECERR | AXI_MSTIF_RD_SLVERR | \
					AXI_MSTIF_RD_ICPRESP | AXI_MSTIF_WR_DECERR | \
					AXI_MSTIF_WR_SLVERR | AXI_MSTIF_WR_ICPRESP)

#define SDHC_SLOT_EXT_INT_STATUS_EN			0x0124
#define SDHC_SLOT_EXT_ERR_STATUS_EN			0x0126

#define SDHC_SLOT_OP_STATUS_CTRL			0x0128
	#define	DELAY_90_DEGREE_MASK_EMMC5		(1 << 7)
	#define DELAY_90_DEGREE_SHIFT_EMMC5		7
#ifdef CONFIG_MMC_XENON_SDHCI_EMMC_PHY_OLD_VER
	#define TUNING_PROG_FIXED_DELAY_MASK		0x7f	/* MAX 0x7f */
#else
	#define TUNING_PROG_FIXED_DELAY_MASK		0xff	/* MAX 0xff */
#endif
	#define FORCE_SEL_INVERSE_CLK_SHIFT		11

#define SDHC_SLOT_FIFO_CTRL				0x012c

#define SDHC_SLOT_eMMC_CTRL				0x0130
	#define ENABLE_DATA_STROBE			(1 << 24)
	#define SET_EMMC_RSTN				(1 << 16)
	#define DISABLE_RD_DATA_CRC			(1 << 14)
	#define DISABLE_CRC_STAT_TOKEN			(1 << 13)
	#define eMMC_VCCQ_MASK				0x3
	#define eMMC_VCCQ_1_8V				0x1
	#define eMMC_VCCQ_1_2V				0x2
	#define	eMMC_VCCQ_3_3V				0x3

#define SDHC_SLOT_STROBE_DLY_CTRL			0x0140
	#define STROBE_DELAY_FIXED_MASK			0xffff

#define SDHC_SLOT_RETUNING_REQ_CTRL			0x0144
	/* retuning compatible */
	#define RETUNING_COMPATIBLE			0x1

#define SDHC_SLOT_AUTO_RETUNING_CTRL			0x0148
	#define ENABLE_AUTO_RETUNING			0x1

#define SDHC_SLOT_EXT_PRESENT_STATE			0x014c
#define SDHC_SLOT_DLL_CUR_DLY_VAL			0x0150
#define SDHC_SLOT_TUNING_CUR_DLY_VAL			0x0154
#define SDHC_SLOT_STROBE_CUR_DLY_VAL			0x0158
#define SDHC_SLOT_SUB_CMD_STRL				0x015c


#define SDHC_SLOT_CQ_TASK_INFO				0x0160
	#define TASK_SLOT_FETCH_INDEX_MASK		0x1f

#define SDHC_SLOT_TUNING_DEBUG_INFO			0x01f0
#define SDHC_SLOT_DATAIN_DEBUG_INFO			0x01f4

/* Tuning Parameter */
#define TMR_RETUN_NO_PRESENT				0xf
#define XENON_MAX_TUN_COUNT				0xe


#define MMC_TIMING_FAKE					0xff

#define DEF_TUNING_COUNT				0x9

struct card_cntx {
	/* When initializing card, Xenon has to adjust
	 * Sampling Fixed delay.
	 * However, at that time, card structure is not
	 * linked to mmc_host.
	 * Thus a card pointer is added here provide
	 * the delay adjustment function with the card structure
	 * of the card during initialization
	 */
	struct mmc_card *delay_adjust_card;
};

void xenon_set_tuning_count(struct sdhci_host *host,
		unsigned int count);

void xenon_set_tuning_mode(struct sdhci_host *host);

void xenon_set_acg(struct sdhci_host *host, bool enable);

void xenon_enable_slot(struct sdhci_host *host, unsigned char slotno);

void xenon_disable_slot(struct sdhci_host *host, unsigned char slotno);

void xenon_set_sdclk_off_idle(struct sdhci_host *host, unsigned char slotno,
		bool enable);

void xenon_enable_slot_parallel_tran(struct sdhci_host *host,
		unsigned char slotno);

void xenon_slot_tuning_setup(struct sdhci_host *host);

int sdhci_xenon_delay_adj(struct sdhci_host *host, struct mmc_ios *ios);

void sdhci_xenon_emmc_voltage_switch(struct sdhci_host *host);

void sdhci_xenon_post_attach(struct mmc_host *mmc);

void sdhci_xenon_bus_test_pre(struct mmc_host *mmc);

void sdhci_xenon_bus_test_post(struct mmc_host *mmc);

void sdhci_xenon_init_card(struct mmc_host *mmc, struct mmc_card *card);

void sdhci_xenon_emmc_hw_reset_prep(struct sdhci_host *host);

int xenon_hs_delay_adj(struct mmc_host *mmc, struct mmc_card *card);

void sdhci_xenon_phy_set(struct sdhci_host *host, unsigned char timing,
			u32 ctrl2);

void sdhci_xenon_reset(struct sdhci_host *host, u8 mask);

void sd_debug(void);

#endif
