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
#include "amp_type.h"
#include "drv_avif.h"
#include "drv_msg.h"
#include "drv_debug.h"
#include "api_dhub.h"
#include "api_avio_dhub.h"
#include "hal_dhub_wrap.h"
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
#include "api_avif_dhub.h"
#include "avif_dhub_config.h"
#endif
#include "drv_aout.h"

static AVIF_CTX avif_ctx;

#if defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE)
static inline INT GetAvifDhubAudioWRChnl(INT isPIPAudio)
{
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
	return avif_dhub_config_ChMap_avif_AUD_WR0_MAIN;
#else
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	if (isPIPAudio) {
		return avif_dhub_config_ChMap_avif_AUD_WR0_PIP;
	} else {
		return avif_dhub_config_ChMap_avif_AUD_WR0_MAIN;
	}
#else
	return avioDhubChMap_aio64b_HDMIRX_CH3_W;
#endif
#endif
}

static void *AIPFifoGetKernelPreRdDMAInfo(AIP_DMA_CMD_FIFO *p_aip_cmd_fifo,
					  INT pair)
{
	void *pHandle;
	INT rd_offset = p_aip_cmd_fifo->kernel_pre_rd_offset;

	if (rd_offset > p_aip_cmd_fifo->size || rd_offset < 0) {
		INT i = 0, fifo_cmd_size = sizeof(AIP_DMA_CMD_FIFO) >> 2;
		INT *temp = (INT *) p_aip_cmd_fifo;

		amp_trace("memory %p is corrupted! corrupted data :\n",
			  p_aip_cmd_fifo);
		for (i = 0; i < fifo_cmd_size; i++) {
			amp_trace("0x%x\n", *temp++);
		}
		rd_offset = 0;
	}
	pHandle = &(p_aip_cmd_fifo->aip_dma_cmd[pair][rd_offset]);
	return pHandle;
}

static void AIPFifoKernelPreRdUpdate(AIP_DMA_CMD_FIFO *p_aip_cmd_fifo, INT adv)
{
	p_aip_cmd_fifo->kernel_pre_rd_offset += adv;
	p_aip_cmd_fifo->kernel_pre_rd_offset %= p_aip_cmd_fifo->size;
}

static void AIPFifoKernelRdUpdate(AIP_DMA_CMD_FIFO *p_aip_cmd_fifo, INT adv)
{
	p_aip_cmd_fifo->kernel_rd_offset += adv;
	p_aip_cmd_fifo->kernel_rd_offset %= p_aip_cmd_fifo->size;
}

static INT AIPFifoCheckKernelFullness(AIP_DMA_CMD_FIFO *p_aip_cmd_fifo)
{
	INT full;

	full = p_aip_cmd_fifo->wr_offset - p_aip_cmd_fifo->kernel_pre_rd_offset;
	if (full < 0)
		full += p_aip_cmd_fifo->size;
	return full;
}

#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
static void HdmirxHandle(AVIF_CTX *hAvifCtx, UINT32 hdmi_port)
{
	HRESULT rc = S_OK;
	UINT32 port_offset = 0;

	UINT8 en0, en1, en2, en3;
	INT hrx_intr = 0xff;
	UINT32 hrxsts0, hrxsts1, hrxen0, hrxen1;
	UINT32 chip_rev;

	if ((hdmi_port >= 0) && (hdmi_port <= 3)) {
		port_offset = (hdmi_port * AVIF_HRX_BASE_OFFSET) << 2;
	} else {
		amp_trace("Invalid HDMI Rx Port %d\n",
			hdmi_port);
		return;
	}

	GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
	GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN1, &hrxen1);
	GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_STATUS, &hrxsts0);
	GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_STATUS1, &hrxsts1);
	hrxen0 = hrxsts0 & hrxen0;
	hrxen1 = hrxsts1 & hrxen1;

	en0 = (hrxen0 & 0xFF);
	en1 = ((hrxen0 & 0xFF00) >> 8);
	en2 = ((hrxen0 & 0xFF0000) >> 16);
	en3 = ((hrxen0 & 0xFF000000) >> 24);

	GA_REG_WORD32_READ(BG2Q4K_CHIP_REV_ADDRESS, &chip_rev);
	hrx_intr = 0xff;
	if ((en2 & (HDMI_RX_INT_VRES_CHG | HDMI_RX_INT_HRES_CHG)) ||
		(en3 & (HDMI_RX_INT_5V_PWR | HDMI_RX_INT_CLOCK_CHANGE))) {
		hrx_intr = HDMIRX_INTR_SYNC;

		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 =
			hrxen0 &
			~((HDMI_RX_INT_SYNC_DET | HDMI_RX_INT_VRES_CHG |
			HDMI_RX_INT_HRES_CHG) << 16);
		hrxen0 = hrxen0 &
			~((HDMI_RX_INT_5V_PWR |
			HDMI_RX_INT_CLOCK_CHANGE) << 24);
		hrxen0 =
			((hrxen0 & 0xFF0000) | (HDMI_RX_INT_AUTH_STARTED << 16) |
			(HDMI_RX_INT_WR_MSSG_STARTED << 24));
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN), hrxen0);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN1), 0x07);
	}
