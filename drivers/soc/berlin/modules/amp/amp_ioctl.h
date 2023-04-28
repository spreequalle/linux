/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _AMP_IOCTL_H_
#define _AMP_IOCTL_H_
#include <linux/version.h>
#include <linux/time.h>

#include "amp_type.h"

#define VPP_IOCTL_VBI_DMA_CFGQ              0xbeef0001
#define VPP_IOCTL_VBI_BCM_CFGQ              0xbeef0002
#define VPP_IOCTL_VDE_BCM_CFGQ              0xbeef0003
#define VPP_IOCTL_GET_MSG                   0xbeef0004
#define VPP_IOCTL_START_BCM_TRANSACTION	    0xbeef0005
#define VPP_IOCTL_BCM_SCHE_CMD              0xbeef0006
#define VPP_IOCTL_INTR_MSG				    0xbeef0007
#define CEC_IOCTL_RX_MSG_BUF_MSG            0xbeef0008
#define VPP_IOCTL_GET_VSYNC                 0xbeef0009

#define VDEC_IOCTL_ENABLE_INT               0xbeef1001

#define AOUT_IOCTL_START_CMD            0xbeef2001
#define AOUT_IOCTL_STOP_CMD             0xbeef2002
#define AIP_IOCTL_START_CMD             0xbeef2003
#define AIP_IOCTL_STOP_CMD              0xbeef2004
#define AIP_AVIF_IOCTL_START_CMD        0xbeef2005
#define AIP_AVIF_IOCTL_STOP_CMD	        0xbeef2006
#define AIP_AVIF_IOCTL_SET_MODE         0xbeef2007
#define AIP_IOCTL_GET_MSG_CMD           0xbeef2008
#define AIP_IOCTL_SemUp_CMD             0xbeef2009
#define APP_IOCTL_INIT_CMD              0xbeef3001
#define APP_IOCTL_START_CMD             0xbeef3002
#define APP_IOCTL_DEINIT_CMD            0xbeef3003
#define APP_IOCTL_GET_MSG_CMD           0xbeef3004
#define AOUT_IOCTL_GET_MSG_CMD          0xbeef3005
#define ZSP_IOCTL_GET_MSG_CMD           0xbeef3006

#define VIP_IOCTL_GET_MSG               0xbeef4001
#define VIP_IOCTL_VBI_BCM_CFGQ          0xbeef4002
#define VIP_IOCTL_SD_WRE_CFGQ           0xbeef4003
#define VIP_IOCTL_SD_RDE_CFGQ           0xbeef4004
#define VIP_IOCTL_SEND_MSG              0xbeef4005
#define VIP_IOCTL_VDE_BCM_CFGQ          0xbeef4006
#define VIP_IOCTL_INTR_MSG              0xbeef4007
#define VIP_HRX_IOCTL_GET_MSG           0xbeef4008
#define VIP_HRX_IOCTL_SEND_MSG          0xbeef4009
#define VIP_HDCP_IOCTL_GET_MSG          0xbeef400a
#define VIP_MSG_DESTROY_ISR_TASK        1

#define AVIF_IOCTL_GET_MSG              0xbeef6001
#define AVIF_IOCTL_VBI_CFGQ             0xbeef6002
#define AVIF_IOCTL_SD_WRE_CFGQ          0xbeef6003
#define AVIF_IOCTL_SD_RDE_CFGQ          0xbeef6004
#define AVIF_IOCTL_SEND_MSG             0xbeef6005
#define AVIF_IOCTL_VDE_CFGQ             0xbeef6006
#define AVIF_IOCTL_INTR_MSG             0xbeef6007

#define AVIF_HRX_IOCTL_GET_MSG          0xbeef6050
#define AVIF_HRX_IOCTL_SEND_MSG         0xbeef6051
#define AVIF_VDEC_IOCTL_GET_MSG         0xbeef6052
#define AVIF_VDEC_IOCTL_SEND_MSG        0xbeef6053
#define AVIF_HRX_IOCTL_ARC_SET          0xbeef6054
#define AVIF_HDCP_IOCTL_GET_MSG         0xbeef6055
#define AVIF_HDCP_IOCTL_SEND_MSG        0xbeef6056
#define HDMIRX_IOCTL_GET_MSG            0xbeef5001
#define HDMIRX_IOCTL_SEND_MSG           0xbeef5002

#define OVP_IOCTL_INTR                  0xbeef7001
#define OVP_IOCTL_GET_MSG               0xbeef7002
#define OVP_IOCTL_DUMMY_INTR            0xbeef7003
#define OVP_INIT_TA_SESSION             0xbeef7004
#define OVP_KILL_TA_SESSION             0xbeef7005

