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

static uint vmetaClk = 0;
module_param(vmetaClk, uint, 0644);

static uint h1CoreClk = 0;
module_param(h1CoreClk, uint, 0644);

static uint gfx3DCoreClk = 0;
module_param(gfx3DCoreClk, uint, 0644);

static uint gfx2DCoreClk = 0;
module_param(gfx2DCoreClk, uint, 0644);

#define CORE_SPEC_NUM(a)     (sizeof(a) / sizeof(struct cpm_core_mod_spec))

struct cpm_core_mod_spec bg4ct_core_modules[] = {
	{"V4G",			//module name
	 "decoderClk",		//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_LOW,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_NOT_CHANGE_GFX_CLK_SRC,	//change GFX clock source
	 &v4gClk		//clock frequency
	 },
	{"VMET",		//module name
	 "decoderPCubeClk",	//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_LOW,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_NOT_CHANGE_GFX_CLK_SRC,	//change GFX clock source
	 &vmetaClk		//clock frequency
	 },
	{"H1",			//module name
	 "H1Clk",		//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_LOW,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_NOT_CHANGE_GFX_CLK_SRC,	//change GFX clock source
	 &h1CoreClk		//clock frequency
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
	{"GFX2",		//module name
	 "gfx2DCoreClk",	//clock name
	 CPM_STATUS_CORE_HIGH,	//status
	 CPM_REQUEST_CORE_HIGH,	//request
	 CPM_REQUEST_CORE_LOW,	//saved_req
	 CPM_REQUEST_IGNORE_OFF,	//ignore
	 CPM_CHANGE_GFX2_CLK_SRC,	//change GFX clock source
	 &gfx2DCoreClk		//clock frequency
	 },
};

#define CORE_PS_NUM(a)     (sizeof(a) / sizeof(struct cpm_core_power_save_module))

/* active standby clock gating and power control */
struct cpm_core_power_save_module bg4ct_core_sb_modules[] = {
	{"HDMI", 0, 0},
	{"ADAC", 0, 0},
	{"VDAC", 0, 0},
	{"VPP", 0, 0},
	{"MAIN", 0, 0},
	{"PIP", 0, 0},
	{"V4G", 0, 0},
	{"VMET", 0, 0},
	{"H1", 0, 0},
	{"ZSP", 0, 0},
	{"OVP", 0, 0},
	{"TEST", 0, 0},
	{"AIO", 0, 0},
	{"TSEN", 0, 0},
	//{"AVIO",    0,     0},
	//{"GPCS",    0,     0},
};

/* low power mode clock gating and power control */
struct cpm_core_power_save_module bg4ct_core_lp_modules[] = {
	{"HDMI", 0, 0},
	{"ADAC", 0, 0},
	{"VDAC", 0, 0},
	{"VPP", 0, 0},
	{"MAIN", 0, 0},
	{"PIP", 0, 0},
	{"V4G", 0, 0},
	{"VMET", 0, 0},
	{"H1", 0, 0},
	{"ZSP", 0, 0},
	{"OVP", 0, 0},
	{"TEST", 0, 0},
	{"AIO", 0, 0},
	{"TSEN", 0, 0},
	{"TSI", 0, 0},
	//{"AVIO",    0,     0},
	//{"GPCS",    0,     0},
};

const struct cpm_core_ctrl_tbl core_ctrl_tbl_bg4ct = {
	CORE_SPEC_NUM(bg4ct_core_modules), bg4ct_core_modules,
	CORE_PS_NUM(bg4ct_core_sb_modules), bg4ct_core_sb_modules,
	CORE_PS_NUM(bg4ct_core_lp_modules), bg4ct_core_lp_modules
};

const struct cpm_core_ctrl_tbl *cur_core_tbl = &core_ctrl_tbl_bg4ct;
