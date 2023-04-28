/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include "api_avio_dhub.h"

#ifdef BG4CDP_OPTIMIZE_SRAM_ALLOCATION
DHUB_channel_config VPP_config[VPP_NUM_OF_CHANNELS] = {
	// BANK0
	{avioDhubChMap_vpp128b_MV_R0, VPP_DHUB_BANK0_START_ADDR,
	 VPP_DHUB_BANK0_START_ADDR + 64, 64, (1024 * 10 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_VMM_VX_R, VPP_DHUB_BANK0_START_ADDR + 1024 * 10,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 10 + 64, 64, (1024 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	// BANK1
	{avioDhubChMap_vpp128b_MV_R1, VPP_DHUB_BANK0_START_ADDR + 1024 * 11,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 11 + 64, 64, (1024 * 5 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT_W, VPP_DHUB_BANK0_START_ADDR + 1024 * 16,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 16 + 64, 64, (512 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT0_R,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 16 + 512,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 16 + 512 + 64, 64, (512 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	//BANK2
	{avioDhubChMap_vpp128b_GFX_PIP_R0,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 17,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 17 + 64, 64, (1024 * 12 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_PIP_R1, VPP_DHUB_BANK0_START_ADDR + 1024 * 29,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 29 + 64, 64, (1024 * 4 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	//BANK3
	{avioDhubChMap_vpp128b_GFX_R1, VPP_DHUB_BANK0_START_ADDR + 1024 * 33,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 33 + 64, 64, (1024 * 16 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT1_R, VPP_DHUB_BANK0_START_ADDR + 1024 * 49,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 49 + 64, 64, (512 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_1DSCL_W0,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 49 + 512,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 49 + 512 + 64, 64, (256 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_1DSCL_W1,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 49 + 512 + 256,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 49 + 512 + 256 + 64, 64, (256 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
};
#else //!BG4CDP_OPTIMIZE_SRAM_ALLOCATION
DHUB_channel_config VPP_config[VPP_NUM_OF_CHANNELS] = {
	// BANK0
	{avioDhubChMap_vpp128b_MV_R0, VPP_DHUB_BANK0_START_ADDR,
	 VPP_DHUB_BANK0_START_ADDR + 64, 64, (1024 * 11 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_VMM_VX_R, VPP_DHUB_BANK0_START_ADDR + 1024 * 11,
	 VPP_DHUB_BANK0_START_ADDR + 1024 * 11 + 64, 64, (1024 + 512 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	// BANK1
	{avioDhubChMap_vpp128b_MV_R1, VPP_DHUB_BANK1_START_ADDR,
	 VPP_DHUB_BANK1_START_ADDR + 64, 64, (1024 * 4 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT_W, VPP_DHUB_BANK1_START_ADDR + 1024 * 4,
	 VPP_DHUB_BANK1_START_ADDR + 1024 * 4 + 64, 64, (1024 * 4 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT0_R, VPP_DHUB_BANK1_START_ADDR + 1024 * 8,
	 VPP_DHUB_BANK1_START_ADDR + 1024 * 8 + 64, 64, (1024 * 4 + 512 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	//BANK2
	{avioDhubChMap_vpp128b_GFX_PIP_R0, VPP_DHUB_BANK2_START_ADDR,
	 VPP_DHUB_BANK2_START_ADDR + 64, 64, (1024 * 8 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_PIP_R1, VPP_DHUB_BANK2_START_ADDR + 1024 * 8,
	 VPP_DHUB_BANK2_START_ADDR + 1024 * 8 + 64, 64, (1024 * 4 + 512 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	//BANK3
	{avioDhubChMap_vpp128b_GFX_R1, VPP_DHUB_BANK3_START_ADDR,
	 VPP_DHUB_BANK3_START_ADDR + 64, 64, (1024 * 6 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT1_R, VPP_DHUB_BANK3_START_ADDR + 1024 * 6,
	 VPP_DHUB_BANK3_START_ADDR + 1024 * 6 + 64, 64, (1024 * 2 + 512 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_1DSCL_W0,
	 VPP_DHUB_BANK3_START_ADDR + 1024 * 8 + 512,
	 VPP_DHUB_BANK3_START_ADDR + 1024 * 8 + 512 + 64, 64, (1024 * 2 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_1DSCL_W1,
	 VPP_DHUB_BANK3_START_ADDR + 1024 * 10 + 512,
	 VPP_DHUB_BANK3_START_ADDR + 1024 * 10 + 512 + 64, 64, (1024 * 2 - 64),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
};
#endif //!BG4CDP_OPTIMIZE_SRAM_ALLOCATION

DHUB_channel_config AG_config[AG_NUM_OF_CHANNELS] = {
	// Bank0
	{avioDhubChMap_aio64b_MA0_R, AG_DHUB_BANK0_START_ADDR,
	 AG_DHUB_BANK0_START_ADDR + 32, 32, (256 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA1_R, AG_DHUB_BANK0_START_ADDR + 256,
	 AG_DHUB_BANK0_START_ADDR + 256 + 32, 32, (256 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA2_R, AG_DHUB_BANK0_START_ADDR + 256 * 2,
	 AG_DHUB_BANK0_START_ADDR + 256 * 2 + 32, 32, (256 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA3_R, AG_DHUB_BANK0_START_ADDR + 256 * 3,
	 AG_DHUB_BANK0_START_ADDR + 256 * 3 + 32, 32, (256 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_SA_R, AG_DHUB_BANK0_START_ADDR + 256 * 4,
	 AG_DHUB_BANK0_START_ADDR + 256 * 4 + 32, 32, (256 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_PDM_MIC_CH1_W, AG_DHUB_BANK0_START_ADDR + 256 * 5,
	 AG_DHUB_BANK0_START_ADDR + 256 * 5 + 32, 32, (256 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_PDM_MIC_CH2_W, AG_DHUB_BANK0_START_ADDR + 256 * 6,
	 AG_DHUB_BANK0_START_ADDR + 256 * 6 + 32, 32, (256 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMI_R, AG_DHUB_BANK0_START_ADDR + 256 * 7,
	 AG_DHUB_BANK0_START_ADDR + 256 * 7 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_BCM_R, AG_DHUB_BANK0_START_ADDR + 256 * 9,
	 AG_DHUB_BANK0_START_ADDR + 256 * 9 + 128, 128, (512 - 128),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_TT_R, AG_DHUB_BANK0_START_ADDR + 256 * 11,
	 AG_DHUB_BANK0_START_ADDR + 256 * 11 + 32, 32, (256 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMIRX_CH3_W, AG_DHUB_BANK0_START_ADDR + 512 * 6,
	 AG_DHUB_BANK0_START_ADDR + 512 * 6 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMIRX_CH4_W, AG_DHUB_BANK0_START_ADDR + 512 * 7,
	 AG_DHUB_BANK0_START_ADDR + 512 * 7 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMIRX_CH5_W, AG_DHUB_BANK0_START_ADDR + 512 * 8,
	 AG_DHUB_BANK0_START_ADDR + 512 * 8 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMIRX_CH6_W, AG_DHUB_BANK0_START_ADDR + 512 * 9,
	 AG_DHUB_BANK0_START_ADDR + 512 * 9 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_SPDIF_R, AG_DHUB_BANK0_START_ADDR + 512 * 10,
	 AG_DHUB_BANK0_START_ADDR + 512 * 10 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_SPDIF_W, AG_DHUB_BANK0_START_ADDR + 512 * 11,
	 AG_DHUB_BANK0_START_ADDR + 512 * 11 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
};

DHUB_channel_config  VPP_config_A0[VPP_NUM_OF_CHANNELS] = {
	{avioDhubChMap_vpp128b_MV_R0, AVIO_DHUB_MV_R0_BASE,
	AVIO_DHUB_MV_R0_BASE+64, 64, (AVIO_DHUB_MV_R0_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_VMM_VX_R, AVIO_DHUB_VMM_VX_R_BASE,
	AVIO_DHUB_VMM_VX_R_BASE+64, 64, (AVIO_DHUB_VMM_VX_R_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_MV_R1, AVIO_DHUB_MV_R1_BASE,
	AVIO_DHUB_MV_R1_BASE+64, 64, (AVIO_DHUB_MV_R1_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT_W, AVIO_DHUB_DINT_W_BASE,
	AVIO_DHUB_DINT_W_BASE+64, 64, (AVIO_DHUB_DINT_W_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT0_R, AVIO_DHUB_DINT0_R_BASE,
	AVIO_DHUB_DINT0_R_BASE+64, 64, (AVIO_DHUB_DINT0_R_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_GFX_PIP_R0, AVIO_DHUB_GFX_PIP_R0_BASE,
	AVIO_DHUB_GFX_PIP_R0_BASE+64, 64, (AVIO_DHUB_GFX_PIP_R0_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_PIP_R1, AVIO_DHUB_PIP_R1_BASE,
	AVIO_DHUB_PIP_R1_BASE+64, 64, (AVIO_DHUB_PIP_R1_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_GFX_R1, AVIO_DHUB_GFX_R1_BASE,
	AVIO_DHUB_GFX_R1_BASE+64, 64, (AVIO_DHUB_GFX_R1_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DINT1_R, AVIO_DHUB_DINT1_R_BASE,
	AVIO_DHUB_DINT1_R_BASE+64, 64, (AVIO_DHUB_DINT1_R_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_1DSCL_W0, AVIO_DHUB_1DSCL_W0_BASE,
	AVIO_DHUB_1DSCL_W0_BASE+64, 64, (AVIO_DHUB_1DSCL_W0_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_1DSCL_W1, AVIO_DHUB_1DSCL_W1_BASE,
	AVIO_DHUB_1DSCL_W1_BASE+64, 64, (AVIO_DHUB_1DSCL_W1_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_MetaData_RD, AVIO_DHUB_MetaData_RD_BASE,
	AVIO_DHUB_MetaData_RD_BASE+64, 64, (AVIO_DHUB_MetaData_RD_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp128b_DBEDR_RD, AVIO_DHUB_DBEDR_RD_BASE,
	AVIO_DHUB_DBEDR_RD_BASE+64, 64, (AVIO_DHUB_DBEDR_RD_SIZE-64),
	dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
};

DHUB_channel_config  AG_config_A0[AG_NUM_OF_CHANNELS] = {
/* Bank0 */
	{avioDhubChMap_aio64b_MA0_R, AG_DHUB_BANK0_START_ADDR,
	AG_DHUB_BANK0_START_ADDR+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA1_R, AG_DHUB_BANK0_START_ADDR+512,
	AG_DHUB_BANK0_START_ADDR+512+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA2_R, AG_DHUB_BANK0_START_ADDR+512*2,
	AG_DHUB_BANK0_START_ADDR+512*2+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA3_R, AG_DHUB_BANK0_START_ADDR+512*3,
	AG_DHUB_BANK0_START_ADDR+512*3+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
/*  avioDhubChMap_aio64b_PDM_MIC_CH3_W renamed as
	avioDhubChMap_aio64b_SA_R  due to Z1/A0 common header file */
	{avioDhubChMap_aio64b_SA_R, AG_DHUB_BANK0_START_ADDR+512*4,
	AG_DHUB_BANK0_START_ADDR+512*4+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_PDM_MIC_CH1_W, AG_DHUB_BANK0_START_ADDR+512*5,
	AG_DHUB_BANK0_START_ADDR+512*5+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_PDM_MIC_CH2_W, AG_DHUB_BANK0_START_ADDR+512*6,
	AG_DHUB_BANK0_START_ADDR+512*6+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMI_R, AG_DHUB_BANK0_START_ADDR+512*7,
	AG_DHUB_BANK0_START_ADDR+512*7+32, 32, (2048-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_BCM_R, AG_DHUB_BANK0_START_ADDR+512*11,
	AG_DHUB_BANK0_START_ADDR+512*11+128, 128, (512-128),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
/*  avioDhubChMap_aio64b_PDM_MIC_CH4_W renamed as
	avioDhubChMap_aio64b_TT_R  due to Z1/A0 common header file */
	{avioDhubChMap_aio64b_TT_R, AG_DHUB_BANK0_START_ADDR+512*12,
	AG_DHUB_BANK0_START_ADDR+512*12+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMIRX_CH3_W, AG_DHUB_BANK0_START_ADDR+512*13,
	AG_DHUB_BANK0_START_ADDR+512*13+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMIRX_CH4_W, AG_DHUB_BANK0_START_ADDR+512*14,
	AG_DHUB_BANK0_START_ADDR+512*14+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMIRX_CH5_W, AG_DHUB_BANK0_START_ADDR+512*15,
	AG_DHUB_BANK0_START_ADDR+512*15+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMIRX_CH6_W, AG_DHUB_BANK0_START_ADDR+512*16,
	AG_DHUB_BANK0_START_ADDR+512*16+32, 32, (512-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_SPDIF_R, AG_DHUB_BANK0_START_ADDR+1024*8+512,
	AG_DHUB_BANK0_START_ADDR+1024*8+512+32, 32, (2048-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_SPDIF_W, AG_DHUB_BANK0_START_ADDR+1024*10+512,
	AG_DHUB_BANK0_START_ADDR+1024*10+512+32, 32, (2048-32),
	dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
};