#if (BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0)
	else if ((chip_rev >= 0xB0) && (hrxen1 & HDMI_RX_INT_PRT)) {
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN1),
					HDMI_RX_INT_PRT);
		hrx_intr = HDMIRX_INTR_PRT;
	}
#endif
	else if (en0 & HDMI_RX_INT_GCP_AVMUTE) {
		hrx_intr = HDMIRX_INTR_AVMUTE;
		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_GCP_AVMUTE);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN), hrxen0);
	} else if ((en3 & HDMI_RX_INT_WR_MSSG_STARTED)) {
		hrx_intr = HDMIRX_INTR_HDCP_2X;
		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_WR_MSSG_STARTED << 24);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN),
			hrxen0);
	} else if (en2 & (HDMI_RX_INT_AUTH_STARTED |
		HDMI_RX_INT_AUTH_COMPLETE)) {
		hrx_intr = HDMIRX_INTR_HDCP;
		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_AUTH_STARTED << 16);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN), hrxen0);
	} else if (en2 & HDMI_RX_INT_VSI_STOP) {
		hrx_intr = HDMIRX_INTR_VSI_STOP;
		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_VSI_STOP << 16);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN), hrxen0);
	} else if (en1 & (HDMI_RX_INT_AVI_INFO | HDMI_RX_INT_VENDOR_PKT)) {
		hrx_intr = HDMIRX_INTR_PKT;
		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 =
			hrxen0 & ~((HDMI_RX_INT_AVI_INFO | HDMI_RX_INT_VENDOR_PKT)
				<< 8);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN), hrxen0);
	} else if (en1 & HDMI_RX_INT_GAMUT_PKT) {
		hrx_intr = HDMIRX_INTR_GMD_PKT;
		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~((HDMI_RX_INT_GAMUT_PKT) << 8);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN), hrxen0);
	} else if (en1 & (HDMI_RX_INT_CHNL_STATUS | HDMI_RX_INT_AUD_INFO)) {
		hrx_intr = HDMIRX_INTR_CHNL_STS;
		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 =
			hrxen0 & ~((HDMI_RX_INT_CHNL_STATUS | HDMI_RX_INT_AUD_INFO)
				<< 8);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN), hrxen0);
	} else if (en0 & HDMI_RX_INT_GCP_COLOR_DEPTH) {
		hrx_intr = HDMIRX_INTR_CLR_DEPTH;
		GA_REG_WORD32_READ(port_offset + RA_HDRX_INTR_EN, &hrxen0);
		hrxen0 = hrxen0 & ~(HDMI_RX_INT_GCP_COLOR_DEPTH);
		GA_REG_WORD32_WRITE((port_offset + RA_HDRX_INTR_EN), hrxen0);
	}

	/* HDMI Rx interrupt */
	if (hrx_intr != 0xff) {
		/* process rx intr */
		MV_CC_MSG_t msg = {
			0,
			hrx_intr, /* send port info also */
			hdmi_port
		};
		if (hrx_intr == HDMIRX_INTR_HDCP_2X) {
			rc = AMPMsgQ_Add(&hAvifCtx->hPEAVIFHDCPMsgQ, &msg);
			if (rc != S_OK) {
				amp_error("[AVIF HDCP isr] HDCP MsgQ full\n");
				return;
			}
			up(&hAvifCtx->avif_hdcp_sem);
		} else {
			rc = AMPMsgQ_Add(&hAvifCtx->hPEAVIFHRXMsgQ, &msg);
			if (rc != S_OK) {
				amp_error("[AVIF HRX isr] MsgQ full\n");
				return;
			}
			up(&hAvifCtx->avif_hrx_sem);
		}
	}

	return;
}
#endif

