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

//Enable this macro to allocate more SRAM to MAIN PIP & GFX Plane
#define BG4CDP_OPTIMIZE_SRAM_ALLOCATION

#define VPP_DHUB_BASE           (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_dHub0)
#define VPP_HBO_SRAM_BASE       (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_tcm0)
#define VPP_NUM_OF_CHANNELS     (avioDhubChMap_vpp128b_DINT1_R+3)
#define AG_DHUB_BASE            (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_dHub0)
#define AG_HBO_SRAM_BASE        (MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AIO64B_DHUB_REG_BASE + RA_aio64bDhub_tcm0)
#define AG_NUM_OF_CHANNELS      (avioDhubChMap_aio64b_SPDIF_W+1)

#define VPP_DHUB_BANK0_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK0_START_ADDR
#define VPP_DHUB_BANK1_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK1_START_ADDR
#define VPP_DHUB_BANK2_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK2_START_ADDR
#define VPP_DHUB_BANK3_START_ADDR       avioDhubTcmMap_vpp128bDhub_BANK3_START_ADDR
#define AG_DHUB_BANK0_START_ADDR        avioDhubTcmMap_aio64bDhub_BANK0_START_ADDR


/***********************************************/
/*          BG4 CDP A0                         */
/************************************************/
#define        AVIO_MEMMAP_VPP128B_DHUB_REG_BASE_A0           0x0
#define        AVIO_MEMMAP_VPP128B_DHUB_REG_SIZE_A0           0x40000
#define        AVIO_MEMMAP_VPP128B_DHUB_REG_DEC_BIT_A0        0x12
#define        AVIO_MEMMAP_AIO64B_DHUB_REG_BASE_A0            0x40000
#define        AVIO_MEMMAP_AIO64B_DHUB_REG_SIZE_A0            0x20000
#define        AVIO_MEMMAP_AIO64B_DHUB_REG_DEC_BIT_A0         0x11
#define        AVIO_MEMMAP_VPP_REG_BASE_A0                    0x60000
#define        AVIO_MEMMAP_VPP_REG_SIZE_A0                    0x20000
#define        AVIO_MEMMAP_VPP_REG_DEC_BIT_A0                 0x11
#define        AVIO_MEMMAP_AVIO_RESERVED0_REG_BASE_A0         0x80000
#define        AVIO_MEMMAP_AVIO_RESERVED0_REG_SIZE_A0         0x20000
#define        AVIO_MEMMAP_AVIO_RESERVED0_REG_DEC_BIT_A0      0x11
#define        AVIO_MEMMAP_AVIO_GBL_BASE_A0                   0xA0000
#define        AVIO_MEMMAP_AVIO_GBL_SIZE_A0                   0x10000
#define        AVIO_MEMMAP_AVIO_GBL_DEC_BIT_A0                0x10
#define        AVIO_MEMMAP_AVIO_BCM_REG_BASE_A0               0xB0000
#define        AVIO_MEMMAP_AVIO_BCM_REG_SIZE_A0               0x10000
#define        AVIO_MEMMAP_AVIO_BCM_REG_DEC_BIT_A0            0x10
#define        AVIO_MEMMAP_AVIO_I2S_REG_BASE_A0               0xC0000
#define        AVIO_MEMMAP_AVIO_I2S_REG_SIZE_A0               0x10000
#define        AVIO_MEMMAP_AVIO_I2S_REG_DEC_BIT_A0            0x10
#define        AVIO_MEMMAP_AVIO_HDMITX_REG_BASE_A0            0xD0000
#define        AVIO_MEMMAP_AVIO_HDMITX_REG_SIZE_A0            0x4000
#define        AVIO_MEMMAP_AVIO_HDMITX_REG_DEC_BIT_A0         0xE
#define        AVIO_MEMMAP_AVIO_EDDC_REG_BASE_A0              0xD4000
#define        AVIO_MEMMAP_AVIO_EDDC_REG_SIZE_A0              0x4000
#define        AVIO_MEMMAP_AVIO_EDDC_REG_DEC_BIT_A0           0xE
#define        AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_BASE_A0        0xD8000
#define        AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_SIZE_A0        0x8000
#define        AVIO_MEMMAP_AVIO_HDMIRXPIPE_REG_DEC_BIT_A0     0xF
#define        AVIO_MEMMAP_AVIO_HDMIRX_REG_BASE_A0            0xE0000
#define        AVIO_MEMMAP_AVIO_HDMIRX_REG_SIZE_A0            0x8000
#define        AVIO_MEMMAP_AVIO_HDMIRX_REG_DEC_BIT_A0         0xF
#define        AVIO_MEMMAP_AVIO_RESERVED3_REG_BASE_A0         0xE8000
#define        AVIO_MEMMAP_AVIO_RESERVED3_REG_SIZE_A0         0x8000
#define        AVIO_MEMMAP_AVIO_RESERVED3_REG_DEC_BIT_A0      0xF
#define        AVIO_MEMMAP_AVIO_RESERVED4_REG_BASE_A0         0xF0000
#define        AVIO_MEMMAP_AVIO_RESERVED4_REG_SIZE_A0         0x8000
#define        AVIO_MEMMAP_AVIO_RESERVED4_REG_DEC_BIT_A0      0xF

