/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>

#include "galois_io.h"
#include "cinclude.h"
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_tsp.h"
#include "drv_msg.h"
#include "drv_debug.h"
#include "amp_memmap.h"

static TSP_CTX tsp_ctx;

irqreturn_t amp_devices_tsp_isr(INT irq, VOID * dev_id)
{
	TSP_CTX *hTspCtx = (TSP_CTX *) dev_id;
	MV_CC_MSG_t msg = { 0 };
	UNSG32 addr, val;
	HRESULT rc;

	addr =
	    MEMMAP_TSP_REG_BASE + RA_TspReg_IntReg +
	    RA_TspIntReg_software_int_status;
	GA_REG_WORD32_READ(addr, &val);

	if (likely(val != 0)) {
		GA_REG_WORD32_WRITE(addr, val);

		msg.m_MsgID = val & 0xffff;
		rc = AMPMsgQ_Add(&hTspCtx->hTSPMsgQ, &msg);
		if (likely(rc == S_OK)) {
			up(&hTspCtx->tsp_sem);
		}
	}

	return IRQ_HANDLED;
}

VOID drv_tsp_open(TSP_CTX * hTspCtx, UINT mask)
{
	UNSG32 addr;

	addr =
	    MEMMAP_TSP_REG_BASE + RA_TspReg_IntReg +
	    RA_TspIntReg_software_int_enable;
	GA_REG_WORD32_WRITE(addr, mask);

	return;
}

VOID drv_tsp_close(TSP_CTX * hTspCtx)
{
	MV_CC_MSG_t msg = { 0 };
	UNSG32 addr;
	INT err;

	addr =
	    MEMMAP_TSP_REG_BASE + RA_TspReg_IntReg +
	    RA_TspIntReg_software_int_enable;
	GA_REG_WORD32_WRITE(addr, 0);
	addr =
	    MEMMAP_TSP_REG_BASE + RA_TspReg_IntReg +
	    RA_TspIntReg_software_int_status;
	GA_REG_WORD32_WRITE(addr, 0);

	do {
		err = AMPMsgQ_DequeueRead(&hTspCtx->hTSPMsgQ, &msg);
	} while (likely(err == 1));

	sema_init(&hTspCtx->tsp_sem, 0);

	return;
}

TSP_CTX *drv_tsp_init(VOID)
{
	UINT err;
	TSP_CTX *hTspCtx = &tsp_ctx;

	memset(hTspCtx, 0, sizeof(TSP_CTX));
	sema_init(&hTspCtx->tsp_sem, 0);
	err = AMPMsgQ_Init(&hTspCtx->hTSPMsgQ, TSP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("drv_tsp_init: hTSPMsgQ init failed, err:%8x\n", err);
		return NULL;
	}

	return hTspCtx;
}

VOID drv_tsp_exit(TSP_CTX * hTspCtx)
{
	UINT err;

	err = AMPMsgQ_Destroy(&hTspCtx->hTSPMsgQ);
	if (unlikely(err != S_OK)) {
		amp_error("drv_tsp_exit: TSP MsgQ Destroy failed, err:%8x\n",
			  err);
	}

	return;
}