AVIF_CTX *drv_avif_init(void)
{
	INT err;
	AVIF_CTX *hAvifCtx = &avif_ctx;

	memset(&avif_ctx, 0, sizeof(AVIF_CTX));

	spin_lock_init(&hAvifCtx->aip_spinlock);
	spin_lock_init(&hAvifCtx->aip_spinlock);

#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	sema_init(&hAvifCtx->avif_sem, 0);
	err = AMPMsgQ_Init(&hAvifCtx->hPEAVIFMsgQ, AVIF_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("PEAVIFMsgQ_Init: falied, err:%8x\n", err);
		return NULL;
	}

	sema_init(&hAvifCtx->avif_hrx_sem, 0);
	err = AMPMsgQ_Init(&hAvifCtx->hPEAVIFHRXMsgQ, AVIF_HRX_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("PEAVIFHRXMsgQ_Init: falied, err:%8x\n", err);
		return NULL;
	}

	sema_init(&hAvifCtx->avif_hdcp_sem, 0);
	err = AMPMsgQ_Init(&hAvifCtx->hPEAVIFHDCPMsgQ, AVIF_HDCP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("PEAVIFHDCPMsgQ_Init: falied, err:%8x\n", err);
		return NULL;
	}
	sema_init(&hAvifCtx->avif_vdec_sem, 0);
	err = AMPMsgQ_Init(&hAvifCtx->hPEAVIFVDECMsgQ, AVIF_VDEC_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("hPEAVIFVDECMsgQ: falied, err:%8x\n", err);
		return NULL;
	}
#endif

	sema_init(&hAvifCtx->aip_sem, 0);
	err = AMPMsgQ_Init(&hAvifCtx->hAIPMsgQ, AIP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("hAIPMsgQ init: failed, err:%8x\n", err);
		return NULL;
	}

	return hAvifCtx;
}

void drv_avif_exit(AVIF_CTX *hAvifCtx)
{
	UINT err;

#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	err = AMPMsgQ_Destroy(&hAvifCtx->hPEAVIFMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("avif MsgQ Destroy: falied, err:%8x\n", err);
	}
	err = AMPMsgQ_Destroy(&hAvifCtx->hPEAVIFHRXMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("AVIF hrx MsgQ Destroy: falied, err:%8x\n", err);
	}
	err = AMPMsgQ_Destroy(&hAvifCtx->hPEAVIFHDCPMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("AVIF hdcp MsgQ Destroy: falied, err:%8x\n", err);
	}
	err = AMPMsgQ_Destroy(&hAvifCtx->hPEAVIFVDECMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("AVIF VDEC MsgQ Destroy: falied, err:%8x\n", err);
	}
#endif

	err = AMPMsgQ_Destroy(&hAvifCtx->hAIPMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("AIP MsgQ Destroy: failed, err:%8x\n", err);
	}
}

