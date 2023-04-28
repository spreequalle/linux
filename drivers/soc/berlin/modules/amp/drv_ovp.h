/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_OVP_H_
#define _DRV_OVP_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <../drivers/staging/android/ion/ion.h>
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_msg.h"

#define OVP_REG_BASE                0xF75E0000
#define OVP_INTR_STS                0x40
#define OVP_ISR_MSGQ_SIZE           8
#define OVP_CC_MSG                  0x00

typedef struct _OVP_CONTEXT_ {
	UINT32 ovp_intr_status[MAX_INTR_NUM];

	AMPMsgQ_t hOVPMsgQ;
	spinlock_t ovp_msg_spinlock;
	struct semaphore ovp_sem;
} OVP_CTX;

OVP_CTX *drv_ovp_init(VOID);
VOID drv_ovp_exit(OVP_CTX * hOvpCtx);
irqreturn_t amp_devices_ovp_isr(INT irq, void *dev_id);

#endif
