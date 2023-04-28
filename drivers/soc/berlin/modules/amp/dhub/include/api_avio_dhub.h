/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __API_AVIO_DHUB_H__
#define __API_AVIO_DHUB_H__

#include "dHub.h"
#include "avioDhub.h"
#include "api_dhub.h"
#include "Galois_memmap.h"
#include "avio_dhub_cfg.h"

typedef struct DHUB_channel_config {
	SIGN32 chanId;
	UNSG32 chanCmdBase;
	UNSG32 chanDataBase;
	SIGN32 chanCmdSize;
	SIGN32 chanDataSize;
	SIGN32 chanMtuSize;
	SIGN32 chanQos;
	SIGN32 chanSelfLoop;
	SIGN32 chanEnable;
} DHUB_channel_config;

extern HDL_dhub2d AG_dhubHandle;
extern HDL_dhub2d VPP_dhubHandle;
#ifdef CONFIG_MV_AMP_COMPONENT_VIP_ENABLE
extern HDL_dhub2d VIP_dhubHandle;
#endif
#if (BERLIN_CHIP_VERSION == BERLIN_BG2_DTV_A0)
extern HDL_dhub2d VPP_Dh3_dhubHandle;
extern DHUB_channel_config AVIO_DHUB3_config[];
extern DHUB_channel_config AVIO_DHUB3_config_MOSD_R0[];
extern DHUB_channel_config AVIO_DHUB3_config_MOSD_R0_R1[];
#endif
extern DHUB_channel_config AG_config[];
extern DHUB_channel_config VPP_config[];
#ifdef CONFIG_MV_AMP_COMPONENT_VIP_ENABLE
extern DHUB_channel_config VIP_config[];
#endif // CONFIG_MV_AMP_COMPONENT_VIP_ENABLE

/******************************************************************************************************************
 *	Function: DhubInitialization
 *	Description: Initialize DHUB .
 *	Parameter : cpuId ------------- cpu ID
 *             dHubBaseAddr -------------  dHub Base address.
 *             hboSramAddr ----- Sram Address for HBO.
 *             pdhubHandle ----- pointer to 2D dhubHandle
 *             dhub_config ----- configuration of AG
 *             numOfChans	 ----- number of channels
 *	Return:		void
******************************************************************************************************************/
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr,
			HDL_dhub2d * pdhubHandle,
			DHUB_channel_config * dhub_config, SIGN32 numOfChans);

void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);

int getDhubChannelInfo(HDL_dhub2d * pdhubHandle, SIGN32 IChannel,
		       T32dHubChannel_CFG * cfg);

void DhubInitialization_128BytesAXIChannel(HDL_dhub2d * pdhubHandle,
					   DHUB_channel_config * dhub_config,
					   UNSG8 uchNumChannels,
					   UNSG32 uiBcmBuf);

void DhubInitialization_128BytesAXI(SIGN32 cpuId, UNSG32 dHubBaseAddr,
				    UNSG32 hboSramAddr,
				    HDL_dhub2d * pdhubHandle,
				    DHUB_channel_config * dhub_config,
				    UNSG8 numOfChans);

#endif //__API_AVIO_DHUB_H__
