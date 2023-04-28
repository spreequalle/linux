/****************************************************************************
*
*    Copyright (C) 2005 - 2015 by Vivante Corp.
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
#include "gc_hal_kernel_allocator.h"

#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mman.h>
#include <asm/atomic.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include "shm_api.h"
#include <linux/dma-mapping.h>

typedef struct _GFX_SHM_STAT{
    unsigned int total_malloc;
    unsigned int current_malloc;
    unsigned int malloc_count;
    unsigned int free_count;
    unsigned int total_free;
}GFX_SHM_STAT;
static GFX_SHM_STAT gfx_shm_statistic;
//extern int maxSHMSize;

struct _gcsSHM_ALLOCATOR_PRIV {

    gctPOINTER p;
};

struct mdl_shm_priv {
    size_t shm_node;
    size_t physical;
};

static gceSTATUS
_ShmAlloc(
    IN gckALLOCATOR Allocator,
    INOUT PLINUX_MDL Mdl,
    IN gctSIZE_T NumPages,
    IN gctUINT32 Flags
    )
{
    gceSTATUS status;
    struct mdl_shm_priv *priv;
    gctSIZE_T bytes = NumPages * PAGE_SIZE;
    gckOS os = Allocator->os;

#if 0
    if ((gfx_shm_statistic.current_malloc + bytes) >= maxSHMSize)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }
#endif

    gcmkONERROR(gckOS_Allocate(os, sizeof(struct mdl_shm_priv), (gctPOINTER *)&priv));

    priv->shm_node = MV_SHM_Malloc(bytes, PAGE_SIZE);

    if (priv->shm_node == ERROR_SHM_MALLOC_FAILED){
        MV_SHM_MemInfo_t info;
        MV_SHM_GetCacheMemInfo(&info);
        gckOS_Print("shm malloc failed: Bytes=%u,total=%d, free=%d, usedmem=%d, maxfree=%d, maxused=%d\n",
            bytes, info.m_totalmem, info.m_freemem, info.m_usedmem, info.m_max_freeblock, info.m_max_usedblock);
        gckOS_Print("gfx shm usage:total_malloc=%u,current_malloc=%u, total_free=%u, malloc_count=%u, free_count=%u\n",
            gfx_shm_statistic.total_malloc, gfx_shm_statistic.current_malloc,gfx_shm_statistic.total_free,
            gfx_shm_statistic.malloc_count, gfx_shm_statistic.free_count);

        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* gpu memory will be released by gpu driver self */
    MV_SHM_Takeover(priv->shm_node);

    /* Get physical address. */
    priv->physical = (size_t)MV_SHM_GetCachePhysAddr(priv->shm_node);

    Mdl->priv = (gctPOINTER)priv;

    gfx_shm_statistic.total_malloc+=bytes;
    gfx_shm_statistic.current_malloc +=bytes;
    gfx_shm_statistic.malloc_count +=1;

    return gcvSTATUS_OK;

OnError:
    return status;
}

static void
_ShmFree(
    IN gckALLOCATOR Allocator,
    IN OUT PLINUX_MDL Mdl
    )
{
    PLINUX_MDL mdl = Mdl;
    struct mdl_shm_priv * priv = (struct mdl_shm_priv *)mdl->priv;

    if (ERROR_SHM_MALLOC_FAILED != priv->shm_node)
    {
        MV_SHM_Free(priv->shm_node);
    }

    kfree(priv);

    gfx_shm_statistic.current_malloc -= mdl->numPages * PAGE_SIZE;
    gfx_shm_statistic.total_free += mdl->numPages * PAGE_SIZE;
    gfx_shm_statistic.free_count += 1;
}

