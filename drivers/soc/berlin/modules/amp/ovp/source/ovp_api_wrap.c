/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "tee_dev_ca_ovp.h"

INT MV_OVP_Get_Clr_IntrSts(UINT32 * puiIntrSts)
{
	HRESULT Ret = S_OK;
#ifdef OVP_IN_TZ
	Ret = OvpGet_Clr_IntrSts(puiIntrSts);
#else
	UINT32 uiOvpIntr;
	GA_REG_WORD32_READ(OVP_REG_BASE + OVP_INTR_STS, &uiOvpIntr);
	*puiIntrSts = uiOvpIntr;
	GA_REG_WORD32_WRITE(OVP_REG_BASE + OVP_INTR_STS, 0x3F);
#endif
	return Ret;
}
