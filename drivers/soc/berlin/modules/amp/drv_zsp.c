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
#include "drv_zsp.h"
#include "drv_msg.h"
#include "drv_debug.h"
#include "api_dhub.h"
#include "api_avio_dhub.h"
#include "hal_dhub_wrap.h"
#include "zspWrapper.h"
#include "amp_memmap.h"

static ZSP_CTX zsp_ctx;

irqreturn_t amp_devices_zsp_isr(INT irq, VOID * dev_id)
{
	UNSG32 addr, v_id;
	T32ZspInt2Soc_status reg;
	MV_CC_MSG_t msg = { 0 };
	ZSP_CTX *hZspCtx = (ZSP_CTX *) dev_id;

	addr = MEMMAP_ZSP_REG_BASE + RA_ZspRegs_Int2Soc + RA_ZspInt2Soc_status;
	GA_REG_WORD32_READ(addr, &(reg.u32));

	addr = MEMMAP_ZSP_REG_BASE + RA_ZspRegs_Int2Soc + RA_ZspInt2Soc_clear;
	v_id = ADSP_ZSPINT2Soc_IRQ0;
	if ((reg.u32) & (1 << v_id)) {
		GA_REG_WORD32_WRITE(addr, v_id);
	}

	msg.m_MsgID = 1 << ADSP_ZSPINT2Soc_IRQ0;
	AMPMsgQ_Add(&hZspCtx->hZSPMsgQ, &msg);
	up(&hZspCtx->zsp_sem);

	return IRQ_HANDLED;
}

ZSP_CTX *drv_zsp_init(VOID)
{
	UINT err;
	ZSP_CTX *hZspCtx = &zsp_ctx;

	memset(hZspCtx, 0, sizeof(ZSP_CTX));
	sema_init(&hZspCtx->zsp_sem, 0);
	err = AMPMsgQ_Init(&hZspCtx->hZSPMsgQ, ZSP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("drv_zsp_init: hZSPMsgQ init failed, err:%8x\n", err);
		return NULL;
	}

	return hZspCtx;
}

VOID drv_zsp_exit(ZSP_CTX * hZspCtx)
{
	UINT err;

	err = AMPMsgQ_Destroy(&hZspCtx->hZSPMsgQ);
	if (unlikely(err != S_OK)) {
		amp_error("drv_zsp_exit: ZSP MsgQ Destroy failed, err:%8x\n",
			  err);
	}
}
