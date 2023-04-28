/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _DRV_VIP_H_
#define _DRV_VIP_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>

#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_msg.h"

#define VIP_HDMI_RX_EOF_INTR_SET		0
#define VIP_HDMI_RX_VSYNC_INTR_SET		1

#define VIP_ISR_MSGQ_SIZE			64
#define DEBUG_TIMER_VALUE			(0xFFFFFFFF)
#define VIP_CC_MSG_TYPE_VIP			0x00


#define    HDMI_RX_INT_GCP_AVMUTE 0x00000004
#define    HDMI_RX_INT_GCP_COLOR_DEPTH 0x00000008
#define    HDMI_RX_INT_GCP_PHASE  0x00000010
#define    HDMI_RX_INT_ACP_PKT	  0x00000020
#define    HDMI_RX_INT_ISRC1_PKT  0x00000040
#define    HDMI_RX_INT_ISRC2_PKT  0x00000080

#define    HDMI_RX_INT_GAMUT_PKT	0x00000100
#define    HDMI_RX_INT_AMD_PKT		0x00000200
#define    HDMI_RX_INT_VENDOR_PKT	0x00000400
#define    HDMI_RX_INT_AVI_INFO		0x00000800
#define    HDMI_RX_INT_SPD_INFO		0x00001000
#define    HDMI_RX_INT_AUD_INFO		0x00002000
#define    HDMI_RX_INT_MPEG_INFO	0x00004000
#define    HDMI_RX_INT_CHNL_STATUS	0x00008000

#define    HDMI_RX_INT_TMDS_MODE	0x00010000
#define    HDMI_RX_INT_AUTH_STARTED	0x00020000
#define    HDMI_RX_INT_AUTH_COMPLETE	0x00040000
#define    HDMI_RX_INT_VSYNC			0x00080000
#define    HDMI_RX_INT_UBITS			0x00100000
#define    HDMI_RX_INT_VSI_STOP			0x00200000
#define    HDMI_RX_INT_SYNC_DET			0x00400000
#define    HDMI_RX_INT_VRES_CHG			0x00800000

#define   HDMI_RX_INT_HRES_CHG				0x01000000
#define   HDMI_RX_INT_CED_CNT_UPDATE		0x02000000
#define   HDMI_RX_INT_5V_PWR				0x04000000
#define   HDMI_RX_INT_CLOCK_CHANGE			0x08000000
#define   HDMI_RX_INT_EDID					0x10000000
#define   HDMI_RX_INT_TMDS_BIT_CLK_RATIO	0x20000000
#define   HDMI_RX_INT_SCRAMBLER_EN			0x40000000
#define   HDMI_RX_INT_RR_ENABLE				0x80000000
#define   HDMI_RX_INT_ALL_INTR				0xFFFFFFFF

#define    HDMI_RX_INT_WR_MSSG_STARTED		  0x00000001
#define    HDMI_RX_INT_WR_MESSAGE_DONE		  0x00000002
#define    HDMI_RX_INT_READ_MESSAGE_DONE	  0x00000004
#define    HDMI_RX_INT_READ_MESSAGE_FAIL	  0x00000008
#define    HDMI_RX_INT_HMAC_SM_RDY			  0x00000010
#define    HDMI_RX_INT_HMAC_HASH_DONE		  0x00000020
#define    HDMI_RX_INT_PHY_PRT_EN			  0x00000040
#define    HDMI_RX_INT_PHY_PRT_EN			  0x00000040
#define    HDMI_RX_INT_DRM_INFO				0x00000080
#define    HDMI_RX_INT_AUD_FIFO_OVER_FLOW	0x00000100
#define    HDMI_RX_INT_AUD_FIFO_UNDER_FLOW	0x00000200
#define    HDMI_RX_INT_ALL_INTR_1			0x000003FF

#define    HDMI_RX_INT_ALL_INTR0	0xFF
#define    HDMI_RX_INT_ALL_INTR1	0xFF00
#define    HDMI_RX_INT_ALL_INTR2	0xFF0000
#define    HDMI_RX_INT_ALL_INTR3	0xFF000000

#define SOC_HDMIRX_BASE			0xF74C0000
#define RA_HDRX_INTR_EN			(SOC_HDMIRX_BASE + 0x01A0)
#define RA_HDRX_INTR_EN1		(SOC_HDMIRX_BASE + 0x01A4)
#define RA_HDRX_INTR_STATUS		(SOC_HDMIRX_BASE + 0x01A8)
#define RA_HDRX_INTR_STATUS1	(SOC_HDMIRX_BASE + 0x01AC)
#define RA_HDRX_INTR_CLR		(SOC_HDMIRX_BASE + 0x01B0)
#define RA_HDRX_INTR_CLR1		(SOC_HDMIRX_BASE + 0x01B4)

#define HDMIRX_INTR_SYNC						0x01
#define HDMIRX_INTR_HDCP						0x02
#define HDMIRX_INTR_EDID						0x03
#define HDMIRX_INTR_PKT							0x04
#define HDMIRX_INTR_AVMUTE						0x05
#define HDMIRX_INTR_TMDS						0x06
#define HDMIRX_INTR_CHNL_STS					0x07
#define HDMIRX_INTR_CLR_DEPTH					0x08
#define HDMIRX_INTR_VSI_STOP					0x09
#define HDMIRX_INTR_GMD_PKT						0x0A
#define HDMIRX_INTR_HDCP_2X						0x0B
#define HDMIRX_INTR_PRT							0x0C

typedef struct _VIP_CONTEXT_ {
	atomic_t vip_isr_msg_err_cnt;
	unsigned int vip_hdmiRxpipe_int_cnt;

	AMPMsgQ_t hVIPMsgQ;
	AMPMsgQ_t hVIPHRXMsgQ;
	AMPMsgQ_t hVIPHDCPMsgQ;
	spinlock_t vip_msg_spinlock;
	struct semaphore vip_sem;
	struct semaphore vip_hrx_sem;
	struct semaphore vip_hdcp_sem;
} VIP_CTX;

VIP_CTX *drv_vip_init(VOID);
VOID drv_vip_exit(VIP_CTX *hVipCtx);
irqreturn_t amp_devices_vip_isr(INT irq, void *dev_id);

#endif	/* _DRV_VPP_H_ */
