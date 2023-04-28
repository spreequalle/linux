/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#include "api_avio_dhub.h"

DHUB_channel_config VPP_config[VPP_NUM_OF_CHANNELS] = {
	// BANK0
	{avioDhubChMap_vpp64b_MV_R0, VPP_DHUB_BANK0_START_ADDR,
	 VPP_DHUB_BANK0_START_ADDR + 32, 32, (4096 - 32),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp64b_MV_R1, VPP_DHUB_BANK0_START_ADDR + 4096,
	 VPP_DHUB_BANK0_START_ADDR + 4096 + 32, 32, (2048 - 32),
	 dHubChannel_CFG_MTU_128byte, 1, 0, 1}
	,
	{avioDhubChMap_vpp64b_LDC_W0, VPP_DHUB_BANK0_START_ADDR + 6144,
	 VPP_DHUB_BANK0_START_ADDR + 6144 + 32, 32, (2048 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	// BANK1
	{avioDhubChMap_vpp64b_DEINT_R0, VPP_DHUB_BANK1_START_ADDR,
	 VPP_DHUB_BANK1_START_ADDR + 32, 32, (8192 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	// BANK2
	{avioDhubChMap_vpp64b_DEINT_R1, VPP_DHUB_BANK2_START_ADDR,
	 VPP_DHUB_BANK2_START_ADDR + 32, 32, (6144 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_vpp64b_OFFLINE_W0, VPP_DHUB_BANK2_START_ADDR + 6144,
	 VPP_DHUB_BANK2_START_ADDR + 6144 + 32, 32, (2048 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	// BANK3
	{avioDhubChMap_vpp64b_OFFLINE_R0, VPP_DHUB_BANK3_START_ADDR,
	 VPP_DHUB_BANK3_START_ADDR + 32, 32, (2048 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_vpp64b_DEINT_W0, VPP_DHUB_BANK3_START_ADDR + 2048,
	 VPP_DHUB_BANK3_START_ADDR + 2048 + 32, 32, (6144 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
};

DHUB_channel_config AG_config[AG_NUM_OF_CHANNELS] = {
	// Bank0
	{avioDhubChMap_aio64b_MA_R0, AG_DHUB_BANK0_START_ADDR,
	 AG_DHUB_BANK0_START_ADDR + 32, 32, (1024 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA_R1, AG_DHUB_BANK0_START_ADDR + 1024,
	 AG_DHUB_BANK0_START_ADDR + 1024 + 32, 32, (1024 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA_R2, AG_DHUB_BANK0_START_ADDR + 1024 * 2,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 2 + 32, 32, (1024 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_MA_R3, AG_DHUB_BANK0_START_ADDR + 1024 * 3,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 3 + 32, 32, (1024 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_SA_R, AG_DHUB_BANK0_START_ADDR + 1024 * 4,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 4 + 32, 32, (1024 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_SPDIF_TX_R, AG_DHUB_BANK0_START_ADDR + 1024 * 5,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 5 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_HDMI_RD,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 5 + 512,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 5 + 512 + 32, 32, (1024 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_BCM_R, AG_DHUB_BANK0_START_ADDR + 1024 * 6 + 512,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 6 + 512 + 512, 512, (1024 - 512),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_SPDIF_RX_W,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 7 + 512,
	 AG_DHUB_BANK0_START_ADDR + 1024 * 7 + 512 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,

	{avioDhubChMap_aio64b_IG_RD, AG_DHUB_BANK1_START_ADDR,
	 AG_DHUB_BANK1_START_ADDR + 32, 32, (8192 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_PG_RD, AG_DHUB_BANK2_START_ADDR,
	 AG_DHUB_BANK2_START_ADDR + 32, 32, (8192 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avioDhubChMap_aio64b_CURSOR_RD, AG_DHUB_BANK3_START_ADDR,
	 AG_DHUB_BANK3_START_ADDR + 32, 32, (8192 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
};