irqreturn_t amp_devices_avif_isr(int irq, void *dev_id)
{
	HDL_semaphore *pSemHandle;
	INT channel;
	INT instat;
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	UINT8 enreg0, enreg1, enreg2, enreg3, enreg4;
	UINT8 masksts0, masksts1, masksts2, masksts3, masksts4;
	UINT8 regval;
	UINT32 hdmi_port = 0;
	UINT32 instat0 = 0, instat1 = 0;
#endif
	AVIF_CTX *hAvifCtx;

#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	hAvifCtx = (AVIF_CTX *)dev_id;
#else
	hAvifCtx = (AVIF_CTX *)&avif_ctx;
#endif

#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	GA_REG_BYTE_READ(CPU_INTR_MASK0_EXTREG_ADDR, &enreg0);
	GA_REG_BYTE_READ(CPU_INTR_MASK0_STATUS_EXTREG_ADDR, &masksts0);

	GA_REG_BYTE_READ(CPU_INTR_MASK1_EXTREG_ADDR, &enreg1);
	GA_REG_BYTE_READ(CPU_INTR_MASK1_STATUS_EXTREG_ADDR, &masksts1);

	GA_REG_BYTE_READ(CPU_INTR_MASK2_EXTREG_ADDR, &enreg2);
	GA_REG_BYTE_READ(CPU_INTR_MASK2_STATUS_EXTREG_ADDR, &masksts2);

	GA_REG_BYTE_READ(CPU_INTR_MASK3_EXTREG_ADDR, &enreg3);
	GA_REG_BYTE_READ(CPU_INTR_MASK3_STATUS_EXTREG_ADDR, &masksts3);

	GA_REG_BYTE_READ(CPU_INTR_MASK4_EXTREG_ADDR, &enreg4);
	GA_REG_BYTE_READ(CPU_INTR_MASK4_STATUS_EXTREG_ADDR, &masksts4);

	instat0 =
	    (masksts0 << 0) | (masksts1 << 8) | (masksts2 << 16) | (masksts3 <<
								    24);

	GA_REG_BYTE_READ(CPU_INTCPU_2_EXTCPU_MASK0INTR_ADDR, &regval);
	instat1 |= regval;
	if (regval & 0x1)
		GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR0_ADDR, 0x00);
	if (regval & 0x2)
		GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR1_ADDR, 0x00);
	if (regval & 0x4)
		GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR2_ADDR, 0x00);
	if (regval & 0x8)
		GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR3_ADDR, 0x00);
	if (regval & 0x10)
		GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR4_ADDR, 0x00);
	if (regval & 0x20)
		GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR5_ADDR, 0x00);
	if (regval & 0x40)
		GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR6_ADDR, 0x00);
	if (regval & 0x80)
		GA_REG_BYTE_WRITE(CPU_INTCPU_2_EXTCPU_INTR7_ADDR, 0x00);

	/* Clear interrupts */
	GA_REG_BYTE_WRITE(CPU_INTR_CLR0_EXTREG_ADDR, masksts0);
	GA_REG_BYTE_WRITE(CPU_INTR_CLR1_EXTREG_ADDR, masksts1);
	GA_REG_BYTE_WRITE(CPU_INTR_CLR2_EXTREG_ADDR, masksts2);
	GA_REG_BYTE_WRITE(CPU_INTR_CLR3_EXTREG_ADDR, masksts3);
	GA_REG_BYTE_WRITE(CPU_INTR_CLR4_EXTREG_ADDR, masksts4);

	if (((masksts1 & 0x0F) & (~enreg1)) ||
		((masksts2 & 0x0F) & (~enreg2))) {
		/* Interrupt for HDMI port 0 */
		if (((masksts1 | masksts2) & 0x01) == 0x01) {
			HdmirxHandle(hAvifCtx, 0);
		}

		/* Interrupt for HDMI port 1 */
		if (((masksts1 | masksts2) & 0x02) == 0x02) {
			HdmirxHandle(hAvifCtx, 1);
		}

		/* Interrupt for HDMI port 2 */
		if (((masksts1 | masksts2) & 0x04) == 0x04) {
			HdmirxHandle(hAvifCtx, 2);
		}

		/* Interrupt for HDMI port 3 */
		if (((masksts1 | masksts2) & 0x08) == 0x08) {
			HdmirxHandle(hAvifCtx, 3);
		}
	}
#endif

	channel = GetAvifDhubAudioWRChnl(hAvifCtx->IsPIPAudio);

	/* DHUB Interrupt status */
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	pSemHandle = dhub_semaphore(&AVIF_dhubHandle.dhub);
#else
	pSemHandle = dhub_semaphore(&AG_dhubHandle.dhub);
#endif
	instat = semaphore_chk_full(pSemHandle, -1);

#ifdef CONFIG_MV_AMP_COMPONENT_AIP_ENABLE
	/* Audio DHUB INterrupt handling */
	{
		INT chanId;

		for (chanId = channel; chanId <= (channel + 3); chanId++) {
			if (bTST(instat, chanId)) {
				semaphore_pop(pSemHandle, chanId, 1);
				semaphore_clr_full(pSemHandle, chanId);
				if (chanId == channel) {
					MV_CC_MSG_t msg = {0, 0, 0};
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
					aip_avif_resume_cmd(hAvifCtx);
#else
					aip_resume_cmd(hAvifCtx);
#endif

					msg.m_MsgID = 1 << chanId;
					AMPMsgQ_Add(&hAvifCtx->hAIPMsgQ, &msg);
					up(&hAvifCtx->aip_sem);
				}
			}
		}
	}
#endif

