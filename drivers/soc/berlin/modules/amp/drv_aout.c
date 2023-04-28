/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>

#include "galois_io.h"
#include "cinclude.h"
#include "amp_type.h"
#include "amp_ioctl.h"
#include "drv_aout.h"
#include "drv_msg.h"
#include "drv_debug.h"
#include "api_dhub.h"
#include "api_avio_dhub.h"
#include "hal_dhub_wrap.h"
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_CDP)
#include "amp_driver.h"
#endif
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
#include "drv_avif.h"
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
#include "drv_app.h"
#endif

/* Static vara List */
irqreturn_t amp_devices_avif_isr(int irq, void *dev_id);
static INT32 pri_audio_chanId[4] = {
#if ((BERLIN_CHIP_VERSION == BERLIN_BG4_CD) || (BERLIN_CHIP_VERSION == BERLIN_BG4_CT) || \
	(BERLIN_CHIP_VERSION == BERLIN_BG4_CT_A0) || (BERLIN_CHIP_VERSION == BERLIN_BG4_CDP))
	avioDhubChMap_aio64b_MA0_R,
	avioDhubChMap_aio64b_MA1_R,
	avioDhubChMap_aio64b_MA2_R,
	avioDhubChMap_aio64b_MA3_R,
#elif(BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
	avioDhubChMap_aio64b_MA_R0,
	avioDhubChMap_aio64b_MA_R1,
	avioDhubChMap_aio64b_MA_R2,
	avioDhubChMap_aio64b_MA_R3,
#else
	avioDhubChMap_ag_MA0_R,
	avioDhubChMap_ag_MA1_R,
	avioDhubChMap_ag_MA2_R,
	avioDhubChMap_ag_MA3_R,
#endif
};

static AOUT_CTX aout_ctx;
/* Don't wait for HDMI path, keep 5 for MULTI, LoRo, SPDIF paths */
static const INT aout_fifo_cmd_wait_num[MAX_OUTPUT_AUDIO + 1] =
    { 0, 0, 0, 0, 0, 0 };

static HRESULT AoutGetChennalAndDhub(INT path, HDL_dhub ** pDhub, int *pChanId)
{
	HRESULT rc = SUCCESS;
	if (MULTI_PATH == path) {
		*pChanId = pri_audio_chanId[0];
		*pDhub = &AG_dhubHandle.dhub;
	} else if (HDMI_PATH == path) {
#if ((BERLIN_CHIP_VERSION == BERLIN_BG4_CD) || (BERLIN_CHIP_VERSION == BERLIN_BG4_CT) || \
        (BERLIN_CHIP_VERSION == BERLIN_BG4_CT_A0) || (BERLIN_CHIP_VERSION == BERLIN_BG4_CDP))
		*pDhub = &AG_dhubHandle.dhub;
		*pChanId = avioDhubChMap_aio64b_HDMI_R;
#elif (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
		*pDhub = &AG_dhubHandle.dhub;
		*pChanId = avioDhubChMap_aio64b_HDMI_RD;
#else
		*pDhub = &VPP_dhubHandle.dhub;
		*pChanId = avioDhubChMap_vpp_HDMI_R;
#endif
#if ((BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && \
        (BERLIN_CHIP_VERSION != BERLIN_BG2_CDP) && \
		(BERLIN_CHIP_VERSION != BERLIN_BG4_CDP) && \
		(BERLIN_CHIP_VERSION != BERLIN_BG4_CD))
	} else if (SPDIF_PATH == path) {
#if (BERLIN_CHIP_VERSION >= BERLIN_BG4_CT)
		*pChanId = avioDhubChMap_aio64b_SPDIF_TX_R;
#else
		*pChanId = avioDhubChMap_ag_SPDIF_R;
#endif
		*pDhub = &AG_dhubHandle.dhub;
#endif
#if ((BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && \
	(BERLIN_CHIP_VERSION != BERLIN_BG2_CDP))
	} else if (LoRo_PATH == path) {
#if (BERLIN_CHIP_VERSION >= BERLIN_BG4_CD)
/* avioDhubChMap_aio64b_SA_R in Z1 is same as PCM MIC CH3
*  in A0
*/
		*pChanId = avioDhubChMap_aio64b_SA_R;
#else
		*pChanId = avioDhubChMap_ag_SA_R;
#endif
		*pDhub = &AG_dhubHandle.dhub;
#endif
	} else {
		//MULTI path not use this function
		amp_trace("%s %d: error path[%d]. ", __FUNCTION__, __LINE__,
			  path);
	}

	return rc;
}

