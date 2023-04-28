/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include "api_avif_dhub.h"

HDL_dhub2d AVIF_dhubHandle;

DHUB_channel_config AVIF_config[AVIF_NUM_OF_CHANNELS] = {
//BANK 0
	{avif_dhub_config_ChMap_avif_SD_WR, AVIF_DHUB_BANK0_START_ADDR,
	 AVIF_DHUB_BANK0_START_ADDR + 32, 32, (4096 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avif_dhub_config_ChMap_avif_SD_RE, AVIF_DHUB_BANK0_START_ADDR + 4096,
	 AVIF_DHUB_BANK0_START_ADDR + 4096 + 32, 32, (4096 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
//BANK 1
	{avif_dhub_config_ChMap_avif_MAIN_WR, AVIF_DHUB_BANK1_START_ADDR,
	 AVIF_DHUB_BANK1_START_ADDR + 32, 32, (8192 + 4096 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
//BANK 2
	{avif_dhub_config_ChMap_avif_VBI_WR, AVIF_DHUB_BANK2_START_ADDR,
	 AVIF_DHUB_BANK2_START_ADDR + 32, 32, (1024 - 32),
	 dHubChannel_CFG_MTU_8byte, 0, 0, 1}
	,
	{avif_dhub_config_ChMap_avif_AUD_WR0_MAIN,
	 AVIF_DHUB_BANK2_START_ADDR + 1024,
	 AVIF_DHUB_BANK2_START_ADDR + 1024 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avif_dhub_config_ChMap_avif_AUD_WR1_MAIN,
	 AVIF_DHUB_BANK2_START_ADDR + 1024 + 512,
	 AVIF_DHUB_BANK2_START_ADDR + 1024 + 512 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avif_dhub_config_ChMap_avif_AUD_WR2_MAIN,
	 AVIF_DHUB_BANK2_START_ADDR + 2048,
	 AVIF_DHUB_BANK2_START_ADDR + 2048 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avif_dhub_config_ChMap_avif_AUD_WR3_MAIN,
	 AVIF_DHUB_BANK2_START_ADDR + 2048 + 512,
	 AVIF_DHUB_BANK2_START_ADDR + 2048 + 512 + 32, 32, (512 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
	{avif_dhub_config_ChMap_avif_AUD_RD0, AVIF_DHUB_BANK2_START_ADDR + 3072,
	 AVIF_DHUB_BANK2_START_ADDR + 3072 + 32, 32, (3072 - 32),
	 dHubChannel_CFG_MTU_128byte, 0, 0, 1}
	,
};
