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

#include "gc_hal_types.h"
#include "gc_hal_enum.h"
#include "gc_hal_kernel_platform_berlin.h"
#include "gc_hal_kernel_hardware.h"

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#define LOW_POWER_DELAY 100 // 1000ms

enum {
    CPM_GFX2D_CALLBACK,
    CPM_GFX3D_CALLBACK,
};

typedef  int (*GFX_CALLBACK)(void* gfx, int gfx_mod);

typedef struct {
    void *          gfx;
    GFX_CALLBACK    before_change;
    GFX_CALLBACK    after_change;
}CPM_GFX_CALLBACK;

int cpm_set_gfx3d_loading(unsigned int loading);
void cpm_register_gfx_callback(CPM_GFX_CALLBACK* callback);
void cpm_unregister_gfx_callback(void);

static gceSTATUS MemAlloc(size_t Size, gctPOINTER *Pointer)
{
    gctPOINTER pointer = gcvNULL;

    if (Pointer == NULL)
    {
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    if (Size == 0)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    pointer = kmalloc(Size, GFP_KERNEL);
    if (pointer == NULL)
    {
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    *Pointer = pointer;
    return gcvSTATUS_OK;
}

static void MemFree(gctPOINTER Pointer)
{
    if (Pointer == NULL)
    {
        return;
    }
    kfree(Pointer);
}

static void *gckPLATFORM_GetDeviceDrvData(gckPLATFORM Platform)
{
    void *data = NULL;

    gcmkASSERT(Platform != NULL);

    while (data == NULL)
    {
        if (Platform->device)
        {
            data = platform_get_drvdata(Platform->device);
        }
        msleep(100);
    }

    return data;
}

GPU_WORK_LOAD QueryGPUWorkLoad(DVFS Dvfs)
{
    gceSTATUS status;
    gctUINT32 coreLoadRegVal = 0U;
    gctUINT32 shLoadRegVal = 0U;
    gctUINT32 coreLoads[4] = {0, 0, 0, 0};
    gctUINT32 shLoads[4] = {0, 0, 0, 0};
    gctUINT32 idx = 0;
    gckOS os = NULL;

    gcmkASSERT(Dvfs != NULL);
    gcmkASSERT(Dvfs->registerMemBase != 0);

    os = Dvfs->hardware->os;

    gcmkONERROR(
        gckOS_ReadRegisterEx(os, gcvCORE_MAJOR, GPU_PULSE_EATER_DEBUG0, &coreLoadRegVal));
    gcmkONERROR(
        gckOS_ReadRegisterEx(os, gcvCORE_MAJOR, GPU_PULSE_EATER_DEBUG1, &shLoadRegVal));

    for (idx = 0; idx < gcmCOUNTOF(coreLoads); idx++)
    {
        coreLoads[idx] = coreLoadRegVal >> idx * 8 & 0xFF;
    }

    for (idx = 0; idx < gcmCOUNTOF(shLoads); idx++)
    {
        shLoads[idx] = shLoadRegVal >> idx * 8 & 0xFF;
    }

    if (shLoads[0] == 1 || coreLoads[0] == 1)
    {
        return GPU_WORK_LOAD_LOW;
    }

    return GPU_WORK_LOAD_HIGH;

OnError:
    printk(KERN_ERR "gfx driver: error reading eater debug register(s)!\n");

    return GPU_WORK_LOAD_HIGH;
}

static gceSTATUS NotifyGPUWorkloadLocked(DVFS Dvfs, GPU_WORK_LOAD WorkLoad)
{
    gceSTATUS status = gcvSTATUS_OK;
    unsigned int loading = 0x0;

    gcmkASSERT(Dvfs != NULL);

    if (WorkLoad == GPU_WORK_LOAD_HIGH)
    {
        loading = 0x1;
    }

    if (cpm_set_gfx3d_loading(loading) < 0)
    {
        gcmkONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    return gcvSTATUS_OK;

OnError:
    printk(KERN_ERR "error notifying GPU workload\n");
    return status;
}

int InitSamplePeriod(DVFS Dvfs, gctUINT32 Period, gctUINT32 CoreFreq)
{
    gceSTATUS status;
    gctUINT32 value, new_value, periodBitsVal;
    gctUINT64 multiple;
    gckOS os = NULL;

    gcmkASSERT(Dvfs != NULL);
    gcmkASSERT(Dvfs->registerMemBase != 0);

    os = Dvfs->hardware->os;

    multiple = ((gctUINT64)Period * CoreFreq) >> (6 + 10);
    periodBitsVal = 0;
    while (multiple > 1) {
        periodBitsVal++;
        multiple = multiple >> 1;
    }

    if (periodBitsVal < 8)
    {
        periodBitsVal = 8;
    }

    if (periodBitsVal > 64)
    {
        periodBitsVal = 64;
    }

    /* currently, GPU power/clock is enabled, so it is safe to write GPU registers */
    gcmkONERROR(
        gckOS_ReadRegisterEx(os, gcvCORE_MAJOR, GPU_PULSE_EATER_CFG, &value));
    new_value = value & 0xFFFF00FF;
    new_value = new_value | (periodBitsVal << 8);
    gcmkONERROR(
        gckOS_WriteRegisterEx(os, gcvCORE_MAJOR, GPU_PULSE_EATER_CFG, new_value));

    return 0;

OnError:
    printk(KERN_ERR "gfx driver: error reading eater config register(s)!\n");

    return -1;
}

static int DVFS_GPUPowerOff(void *Data, int Mode)
{
    gceSTATUS status = 0;
    gceCORE core = 0;
    gckGALDEVICE galDev = NULL;

    gcmkASSERT(Data != NULL);

    if (Mode == CPM_GFX2D_CALLBACK)
    {
        core = gcvCORE_2D;
    }
    else if (Mode == CPM_GFX3D_CALLBACK)
    {
        core = gcvCORE_MAJOR;
    }
    else
    {
        printk(KERN_ERR "gpu: driver: invalid cpm change mode\n");
        return -1;
    }

    galDev = (gckGALDEVICE)Data;
    gcmkONERROR(gckHARDWARE_SetPowerManagementState(galDev->kernels[core]->hardware, gcvPOWER_OFF));
    return 0;

OnError:
    printk(KERN_ERR "gpu driver: error powering off %s\n",
           core == gcvCORE_MAJOR ? "major core" : "2D core");
    return -1;
}

static int DVFS_GPUPowerOn(void *Data, int Mode)
{
    gceSTATUS status = 0;
    gckGALDEVICE galDev = NULL;
    gceCORE core = 0;

    gcmkASSERT(Data != NULL);

    if (Mode == CPM_GFX2D_CALLBACK)
    {
        core = gcvCORE_2D;
    }
    else if (Mode == CPM_GFX3D_CALLBACK)
    {
        core = gcvCORE_MAJOR;
    }
    else
    {
        printk(KERN_ERR "gpu: driver: invalid cpm change mode\n");
        return -1;
    }

    galDev = (gckGALDEVICE)Data;
    gcmkONERROR(gckHARDWARE_SetPowerManagementState(galDev->kernels[core]->hardware, gcvPOWER_ON));
    return 0;

OnError:
    printk(KERN_ERR "gpu driver: error powering on %s\n",
           core == gcvCORE_MAJOR ? "major core" : "2D core");
    return -1;
}

static int dvfsRoutine(void *Data)
{
    int clkState = 0;
    DVFS dvfs = NULL;
    BG2QPriv priv = NULL;
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE galDev = NULL;
    gctBOOL needInitSample = gcvTRUE;
    CPM_GFX_CALLBACK cpmCb;

    if (Data == NULL)
    {
        printk(KERN_ERR "gpu driver: null pointer passed to dvfsRoutine.\n");
        return -1;
    }

    dvfs = (DVFS)Data;
    priv = dvfs->platform_priv;

    galDev = (gckGALDEVICE)gckPLATFORM_GetDeviceDrvData(priv->platform);
    dvfs->hardware = galDev->kernels[gcvCORE_MAJOR]->hardware;
    dvfs->os = galDev->os;

    cpmCb.before_change = DVFS_GPUPowerOff;
    cpmCb.after_change = DVFS_GPUPowerOn;
    cpmCb.gfx = galDev;
    cpm_register_gfx_callback(&cpmCb);

    while (!kthread_should_stop())
    {
        gckOS_AcquireMutex(dvfs->os, dvfs->hardware->powerMutex, gcvINFINITE);
        clkState = atomic_read(&dvfs->clkState);
        if (clkState == GPU_CLK_ON)
        {
            GPU_WORK_LOAD workload = GPU_WORK_LOAD_UNKNOWN;
            if (needInitSample == gcvTRUE)
            {
                InitSamplePeriod(dvfs, GPU_SAMPLE_PERRIOD, GPU_CORE_FREQ);
                needInitSample = gcvFALSE;
            }
            workload = QueryGPUWorkLoad(dvfs);
            gckOS_ReleaseMutex(dvfs->os, dvfs->hardware->powerMutex);

            status = NotifyGPUWorkloadLocked(dvfs, workload);
            if (status != gcvSTATUS_OK)
            {
                printk(KERN_ERR "gpu driver: error notifying TA.\n");
            }

            msleep(GPU_QUERY_INTERVAL);
        }
        else if (clkState == GPU_CLK_OFF)
        {
            gckOS_ReleaseMutex(dvfs->os, dvfs->hardware->powerMutex);
            status = NotifyGPUWorkloadLocked(dvfs, GPU_WORK_LOAD_LOW);
            if (status != gcvSTATUS_OK)
            {
                printk(KERN_ERR "gpu driver: error notifying TA.\n");
            }

            wait_event_interruptible(dvfs->queue,
                                     atomic_read(&dvfs->clkState) == (int)GPU_CLK_ON);
        }
        else
        {
            gckOS_ReleaseMutex(dvfs->os, dvfs->hardware->powerMutex);
            wait_event_interruptible(dvfs->queue,
                                     atomic_read(&dvfs->clkState) != (int)GPU_CLK_UNKNOWN);
        }
    }

    cpm_unregister_gfx_callback();

    return 0;
}

gceSTATUS DVFS_Construct(INOUT BG2QPriv Priv)
{
    gceSTATUS status = gcvSTATUS_OK;
    DVFS dvfs = gcvNULL;
    struct task_struct *task = NULL;
    struct workqueue_struct *wq = NULL;

    gcmkASSERT(Priv != gcvNULL);
    gcmkONERROR(MemAlloc(gcmSIZEOF(struct _DVFS), (gctPOINTER *)&dvfs));
    gcmkONERROR(gckOS_ZeroMemory(dvfs, gcmSIZEOF(struct _DVFS)));

    init_waitqueue_head(&dvfs->queue);
    atomic_set(&dvfs->clkState, (int)GPU_CLK_UNKNOWN);
    dvfs->platform_priv = Priv;

    task = kthread_run(dvfsRoutine, dvfs, "gpu cpm service");
    if (task == NULL)
    {
        printk(KERN_ERR "gpu driver: error creating gpu cpm service.\n");
        gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
    }

    wq = create_singlethread_workqueue("gpu low power request service");
    if (wq == NULL)
    {
        printk(KERN_ERR "gpu driver: error creating work queue.\n");
        gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
    }

    dvfs->task = task;

    Priv->dvfs = dvfs;
    return gcvSTATUS_OK;

OnError:

    if (task != NULL)
    {
        kthread_stop(task);
    }

    if (dvfs != gcvNULL)
    {
        MemFree(dvfs);
    }

    return status;
}

gceSTATUS DVFS_SetClkState(DVFS Dvfs, GPU_CLK_STATE State)
{
    GPU_CLK_STATE oldState = GPU_CLK_UNKNOWN;

    if (Dvfs == gcvNULL)
    {
        printk(KERN_ERR "gpu driver: null pointer.");
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    oldState = atomic_read(&Dvfs->clkState);
    if (oldState == State)
    {
        return gcvSTATUS_OK;
    }

    switch (State)
    {
    case GPU_CLK_ON:
    case GPU_CLK_OFF:
        atomic_set(&Dvfs->clkState, (int)State);
        wake_up_interruptible(&Dvfs->queue);
        break;
    default:
        break;
    }

    return gcvSTATUS_OK;
}

gceSTATUS DVFS_Destroy(INOUT BG2QPriv Priv)
{
    DVFS dvfs = gcvNULL;

    gcmkASSERT(Priv != gcvNULL);
    gcmkASSERT(Priv->dvfs != gcvNULL);

    dvfs = Priv->dvfs;
    dvfs->platform_priv = gcvNULL;
    Priv->dvfs = gcvNULL;

    if (dvfs->task)
    {
        wake_up_interruptible(&dvfs->queue);
        kthread_stop(dvfs->task);
    }
    MemFree(dvfs);

    return gcvSTATUS_OK;
}
