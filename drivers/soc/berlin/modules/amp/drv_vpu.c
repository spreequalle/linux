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
#include "drv_vpu.h"
#include "drv_msg.h"
#include "drv_debug.h"
#include "api_dhub.h"
#include "api_avio_dhub.h"
#include "hal_dhub_wrap.h"
#include "amp_driver.h"

static VPU_CTX vpu_ctx;

extern INT amp_irqs[IRQ_AMP_MAX];

irqreturn_t amp_devices_vdec_isr(int irq, VOID * dev_id)
{
	VPU_CTX *hVpuCtx;

	hVpuCtx = (VPU_CTX *) dev_id;
	if (hVpuCtx == NULL) {
		amp_error("amp_devices_vdec_isr: hVpuCtx=null!\n");
	}

	/* disable interrupt */
	disable_irq_nosync(irq);

#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
	if (irq == amp_irqs[IRQ_VMETA]) {
		HRESULT ret = S_OK;
		MV_CC_MSG_t msg = { 0 };
		msg.m_Param1 = ++hVpuCtx->vdec_int_cnt;
		ret = AMPMsgQ_Add(&hVpuCtx->hVPUMsgQ, &msg);
		if (ret != S_OK) {
			if (!atomic_read(&hVpuCtx->vdec_isr_msg_err_cnt)) {
				amp_error("[vdec isr] MsgQ full\n");
			}
			atomic_inc(&hVpuCtx->vdec_isr_msg_err_cnt);
			return IRQ_HANDLED;
		}
		up(&hVpuCtx->vdec_vmeta_sem);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
	if (irq == amp_irqs[IRQ_V2G]) {
		up(&hVpuCtx->vdec_v2g_sem);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_H1_ENABLE
	if (irq == amp_irqs[IRQ_H1]) {
		up(&hVpuCtx->vdec_v2g_sem);
	}
#endif

	return IRQ_HANDLED;
}

VPU_CTX *drv_vpu_init(VOID)
{
	VPU_CTX *hVpuCtx = &vpu_ctx;
	UINT err;

	memset(hVpuCtx, 0, sizeof(VPU_CTX));

#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
	sema_init(&hVpuCtx->vdec_v2g_sem, 0);
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
	sema_init(&hVpuCtx->vdec_vmeta_sem, 0);
#endif

	err = AMPMsgQ_Init(&hVpuCtx->hVPUMsgQ, VDEC_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("drv_vpu_init: hVPUMsgQ init failed, err:%8x\n", err);
		return NULL;
	}

	return hVpuCtx;
}

VOID drv_vpu_exit(VPU_CTX * hVpuCtx)
{
	UINT err;
	err = AMPMsgQ_Destroy(&hVpuCtx->hVPUMsgQ);
	if (unlikely(err != S_OK)) {
		amp_error("drv_app_exit: failed, err:%8x\n", err);
		return;
	}
}
