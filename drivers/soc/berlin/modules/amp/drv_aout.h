/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_AOUT_H_
#define _DRV_AOUT_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <../drivers/staging/android/ion/ion.h>
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_msg.h"

#define AOUT_ISR_MSGQ_SIZE			128

typedef struct _AOUT_CONTEXT_ {
	INT aout_int_cnt[MAX_OUTPUT_AUDIO];

	AOUT_ION_HANDLE gstAoutIon;
	AOUT_PATH_CMD_FIFO *p_hdmi_fifo;
	AOUT_PATH_CMD_FIFO *p_spdif_fifo;
	AOUT_PATH_CMD_FIFO *p_sa_fifo;
	AOUT_PATH_CMD_FIFO *p_ma_fifo;
	AOUT_PATH_CMD_FIFO *p_arc_fifo;

	spinlock_t aout_spinlock;
	struct semaphore aout_sem;
	spinlock_t aout_msg_spinlock;
	AMPMsgQ_t hAoutMsgQ;
	int is_spdifrx_enabled;	//only for BG4_DTV
} AOUT_CTX;

AOUT_CTX *drv_aout_init(VOID);
VOID drv_aout_exit(AOUT_CTX * hAoutCtx);
VOID send_msg2aout(MV_CC_MSG_t * pMsg);
VOID aout_resume(INT path_id);

VOID aout_start_cmd(AOUT_CTX * hAoutCtx, INT * aout_info, VOID * param);
VOID aout_stop_cmd(AOUT_CTX * hAoutCtx, INT path_id);
VOID aout_resume_cmd(AOUT_CTX * hAoutCtx, INT path_id);
irqreturn_t amp_devices_aout_isr(int irq, void *dev_id);

VOID *AoutFifoGetKernelRdDMAInfo(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo,
				 INT pair);
VOID AoutFifoKernelRdUpdate(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo, INT adv);
INT AoutFifoCheckKernelFullness(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo);

#endif //_DRV_AOUT_H_
