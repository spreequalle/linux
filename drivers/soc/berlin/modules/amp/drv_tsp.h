/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_TSP_H_
#define _DRV_TSP_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <../drivers/staging/android/ion/ion.h>
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_msg.h"

#define     TSP_ISR_MSGQ_SIZE                              32

#define     TSP_ISR_START                                  0x1
#define     TSP_ISR_STOP                                   0x2
#define     TSP_ISR_WAKEUP                                 0x3

#define     RA_TspReg_IntReg                               0xF344
#define     RA_TspIntReg_software_int_enable               0x0028
#define     RA_TspIntReg_software_int_status               0x002C
#define     RA_TspIntReg_software_int_set                  0x0030

typedef struct _TSP_CONTEXT_ {
	AMPMsgQ_t hTSPMsgQ;
	struct semaphore tsp_sem;
} TSP_CTX;

irqreturn_t amp_devices_tsp_isr(INT irq, void *dev_id);
VOID drv_tsp_open(TSP_CTX * hTspCtx, UINT mask);
VOID drv_tsp_close(TSP_CTX * hTspCtx);
TSP_CTX *drv_tsp_init(VOID);
VOID drv_tsp_exit(TSP_CTX * hTspCtx);
#endif //_DRV_TSP_H_
