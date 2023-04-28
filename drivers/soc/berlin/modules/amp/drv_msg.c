/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include "amp_type.h"
#include "drv_msg.h"
#include "drv_debug.h"

HRESULT AMPMsgQ_Add(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg)
{
	INT wr_offset;

	if (NULL == pMsgQ->pMsg || pMsg == NULL)
		return S_FALSE;

	wr_offset = pMsgQ->wr_number - pMsgQ->rd_number;

	if (wr_offset == -1 || wr_offset == (pMsgQ->q_length - 2)) {
		return S_FALSE;
	} else {
		memcpy((CHAR *) & pMsgQ->pMsg[pMsgQ->wr_number], (CHAR *) pMsg,
		       sizeof(MV_CC_MSG_t));
		pMsgQ->wr_number++;
		pMsgQ->wr_number %= pMsgQ->q_length;
	}

	return S_OK;
}

HRESULT AMPMsgQ_ReadTry(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg)
{
	INT rd_offset;

	if (NULL == pMsgQ->pMsg || pMsg == NULL)
		return S_FALSE;

	rd_offset = pMsgQ->rd_number - pMsgQ->wr_number;

	if (rd_offset != 0) {
		memcpy((CHAR *) pMsg, (CHAR *) & pMsgQ->pMsg[pMsgQ->rd_number],
		       sizeof(MV_CC_MSG_t));
		return S_OK;
	} else {
		amp_trace("read message queue failed r: %d w: %d\n",
			  pMsgQ->rd_number, pMsgQ->wr_number);
		return S_FALSE;
	}
}

HRESULT AMPMsgQ_ReadFinish(AMPMsgQ_t * pMsgQ)
{
	INT rd_offset;

	rd_offset = pMsgQ->rd_number - pMsgQ->wr_number;

	if (rd_offset != 0) {
		pMsgQ->rd_number++;
		pMsgQ->rd_number %= pMsgQ->q_length;

		return S_OK;
	} else {
		return S_FALSE;
	}
}

HRESULT AMPMsgQ_Destroy(AMPMsgQ_t * pMsgQ)
{
	if (pMsgQ == NULL) {
		return E_FAIL;
	}

	if (pMsgQ->pMsg) {
		kfree(pMsgQ->pMsg);
	}
	return S_OK;
}

INT AMPMsgQ_DequeueRead(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg)
{
	INT fullness;

	if (NULL == pMsgQ->pMsg || pMsg == NULL)
		return S_FALSE;

	fullness = pMsgQ->wr_number - pMsgQ->rd_number;
	if (fullness < 0) {
		fullness += pMsgQ->q_length;
	}

	if (fullness) {
		if (pMsg)
			memcpy((void *)pMsg,
			       (void *)&pMsgQ->pMsg[pMsgQ->rd_number],
			       sizeof(MV_CC_MSG_t));

		pMsgQ->rd_number++;
		if (pMsgQ->rd_number >= pMsgQ->q_length)
			pMsgQ->rd_number -= pMsgQ->q_length;

		return 1;
	}

	return 0;
}

INT AMPMsgQ_Fullness(AMPMsgQ_t * pMsgQ)
{
	INT fullness;

	fullness = pMsgQ->wr_number - pMsgQ->rd_number;
	if (fullness < 0) {
		fullness += pMsgQ->q_length;
	}

	return fullness;
}

HRESULT AMPMsgQ_Init(AMPMsgQ_t * pAMPMsgQ, UINT q_length)
{
	pAMPMsgQ->q_length = q_length;
	pAMPMsgQ->rd_number = pAMPMsgQ->wr_number = 0;
	pAMPMsgQ->pMsg =
	    (MV_CC_MSG_t *) kmalloc(sizeof(MV_CC_MSG_t) * q_length, GFP_ATOMIC);

	if (pAMPMsgQ->pMsg == NULL) {
		return E_OUTOFMEMORY;
	}

	pAMPMsgQ->Destroy = AMPMsgQ_Destroy;
	pAMPMsgQ->Add = AMPMsgQ_Add;
	pAMPMsgQ->ReadTry = AMPMsgQ_ReadTry;
	pAMPMsgQ->ReadFin = AMPMsgQ_ReadFinish;
//    pAMPMsgQ->Fullness =   AMPMsgQ_Fullness;

	return S_OK;
}

void AMPMsgQ_Post(AMPMsgQ_t * pMsgQ, INT id)
{
	MV_CC_MSG_t msg = { 0, };
	msg.m_MsgID = (1 << id);
	AMPMsgQ_Add(pMsgQ, &msg);
}