gctINT
_ShmMapUser(
    gckALLOCATOR Allocator,
    PLINUX_MDL Mdl,
    PLINUX_MDL_MAP MdlMap,
    gctBOOL Cacheable
    )
{
    PLINUX_MDL      mdl = Mdl;
    PLINUX_MDL_MAP  mdlMap = MdlMap;
    struct mdl_shm_priv * priv = (struct mdl_shm_priv *)mdl->priv;

    gcmkHEADER_ARG("Allocator=%p Mdl=%p MdlMap=%p gctBOOL=%d", Allocator, Mdl, MdlMap, Cacheable);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    mdlMap->vmaAddr = (gctSTRING)vm_mmap(gcvNULL,
                    0L,
                    mdl->numPages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    0);
#else
    down_write(&current->mm->mmap_sem);

    mdlMap->vmaAddr = (gctSTRING)do_mmap_pgoff(gcvNULL,
                    0L,
                    mdl->numPages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    0);

    up_write(&current->mm->mmap_sem);
#endif

    gcmkTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_OS,
        "%s(%d): vmaAddr->0x%X for phys_addr->0x%X",
        __FUNCTION__, __LINE__,
        (gctUINT32)(gctUINTPTR_T)mdlMap->vmaAddr,
        (gctUINT32)(gctUINTPTR_T)mdl
        );

    if (IS_ERR(mdlMap->vmaAddr))
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_OS,
            "%s(%d): do_mmap_pgoff error",
            __FUNCTION__, __LINE__
            );

        mdlMap->vmaAddr = gcvNULL;

        gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_MEMORY);
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    down_write(&current->mm->mmap_sem);

    mdlMap->vma = find_vma(current->mm, (unsigned long)mdlMap->vmaAddr);

    if (mdlMap->vma == gcvNULL)
    {
        up_write(&current->mm->mmap_sem);

        gcmkTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_OS,
            "%s(%d): find_vma error",
            __FUNCTION__, __LINE__
            );

        mdlMap->vmaAddr = gcvNULL;

        gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_RESOURCES);
        return gcvSTATUS_OUT_OF_RESOURCES;
    }

    mdlMap->vma->vm_flags |= gcdVM_FLAGS;

    if (Cacheable == gcvFALSE)
    {
        /* Make this mapping non-cached. */
        mdlMap->vma->vm_page_prot = gcmkPAGED_MEMROY_PROT(mdlMap->vma->vm_page_prot);
    }

    mdlMap->vma->vm_pgoff = 0;

    if (remap_pfn_range(mdlMap->vma,
                        mdlMap->vma->vm_start,
                        priv->physical >> PAGE_SHIFT,
                        mdl->numPages*PAGE_SIZE,
                        mdlMap->vma->vm_page_prot) < 0)
    {
        up_write(&current->mm->mmap_sem);

        gcmkTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): remap_pfn_range error.",
            __FUNCTION__, __LINE__
            );

        mdlMap->vmaAddr = gcvNULL;

        gcmkFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_RESOURCES);
        return gcvSTATUS_OUT_OF_RESOURCES;
    }

    up_write(&current->mm->mmap_sem);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

}

void
_ShmUnmapUser(
    IN gckALLOCATOR Allocator,
    IN gctPOINTER Logical,
    IN gctUINT32 Size
    )
{
    if (unlikely(current->mm == gcvNULL))
    {
        /* Do nothing if process is exiting. */
        return;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
    if (vm_munmap((unsigned long)Logical, Size) < 0)
    {
        gcmkTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): vm_munmap failed",
                __FUNCTION__, __LINE__
                );
    }
#else
    down_write(&current->mm->mmap_sem);
    if (do_munmap(current->mm, (unsigned long)Logical, Size) < 0)
    {
        gcmkTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): do_munmap failed",
                __FUNCTION__, __LINE__
                );
    }
    up_write(&current->mm->mmap_sem);
#endif
}

gceSTATUS
_ShmMapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    OUT gctPOINTER *Logical
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
_ShmUnmapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    OUT gctPOINTER Logical
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
_ShmLogicalToPhysical(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical,
    IN gctUINT32 ProcessID,
    OUT gctPHYS_ADDR_T * Physical
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
_ShmPhysical(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctUINT32 Offset,
    OUT gctPHYS_ADDR_T * Physical
    )
{
    struct mdl_shm_priv *priv = Mdl->priv;
    *Physical = priv->physical;
    return gcvSTATUS_OK;
}

extern gceSTATUS
_DefaultCache(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical,
    IN gctUINT32 Physical,
    IN gctUINT32 Bytes,
    IN gceCACHEOPERATION Operation
    );

/* Operations. */
gcsALLOCATOR_OPERATIONS ShmAllocatorOperations = {
    .Alloc              = _ShmAlloc,
    .Free               = _ShmFree,
    .MapUser            = _ShmMapUser,
    .UnmapUser          = _ShmUnmapUser,
    .MapKernel          = _ShmMapKernel,
    .UnmapKernel        = _ShmUnmapKernel,
    .LogicalToPhysical  = _ShmLogicalToPhysical,
    .Cache              = _DefaultCache,
    .Physical           = _ShmPhysical,
};

void
_ShmAllocatorDestructor(
    IN void* PrivateData
    )
{
    struct _gcsSHM_ALLOCATOR_PRIV *priv = (struct _gcsSHM_ALLOCATOR_PRIV *)PrivateData;

    kfree(priv);
}

/* Entry. */
gceSTATUS
_ShmAlloctorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator = gcvNULL;
    struct _gcsSHM_ALLOCATOR_PRIV *priv = gcvNULL;

    gcmkONERROR(
        gckALLOCATOR_Construct(Os, &ShmAllocatorOperations, &allocator));

    priv = kzalloc(sizeof(struct _gcsSHM_ALLOCATOR_PRIV), GFP_KERNEL);

    if (!priv)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    allocator->privateData = priv;
    allocator->privateDataDestructor = _ShmAllocatorDestructor;

    allocator->capability = gcvALLOC_FLAG_CONTIGUOUS;

    *Allocator = allocator;

    return gcvSTATUS_OK;

OnError:
    if (priv)
    {
        kfree(priv);
    }

    if (allocator)
    {
        gckOS_Free(Os, allocator);
    }
    return status;
}

