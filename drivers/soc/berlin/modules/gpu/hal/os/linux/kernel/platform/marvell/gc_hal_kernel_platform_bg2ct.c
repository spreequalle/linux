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
#include "gc_hal_kernel_platform.h"

int debugSHM = 0;
module_param(debugSHM, int, 0644);

int maxSHMSize = 536870912;    //set to 512MB
module_param(maxSHMSize, int, 0644);

gctBOOL
_NeedAddDevice(
    IN gckPLATFORM Platform
    )
{
    return gcvTRUE;
}

gceSTATUS
_AdjustParam(
    IN gckPLATFORM Platform,
    OUT gcsMODULE_PARAMETERS *Args
    )
{
    /* 3D. */
    Args->registerMemBase = 0xf7bc0000;
    Args->registerMemSize = 2 << 10;
    Args->irqLine = 37;

#if SOC_BERLIN2Q
    /* 2D. */
    Args->registerMemBase2D = 0xF7EF0000;
    Args->registerMemSize2D = 2 << 10;
    Args->irqLine2D = 38;
#endif

    /* gcvPOOL_SYSTEM. */
#if defined(USE_SHM) && USE_SHM
    Args->contiguousSize = 0;
#elif USE_GALOIS_SHM
    Args->contiguousBase = 0;
    Args->contiguousSize = 512;
#else
    Args->contiguousBase = CONFIG_MV88DE3015_GPUMEM_START;
    Args->contiguousSize = CONFIG_MV88DE3015_GPUMEM_SIZE;
#endif

#if USE_HARDWARE_MMU
    Args->physSize = 0x80000000;
#endif

    return gcvSTATUS_OK;
}

gcsPLATFORM_OPERATIONS platformOperations =
{
    .needAddDevice = _NeedAddDevice,
    .adjustParam   = _AdjustParam,
};

void
gckPLATFORM_QueryOperations(
    IN gcsPLATFORM_OPERATIONS ** Operations
    )
{
     *Operations = &platformOperations;
}
