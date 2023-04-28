/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


 /********************************************************************************
 * Marvell BSD License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File under the following licensing terms.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *	Redistributions of source code must retain the above copyright notice,
 *	this list of conditions and the following disclaimer.
 *
 *	Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 *	Neither the name of Marvell nor the names of its contributors may be
 *	used to endorse or promote products derived from this software without
 *	specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************/

#ifndef	__GALOIS_CFG_H__
#define	__GALOIS_CFG_H__

/*!
 * IO read/write through software/hardware socket
 * #define	__MEMIO_SOCKET
 */

/*!
 * IO read/write through PCI-Express bus
 * #define	__MEMIO_PCIE
 */

/*!
 * IO read/write directly using MEMI/O
 */
#define	__MEMIO_DIRECT

/*!
 * Program is running under little endian system.
 */
#ifndef __LITTLE_ENDIAN
#define	__LITTLE_ENDIAN
#endif

/*!
 * Program is running under big endian system.
 * #define	__BIG_ENDIAN
 */

/*!
 * Program is running under Linux on ARM processor
 */
#define	__LINUX_ARM

/*!
 * Program is running under VxWorks on ARM processor
 * #define	__VxWORKS_ARM
 */

#endif /* __GALOIS_CFG_H__     */