#define VPP_DHUB_BASE_A0 \
					(MEMMAP_AVIO_REG_BASE +\
					AVIO_MEMMAP_VPP128B_DHUB_REG_BASE_A0 +\
					RA_vpp128bDhub_dHub0)
#define VPP_HBO_SRAM_BASE_A0 \
					(MEMMAP_AVIO_REG_BASE + \
					AVIO_MEMMAP_VPP128B_DHUB_REG_BASE_A0 +\
					RA_vpp128bDhub_tcm0)
#define VPP_NUM_OF_CHANNELS_A0  (avioDhubChMap_vpp128b_DBEDR_RD + 1)

#define AG_DHUB_BASE_A0 \
					(MEMMAP_AVIO_REG_BASE +\
					AVIO_MEMMAP_AIO64B_DHUB_REG_BASE_A0 +\
					RA_aio64bDhub_dHub0)
#define AG_HBO_SRAM_BASE_A0 \
					(MEMMAP_AVIO_REG_BASE +\
					AVIO_MEMMAP_AIO64B_DHUB_REG_BASE_A0 +\
					RA_aio64bDhub_tcm0)
#define AG_NUM_OF_CHANNELS_A0      (avioDhubChMap_aio64b_SPDIF_W+1)

#define AVIO_DHUB_MV_R0_SIZE       (1024*8)
#define AVIO_DHUB_VMM_VX_R_SIZE    ((1024*3)/4)
#define AVIO_DHUB_MV_R1_SIZE       (1024*5)
#define AVIO_DHUB_DINT_W_SIZE      ((1024*7)/2)
#define AVIO_DHUB_DINT0_R_SIZE     (1024*8)
#define AVIO_DHUB_GFX_PIP_R0_SIZE  (1024*12)
#define AVIO_DHUB_PIP_R1_SIZE      (1024*4)
#define AVIO_DHUB_GFX_R1_SIZE      (1024*12)
#define AVIO_DHUB_DINT1_R_SIZE     (1024*8)
#define AVIO_DHUB_1DSCL_W0_SIZE    (1024*5)
#define AVIO_DHUB_1DSCL_W1_SIZE    ((1024*5)/2)
#define AVIO_DHUB_MetaData_RD_SIZE ((1024*1)/4)
#define AVIO_DHUB_DBEDR_RD_SIZE    (1024*6)

#define AVIO_DHUB_MV_R0_BASE       (VPP_DHUB_BANK0_START_ADDR)
#define AVIO_DHUB_VMM_VX_R_BASE \
							(AVIO_DHUB_MV_R0_BASE +\
							AVIO_DHUB_MV_R0_SIZE)
#define AVIO_DHUB_MV_R1_BASE \
						(AVIO_DHUB_VMM_VX_R_BASE +\
						AVIO_DHUB_VMM_VX_R_SIZE)
#define AVIO_DHUB_DINT_W_BASE \
						(AVIO_DHUB_MV_R1_BASE +\
						AVIO_DHUB_MV_R1_SIZE)
#define AVIO_DHUB_DINT0_R_BASE \
						(AVIO_DHUB_DINT_W_BASE +\
						AVIO_DHUB_DINT_W_SIZE)
#define AVIO_DHUB_GFX_PIP_R0_BASE \
						(AVIO_DHUB_DINT0_R_BASE +\
						AVIO_DHUB_DINT0_R_SIZE)
#define AVIO_DHUB_PIP_R1_BASE \
						(AVIO_DHUB_GFX_PIP_R0_BASE +\
						AVIO_DHUB_GFX_PIP_R0_SIZE)
#define AVIO_DHUB_GFX_R1_BASE \
						(AVIO_DHUB_PIP_R1_BASE +\
						AVIO_DHUB_PIP_R1_SIZE)
#define AVIO_DHUB_DINT1_R_BASE \
						(AVIO_DHUB_GFX_R1_BASE +\
						AVIO_DHUB_GFX_R1_SIZE)
#define AVIO_DHUB_1DSCL_W0_BASE \
						(AVIO_DHUB_DINT1_R_BASE +\
						AVIO_DHUB_DINT1_R_SIZE)
#define AVIO_DHUB_1DSCL_W1_BASE \
						(AVIO_DHUB_1DSCL_W0_BASE +\
						AVIO_DHUB_1DSCL_W0_SIZE)
#define AVIO_DHUB_MetaData_RD_BASE \
						(AVIO_DHUB_1DSCL_W1_BASE +\
						AVIO_DHUB_1DSCL_W1_SIZE)
#define AVIO_DHUB_DBEDR_RD_BASE \
						(AVIO_DHUB_MetaData_RD_BASE +\
						AVIO_DHUB_MetaData_RD_SIZE)

#endif /*__AVIO_DHUB_CFG_H__*/