#define POWER_IOCTL_DISABLE_INT         0xbeef8001
#define POWER_IOCTL_ENABLE_INT          0xbeef8002
#define POWER_IOCTL_WAIT_RESUME         0xbeef8003
#define POWER_IOCTL_START_RESUME        0xbeef8004

#define TSP_IOCTL_CMD_MSG               0xbeef9001
#define TSP_IOCTL_GET_MSG               0xbeef9002

#define MAX_INTR_NUM 0x20

#define VPP_CEC_INT_EN      1
#define VPP_INT_EN         2

#define VIP_INT_EN      1
#define VMETA_INT_EN        1
#define V2G_INT_EN          2
#define H1_INT_EN           4

enum {
	IRQ_ZSP_MODULE = 0,
	IRQ_VPP_MODULE,
	IRQ_VDEC_MODULE,
	IRQ_AVIN_MODULE,
	IRQ_AOUT_MODULE,
	IRQ_OVP_MODULE,
	IRQ_TSP_MODULE,
	IRQ_VIP_MODULE,
	IRQ_MAX_MODULE,
};

typedef struct INTR_MSG_T {
	UINT32 DhubSemMap;
	UINT32 Enable;
} INTR_MSG;

typedef enum {
	MULTI_PATH = 0,
	LoRo_PATH = 1,
	SPDIF_PATH = 2,
	HDMI_PATH = 3,
	MAX_OUTPUT_AUDIO = 5,
} AUDIO_PATH;

typedef struct {
	struct ion_handle *ionh[MAX_OUTPUT_AUDIO];
} AOUT_ION_HANDLE;

typedef struct {
	struct ion_handle *ionh;
} AIP_ION_HANDLE;

typedef struct aout_dma_info_t {
	UINT32 addr0;
	UINT32 size0;
	UINT32 addr1;
	UINT32 size1;
} AOUT_DMA_INFO;

/* !!!! Be careful:
   the depth must align with user space configuration which is in audio_cfg.h */
#define AOUT_PATH_CMD_DEPTH 16

typedef struct aout_path_cmd_fifo_t {
	AOUT_DMA_INFO aout_dma_info[4][AOUT_PATH_CMD_DEPTH];
	UINT32 update_pcm[AOUT_PATH_CMD_DEPTH];
	UINT32 takeout_size[AOUT_PATH_CMD_DEPTH];
	UINT32 size;
	UINT32 wr_offset;
	UINT32 rd_offset;
	UINT32 kernel_rd_offset;
	UINT32 zero_buffer;
	UINT32 zero_buffer_size;
	UINT32 fifo_underflow;
	UINT32 aout_pre_read;
	UINT32 underflow_cnt;
	UINT64 ts_sec;
	UINT64 ts_nsec;
} AOUT_PATH_CMD_FIFO;

typedef struct aip_dma_cmd_t {
	UINT32 addr0;
	UINT32 size0;
	UINT32 addr1;
	UINT32 size1;
} AIP_DMA_CMD;

typedef struct aip_cmd_fifo_t {
	AIP_DMA_CMD aip_dma_cmd[4][8];
	UINT32 update_pcm[8];
	UINT32 takein_size[8];
	UINT32 size;
	UINT32 wr_offset;
	UINT32 rd_offset;
	UINT32 kernel_rd_offset;
	UINT32 prev_fifo_overflow_cnt;
	/* used by kernel */
	UINT32 kernel_pre_rd_offset;
	UINT32 overflow_buffer;
	UINT32 overflow_buffer_size;
	UINT32 fifo_overflow;
	UINT32 fifo_overflow_cnt;
} AIP_DMA_CMD_FIFO;

typedef struct {
	UINT uiShmOffset;	/**< Not used by kernel */
	UINT *unaCmdBuf;
	UINT *cmd_buffer_base;
	UINT max_cmd_size;
	UINT cmd_len;
	UINT cmd_buffer_hw_base;
	VOID *p_cmd_tag;
} APP_CMD_BUFFER;

typedef struct {
	UINT uiShmOffset;	/**< Not used by kernel */
	APP_CMD_BUFFER in_coef_cmd[8];
	APP_CMD_BUFFER out_coef_cmd[8];
	APP_CMD_BUFFER in_data_cmd[8];
	APP_CMD_BUFFER out_data_cmd[8];
	UINT size;
	UINT wr_offset;
	UINT rd_offset;
	UINT kernel_rd_offset;
/************** used by Kernel *****************/
	UINT kernel_idle;
} HWAPP_CMD_FIFO;

#endif //_AMP_IOCTL_H_