static HRESULT AoutGetChennal(INT path, int *pChanId)
{
	HRESULT rc = SUCCESS;

	if (HDMI_PATH == path) {
#if ((BERLIN_CHIP_VERSION == BERLIN_BG4_CD) || (BERLIN_CHIP_VERSION == BERLIN_BG4_CT) || \
        (BERLIN_CHIP_VERSION == BERLIN_BG4_CT_A0) || (BERLIN_CHIP_VERSION == BERLIN_BG4_CDP))
		*pChanId = avioDhubChMap_aio64b_HDMI_R;
#elif (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
		*pChanId = avioDhubChMap_aio64b_HDMI_RD;
#else
		*pChanId = avioDhubChMap_vpp_HDMI_R;
#endif
#if ((BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && \
            (BERLIN_CHIP_VERSION != BERLIN_BG2_CDP) && \
            (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP) && \
            (BERLIN_CHIP_VERSION != BERLIN_BG4_CD))
	} else if (SPDIF_PATH == path) {
#if (BERLIN_CHIP_VERSION >= BERLIN_BG4_CT)
		*pChanId = avioDhubChMap_aio64b_SPDIF_TX_R;
#else
		*pChanId = avioDhubChMap_ag_SPDIF_R;
#endif
#endif /* (BERLIN_CHIP_VERSION != BERLIN_BG2_CD) */
#if ((BERLIN_CHIP_VERSION != BERLIN_BG2_CD) && \
            (BERLIN_CHIP_VERSION != BERLIN_BG2_CDP))
	} else if (LoRo_PATH == path) {
#if (BERLIN_CHIP_VERSION >= BERLIN_BG4_CD)
/* avioDhubChMap_aio64b_SA_R in Z1 is same as PCM MIC CH3
*  in A0
*/
		*pChanId = avioDhubChMap_aio64b_SA_R;
#else
		*pChanId = avioDhubChMap_ag_SA_R;
#endif
#endif
	} else {
		//MULTI path not use this function
		amp_trace("%s %d: error path[%d]. ", __FUNCTION__, __LINE__,
			  path);
	}

	return rc;
}

static void *AoutFifoGetKernelRdDMAInfoByIndex(AOUT_PATH_CMD_FIFO *
					       p_aout_cmd_fifo, int pair)
{
	void *pHandle;

	pHandle =
	    &(p_aout_cmd_fifo->aout_dma_info[pair]
	      [p_aout_cmd_fifo->aout_pre_read]);

	return pHandle;
}

VOID *AoutFifoGetKernelRdDMAInfo(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo, INT pair)
{
	VOID *pHandle;
	INT rd_offset = p_aout_cmd_fifo->kernel_rd_offset;
	if (rd_offset > p_aout_cmd_fifo->size || rd_offset < 0) {
		INT i = 0, fifo_cmd_size = sizeof(AOUT_PATH_CMD_FIFO) >> 2;
		INT *temp = (INT *) p_aout_cmd_fifo;
		amp_trace
		    ("AOUT FIFO memory %p is corrupted! corrupted data :\n",
		     p_aout_cmd_fifo);
		for (i = 0; i < fifo_cmd_size; i++) {
			amp_trace("0x%x\n", *temp++);
		}
		rd_offset = 0;
	}
	pHandle = &(p_aout_cmd_fifo->aout_dma_info[pair][rd_offset]);

	return pHandle;
}

VOID AoutFifoKernelRdUpdate(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo, INT adv)
{
	struct timespec kernel_update_time;
	p_aout_cmd_fifo->kernel_rd_offset += adv;
	if (p_aout_cmd_fifo->kernel_rd_offset >= p_aout_cmd_fifo->size)
		p_aout_cmd_fifo->kernel_rd_offset -= p_aout_cmd_fifo->size;
	ktime_get_ts(&kernel_update_time);
	p_aout_cmd_fifo->ts_sec = kernel_update_time.tv_sec;
	p_aout_cmd_fifo->ts_nsec = kernel_update_time.tv_nsec;
}

