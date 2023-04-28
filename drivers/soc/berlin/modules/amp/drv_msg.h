/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_MSG_H_
#define _DRV_MSG_H_
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION (3,10,0)
#include <../drivers/staging/android/ion/ion.h>
#else
#include <linux/ion.h>
#endif

#include "amp_type.h"

typedef struct amp_message_queue {
	UINT q_length;
	UINT rd_number;
	UINT wr_number;
	MV_CC_MSG_t *pMsg;

	 HRESULT(*Add) (struct amp_message_queue * pMsgQ, MV_CC_MSG_t * pMsg);
	 HRESULT(*ReadTry) (struct amp_message_queue * pMsgQ,
			    MV_CC_MSG_t * pMsg);
	 HRESULT(*ReadFin) (struct amp_message_queue * pMsgQ);
	 HRESULT(*Destroy) (struct amp_message_queue * pMsgQ);
} AMPMsgQ_t;

INT AMPMsgQ_DequeueRead(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg);
INT AMPMsgQ_Fullness(AMPMsgQ_t * pMsgQ);
HRESULT AMPMsgQ_Init(AMPMsgQ_t * pAMPMsgQ, UINT q_length);
HRESULT AMPMsgQ_Destroy(AMPMsgQ_t * pMsgQ);
void AMPMsgQ_Post(AMPMsgQ_t * pMsgQ, INT id);
HRESULT AMPMsgQ_Add(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg);
HRESULT AMPMsgQ_ReadTry(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg);
HRESULT AMPMsgQ_ReadFinish(AMPMsgQ_t * pMsgQ);

#endif //_DRV_MSG_H_
