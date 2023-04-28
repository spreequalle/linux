/****************************************************************************
*
*    Copyright (C) 2005 - 2014 by Vivante Corp.
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


#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_platform.h"
#include "gc_hal_kernel_platform_berlin.h"
#include "gc_hal_kernel_device.h"
#include "gc_hal_driver.h"

#if USE_PLATFORM_DRIVER
#   include <linux/platform_device.h>
#endif
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>

#if !defined(SHM_FROM_SYSTEM) || !SHM_FROM_SYSTEM
#include <asm/cacheflush.h>
#endif

struct bg2q_priv bg2qPriv;


gceSTATUS
_AdjustParam(
    IN gckPLATFORM Platform,
    OUT gcsMODULE_PARAMETERS *Args
    )
{
    struct bg2q_priv* priv = Platform->priv;

    struct device_node *np;
    struct device *galcore = NULL;
    struct resource res;
    struct clk *clk = ERR_PTR(-ENOENT);
    int rc = 0;
#ifdef CONFIG_ARM64
    u64 value;
#else
    u32 value;
#endif

    /***
        get the galcore information */
    galcore = &Platform->device->dev;
    np = galcore->of_node;
#ifdef CONFIG_ARM64
    rc = of_property_read_u64(np, "marvell,nonsecure-mem-base", &value);
#else
    rc = of_property_read_u32(np, "marvell,nonsecure-mem-base", &value);
#endif
    if (rc)
    {
        printk("gpu warning: of_property_read_uxx for nonsecure-mem-base failed!\n");
    }
    else
    {
        Args->contiguousBase = (gctUINT32)value;
    }

#ifdef CONFIG_ARM64
    rc = of_property_read_u64(np, "marvell,nonsecure-mem-size", &value);
#else
    rc = of_property_read_u32(np, "marvell,nonsecure-mem-size", &value);
#endif
    if (rc)
    {
        printk("gpu warning: of_property_read_uxx for nonsecure-mem-size failed!\n");
    }
    else
    {
        Args->contiguousSize = (gctUINT32)value;
    }

#ifdef CONFIG_ARM64
    rc = of_property_read_u64(np, "marvell,phy-mem-size", &value);
#else
    rc = of_property_read_u32(np, "marvell,phy-mem-size", &value);
#endif
    if (rc)
    {
        printk("gpu warning: of_property_read_uxx for phy-mem-size failed!\n");
    }
    else
    {
        Args->physSize = (gctUINT32)value;
    }
    np = NULL;

    /***
        query 3d core information */
    np = of_find_compatible_node(galcore->of_node, NULL, "vivante,core3d");
    if (!np)
    {
        printk("gpu error: of_find_compatible_node for core3d failed!\n");
        goto err_exit;
    }

    /* get the reg section */
    rc = of_address_to_resource(np, 0, &res);
    if (rc)
    {
        printk("gpu warning: of_address_to_resource for core3d failed!\n");
    }
    else
    {
        Args->registerMemBase = res.start;
        Args->registerMemSize = resource_size(&res);
    }

    /* get the interrupt section */
    rc = of_irq_to_resource(np, 0, &res);
    if (!rc)
    {
        printk("gpu warning: of_irq_to_resource for core3d failed, rc=%d!\n", rc);
    }
    else
    {
        Args->irqLine = rc;
    }

    /* get the clock section */
    clk = of_clk_get_by_name(np, "core");
    if (IS_ERR(clk))
    {
        printk("gpu warning: of_clk_get_by_name for core3d core failed, clk=%p!\n", clk);
    }
    else
    {
        priv->coreClk3D = clk;
        //clk_prepare_enable(clk);
    }

    clk = of_clk_get_by_name(np, "system");
    if (IS_ERR(clk))
    {
        printk("gpu warning: of_clk_get_by_name for core3d system failed, clk=%p!\n", clk);
    }
    else
    {
        priv->sysClk3D = clk;
        clk_prepare_enable(clk);
    }

    clk = of_clk_get_by_name(np, "shader");
    if (IS_ERR(clk))
    {
        printk("gpu warning: of_clk_get_by_name for core3d shader failed, clk=%p!\n", clk);
    }
    else
    {
        priv->shClk3D = clk;
        //clk_prepare_enable(clk);
    }

    of_node_put(np);
    np = NULL;

    /***
        get the gpu2d information */
    np = of_find_compatible_node(galcore->of_node, NULL, "vivante,core2d");
    if (!np)
    {
        /* no 2d gpu, just goto exit */
        goto err_exit;
    }

    /* get the reg section */
    rc = of_address_to_resource(np, 0, &res);
    if (rc)
    {
        printk("gpu warning: of_address_to_resource for berlin-gpu2d failed!\n");
    }
    else
    {
        Args->registerMemBase2D = res.start;
        Args->registerMemSize2D = resource_size(&res);
    }

    /* get the interrupt section */
    rc = of_irq_to_resource(np, 0, &res);
    if (!rc)
    {
        printk("gpu warning: of_irq_to_resource for berlin-gpu2d failed!\n");
    }
    else
    {
        Args->irqLine2D = rc;
    }

    /* get the clock section */
    clk = of_clk_get_by_name(np, "core");
    if (IS_ERR(clk))
    {
        printk("gpu warning: of_clk_get_by_name for core2d core failed, clk=%p!\n", clk);
    }
    else
    {
        priv->coreClk2D = clk;
        //clk_prepare_enable(clk);
    }

    clk = of_clk_get_by_name(np, "axi");
    if (IS_ERR(clk))
    {
        printk("gpu warning: of_clk_get_by_name for core2d axi failed, clk=%p!\n", clk);
    }
    else
    {
        priv->axiClk2D = clk;
        //clk_prepare_enable(clk);
    }

    of_node_put(np);

    return gcvSTATUS_OK;
