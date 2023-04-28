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
#include <linux/stddef.h>
#include "cpm_ca.h"
#include "cpm_driver.h"

static uint v4gClk = 0;
module_param(v4gClk, uint, 0644);

static uint gfx3DCoreClk = 0;
module_param(gfx3DCoreClk, uint, 0644);

#define CORE_SPEC_NUM(a)     (sizeof(a) / sizeof(struct cpm_core_mod_spec))

struct cpm_core_mod_spec bg4dtv_core_modules[] = {
	{"V4G",			//module name
	 "v4gclk",		//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_LOW,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_NOT_CHANGE_GFX_CLK_SRC,	//change GFX clock source
	 &v4gClk		//clock frequency
	 },
	{"GFX3",		//module name
	 "gfx3DCoreClk",	//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_LOW,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_NOT_CHANGE_GFX_CLK_SRC,	//change GFX clock source
	 &gfx3DCoreClk		//clock frequency
	 },
};

#define CORE_PS_NUM(a)     (sizeof(a) / sizeof(struct cpm_core_power_save_module))

/* active standby clock gating and power control */
struct cpm_core_power_save_module bg4dtv_core_sb_modules[] = {
	{"HDMI", 0, 0},
	{"ADAC", 0, 0},
	{"VPP", 0, 0},
};

/* low power mode clock gating and power control */
struct cpm_core_power_save_module bg4dtv_core_lp_modules[] = {
	{"HDMI", 0, 0},
	{"ADAC", 0, 0},
	{"VPP", 0, 0},
};

const struct cpm_core_ctrl_tbl core_ctrl_tbl_bg4dtv = {
	CORE_SPEC_NUM(bg4dtv_core_modules), bg4dtv_core_modules,
	CORE_PS_NUM(bg4dtv_core_sb_modules), bg4dtv_core_sb_modules,
	CORE_PS_NUM(bg4dtv_core_lp_modules), bg4dtv_core_lp_modules
};

const struct cpm_core_ctrl_tbl *cur_core_tbl = &core_ctrl_tbl_bg4dtv;
