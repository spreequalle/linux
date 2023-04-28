/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __AVIO_DHUB_CFG_H__
#define __AVIO_DHUB_CFG_H__
#include "dHub.h"
#include "avioDhub.h"
#include "avio_memmap.h"

#define VPP_DHUB_BASE           (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_dHub0)
#define VPP_HBO_SRAM_BASE       (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_tcm0)
#define VPP_NUM_OF_CHANNELS     (avioDhubChMap_vpp128b_DINT1_R+1)
#define AG_DHUB_BASE            (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_dHub0)
#define AG_HBO_SRAM_BASE        (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_tcm0)
#define AG_NUM_OF_CHANNELS      (avioDhubChMap_aio64b_TT_R+1)

#define VPP_DHUB_BANK0_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK0_START_ADDR
#define VPP_DHUB_BANK1_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK1_START_ADDR
#define VPP_DHUB_BANK2_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK2_START_ADDR
#define VPP_DHUB_BANK3_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK3_START_ADDR
#define AG_DHUB_BANK0_START_ADDR        avioDhubTcmMap_aio64bDhub_BANK0_START_ADDR

#endif //__AVIO_DHUB_CFG_H__