#ifdef KCONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE
	if (bTST(instat, avif_dhub_config_ChMap_avif_AUD_RD0)) {
		MV_CC_MSG_t msg = { 0, 0, 0 };

		semaphore_pop(pSemHandle,
			avif_dhub_config_ChMap_avif_AUD_RD0, 1);
		semaphore_clr_full(pSemHandle,
			avif_dhub_config_ChMap_avif_AUD_RD0);
		arc_copy_spdiftx_data(hAvifCtx);

#if (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
		msg.m_MsgID = 1 << avioDhubChMap_aio64b_SPDIF_TX_R;
#else
		msg.m_MsgID = 1 << avioDhubChMap_ag_SPDIF_R;
#endif
		send_msg2aout(&msg);
	}
#endif

#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	/* Send VDE interrupt info to AVIF driver */
	if (instat0 || instat1) {
		MV_CC_MSG_t msg = {
			0,
			instat0,
			instat1
		};
		rc = AMPMsgQ_Add(&hAvifCtx->hPEAVIFMsgQ, &msg);
		if (rc != S_OK) {
			if (!atomic_read(&hAvifCtx->avif_isr_msg_err_cnt)) {
				amp_error("[AVIF isr] MsgQ full\n");
			}
			atomic_inc(&hAvifCtx->avif_isr_msg_err_cnt);
			return IRQ_HANDLED;
		}
		up(&hAvifCtx->avif_sem);
	}
#endif

	return IRQ_HANDLED;
}

#endif

#if defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
void aip_start_cmd(AVIF_CTX *hAvifCtx, INT *aip_info, void *param)
{
	INT *p = aip_info;
	INT chanId;
	HDL_dhub *dhub = NULL;
	AIP_DMA_CMD *p_dma_cmd;
	AIP_DMA_CMD_FIFO *pCmdFifo = NULL;

	if (!hAvifCtx) {
		amp_error("aip_resume_cmd null handler!\n");
		return;
	}

	hAvifCtx->aip_source = aip_info[2];
	hAvifCtx->p_aip_cmdfifo = pCmdFifo = (AIP_DMA_CMD_FIFO *) param;

	pCmdFifo = hAvifCtx->p_aip_cmdfifo;
	if (!pCmdFifo) {
		amp_trace("aip_resume_cmd:p_aip_fifo is NULL\n");
		return;
	}

	if (*p == 1) {
		hAvifCtx->aip_i2s_pair = 1;
		p_dma_cmd =
		    (AIP_DMA_CMD *) AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, 0);

		if (AIP_SOUECE_SPDIF == hAvifCtx->aip_source) {
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_DTV)
			chanId = avioDhubChMap_vpp_SPDIF_W;
			dhub = &VPP_dhubHandle.dhub;
			wrap_dhub_channel_write_cmd(dhub, chanId,
						    p_dma_cmd->addr0,
						    p_dma_cmd->size0, 0, 0, 0,
						    1, 0, 0);
#else
			chanId = avioDhubChMap_aio64b_SPDIF_RX_W;
			dhub = &AG_dhubHandle.dhub;
			dhub_channel_write_cmd(dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
#endif
#endif
		} else {
#if ((BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0) && \
		(BERLIN_CHIP_VERSION != BERLIN_BG4_DTV))
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
			chanId = avioDhubChMap_ag_PDM_MIC_ch1;
#else
			chanId = avioDhubChMap_aio64b_HDMIRX_CH3_W;
#endif
			dhub = &AG_dhubHandle.dhub;
			dhub_channel_write_cmd(dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
#endif
		}
		AIPFifoKernelPreRdUpdate(hAvifCtx->p_aip_cmdfifo, 1);

		/* push 2nd dHub command */
		p_dma_cmd =
			(AIP_DMA_CMD *) AIPFifoGetKernelPreRdDMAInfo(
						hAvifCtx->p_aip_cmdfifo, 0);
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_DTV)
		if (chanId == avioDhubChMap_vpp_SPDIF_W) {
			wrap_dhub_channel_write_cmd(dhub, chanId,
						    p_dma_cmd->addr0,
						    p_dma_cmd->size0, 0, 0, 0,
						    1, 0, 0);
		}