err_exit:
    return gcvSTATUS_OUT_OF_RESOURCES;
}

gceSTATUS
_AllocPriv(
    IN gckPLATFORM Platform
    )
{
    bg2qPriv.platform = Platform;

#if gcdSOC_PLATFORM_DVFS
    {
        gceSTATUS status = gcvSTATUS_OK;
        status = DVFS_Construct(&bg2qPriv);
        if (status != gcvSTATUS_OK)
        {
            return status;
        }
    }
#endif

    Platform->priv = &bg2qPriv;
    return gcvSTATUS_OK;
}

gceSTATUS
_FreePriv(
    IN gckPLATFORM Platform
    )
{
    gceSTATUS status = gcvSTATUS_OK;

#if gcdSOC_PLATFORM_DVFS
    status = DVFS_Destroy(Platform->priv);
#endif

    return status;
}

gctBOOL
_NeedAddDevice(
    IN gckPLATFORM Platform
    )
{
    return gcvFALSE;
}

gceSTATUS
_SetClock(
    IN gckPLATFORM Platform,
    IN gceCORE GPU,
    IN gctBOOL Enable
    )
{
    struct bg2q_priv* priv = Platform->priv;

    if (GPU == gcvCORE_MAJOR)
    {
        if (Enable == gcvTRUE)
        {
            if (priv->coreClk3D)
            {
                clk_prepare_enable(priv->coreClk3D);
            }
            if (priv->sysClk3D)
            {
                //clk_prepare_enable(priv->sysClk3D);
            }
            if (priv->shClk3D)
            {
                clk_prepare_enable(priv->shClk3D);
            }
#if gcdSOC_PLATFORM_DVFS
            DVFS_SetClkState(priv->dvfs, GPU_CLK_ON);
#endif
        }
        else
        {
#if gcdSOC_PLATFORM_DVFS
            DVFS_SetClkState(priv->dvfs, GPU_CLK_OFF);
#endif
            if (priv->coreClk3D)
            {
                clk_disable_unprepare(priv->coreClk3D);
            }
            if (priv->sysClk3D)
            {
                //clk_disable_unprepare(priv->sysClk3D);
            }
            if (priv->shClk3D)
            {
                clk_disable_unprepare(priv->shClk3D);
            }
        }
    }
    else if (GPU == gcvCORE_2D)
    {
        if (Enable == gcvTRUE)
        {
            if (priv->coreClk2D)
            {
                clk_prepare_enable(priv->coreClk2D);
            }
            if (priv->axiClk2D)
            {
                clk_prepare_enable(priv->axiClk2D);
            }
        }
        else
        {
            if (priv->coreClk2D)
            {
                clk_disable_unprepare(priv->coreClk2D);
            }
            if (priv->axiClk2D)
            {
                clk_disable_unprepare(priv->axiClk2D);
            }
        }
    }

    return gcvSTATUS_OK;
}

static gceSTATUS QueryProcessPageTable(
    IN gctPOINTER Logical,
    OUT gctUINT32 * Address
    )
{
    spinlock_t *lock;
    gctUINTPTR_T logical = (gctUINTPTR_T)Logical;
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    if (!current->mm)
    {
        return gcvSTATUS_NOT_FOUND;
    }

    pgd = pgd_offset(current->mm, logical);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
    {
        return gcvSTATUS_NOT_FOUND;
    }

    pud = pud_offset(pgd, logical);
    if (pud_none(*pud) || pud_bad(*pud))
    {
        return gcvSTATUS_NOT_FOUND;
    }

    pmd = pmd_offset(pud, logical);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
    {
        return gcvSTATUS_NOT_FOUND;
    }

    pte = pte_offset_map_lock(current->mm, pmd, logical, &lock);
    if (!pte)
    {
        return gcvSTATUS_NOT_FOUND;
    }

    if (!pte_present(*pte))
    {
        pte_unmap_unlock(pte, lock);
        return gcvSTATUS_NOT_FOUND;
    }

    *Address = (pte_pfn(*pte) << PAGE_SHIFT) | (logical & ~PAGE_MASK);
    pte_unmap_unlock(pte, lock);

    return gcvSTATUS_OK;
}

