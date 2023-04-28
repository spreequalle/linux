/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "tee_ca_dhub.h"
#include "hal_dhub_wrap.h"

extern unsigned char gAmp_EnableTEE;
extern unsigned int berlin_chip_revision;

void wrap_DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr,
			     UNSG32 hboSramAddr, HDL_dhub2d * pdhubHandle,
			     DHUB_channel_config * dhub_config,
			     SIGN32 numOfChans)
{
	if (gAmp_EnableTEE) {
		if (dHubBaseAddr == VPP_DHUB_BASE) {
			tz_DhubInitialization(cpuId, dHubBaseAddr, hboSramAddr, 0, 0,
					      numOfChans);
		} else {
			DhubInitialization(cpuId, dHubBaseAddr, hboSramAddr,
					   pdhubHandle, dhub_config, numOfChans);
		}
	} else {
		DhubInitialization(cpuId, dHubBaseAddr, hboSramAddr, pdhubHandle,
				   dhub_config, numOfChans);
	}
}

void wrap_DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[])
{
	if (gAmp_EnableTEE) {
		if (hdl == &VPP_dhubHandle.dhub)
			tz_DhubChannelClear(hdl, id, cfgQ);
		else
			DhubChannelClear(hdl, id, cfgQ);
	} else {
		DhubChannelClear(hdl, id, cfgQ);
	}
}

UNSG32 wrap_dhub_channel_write_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
				   SIGN32 id,	/*! Channel ID in $dHubReg ! */
				   UNSG32 addr,	/*! CMD: buffer address ! */
				   SIGN32 size,	/*! CMD: number of bytes to transfer ! */
				   SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
				   SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
				   SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
				   SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
				   T64b cfgQ[],	/*! Pass NULL to directly update dHub, or
						   Pass non-zero to receive programming sequence
						   in (adr,data) pairs
						   ! */
				   UNSG32 * ptr	/*! Pass in current cmdQ pointer (in 64b word),
						   & receive updated new pointer,
						   Pass NULL to read from HW
						   ! */
    )
{
	if (gAmp_EnableTEE) {
		if (hdl == &VPP_dhubHandle.dhub) {
			return tz_dhub_channel_write_cmd(hdl, id, addr, size, semOnMTU,
							 chkSemId, updSemId, interrupt,
							 cfgQ, ptr);
		} else {
			return dhub_channel_write_cmd(hdl, id, addr, size, semOnMTU,
						      chkSemId, updSemId, interrupt,
						      cfgQ, ptr);
		}
	} else {
		return dhub_channel_write_cmd(hdl, id, addr, size, semOnMTU, chkSemId,
					      updSemId, interrupt, cfgQ, ptr);
	}
}

void wrap_dhub_channel_generate_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
				    SIGN32 id,	/*! Channel ID in $dHubReg ! */
				    UNSG32 addr,	/*! CMD: buffer address ! */
				    SIGN32 size,	/*! CMD: number of bytes to transfer ! */
				    SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
				    SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
				    SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
				    SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
				    SIGN32 * pData)
{
	if (gAmp_EnableTEE) {
		if (hdl == &VPP_dhubHandle.dhub) {
			tz_dhub_channel_generate_cmd(hdl, id, addr, size, semOnMTU,
						     chkSemId, updSemId, interrupt,
						     pData);
		} else {
			dhub_channel_generate_cmd(hdl, id, addr, size, semOnMTU,
						  chkSemId, updSemId, interrupt, pData);
		}
	} else {
		dhub_channel_generate_cmd(hdl, id, addr, size, semOnMTU, chkSemId,
					  updSemId, interrupt, pData);
	}
}

void *wrap_dhub_semaphore(void *hdl	/*  Handle to HDL_dhub */
    )
{

	if (gAmp_EnableTEE) {
		if (hdl == &VPP_dhubHandle.dhub)
			return hdl;
		else
			return dhub_semaphore(hdl);
	} else {
		return dhub_semaphore(hdl);
	}
}

void wrap_semaphore_pop(void *hdl,	/*  Handle to HDL_semaphore */
			SIGN32 id,	/*  Semaphore ID in $SemaHub */
			SIGN32 delta	/*  Delta to pop as a consumer */
    )
{

	if (gAmp_EnableTEE) {
		if (hdl == &VPP_dhubHandle.dhub)
			tz_semaphore_pop(hdl, id, delta);
		else
			semaphore_pop(hdl, id, delta);
	} else {
		semaphore_pop(hdl, id, delta);
	}
}

void wrap_semaphore_clr_full(void *hdl,	/*  Handle to HDL_semaphore */
			     SIGN32 id	/*  Semaphore ID in $SemaHub */
    )
{
	if (gAmp_EnableTEE) {
		if (hdl == &VPP_dhubHandle.dhub)
			tz_semaphore_clr_full(hdl, id);
		else
			semaphore_clr_full(hdl, id);
	} else {
		semaphore_clr_full(hdl, id);
	}
}

UNSG32 wrap_semaphore_chk_full(void *hdl,	/*Handle to HDL_semaphore */
			       SIGN32 id	/*Semaphore ID in $SemaHub
						   -1 to return all 32b of the interrupt status
						 */
    )
{
	if (gAmp_EnableTEE) {
		if (hdl == &VPP_dhubHandle.dhub)
			return tz_semaphore_chk_full(hdl, id);
		else
			return semaphore_chk_full(hdl, id);
	} else {
		return semaphore_chk_full(hdl, id);
	}
}

void wrap_semaphore_intr_enable(void *hdl,	/*! Handle to HDL_semaphore ! */
				SIGN32 id,	/*! Semaphore ID in $SemaHub ! */
				SIGN32 empty,	/*! Interrupt enable for CPU at condition 'empty' ! */
				SIGN32 full,	/*! Interrupt enable for CPU at condition 'full' ! */
				SIGN32 almostEmpty,	/*! Interrupt enable for CPU at condition 'almostEmpty' ! */
				SIGN32 almostFull,	/*! Interrupt enable for CPU at condition 'almostFull' ! */
				SIGN32 cpu	/*! CPU ID (0/1/2) ! */
    )
{
	if (gAmp_EnableTEE) {
		if (hdl == &VPP_dhubHandle.dhub) {
			tz_semaphore_intr_enable(hdl, id, empty, full, almostEmpty,
						 almostFull, cpu);
		} else {
			semaphore_intr_enable(hdl, id, empty, full, almostEmpty,
					      almostFull, cpu);
		}
	} else {
		semaphore_intr_enable(hdl, id, empty, full, almostEmpty, almostFull,
				      cpu);
	}
}