#else
		if (chanId == avioDhubChMap_aio64b_SPDIF_RX_W) {
			dhub_channel_write_cmd(dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
		}
#endif
		else
#endif
		{
			dhub_channel_write_cmd(dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
		}
		AIPFifoKernelPreRdUpdate(hAvifCtx->p_aip_cmdfifo, 1);
	} else if (*p == 4) {
#if ((BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0) && \
			(BERLIN_CHIP_VERSION != BERLIN_BG4_DTV))
		UINT pair;

		hAvifCtx->aip_i2s_pair = 4;
		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
			    AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
			chanId = avioDhubChMap_ag_PDM_MIC_ch1 + pair;
#else
			chanId = avioDhubChMap_aio64b_HDMIRX_CH3_W +
			pair;
#endif
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
		}

		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);

		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
				AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
			chanId = avioDhubChMap_ag_PDM_MIC_ch1 + pair;
#else
			chanId = avioDhubChMap_aio64b_HDMIRX_CH3_W +
						pair;
#endif
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
#endif
	}
}

void aip_stop_cmd(AVIF_CTX *hAvifCtx)
{
	if (!hAvifCtx) {
		amp_error("aip_stop_cmd null handler!\n");
		return;
	}
	hAvifCtx->p_aip_cmdfifo = NULL;
}

void aip_resume_cmd(AVIF_CTX *hAvifCtx)
{
	AIP_DMA_CMD *p_dma_cmd;
	HDL_dhub *dhub = NULL;
	UINT chanId;
	INT pair;
	AIP_DMA_CMD_FIFO *pCmdFifo;

	if (!hAvifCtx) {
		amp_error("aip_resume_cmd null handler!\n");
		return;
	}

	pCmdFifo = hAvifCtx->p_aip_cmdfifo;
	if (!pCmdFifo) {
		amp_trace("aip_resume_cmd:p_aip_fifo is NULL\n");
		return;
	}

	spin_lock(&hAvifCtx->aip_spinlock);

	if (!pCmdFifo->fifo_overflow) {
		AIPFifoKernelRdUpdate(pCmdFifo, 1);
	}

	if (AIPFifoCheckKernelFullness(pCmdFifo)) {
		pCmdFifo->fifo_overflow = 0;
		for (pair = 0; pair < hAvifCtx->aip_i2s_pair; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
				AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			if (AIP_SOUECE_SPDIF == hAvifCtx->aip_source) {
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_DTV)
				chanId = avioDhubChMap_vpp_SPDIF_W;
				dhub = &VPP_dhubHandle.dhub;
				wrap_dhub_channel_write_cmd(dhub, chanId,
							    p_dma_cmd->addr0,
							    p_dma_cmd->size0, 0,
							    0, 0,
							    p_dma_cmd->
							    addr1 ? 0 : 1, 0,
							    0);
				if (p_dma_cmd->addr1) {
					wrap_dhub_channel_write_cmd(dhub,
								    chanId,
								    p_dma_cmd->
								    addr1,
								    p_dma_cmd->
								    size1, 0, 0,
								    0, 1, 0, 0);
				}
#else
				chanId = avioDhubChMap_aio64b_SPDIF_RX_W;
				dhub = &AG_dhubHandle.dhub;
				dhub_channel_write_cmd(dhub, chanId,
						       p_dma_cmd->addr0,
						       p_dma_cmd->size0, 0, 0,
						       0,
						       p_dma_cmd->addr1 ? 0 : 1,
						       0, 0);
				if (p_dma_cmd->addr1) {
					dhub_channel_write_cmd(dhub,
							       chanId,
							       p_dma_cmd->addr1,
							       p_dma_cmd->size1,
							       0, 0, 0, 1, 0,
							       0);
				}
#endif
#endif
			} else {
#if ((BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0) && \
		(BERLIN_CHIP_VERSION != BERLIN_BG4_DTV))
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
				chanId = avioDhubChMap_ag_PDM_MIC_ch1 + pair;
#else
				chanId = avioDhubChMap_aio64b_HDMIRX_CH3_W
							+ pair;
#endif
				dhub = &AG_dhubHandle.dhub;
				dhub_channel_write_cmd(dhub, chanId,
						       p_dma_cmd->addr0,
						       p_dma_cmd->size0, 0, 0,
						       0,
						       p_dma_cmd->addr1 ? 0 : 1,
						       0, 0);
				if (p_dma_cmd->addr1) {
					dhub_channel_write_cmd(dhub,
							       chanId,
							       p_dma_cmd->addr1,
							       p_dma_cmd->size1,
							       0, 0, 0, 1, 0,
							       0);
				}
#endif
			}
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	} else {
		pCmdFifo->fifo_overflow = 1;
		pCmdFifo->fifo_overflow_cnt++;
		for (pair = 0; pair < hAvifCtx->aip_i2s_pair; pair++) {
			/* FIXME:
			 *chanid should be changed if 4 pair is supported
			 */
			if (AIP_SOUECE_SPDIF == hAvifCtx->aip_source) {
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_DTV)
				chanId = avioDhubChMap_vpp_SPDIF_W;
				dhub = &VPP_dhubHandle.dhub;
				wrap_dhub_channel_write_cmd(dhub, chanId,
						pCmdFifo->
						overflow_buffer,
						pCmdFifo->
						overflow_buffer_size,
						0, 0, 0, 1, 0, 0);
#else
				chanId = avioDhubChMap_aio64b_SPDIF_RX_W;
				dhub = &AG_dhubHandle.dhub;
				dhub_channel_write_cmd(dhub, chanId,
						       pCmdFifo->
						       overflow_buffer,
						       pCmdFifo->
						       overflow_buffer_size, 0,
						       0, 0, 1, 0, 0);
#endif
#endif
			} else {
#if ((BERLIN_CHIP_VERSION >= BERLIN_BG2_DTV_A0) && \
			(BERLIN_CHIP_VERSION != BERLIN_BG4_DTV))
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
				chanId = avioDhubChMap_ag_PDM_MIC_ch1 + pair;
#else
				chanId = avioDhubChMap_aio64b_HDMIRX_CH3_W
							+ pair;
#endif
				dhub = &AG_dhubHandle.dhub;
				dhub_channel_write_cmd(dhub, chanId,
						       pCmdFifo->
						       overflow_buffer,
						       pCmdFifo->
						       overflow_buffer_size, 0,
						       0, 0, 1, 0, 0);
#endif
			}
		}
	}

	spin_unlock(&hAvifCtx->aip_spinlock);
}

