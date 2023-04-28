/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_VPP_H_
#define _DRV_VPP_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <../drivers/staging/android/ion/ion.h>
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_msg.h"

#define VPP_ISR_MSGQ_SIZE			64
#define DEBUG_TIMER_VALUE           (0xFFFFFFFF)
#define VPP_CC_MSG_TYPE_VPP				0x00
#define VPP_CC_MSG_TYPE_CEC				0x01

typedef struct VPP_DMA_INFO_T {
	uintptr_t DmaAddr;
	UINT32 DmaLen;
	UINT32 cpcbID;
} VPP_DMA_INFO;

typedef struct VPP_CEC_RX_MSG_BUF_T {
	UINT8 buf[16];
	UINT8 len;
} VPP_CEC_RX_MSG_BUF;

typedef struct amp_irq_profiler {
	unsigned long long vppCPCB0_intr_curr;
	unsigned long long vppCPCB0_intr_last;
	unsigned long long vpp_task_sched_last;
	unsigned long long vpp_isr_start;

	unsigned long long vpp_isr_end;
	unsigned long vpp_isr_time_last;

	unsigned long vpp_isr_time_max;
	unsigned long vpp_isr_instat_max;

	INT vpp_isr_last_instat;

} amp_irq_profiler_t;

typedef struct _VPP_CONTEXT_ {
	VPP_DMA_INFO dma_info[3];
	VPP_DMA_INFO vbi_bcm_info[3];
	VPP_DMA_INFO vde_bcm_info[3];
	UINT bcm_sche_cmd[6][2];
	VPP_CEC_RX_MSG_BUF rx_buf;
	UINT32 vpp_intr_status[MAX_INTR_NUM];
	atomic_t vpp_isr_msg_err_cnt;
	int vbi_bcm_cmd_fullcnt;
	int vde_bcm_cmd_fullcnt;

	AMPMsgQ_t hVPPMsgQ;
	spinlock_t vpp_msg_spinlock;
	spinlock_t bcm_spinlock;
	struct semaphore vpp_sem;
	struct semaphore vsync_sem;
	atomic64_t vsynctime;
	INT64 lastvsynctime;
	INT is_vsync_measure_enabled;
	UINT32 vsync_period_max;
	UINT32 vsync_period_min;
	INT vpp_cpcb0_vbi_int_cnt;
	UINT vpp_intr_timestamp;
#ifdef CONFIG_IRQ_LATENCY_PROFILE
	amp_irq_profiler_t amp_irq_profiler;
#endif
	int is_spdifrx_enabled;
	UINT8 is_cec_rx_enabled;
} VPP_CTX;

VPP_CTX *drv_vpp_init(VOID);
VOID drv_vpp_exit(VPP_CTX * hVppCtx);
irqreturn_t amp_devices_vpp_isr(INT irq, void *dev_id);
irqreturn_t amp_devices_vpp_cec_isr(int irq, void *dev_id);
VOID amp_vpp_cec_save_state(VPP_CTX * hVppCtx);
VOID amp_vpp_cec_restore_state(VPP_CTX * hVppCtx);

//CEC MACRO
#define SM_APB_ICTL1_BASE					0xf7fce000
#define SM_APB_GPIO0_BASE					0xf7fcc000
#define SM_APB_GPIO1_BASE					0xf7fc5000
#define APB_GPIO_PORTA_EOI				0x4c
#define APB_GPIO_INTSTATUS				0x40
#define APB_ICTL_IRQ_STATUS_L				0x20
#define BE_CEC_INTR_TX_SFT_FAIL			0x2008	// puneet
#define BE_CEC_INTR_TX_FAIL				0x200F
#define BE_CEC_INTR_TX_COMPLETE			0x0010
#define BE_CEC_INTR_RX_COMPLETE			0x0020
#define BE_CEC_INTR_RX_FAIL				0x00C0
#define SOC_SM_CEC_BASE					0xF7FE1000
#define CEC_INTR_STATUS0_REG_ADDR		0x0058
#define CEC_INTR_STATUS1_REG_ADDR		0x0059
#define CEC_INTR_ENABLE0_REG_ADDR		0x0048
#define CEC_INTR_ENABLE1_REG_ADDR		0x0049
#define CEC_RDY_ADDR						0x0008
#define CEC_RX_BUF_READ_REG_ADDR			0x0068
#define CEC_RX_EOM_READ_REG_ADDR			0x0069
#define CEC_TOGGLE_FOR_READ_REG_ADDR		0x0004
#define CEC_RX_RDY_ADDR					0x000c	//01
#define CEC_RX_FIFO_DPTR					0x0087
#endif //_DRV_VPP_H_
