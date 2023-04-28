/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __API_AVIF_DHUB_H__
#define __API_AVIF_DHUB_H__
#include "dHub.h"
#include "api_dhub.h"
#include "avif_dhub_config.h"
#include "api_avio_dhub.h"

#define MEMMAP_AVIF_DHUB_REG_BASE (HIF_CPU_DHUB_BASE_OFF)

#define AVIF_DHUB_BASE          (MEMMAP_AVIF_DHUB_REG_BASE + RA_avif_dhub_config_dHub0)
#define AVIF_HBO_SRAM_BASE      (MEMMAP_AVIF_DHUB_REG_BASE + RA_avif_dhub_config_tcm0)
#define AVIF_NUM_OF_CHANNELS    (avif_dhub_config_ChMap_avif_AUD_RD0+1)

#if (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
#define AVIF_DHUB_BANK0_START_ADDR        (0)	// Size: 0x2000
#define AVIF_DHUB_BANK1_START_ADDR        (1024*8)	// Size: 0x3C00
#define AVIF_DHUB_BANK2_START_ADDR        (1024*23)	// Size: 0x1800

#elif (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
#define AVIF_DHUB_BANK0_START_ADDR        (0)
#define AVIF_DHUB_BANK1_START_ADDR        (1024*8)
#define AVIF_DHUB_BANK2_START_ADDR        (1024*23)
#define AVIF_DHUB_BANK3_START_ADDR        (1024*35)

#else	/*  */
#define AVIF_DHUB_BANK0_START_ADDR        (0)
#define AVIF_DHUB_BANK1_START_ADDR        (1024*8)
#define AVIF_DHUB_BANK2_START_ADDR        (1024*20)
#define AVIF_DHUB_BANK3_START_ADDR        (1024*32)
#endif	/*  */

#define HIF_CPU_MADDR_BASE_OFF  0xF7980000
#define HIF_CPU_ITCM_BASE_OFF   0xF7990000
#define HIF_CPU_DHUB_BASE_OFF   0xF79A0000
extern HDL_dhub2d AVIF_dhubHandle;
extern DHUB_channel_config AVIF_config[];

#endif //__API_AVIF_DHUB_H__
