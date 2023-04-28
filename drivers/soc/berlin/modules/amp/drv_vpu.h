/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_VPU_H_
#define _DRV_VPU_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <../drivers/staging/android/ion/ion.h>
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_msg.h"

#define VDEC_ISR_MSGQ_SIZE			16

enum VPU_HW_ID {
	VPU_VMETA = 0,
	VPU_V2G = 1,
	VPU_G2 = 2,
	VPU_G1 = 3,
	VPU_H1_0 = 4,
	VPU_H1_1 = 5,
	VPU_HW_IP_NUM,
};

typedef struct _VPU_CONTEXT_ {
	AMPMsgQ_t hVPUMsgQ;

	UINT vdec_int_cnt;
	UINT vdec_enable_int_cnt;
	atomic_t vdec_isr_msg_err_cnt;

	struct semaphore vdec_v2g_sem;
	struct semaphore vdec_vmeta_sem;
	INT app_int_cnt;
} VPU_CTX;

VPU_CTX *drv_vpu_init(VOID);
VOID drv_vpu_exit(VPU_CTX * hVpuCtx);
irqreturn_t amp_devices_vdec_isr(INT irq, VOID * dev_id);

#endif //_DRV_VPU_H_
