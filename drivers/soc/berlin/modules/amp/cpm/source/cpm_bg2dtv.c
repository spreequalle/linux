/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "cpm_ca.h"
#include "cpm_driver.h"
#include <linux/stddef.h>

static uint v2gClk = 0;
module_param(v2gClk, uint, 0644);

static uint vmetaClk = 0;
module_param(vmetaClk, uint, 0644);

static uint gfx3DClk = 0;
module_param(gfx3DClk, uint, 0644);

static uint gfx2DClk = 0;
module_param(gfx2DClk, uint, 0644);

#define CORE_SPEC_NUM(a)     (sizeof(a) / sizeof(struct cpm_core_mod_spec))

struct cpm_core_mod_spec bg2dtv_core_modules[] = {
	{"V2G",			//module name
	 "V2gClk",		//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_LOW,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_NOT_CHANGE_GFX_CLK_SRC,	//change GFX clock source
	 &v2gClk		//clock frequency
	 },
	{"VMET",		//module name
	 "vScopeClk",		//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_LOW,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_NOT_CHANGE_GFX_CLK_SRC,	//change GFX clock source
	 &vmetaClk		//clock frequency
	 },
	{"GFX3",		//module name
	 "gfx3DCoreClk",	//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_LOW,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_NOT_CHANGE_GFX_CLK_SRC,	//change GFX clock source
	 &gfx3DClk		//clock frequency
	 },
	{"GFX2",		//module name
	 "gfx2DClk",		//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_HIGH,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_CHANGE_GFX2_CLK_SRC,	//change GFX clock source
	 &gfx2DClk		//clock frequency
	 },
};

const struct cpm_core_ctrl_tbl core_ctrl_tbl_bg2dtv = {
	CORE_SPEC_NUM(bg2dtv_core_modules), bg2dtv_core_modules,
	0, NULL
};

const struct cpm_core_ctrl_tbl *cur_core_tbl = &core_ctrl_tbl_bg2dtv;
