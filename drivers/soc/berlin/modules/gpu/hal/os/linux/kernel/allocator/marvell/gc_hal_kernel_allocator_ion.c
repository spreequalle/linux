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

#include "gc_hal_kernel_platform.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION (3,10,0)
#include <../drivers/staging/android/ion/ion.h>
#else
#include <linux/ion.h>
#endif

#define ION_AF_GFX_NONSECURE (ION_A_NS | ION_A_CG | ION_A_FC)
#define ION_AF_GFX_SECURE    (ION_A_FS | ION_A_CG | ION_A_FC)

#define _GC_OBJ_ZONE    gcvZONE_OS

#define RESERVE_MEM_THRESHOLD (0x1 << 24)

typedef struct _gcsION_PRIV {
    struct ion_client * ionClient;
    unsigned int securePoolID;
    unsigned int nonSecurePoolID;
    gctUINT32 secureUsage;
    gctUINT32 nonSecureUsage;
} gcsION_PRIV, *gcsION_PRIV_PTR;

struct mdl_priv {
    gctUINT32 physical;
    struct ion_handle       *ionHandle;
    gctUINT32               gid;
};

int gc_ion_usage_show(struct seq_file* m, void* data)
{
    gcsINFO_NODE *node = m->private;
    gckALLOCATOR Allocator = node->device;
    gcsION_PRIV_PTR priv = Allocator->privateData;

    seq_printf(m, "secure:  %u bytes\n", priv->secureUsage);
    seq_printf(m, "non-secure: %u bytes\n", priv->nonSecureUsage);

    return 0;
}

static gcsINFO InfoList[] =
{
    {"ionUsage", gc_ion_usage_show},
};

static void
_IonAllocatorDebugfsInit(
    IN gckALLOCATOR Allocator,
    IN gckDEBUGFS_DIR Root
    )
{
    gcmkVERIFY_OK(
        gckDEBUGFS_DIR_Init(&Allocator->debugfsDir, Root->root, "ion"));

    gcmkVERIFY_OK(gckDEBUGFS_DIR_CreateFiles(
        &Allocator->debugfsDir,
        InfoList,
        gcmCOUNTOF(InfoList),
        Allocator
        ));
}

static void
_IonAllocatorDebugfsCleanup(
    IN gckALLOCATOR Allocator
    )
{
    gcmkVERIFY_OK(gckDEBUGFS_DIR_RemoveFiles(
        &Allocator->debugfsDir,
        InfoList,
        gcmCOUNTOF(InfoList)
        ));

    gckDEBUGFS_DIR_Deinit(&Allocator->debugfsDir);
}

