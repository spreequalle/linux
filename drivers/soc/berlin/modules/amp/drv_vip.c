/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "galois_io.h"
#include "cinclude.h"
#include "api_dhub.h"
#include "avioDhub.h"
#include "api_avio_dhub.h"
#include "hal_dhub_wrap.h"
#include "Galois_memmap.h"

#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_vip.h"
#include "drv_msg.h"
#include "drv_debug.h"
#ifdef CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE
#include "drv_avif.h"
#endif
#include "drv_aout.h"

#define HRX_INT_VSYNC		0x00080000

static VIP_CTX vip_ctx;
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_CDP)
/* HDMI EOF Intr will be generated for
* patgen as well as for HDMI Rx */
#define avioDhubSemMap_vip_HDMIRxEOF_intr \
	avioDhubSemMap_vpp128b_vpp_inr9
/* HDMI Vsync will be generated
* only for HDMI Rx input in the HDMI Pipe, not for patgen */
#define avioDhubSemMap_vip_HDMIVSync_intr \
	avioDhubSemMap_vpp128b_vpp_inr10
#endif

#ifdef	HPD_DEBOUNCE
#define HPD_DELAY_CNT (6)
static int hpd_debounce_delaycnt;
#endif

static void VipHdmirxHandle(VIP_CTX *hVipCtx)
{
	HRESULT rc = S_OK;

	INT hrx_intr = 0xff;
	INT hrx_hdcp_intr = 0xff;
	UINT32 hrxsts0, hrxsts1, hrxen0, hrxen1;

	GA_REG_WORD32_READ(RA_HDRX_INTR_EN, &hrxen0);
	GA_REG_WORD32_READ(RA_HDRX_INTR_EN1, &hrxen1);
	GA_REG_WORD32_READ(RA_HDRX_INTR_STATUS, &hrxsts0);
	GA_REG_WORD32_READ(RA_HDRX_INTR_STATUS1, &hrxsts1);

	if (hrxsts1 & HDMI_RX_INT_WR_MSSG_STARTED)
		hrx_hdcp_intr = HDMIRX_INTR_HDCP_2X;

	hrx_intr = 0xff;
	if (hrxsts0 & (HDMI_RX_INT_VRES_CHG |
		HDMI_RX_INT_HRES_CHG |
		HDMI_RX_INT_5V_PWR | HDMI_RX_INT_CLOCK_CHANGE)) {
		hrx_intr = HDMIRX_INTR_SYNC;

		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN,
			HDMI_RX_INT_AUTH_STARTED);
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN1,
			HDMI_RX_INT_WR_MSSG_STARTED);
	} else if (hrxsts1 & HDMI_RX_INT_PHY_PRT_EN) {
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN1,
			HDMI_RX_INT_PHY_PRT_EN);
		hrx_intr = HDMIRX_INTR_PRT;
	} else if (hrxsts0 & HDMI_RX_INT_GCP_AVMUTE) {
		hrx_intr = HDMIRX_INTR_AVMUTE;
		GA_REG_WORD32_READ(RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_GCP_AVMUTE);
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN, hrxen0);
	} else if (hrxsts0 & (HDMI_RX_INT_AUTH_STARTED |
		HDMI_RX_INT_AUTH_COMPLETE)) {
		hrx_intr = HDMIRX_INTR_HDCP;
		GA_REG_WORD32_READ(RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 =  hrxen0 & ~(HDMI_RX_INT_AUTH_STARTED);
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN, hrxen0);
	} else if (hrxsts0 &
		HDMI_RX_INT_VSI_STOP) {
		hrx_intr = HDMIRX_INTR_VSI_STOP;
		GA_REG_WORD32_READ(RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_VSI_STOP);
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN, hrxen0);
	} else if (hrxsts0 & (HDMI_RX_INT_AVI_INFO |
		HDMI_RX_INT_VENDOR_PKT)) {
		hrx_intr = HDMIRX_INTR_PKT;
		GA_REG_WORD32_READ(RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_AVI_INFO |
			HDMI_RX_INT_VENDOR_PKT);
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN, hrxen0);
	} else if (hrxsts0 &
		HDMI_RX_INT_GAMUT_PKT) {
		hrx_intr = HDMIRX_INTR_GMD_PKT;
		GA_REG_WORD32_READ(RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_GAMUT_PKT);
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN, hrxen0);
	} else if (hrxsts0 & (HDMI_RX_INT_CHNL_STATUS |
		HDMI_RX_INT_AUD_INFO)) {
		hrx_intr = HDMIRX_INTR_CHNL_STS;
		GA_REG_WORD32_READ(RA_HDRX_INTR_EN,
			&hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_CHNL_STATUS |
			HDMI_RX_INT_AUD_INFO);
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN, hrxen0);
	} else if (hrxsts0 & HDMI_RX_INT_GCP_COLOR_DEPTH) {
		hrx_intr = HDMIRX_INTR_CLR_DEPTH;
		GA_REG_WORD32_READ(RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_GCP_COLOR_DEPTH);
		GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN, hrxen0);
	}

	/* HDMI Rx interrupt */
	if (hrx_intr != 0xff) {
		/* process rx intr */
		MV_CC_MSG_t msg = {
		0,
		hrx_intr, /*send port info also */
		hrxsts0
	};

		rc = AMPMsgQ_Add(&hVipCtx->hVIPHRXMsgQ, &msg);
		if (rc != S_OK) {
			amp_error("[AVIF HRX isr] MsgQ full\n");
			return;
		}
		up(&hVipCtx->vip_hrx_sem);
	}

	if (hrx_hdcp_intr != 0xff) {
		/* process rx intr */
		MV_CC_MSG_t msg_hdcp = {
			0,
			hrx_hdcp_intr,
			hrxsts1
		};

		if (hrx_hdcp_intr == HDMIRX_INTR_HDCP_2X) {
			rc = AMPMsgQ_Add(&hVipCtx->hVIPHDCPMsgQ,
				&msg_hdcp);
			if (rc != S_OK) {
				amp_error(
					"[AVIF HDCP isr] HDCP MsgQ full\n");
				return;
			}
			up(&hVipCtx->vip_hdcp_sem);
		}
	}

}


