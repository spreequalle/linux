/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


////////////////////////////////////////////////////////////////////////////////
//! \file dhub_cmd.h
//!
//! \brief Header file of commands for TrustZone.
//!
//! Purpose: This is used for controlling access to dhub API in TrustZone.
//!
//!
//! Note:
////////////////////////////////////////////////////////////////////////////////

#ifndef __DHUB_CMD_H__
#define __DHUB_CMD_H__

#define TA_DHUB_UUID {0x13a7d413, 0x1b94, 0x4780, {0x84, 0xac, 0x49, 0xad, 0x0b, 0x03, 0x17, 0x67}}

/* enum for DHUB commands */
typedef enum {
	DHUB_INIT,
	DHUB_CHANNEL_CLEAR,

	DHUB_CHANNEL_WRITECMD,
	DHUB_CHANNEL_GENERATECMD,

	DHUB_SEM_POP,
	DHUB_SEM_CLR_FULL,
	DHUB_SEM_CHK_FULL,
	DHUB_SEM_INTR_ENABLE,

	DHUB_PASSSHM,

} DHUB_CMD_ID;

#endif /* __DHUB_CMD_H__ */