static gceSTATUS
_IonAlloc(
    IN gckALLOCATOR Allocator,
    INOUT PLINUX_MDL Mdl,
    IN gctSIZE_T NumPages,
    IN gctUINT32 Flags
    )
{
    gceSTATUS status;
    unsigned int poolID, gid;
    gctBOOL ionAlloc = gcvFALSE;
    gctINT32 ionMemUsageMode, address;
    struct ion_handle *handle = NULL;
    struct mdl_priv *priv = NULL;
    size_t size;
    gcsION_PRIV_PTR allocatorPriv = (gcsION_PRIV_PTR)Allocator->privateData;

    /* Comptible with old code. */
    gctUINT32 bytes = NumPages * PAGE_SIZE;
    gctUINT32 *Bytes = &bytes;
    gckOS os = Allocator->os;
    gctBOOL contiguous = (Flags & gcvALLOC_FLAG_NON_CONTIGUOUS) ? gcvFALSE : gcvTRUE;
    gctBOOL secure = (Flags & gcvALLOC_FLAG_SECURITY) ? gcvTRUE : gcvFALSE;
    gctUINT32 contiguousSize;

    gcmkHEADER_ARG("Os=0x%X Bytes=%lu Flags=%x", os, *Bytes, Flags);

    gcmkONERROR(gckOS_QueryOption(os, "contiguousSize", &contiguousSize));
    gcmkONERROR(gckOS_QueryOption(os, "ionMemUsageMode", (gctUINT32 *)&ionMemUsageMode));

    switch (ionMemUsageMode)
    {
    case gcvIONMEM_USAGE_FULL:
#if defined(SHM_FROM_SYSTEM) && SHM_FROM_SYSTEM
        status = gcvSTATUS_OK;
        break;
#endif
    case gcvIONMEM_USAGE_CONTIGUOUS:
        status = (contiguous == gcvTRUE) ? gcvSTATUS_OK : gcvSTATUS_OUT_OF_MEMORY;
        break;
    case gcvIONMEM_USAGE_DEFAULT:
        status = gcvSTATUS_OK;
        if (contiguousSize <= RESERVE_MEM_THRESHOLD && (Flags & gcvALLOC_FLAG_ION) == 0)
        {
            status = gcvSTATUS_OUT_OF_MEMORY;
        }

        if (contiguous == gcvFALSE)
        {
            status = gcvSTATUS_OUT_OF_MEMORY;
        }
        break;
    case gcvIONMEM_USAGE_NONE:
        status = gcvSTATUS_OUT_OF_MEMORY;
        break;
    default:
        printk(KERN_ERR "gpu driver: invalid value %d for ionMemUsageMode\n", ionMemUsageMode);
        status = gcvSTATUS_OUT_OF_MEMORY;
        break;
    }
    gcmkONERROR(status);

    /* Allocate ion memory. */
    poolID = secure ? allocatorPriv->securePoolID : allocatorPriv->nonSecurePoolID;
    handle = ion_alloc(allocatorPriv->ionClient, *Bytes, PAGE_SIZE, 1 << poolID, ION_NONCACHED);
    if (IS_ERR(handle))
    {
        printk(KERN_ERR "gfx driver: ion_alloc failed, error code=%p!\n", handle);
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }
    ionAlloc = gcvTRUE;

    /* Get physical address of the ion handle. */
    if (ion_phys(allocatorPriv->ionClient, handle, (ion_phys_addr_t *)&address, &size) < 0)
    {
        printk(KERN_ERR "gfx driver: ion_phys failed!\n");
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* Export and get global id. */
    if (ion_getgid(handle, &gid) < 0)
    {
        printk(KERN_ERR "gfx driver: ion_getgid failed!\n");
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    gcmkONERROR(gckOS_Allocate(os, sizeof(*priv), (gctPOINTER *)&priv));
    priv->physical = address;
    priv->ionHandle = handle;
    priv->gid = gid;

    /* Do not map to kernel space. Actuallty ION calls __arm_ioremap().
       It occupies a new vmalloc area in the kernel space. */
    Mdl->addr = gcvNULL;
    Mdl->pagedMem = 1;
    Mdl->priv = priv;
    Mdl->gid = gid;
    Mdl->secure = secure;

    /* Why use PAGE_CACHE_SIZE instead of PAGE_SIZE? */
    Mdl->numPages = GetPageCount(size, 0);
#if defined(SHM_FROM_SYSTEM) && SHM_FROM_SYSTEM
    /* Wraps nonContiguousPages because Mdl->contiguous is set outside this ballback. */
    if (contiguous)
    {
        Mdl->u.contiguousPages = phys_to_page(address);
    }
    else
    {
        int i = 0;
        struct page **pages = NULL;
        gcmkONERROR(gckOS_Allocate(os, sizeof(struct page *) * Mdl->numPages, (gctPOINTER *)&pages));
        gcmkONERROR(gckOS_ZeroMemory(pages, sizeof(struct page *) * Mdl->numPages));
        for (i = 0; i < Mdl->numPages; i++)
        {
            pages[i] = phys_to_page(address + i * PAGE_SIZE);
        }
        Mdl->u.nonContiguousPages = pages;
    }
#else
    Mdl->u.contiguousPages = gcvNULL;
#endif

    *Bytes = size;

    if (secure)
    {
        allocatorPriv->secureUsage += size;
    }
    else
    {
        allocatorPriv->nonSecureUsage += size;
    }

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (priv)
    {
        gckOS_Free(os, priv);
    }

    if (ionAlloc)
    {
        ion_free(allocatorPriv->ionClient, handle);
    }

    /* Return the status. */
    gcmkFOOTER();
    return status;
}

static void
_IonFree(
    IN gckALLOCATOR Allocator,
    IN OUT PLINUX_MDL Mdl
    )
{
    gcsION_PRIV_PTR allocatorPriv = (gcsION_PRIV_PTR)Allocator->privateData;
    struct mdl_priv * mdlPriv = (struct mdl_priv *)Mdl->priv;

    /* Free ion memory. */
    ion_free(allocatorPriv->ionClient, mdlPriv->ionHandle);

    if (Mdl->secure)
    {
        allocatorPriv->secureUsage -= Mdl->numPages * PAGE_CACHE_SIZE;
    }
    else
    {
        allocatorPriv->nonSecureUsage -= Mdl->numPages * PAGE_CACHE_SIZE;
    }
}

gctINT
_IonMapUser(
    gckALLOCATOR Allocator,
    PLINUX_MDL Mdl,
    PLINUX_MDL_MAP MdlMap,
    gctBOOL Cacheable
    )
{
    PLINUX_MDL      mdl = Mdl;
    PLINUX_MDL_MAP  mdlMap = MdlMap;
    struct mdl_priv *priv = mdl->priv;

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
_IonUnmapUser(
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

extern gceSTATUS
_DefaultUnmapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical
    );

extern gceSTATUS
_DefaultLogicalToPhysical(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical,
    IN gctUINT32 ProcessID,
    OUT gctPHYS_ADDR_T * Physical
    );

extern gceSTATUS
_DefaultCache(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical,
    IN gctUINT32 Physical,
    IN gctUINT32 Bytes,
    IN gceCACHEOPERATION Operation
    );

gceSTATUS
_IonPhysical(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctUINT32 Offset,
    OUT gctPHYS_ADDR_T * Physical
    )
{
    struct mdl_priv *priv = Mdl->priv;
    *Physical = priv->physical + Offset * PAGE_SIZE;
    return gcvSTATUS_OK;
}

gceSTATUS
_DefaultMapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    OUT gctPOINTER *Logical
    );

gceSTATUS
_DefaultUnmapKernel(
    IN gckALLOCATOR Allocator,
    IN PLINUX_MDL Mdl,
    IN gctPOINTER Logical
    );

/* Default allocator operations. */
gcsALLOCATOR_OPERATIONS IonAllocatorOperations = {
    .Alloc              = _IonAlloc,
    .Free               = _IonFree,
    .MapUser            = _IonMapUser,
    .UnmapUser          = _IonUnmapUser,
    .MapKernel          = _DefaultMapKernel,
    .UnmapKernel        = _DefaultUnmapKernel,
    .LogicalToPhysical  = _DefaultLogicalToPhysical,
    .Cache              = _DefaultCache,
    .Physical           = _IonPhysical,
};

void
_IonPrivateDataDestructor(
    IN void* PrivateData
    )
{
    gcsION_PRIV_PTR priv = (gcsION_PRIV_PTR)PrivateData;

    ion_client_destroy(priv->ionClient);

    kfree(priv);
}

static gceSTATUS
_IonPrivateDataConstruct(
    IN gckOS Os,
    INOUT gcsION_PRIV_PTR PrivateData
    )
{
    gceSTATUS status;
    int nPools = 0;
    unsigned int poolID = 0;
    struct ion_heap_info *poolInfo = NULL;

    gcmkASSERT(PrivateData != gcvNULL);

    PrivateData->ionClient = ion_client_create(ion_get_dev(), "gc ion");

    if (IS_ERR(PrivateData->ionClient))
    {
        printk(KERN_ERR "gfx driver: failed to create ion client!\n");
        gcmkONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Get ion pool info. */
    if (ion_get_heap_num(&nPools) < 0)
    {
        printk(KERN_ERR "gfx driver: ion_get_heap_num failed!\n");
        gcmkONERROR(gcvSTATUS_CONTEXT_LOSSED);
    }

    gcmkONERROR(
        gckOS_Allocate(Os, sizeof(*poolInfo) * nPools, (gctPOINTER *)&poolInfo));
    gckOS_ZeroMemory(poolInfo, sizeof(*poolInfo) * nPools);

    if (ion_get_heap_info(poolInfo) < 0)
    {
        printk(KERN_ERR "gfx driver: ion_get_heap_info failed!\n");
        gcmkONERROR(gcvSTATUS_CONTEXT_LOSSED);
    }

    /* Query secure and non-secure pool IDs. */
    PrivateData->securePoolID = PrivateData->nonSecurePoolID = nPools;
    for (poolID = 0; poolID < nPools; poolID++)
    {
        if ((poolInfo[poolID].attribute & ION_AF_GFX_SECURE) == ION_AF_GFX_SECURE)
        {
            PrivateData->securePoolID = poolID;
        }
        else if ((poolInfo[poolID].attribute & ION_AF_GFX_NONSECURE) == ION_AF_GFX_NONSECURE)
        {
            PrivateData->nonSecurePoolID = poolID;
        }

        if (PrivateData->securePoolID < nPools &&
            PrivateData->nonSecurePoolID < nPools)
        {
            break;
        }
    }
    gckOS_Free(Os, poolInfo);
    poolInfo = gcvNULL;

    if (PrivateData->securePoolID == nPools ||
        PrivateData->nonSecurePoolID == nPools)
    {
        printk(KERN_ERR "gfx driver: no matched ion pool!\n");
        gcmkONERROR(gcvSTATUS_NOT_FOUND);
    }

    PrivateData->secureUsage = 0;
    PrivateData->nonSecureUsage = 0;

    return gcvSTATUS_OK;

OnError:
    if (poolInfo)
    {
        gckOS_Free(Os, poolInfo);
    }

    if (PrivateData->ionClient)
    {
        ion_client_destroy(PrivateData->ionClient);
    }
    return status;
}

/* Default allocator entry. */
gceSTATUS
_IonAlloctorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator = gcvNULL;
    gcsION_PRIV_PTR priv = gcvNULL;

    gcmkONERROR(
        gckALLOCATOR_Construct(Os, &IonAllocatorOperations, &allocator));

    priv = kzalloc(sizeof(gcsION_PRIV), GFP_KERNEL);
    if (!priv)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    gcmkONERROR(_IonPrivateDataConstruct(Os, priv));

    allocator->privateData = priv;
    allocator->privateDataDestructor = _IonPrivateDataDestructor;

    allocator->debugfsInit = _IonAllocatorDebugfsInit;
    allocator->debugfsCleanup = _IonAllocatorDebugfsCleanup;

    allocator->capability = gcvALLOC_FLAG_CONTIGUOUS
                          | gcvALLOC_FLAG_NON_CONTIGUOUS
                          | gcvALLOC_FLAG_SECURITY
                          | gcvALLOC_FLAG_ION
                          | gcvALLOC_FLAG_MEMLIMIT
                          ;

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


