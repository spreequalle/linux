/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __OVP_CMD_H__
#define __OVP_CMD_H__

#define TAOVP_UUID {0x63991704, 0xFACB, 0xabcd, {0xBF, 0xFB, 0xC8, 0xD6, 0x7F, 0xBF, 0x88, 0x91}}

#define UINT32  unsigned int
#define INT     int
#define HRESULT int
#define S_OK    0

/* enum for VPP commands */
typedef enum {
	OVP_CREATE,
	OVP_DESTROY,
	OVP_GETFRAMESWAIT,
	OVP_PASSSHM,
	OVP_GET_CLR_INTR_STS,
} OVP_CMD_ID;

int OvpInitialize(void);
void OvpCloseSession(void);
int OvpGet_Clr_IntrSts(unsigned int *puiIntrSts);
#endif