irqreturn_t amp_devices_vip_isr(INT irq, VOID *dev_id)
{
	INT instat;
	HDL_semaphore *pSemHandle;
	HRESULT ret = S_OK;
	VIP_CTX *hVipCtx = (VIP_CTX *)dev_id;
	MV_CC_MSG_t msg;
	UINT32 hrxsts0;

	msg.m_MsgID = VIP_CC_MSG_TYPE_VIP;
	hVipCtx->vip_hdmiRxpipe_int_cnt++;

	/* VIP interrupt handling  */
	pSemHandle = dhub_semaphore(&VPP_dhubHandle.dhub);
	instat = semaphore_chk_full(pSemHandle, -1);

	if (bTST(instat,
		avioDhubSemMap_vip_HDMIRxEOF_intr)) {
		msg.m_Param1 = 1 << VIP_HDMI_RX_EOF_INTR_SET;
		semaphore_pop(pSemHandle,
			avioDhubSemMap_vpp128b_vpp_inr9, 1);
		semaphore_clr_full(pSemHandle,
			avioDhubSemMap_vpp128b_vpp_inr9);
	} else if (bTST(instat,
		avioDhubSemMap_vip_HDMIVSync_intr))	{
		unsigned int hrx_intr = 0;

		GA_REG_WORD32_READ(RA_HDRX_INTR_STATUS, &hrxsts0);
		VipHdmirxHandle(hVipCtx);
		if (hrxsts0 & HDMI_RX_INT_VSYNC) {
			GA_REG_WORD32_READ(RA_HDRX_INTR_EN, &hrx_intr);
			hrx_intr = (hrx_intr & (~HRX_INT_VSYNC));
			GA_REG_WORD32_WRITE(RA_HDRX_INTR_EN, hrx_intr);
			msg.m_Param1 = 1 << VIP_HDMI_RX_VSYNC_INTR_SET;
		}

		semaphore_pop(pSemHandle,
			avioDhubSemMap_vpp128b_vpp_inr10, 1);
		semaphore_clr_full(pSemHandle,
			avioDhubSemMap_vpp128b_vpp_inr10);

	} else {
		atomic_inc(&hVipCtx->vip_isr_msg_err_cnt);
		return IRQ_HANDLED;
	}

	spin_lock(&hVipCtx->vip_msg_spinlock);
	ret = AMPMsgQ_Add(&hVipCtx->hVIPMsgQ, &msg);
	spin_unlock(&hVipCtx->vip_msg_spinlock);
	if (ret != S_OK) {
		if (!atomic_read(&hVipCtx->vip_isr_msg_err_cnt)) {
			amp_error
				("[vip isr] MsgQ full\n");
		}
		atomic_inc(&hVipCtx->vip_isr_msg_err_cnt);
		return IRQ_HANDLED;
	}
	up(&hVipCtx->vip_sem);

	return IRQ_HANDLED;
}

VIP_CTX *drv_vip_init(VOID)
{
	VIP_CTX *hVipCtx = &vip_ctx;
	UINT err;

	memset(hVipCtx, 0, sizeof(VIP_CTX));

	sema_init(&hVipCtx->vip_sem, 0);
	sema_init(&hVipCtx->vip_hrx_sem, 0);
	sema_init(&hVipCtx->vip_hdcp_sem, 0);
	spin_lock_init(&hVipCtx->vip_msg_spinlock);

	err = AMPMsgQ_Init(&hVipCtx->hVIPMsgQ,
		VIP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error(
		"drv_vip_init: hVIPMsgQ init failed, err:%8x\n",
		err);
		return NULL;
	}

	err = AMPMsgQ_Init(&hVipCtx->hVIPHRXMsgQ,
		VIP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error(
		"drv_vip_init: hVIPHRXMsgQ init failed, err:%8x\n",
		err);
		return NULL;
	}

	err = AMPMsgQ_Init(&hVipCtx->hVIPHDCPMsgQ,
			VIP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error(
		"drv_vip_init: hVIPHDCPMsgQ init failed, err:%8x\n",
		err);
		return NULL;
	}

	return hVipCtx;
}

VOID drv_vip_exit(VIP_CTX *hVipCtx)
{
	UINT err;

	err = AMPMsgQ_Destroy(&hVipCtx->hVIPMsgQ);
	if (unlikely(err != S_OK)) {
		amp_error(
		"vip MsgQ Destroy: failed, err:%8x\n",
		err);
		return;
	}
	err = AMPMsgQ_Destroy(&hVipCtx->hVIPHRXMsgQ);
	if (unlikely(err != S_OK)) {
		amp_error(
			"vip HRX MsgQ Destroy: failed, err:%8x\n",
			err);
		return;
	}
	err = AMPMsgQ_Destroy(&hVipCtx->hVIPHDCPMsgQ);
	if (unlikely(err != S_OK)) {
		amp_error(
		"vip HDCP MsgQ Destroy: failed, err:%8x\n",
		err);
		return;
	}
}
