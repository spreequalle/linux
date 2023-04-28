/****************************************************************************
*
*    Copyright (C) 2005 - 2014 by Marvell Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/

#ifndef __gc_hal_kernel_platform_pm_h_
#define __gc_hal_kernel_platform_pm_h_

#include "gc_hal_kernel_linux.h"

#include <linux/kernel.h>
#include <linux/wait.h>
#include <asm/atomic.h>

#define GPU_SAMPLE_PERRIOD  5      /* 50 ms */
#define GPU_QUERY_INTERVAL  (4 * GPU_SAMPLE_PERRIOD * 10)
#define GPU_CORE_FREQ       (450*1000000)     /*450 MHZ */

#define GPU_PULSE_EATER_CFG     0x010C
#define GPU_PULSE_EATER_DEBUG0  0x0110 /* 3D core workload */
#define GPU_PULSE_EATER_DEBUG1  0x0114 /* 3D shader workload */

#define MAX_CHIP_NAME_LEN 256

struct bg2q_priv;

typedef enum _GPU_CLK_STATE {
    GPU_CLK_UNKNOWN,
    GPU_CLK_ON,
    GPU_CLK_OFF
} GPU_CLK_STATE;

typedef enum _GPU_WORK_LOAD {
    GPU_WORK_LOAD_UNKNOWN,
    GPU_WORK_LOAD_HIGH,
    GPU_WORK_LOAD_LOW,
} GPU_WORK_LOAD;

typedef struct _DVFS {
    /* hw and platform information */
    gckOS                       os;
    gckHARDWARE                 hardware;
    struct bg2q_priv            *platform_priv;

    /* dvfs task and synchronization */
    struct task_struct          *task;
    atomic_t                    clkState;
    wait_queue_head_t           queue;
} *DVFS;

typedef struct bg2q_priv {
    DVFS            dvfs;
    struct clk      *coreClk3D;
    struct clk      *sysClk3D;
    struct clk      *shClk3D;
    struct clk      *coreClk2D;
    struct clk      *axiClk2D;

    /* lock is to prevent the concurrency access to clock gating register bits */
    gckPLATFORM     platform;
} *BG2QPriv;

static inline uint32_t get_u32_mask(uint32_t bits, uint32_t offset)
{
    return ((0x1U << bits) - 1U) << offset;
}

static inline void set_u32_bits
(
    uint32_t *to,
    uint32_t bits,
    uint32_t offset,
    uint32_t value
)
{
    *to &= ~(get_u32_mask(bits, offset));
    *to |= value << offset;
}

static inline uint32_t get_u32_bits
(
    uint32_t bits,
    uint32_t offset,
    uint32_t from
)
{
    return from >> offset & ((0x1U << bits) - 1U);
}

gceSTATUS DVFS_Construct(INOUT BG2QPriv Priv);
gceSTATUS DVFS_Destroy(INOUT BG2QPriv Priv);
gceSTATUS DVFS_SetClkState(DVFS Dvfs, GPU_CLK_STATE State);

#endif //  __gc_hal_kernel_platform_pm_h_
