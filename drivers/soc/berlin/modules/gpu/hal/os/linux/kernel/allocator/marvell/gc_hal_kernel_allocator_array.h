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


extern gceSTATUS
_DefaultAlloctorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    );

#if defined(USE_ION) && USE_ION
extern gceSTATUS
_IonAlloctorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    );
#endif

#if defined(USE_SHM) && USE_SHM
gceSTATUS
_ShmAlloctorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    );
#endif

gcsALLOCATOR_DESC allocatorArray[] =
{
#if defined(USE_ION) && USE_ION
    /* ION allocator. */
    gcmkDEFINE_ALLOCATOR_DESC("ion", _IonAlloctorInit),
#endif

#if defined(USE_SHM) && USE_SHM
    /* SHM allocator. */
    gcmkDEFINE_ALLOCATOR_DESC("shm", _ShmAlloctorInit),
#endif

    /* Default allocator. */
    gcmkDEFINE_ALLOCATOR_DESC("default", _DefaultAlloctorInit),
};