INT AoutFifoCheckKernelFullness(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo)
{
	INT full;
	full = p_aout_cmd_fifo->wr_offset - p_aout_cmd_fifo->kernel_rd_offset;
	if (full < 0)
		full += p_aout_cmd_fifo->size;
	return full;
}

static INT AoutFifoCheckKernelPreFullness(AOUT_PATH_CMD_FIFO * p_aout_cmd_fifo)
{
	INT full;

	full = p_aout_cmd_fifo->wr_offset - p_aout_cmd_fifo->aout_pre_read;
	if (full < 0)
		full += p_aout_cmd_fifo->size;
	return full;
}

irqreturn_t amp_devices_aout_isr(int irq, void *dev_id)
{
	int instat;
	UNSG32 chanId;
	HDL_semaphore *pSemHandle;
	AOUT_CTX *hAoutCtx = (AOUT_CTX *) dev_id;
	HRESULT rc;
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_CDP)
#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
	if (amp_get_vipenable_status())
		amp_devices_avif_isr(irq, NULL);
#endif
#endif

	pSemHandle = dhub_semaphore(&AG_dhubHandle.dhub);
	instat = semaphore_chk_full(pSemHandle, -1);

	for (chanId = pri_audio_chanId[0]; chanId <= pri_audio_chanId[3];
	     chanId++) {
		if (bTST(instat, chanId)) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);

			if (chanId == pri_audio_chanId[0]) {
				MV_CC_MSG_t msg = { 0, 0, 0 };
				aout_resume_cmd(hAoutCtx, MULTI_PATH);
				msg.m_MsgID = 1 << chanId;
				spin_lock(&hAoutCtx->aout_msg_spinlock);
				rc = AMPMsgQ_Add(&hAoutCtx->hAoutMsgQ, &msg);
				spin_unlock(&hAoutCtx->aout_msg_spinlock);
				if (S_OK == rc) {
					up(&hAoutCtx->aout_sem);
				}
			}
		}
	}