#if defined(SHM_FROM_SYSTEM) && SHM_FROM_SYSTEM
static gceSTATUS CacheOperation(
    IN gctUINT32 Physical,
    IN gctUINT32 Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    switch (Operation) {
    case gcvCACHE_INVALIDATE:
        dma_sync_single_for_cpu(NULL, Physical,
                Bytes, DMA_FROM_DEVICE);
        break;

    case gcvCACHE_CLEAN:
        dma_sync_single_for_device(NULL, Physical,
                Bytes, DMA_TO_DEVICE);
        break;

    case gcvCACHE_FLUSH:
        dma_sync_single_for_device(NULL, Physical,
                Bytes, DMA_TO_DEVICE);
        dma_sync_single_for_cpu(NULL, Physical,
                Bytes, DMA_FROM_DEVICE);
        break;

    case gcvCACHE_MEMORY_BARRIER:
        mb();
        break;
    default:
        break;
    }

    return gcvSTATUS_OK;
}

#else

static gceSTATUS CacheOperation(
    IN gctUINT32 Physical,
    IN gctUINT32 Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    if (Physical == gcvINVALID_ADDRESS)
    {
        printk(KERN_ERR "gpu driver: %s: invalid physical address 0x%x\n",
               __FUNCTION__, Physical);
        return gcvSTATUS_INVALID_ADDRESS;
    }

    switch (Operation) {
    case gcvCACHE_INVALIDATE:
        outer_inv_range(Physical, Physical + Bytes);
        break;

    case gcvCACHE_CLEAN:
        outer_clean_range(Physical, Physical + Bytes);
        break;

    case gcvCACHE_FLUSH:
        outer_flush_range(Physical, Physical + Bytes);
        break;

    default:
        printk(KERN_ERR "gpu driver: %s: invalid cache operation %d\n",
               __FUNCTION__, (gctINT32)Operation);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS InnerCacheOperation(
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    if (Logical == NULL)
    {
        printk(KERN_ERR "gpu driver: %s: invalid virtual address %p\n",
               __FUNCTION__, Logical);
        return gcvSTATUS_INVALID_ADDRESS;
    }

    switch (Operation)
    {
    case gcvCACHE_INVALIDATE:
        dmac_map_area((const void *)Logical,
            Bytes, DMA_FROM_DEVICE);
        break;

    case gcvCACHE_CLEAN:
        dmac_map_area((const void *)Logical,
            Bytes, DMA_TO_DEVICE);
        break;

    case gcvCACHE_FLUSH:
        dmac_flush_range((const void *)Logical,
                (const void *)Logical + Bytes);
        break;

    case gcvCACHE_MEMORY_BARRIER:
        mb();
        break;

    default:
        printk(KERN_ERR "gpu driver: %s: invalid cache operation %d\n",
               __FUNCTION__, (gctINT32)Operation);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return gcvSTATUS_OK;
}
#endif

static gceSTATUS _Cache(
    IN gckPLATFORM Platform,
    IN gctUINT32 ProcessID,
    IN gctPHYS_ADDR Handle,
    IN gctUINT32 Physical,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    gceSTATUS status;
    gctUINT32 paddr = 0;
    gctPOINTER vaddr;
    gctUINT32 offset, bytes, left;
    PLINUX_MDL mdl = NULL;

    mdl = (PLINUX_MDL)Handle;
    if (mdl && mdl->secure)
    {
        // Do not access secure memory!
        return gcvSTATUS_OK;
    }

#if !defined(SHM_FROM_SYSTEM) || !SHM_FROM_SYSTEM
    if (Logical)
    {
        gcmkONERROR(InnerCacheOperation(Logical, Bytes, Operation));
    }
#endif

    if (Physical != gcvINVALID_ADDRESS)
    {
        /* Non paged memory or gcvPOOL_USER surface */
        gcmkONERROR(CacheOperation(Physical, Bytes, Operation));
    }
    else
    {
        /* Virtual memory */
        vaddr = Logical;
        left = Bytes;
        while (left)
        {
            /* Handle (part of) current page. */
            offset = (gctUINTPTR_T)vaddr & ~PAGE_MASK;
            bytes = gcmMIN(left, PAGE_SIZE - offset);
            gcmkONERROR(QueryProcessPageTable(vaddr, &paddr));
            gcmkONERROR(CacheOperation(paddr, bytes, Operation));
            vaddr = (gctUINT8_PTR)vaddr + bytes;
            left -= bytes;
        }
    }

    mb();

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    return status;
}

gcsPLATFORM_OPERATIONS platformOperations = {
    .adjustParam   = _AdjustParam,
    .needAddDevice = _NeedAddDevice,
    .setClock      = _SetClock,
    .allocPriv     = _AllocPriv,
    .freePriv      = _FreePriv,
    .cache         = _Cache,
};

void
gckPLATFORM_QueryOperations(
    IN gcsPLATFORM_OPERATIONS ** Operations
    )
{
     *Operations = &platformOperations;
}