VOID aip_resume(VOID)
{
	aip_resume_cmd(&avif_ctx);
}

VOID send_msg2avif(MV_CC_MSG_t *pMsg)
{
	AVIF_CTX *hAvifCtx = &avif_ctx;
	INT channel = GetAvifDhubAudioWRChnl(hAvifCtx->IsPIPAudio);

	pMsg->m_MsgID = (1 << channel);
	AMPMsgQ_Add(&hAvifCtx->hAIPMsgQ, pMsg);
	up(&hAvifCtx->aip_sem);
}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE
VOID arc_copy_spdiftx_data(AVIF_CTX *hAvifCtx)
{
	AOUT_DMA_INFO *p_dma_info;
	UINT chanId;
	AOUT_PATH_CMD_FIFO *p_arc_fifo = &hAvifCtx->arc_fifo;

	if (!p_arc_fifo->fifo_underflow)
		AoutFifoKernelRdUpdate(p_arc_fifo, 1);

	if (AoutFifoCheckKernelFullness(p_arc_fifo)) {
		p_arc_fifo->fifo_underflow = 0;
		p_dma_info = (AOUT_DMA_INFO *)
		    AoutFifoGetKernelRdDMAInfo(p_arc_fifo, 0);
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
		chanId = avioDhubChMap_aio64b_SPDIF_TX_R;
#else
		chanId = avioDhubChMap_ag_SPDIF_R;
#endif
		dhub_channel_write_cmd(&AVIF_dhubHandle.dhub,
				       avif_dhub_config_ChMap_avif_AUD_RD0,
				       p_dma_info->addr0,
				       p_dma_info->size0, 0, 0, 0,
				       p_dma_info->size1 ? 0 : 1, 0, 0);
		if (p_dma_info->size1) {
			dhub_channel_write_cmd(&AVIF_dhubHandle.dhub,
					avif_dhub_config_ChMap_avif_AUD_RD0,
					p_dma_info->addr1,
					p_dma_info->size1, 0, 0,
					0, 1, 0, 0);
		}
	} else {
		p_arc_fifo->fifo_underflow = 1;
		p_arc_fifo->underflow_cnt++;
		dhub_channel_write_cmd(&AVIF_dhubHandle.dhub,
				       avif_dhub_config_ChMap_avif_AUD_RD0,
				       p_arc_fifo->zero_buffer,
				       p_arc_fifo->zero_buffer_size, 0, 0, 0, 1,
				       0, 0);
	}
	return;
}

