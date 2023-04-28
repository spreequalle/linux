/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _PE_MEMMAP_H_
#define _PE_MEMMAP_H_

#define        MEMMAP_VIP_DHUB_REG_BASE                    0xF7AE0000
#define        MEMMAP_VIP_DHUB_REG_SIZE                    0x20000
#define        MEMMAP_VIP_DHUB_REG_DEC_BIT                 0x11

#define        MEMMAP_AVIF_REG_BASE                        0xF7980000
#define        MEMMAP_AVIF_REG_SIZE                        0x80000
#define        MEMMAP_AVIF_REG_DEC_BIT                     0x13

#define        MEMMAP_ZSP_REG_BASE                         0xF7C00000
#define        MEMMAP_ZSP_REG_SIZE                         0x80000
#define        MEMMAP_ZSP_REG_DEC_BIT                      0x13

#define        MEMMAP_AG_DHUB_REG_BASE                     0xF7D00000
#define        MEMMAP_AG_DHUB_REG_SIZE                     0x20000
#define        MEMMAP_AG_DHUB_REG_DEC_BIT                  0x11

#define        MEMMAP_VPP_DHUB_REG_BASE                    0xF7F40000
#define        MEMMAP_VPP_DHUB_REG_SIZE                    0x20000
#define        MEMMAP_VPP_DHUB_REG_DEC_BIT                 0x11

#define        MEMMAP_TSP_REG_BASE                         0xF7A40000
#define        MEMMAP_TSP_REG_SIZE                         0x40000
#define        MEMMAP_TSP_REG_DEC_BIT                      0x12

#endif //_PE_MEMMAP_H_
