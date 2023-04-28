/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_APP_H_
#define _DRV_APP_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <../drivers/staging/android/ion/ion.h>
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_msg.h"

#define APP_ISR_MSGQ_SIZE			16

typedef struct {
	struct ion_handle *ionh;
} APP_ION_HANDLE;

typedef struct _APP_CONTEXT_ {
	HWAPP_CMD_FIFO *p_app_fifo;
	APP_ION_HANDLE gstAppIon;

	AMPMsgQ_t hAPPMsgQ;
	spinlock_t app_spinlock;
	struct semaphore app_sem;
	INT app_int_cnt;
} APP_CTX;

APP_CTX *drv_app_init(VOID);
VOID drv_app_exit(APP_CTX * hAppCtx);

VOID app_resume(VOID);
VOID send_msg2app(MV_CC_MSG_t * pMsg);
VOID app_start_cmd(APP_CTX * hAppCtx, HWAPP_CMD_FIFO * p_app_cmd_fifo);
VOID app_resume_cmd(APP_CTX * hAppCtx, HWAPP_CMD_FIFO * p_app_cmd_fifo);
#endif //_DRV_APP_H_