void aip_avif_start_cmd(AVIF_CTX *hAvifCtx, INT *aip_info, void *param)
{
	INT *p = aip_info;
	INT channel, pair;
	AIP_DMA_CMD *p_dma_cmd;
	AIP_DMA_CMD_FIFO *pCmdFifo;

	if (!hAvifCtx) {
		amp_error("aip_avif_start_cmd null handler!\n");
		return;
	}

	hAvifCtx->p_aip_cmdfifo = pCmdFifo = (AIP_DMA_CMD_FIFO *) param;
	if (!pCmdFifo) {
		amp_trace("aip_avif_start_cmd: p_aip_fifo is NULL\n");
		return;
	}

	channel = GetAvifDhubAudioWRChnl(hAvifCtx->IsPIPAudio);
	if (*p == 1) {
		hAvifCtx->aip_i2s_pair = 1;
		pCmdFifo = (AIP_DMA_CMD_FIFO *) param;
		p_dma_cmd =
		    (AIP_DMA_CMD *) AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, 0);

		dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, channel,
				       p_dma_cmd->addr0, p_dma_cmd->size0, 0, 0,
				       0, 1, 0, 0);
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);

		/* push 2nd dHub command */
		p_dma_cmd =
		    (AIP_DMA_CMD *) AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, 0);
		dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, channel,
				       p_dma_cmd->addr0, p_dma_cmd->size0, 0, 0,
				       0, 1, 0, 0);
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	} else if (*p == 4) {
		/* 4 I2S will be INTroduced since BG2 A0 */
		hAvifCtx->aip_i2s_pair = 4;

		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
			    AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			dhub_channel_write_cmd(&AVIF_dhubHandle.dhub,
					       (channel + pair),
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);

		/* push 2nd dHub command */
		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
			    AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			dhub_channel_write_cmd(&AVIF_dhubHandle.dhub,
					       (channel + pair),
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0, 1, 0,
					       0);
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	}
}

void aip_avif_stop_cmd(AVIF_CTX *hAvifCtx)
{
	if (!hAvifCtx) {
		amp_error("aip_stop_cmd null handler!\n");
		return;
	}
	hAvifCtx->p_aip_cmdfifo = NULL;
}

void aip_avif_resume_cmd(AVIF_CTX *hAvifCtx)
{
	AIP_DMA_CMD *p_dma_cmd;
	UINT chanId, channel;
	INT pair;
	AIP_DMA_CMD_FIFO *pCmdFifo;

	if (!hAvifCtx) {
		amp_error("aip_avif_resume_cmd null handler!\n");
		return;
	}

	pCmdFifo = hAvifCtx->p_aip_cmdfifo;
	if (!pCmdFifo) {
		amp_trace("aip_avif_resume_cmd:p_aip_fifo is NULL\n");
		return;
	}

	if (!pCmdFifo->fifo_overflow)
		AIPFifoKernelRdUpdate(pCmdFifo, 1);

	channel = GetAvifDhubAudioWRChnl(hAvifCtx->IsPIPAudio);

	if (AIPFifoCheckKernelFullness(pCmdFifo)) {
		pCmdFifo->fifo_overflow = 0;
		for (pair = 0; pair < hAvifCtx->aip_i2s_pair; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
			    AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			chanId = channel + pair;
			dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, chanId,
					       p_dma_cmd->addr0,
					       p_dma_cmd->size0, 0, 0, 0,
					       p_dma_cmd->addr1 ? 0 : 1, 0, 0);
			if (p_dma_cmd->addr1) {
				dhub_channel_write_cmd(&AVIF_dhubHandle.dhub,
						       chanId, p_dma_cmd->addr1,
						       p_dma_cmd->size1, 0, 0,
						       0, 1, 0, 0);
			}
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	} else {
		pCmdFifo->fifo_overflow = 1;
		pCmdFifo->fifo_overflow_cnt++;
		for (pair = 0; pair < hAvifCtx->aip_i2s_pair; pair++) {
			/* FIXME:
			 * chanid should be changed if 4 pair is supported
			 */
			chanId = channel + pair;
			dhub_channel_write_cmd(&AVIF_dhubHandle.dhub, chanId,
					       pCmdFifo->overflow_buffer,
					       pCmdFifo->overflow_buffer_size,
					       0, 0, 0, 1, 0, 0);
		}
	}
}

#endif /* CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE */
