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
#include "drv_ovp.h"
#include "drv_debug.h"
#include "tee_dev_ca_ovp.h"

static OVP_CTX ovp_ctx;

irqreturn_t amp_devices_ovp_isr(INT irq, VOID * dev_id)
{
	OVP_CTX *hOvpCtx = (OVP_CTX *) dev_id;
	UINT32 ovp_intr = 0;
	HRESULT ret = S_OK;

	MV_OVP_Get_Clr_IntrSts(&ovp_intr);

	if (ovp_intr) {
		if (hOvpCtx->ovp_intr_status[0]) {
			MV_CC_MSG_t msg = { OVP_CC_MSG,
				ovp_intr
			};

			spin_lock(&hOvpCtx->ovp_msg_spinlock);
			ret = AMPMsgQ_Add(&hOvpCtx->hOVPMsgQ, &msg);
			spin_unlock(&hOvpCtx->ovp_msg_spinlock);
			//if (ret != S_OK)
			{
				up(&hOvpCtx->ovp_sem);
			}
		}
	}

	return IRQ_HANDLED;
}

OVP_CTX *drv_ovp_init(VOID)
{
	OVP_CTX *hOvpCtx = &ovp_ctx;
	UINT err;

	memset(hOvpCtx, 0, sizeof(OVP_CTX));

	sema_init(&hOvpCtx->ovp_sem, 0);
	spin_lock_init(&hOvpCtx->ovp_msg_spinlock);
	err = AMPMsgQ_Init(&hOvpCtx->hOVPMsgQ, OVP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("drv_vpp_init: hVPPMsgQ init failed, err:%8x\n", err);
		return NULL;
	}

	return hOvpCtx;
}

VOID drv_ovp_exit(OVP_CTX * hOvpCtx)
{
	UINT err;
	err = AMPMsgQ_Destroy(&hOvpCtx->hOVPMsgQ);
	if (unlikely(err != S_OK)) {
		amp_error("vpp MsgQ Destroy: failed, err:%8x\n", err);
		return;
	}
}
