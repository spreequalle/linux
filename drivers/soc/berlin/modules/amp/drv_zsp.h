/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_ZSP_H_
#define _DRV_ZSP_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <../drivers/staging/android/ion/ion.h>
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_msg.h"

#define ZSP_ISR_MSGQ_SIZE           16
#define ADSP_ZSPINT2Soc_IRQ0        0

typedef struct _ZSP_CONTEXT_ {
	AMPMsgQ_t hZSPMsgQ;
	struct semaphore zsp_sem;
} ZSP_CTX;

ZSP_CTX *drv_zsp_init(VOID);
VOID drv_zsp_exit(ZSP_CTX * hZspCtx);
irqreturn_t amp_devices_zsp_isr(INT irq, void *dev_id);
#endif //_DRV_ZSP_H_
