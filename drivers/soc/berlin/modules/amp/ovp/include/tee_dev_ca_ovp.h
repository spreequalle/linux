/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TEE_CA_OVP_H
#define TEE_CA_OVP_H
/** DHUB_API in trust zone
 */

#include    "ctypes.h"
#include    "dev_ovp_cmd.h"
/*structure define*/

#ifdef  __cplusplus
extern "C" {
#endif

	int MV_OVP_Get_Clr_IntrSts(unsigned int *puiIntrSts);

#ifdef  __cplusplus
}
#endif
/** DHUB_API in trust zone
 */
#endif
/** ENDOFFILE: tee_ca_dhub.h
 */