/* HDMI is part of AG DHUB for BG4 CT/CD/DTV */
#if (BERLIN_CHIP_VERSION >= BERLIN_BG4_CD)
	rc = AoutGetChennal(HDMI_PATH, &chanId);
	if ((rc == SUCCESS) && (bTST(instat, chanId))) {
		MV_CC_MSG_t msg = { 0, 0, 0 };
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		aout_resume_cmd(hAoutCtx, HDMI_PATH);
		msg.m_MsgID = 1 << chanId;
		spin_lock(&hAoutCtx->aout_msg_spinlock);
		rc = AMPMsgQ_Add(&hAoutCtx->hAoutMsgQ, &msg);
		spin_unlock(&hAoutCtx->aout_msg_spinlock);
		if (S_OK == rc) {
			up(&hAoutCtx->aout_sem);
		}
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_AIP_ENABLE
#if (BERLIN_CHIP_VERSION == BERLIN_BG2_DTV_A0)
	chanId = avioDhubChMap_ag_PDM_MIC_ch1;
	if (bTST(instat, chanId)) {
		MV_CC_MSG_t msg = { 0, 0, 0 };
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		aip_resume();
		send_msg2avif(&msg);
	}
#endif
#endif

#ifdef KCONFIG_MV_AMP_AUDIO_PATH_20_ENABLE
	rc = AoutGetChennal(LoRo_PATH, &chanId);
	if ((rc == SUCCESS) && (bTST(instat, chanId))) {
		MV_CC_MSG_t msg = { 0, 0, 0 };
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		aout_resume_cmd(hAoutCtx, LoRo_PATH);

		msg.m_MsgID = 1 << chanId;
		spin_lock(&hAoutCtx->aout_msg_spinlock);
		rc = AMPMsgQ_Add(&hAoutCtx->hAoutMsgQ, &msg);
		spin_unlock(&hAoutCtx->aout_msg_spinlock);
		if (S_OK == rc) {
			up(&hAoutCtx->aout_sem);
		}
	}
#endif

#ifdef KCONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE
	rc = AoutGetChennal(SPDIF_PATH, &chanId);
	if ((rc == SUCCESS) && (bTST(instat, chanId))) {
		MV_CC_MSG_t msg = { 0, 0, 0 };
		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		aout_resume_cmd(hAoutCtx, SPDIF_PATH);

		msg.m_MsgID = 1 << chanId;
		spin_lock(&hAoutCtx->aout_msg_spinlock);
		rc = AMPMsgQ_Add(&hAoutCtx->hAoutMsgQ, &msg);
		spin_unlock(&hAoutCtx->aout_msg_spinlock);
		if (S_OK == rc) {
			up(&hAoutCtx->aout_sem);
		}
	}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
	chanId = avioDhubSemMap_ag_app_intr2;
	if (bTST(instat, chanId)) {
		MV_CC_MSG_t msg = { 0, 0, 0 };

		semaphore_pop(pSemHandle, chanId, 1);
		semaphore_clr_full(pSemHandle, chanId);
		app_resume();

		msg.m_MsgID = (1 << avioDhubSemMap_ag_app_intr2);
		send_msg2app(&msg);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_AIP_ENABLE
#if(BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
	if (hAoutCtx->is_spdifrx_enabled) {
		UINT chanId = avioDhubChMap_aio64b_SPDIF_RX_W;
		if (bTST(instat, chanId)) {
			semaphore_pop(pSemHandle, chanId, 1);
			semaphore_clr_full(pSemHandle, chanId);
			aip_resume();
		}
	}
#endif
#endif

	return IRQ_HANDLED;
}

VOID send_msg2aout(MV_CC_MSG_t * pMsg)
{
	AOUT_CTX *hAoutCtx = &aout_ctx;

	spin_lock(&hAoutCtx->aout_msg_spinlock);
	AMPMsgQ_Add(&hAoutCtx->hAoutMsgQ, pMsg);
	spin_unlock(&hAoutCtx->aout_msg_spinlock);
	up(&hAoutCtx->aout_sem);
}

VOID aout_resume(INT path_id)
{
	aout_resume_cmd(&aout_ctx, path_id);
}

VOID aout_start_cmd(AOUT_CTX * hAoutCtx, INT * aout_info, VOID * param)
{
	INT *p = aout_info;
	INT i, chanId = 0;
	HDL_dhub *dHub_handle;
	AOUT_DMA_INFO *p_dma_info;
	HRESULT rc = SUCCESS;

	if (*p == MULTI_PATH) {
		hAoutCtx->p_ma_fifo = (AOUT_PATH_CMD_FIFO *) param;
		for (i = 0; i < 4; i++) {
			p_dma_info = (AOUT_DMA_INFO *)
			    AoutFifoGetKernelRdDMAInfo(hAoutCtx->p_ma_fifo, i);
			chanId = pri_audio_chanId[i];
			dhub_channel_write_cmd(&AG_dhubHandle.dhub, chanId,
					       p_dma_info->addr0,
					       p_dma_info->size0, 0, 0, 0,
					       i == 0 ? 1 : 0, 0, 0);
		}
	}
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_20_ENABLE
	else if (*p == LoRo_PATH) {
		hAoutCtx->p_sa_fifo = (AOUT_PATH_CMD_FIFO *) param;
		p_dma_info =
		    (AOUT_DMA_INFO *)
		    AoutFifoGetKernelRdDMAInfo(hAoutCtx->p_sa_fifo, 0);
		rc = AoutGetChennalAndDhub(*p, &dHub_handle, &chanId);
		if (rc == SUCCESS) {

			dhub_channel_write_cmd(dHub_handle, chanId,
					       p_dma_info->addr0,
					       p_dma_info->size0, 0, 0, 0, 1, 0,
					       0);
		}
	}
#endif
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE
	else if (*p == SPDIF_PATH) {
		int dhub_space;

		hAoutCtx->p_spdif_fifo = (AOUT_PATH_CMD_FIFO *) param;
		p_dma_info =
		    (AOUT_DMA_INFO *)
		    AoutFifoGetKernelRdDMAInfo(hAoutCtx->p_spdif_fifo, 0);
		rc = AoutGetChennalAndDhub(*p, &dHub_handle, &chanId);
		if (rc == SUCCESS) {
			dhub_channel_write_cmd(dHub_handle, chanId,
					       p_dma_info->addr0,
					       p_dma_info->size0, 0, 0, 0, 1, 0,
					       0);
		}
	}
#endif
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE
	else if (*p == HDMI_PATH) {
		hAoutCtx->p_hdmi_fifo = (AOUT_PATH_CMD_FIFO *) param;
		p_dma_info =
		    (AOUT_DMA_INFO *)
		    AoutFifoGetKernelRdDMAInfo(hAoutCtx->p_hdmi_fifo, 0);
		rc = AoutGetChennalAndDhub(*p, &dHub_handle, &chanId);
		if (rc == SUCCESS) {
			wrap_dhub_channel_write_cmd(dHub_handle, chanId,
						    p_dma_info->addr0,
						    p_dma_info->size0, 0, 0, 0,
						    1, 0, 0);
		}
	}
#endif
}

VOID aout_stop_cmd(AOUT_CTX * hAoutCtx, INT path_id)
{
	if (path_id == MULTI_PATH) {
		hAoutCtx->p_ma_fifo = NULL;
	}
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_20_ENABLE
	else if (path_id == LoRo_PATH) {
		hAoutCtx->p_sa_fifo = NULL;
	}
#endif
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE
	else if (path_id == SPDIF_PATH) {
		hAoutCtx->p_spdif_fifo = NULL;
	}
#endif
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE
	else if (path_id == HDMI_PATH) {
		hAoutCtx->p_hdmi_fifo = NULL;
	}
#endif
}

VOID aout_resume_cmd(AOUT_CTX * hAoutCtx, INT path_id)
{
	AOUT_DMA_INFO *p_dma_info;
	UINT i, chanId, chanId_real, loop_path_cnt;
	int kernel_full;
	HDL_dhub *dHub_handle;
	unsigned int dhub_space;
	HRESULT rc = SUCCESS;
	AOUT_PATH_CMD_FIFO *p_path_fifo = NULL;
	UNSG32(*fp_dhub_channel_write_cmd) (void *hdl,  /*!    Handle to HDL_dhub ! */
		SIGN32 id,  /*!    Channel ID in $dHubReg ! */
		UNSG32 addr,        /*!    CMD: buffer address ! */
		SIGN32 size,        /*!    CMD: number of bytes to transfer ! */
		SIGN32 semOnMTU,    /*!    CMD: semaphore operation at CMD/MTU (0/1) ! */
		SIGN32 chkSemId,    /*!    CMD: non-zero to check semaphore ! */
		SIGN32 updSemId,    /*!    CMD: non-zero to update semaphore ! */
		SIGN32 interrupt,   /*!    CMD: raise interrupt at CMD finish ! */
		T64b cfgQ[],        /*!    Pass NULL to directly update dHub, or
									Pass non-zero to receive programming sequence
									in (adr,data) pairs
							! */
		UNSG32 * ptr        /*!    Pass in current cmdQ pointer (in 64b word),
									& receive updated new pointer,
									Pass NULL to read from HW
							! */
		) = NULL;

	if (path_id > MAX_OUTPUT_AUDIO) {
		amp_error("path_id == %d > MAX_OUTPUT_AUDIO!\n", path_id);
		return;
	}

	if (!hAoutCtx) {
		amp_error("hAoutCtx == NULL!\n");
		return;
	}

	hAoutCtx->aout_int_cnt[path_id]++;

	switch (path_id) {
	case MULTI_PATH:
		p_path_fifo = hAoutCtx->p_ma_fifo;
		loop_path_cnt = 4;
		fp_dhub_channel_write_cmd = dhub_channel_write_cmd;
		break;
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_20_ENABLE
	case LoRo_PATH:
		p_path_fifo = hAoutCtx->p_sa_fifo;
		loop_path_cnt = 1;
		fp_dhub_channel_write_cmd = dhub_channel_write_cmd;
		break;
#endif
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE
	case SPDIF_PATH:
		p_path_fifo = hAoutCtx->p_spdif_fifo;
		loop_path_cnt = 1;
		fp_dhub_channel_write_cmd = dhub_channel_write_cmd;
#ifdef CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE
		if ((!p_path_fifo) || (!hAoutCtx->p_arc_fifo))
			return;
		memcpy(hAoutCtx->p_arc_fifo, p_path_fifo,
		       sizeof(AOUT_PATH_CMD_FIFO));
#endif
		break;
#endif
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE
	case HDMI_PATH:
		p_path_fifo = hAoutCtx->p_hdmi_fifo;
		loop_path_cnt = 1;
		fp_dhub_channel_write_cmd = wrap_dhub_channel_write_cmd;
		break;
#endif
	default:
		amp_error("invalid path, set loop_path_cnt to zero\n");
		loop_path_cnt = 0;
		break;
	}

	rc = AoutGetChennalAndDhub(path_id, &dHub_handle, &chanId);
	if (rc != SUCCESS) {
		return;
	}

	if (p_path_fifo && !p_path_fifo->fifo_underflow)
		AoutFifoKernelRdUpdate(p_path_fifo, 1);

	kernel_full = AoutFifoCheckKernelFullness(p_path_fifo);
	if (kernel_full) {
		p_path_fifo->fifo_underflow = 0;

		for (i = 0; i < loop_path_cnt; i++) {
			chanId_real = chanId + i;
			dhub_space = hbo_queue_getspace(&dHub_handle->hbo,
					       dhub_id2hbo_cmdQ
					       (chanId_real));
			p_dma_info = (AOUT_DMA_INFO *)AoutFifoGetKernelRdDMAInfo(p_path_fifo, i);
			if (p_dma_info->size1) {
				if (dhub_space < 2)
					return;
			}
			kernel_full--;

			rc = fp_dhub_channel_write_cmd(dHub_handle,
						       chanId_real,
						       p_dma_info->addr0,
						       p_dma_info->size0,
						       0, 0, 0,
						       (p_dma_info->size1
							== 0
							&& i ==
							0) ? 1 : 0, 0,
						       0);
			if (p_dma_info->size1) {
				rc = fp_dhub_channel_write_cmd
				    (dHub_handle, chanId_real,
				     p_dma_info->addr1,
				     p_dma_info->size1, 0, 0, 0,
				     (i == 0) ? 1 : 0, 0, 0);
				if (rc == 0) {
					printk(KERN_ERR
					       "p_dma_info->size1 : frame not aligned, "
					       " shouldn't happen!!!!!\n");
				}
			}
		}	// end for
	} else {
		p_path_fifo->fifo_underflow = 1;
		p_path_fifo->underflow_cnt++;

		for (i = 0; i < loop_path_cnt; i++) {
			chanId_real = chanId + i;
			rc = fp_dhub_channel_write_cmd(dHub_handle,
						       chanId_real,
						       p_path_fifo->zero_buffer,
						       p_path_fifo->zero_buffer_size,
						       0, 0, 0, i == 0 ? 1 : 0,
						       0, 0);
		}
	}
}

AOUT_CTX *drv_aout_init(VOID)
{
	AOUT_CTX *hAoutCtx = &aout_ctx;
	UINT err;

	memset(hAoutCtx, 0, sizeof(AOUT_CTX));

	spin_lock_init(&hAoutCtx->aout_spinlock);
	spin_lock_init(&hAoutCtx->aout_msg_spinlock);

	sema_init(&hAoutCtx->aout_sem, 0);
	err = AMPMsgQ_Init(&hAoutCtx->hAoutMsgQ, AOUT_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		amp_error("hAoutMsgQ init: failed, err:%8x\n", err);
		return NULL;
	}

	return hAoutCtx;
}

VOID drv_aout_exit(AOUT_CTX * hAoutCtx)
{
	UINT err;

	err = AMPMsgQ_Destroy(&hAoutCtx->hAoutMsgQ);
	if (unlikely(err != S_OK)) {
		amp_trace("aout MsgQ Destroy: failed, err:%8x\n", err);
	}
}
