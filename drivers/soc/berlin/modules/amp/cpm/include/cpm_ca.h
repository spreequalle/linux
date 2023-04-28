/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _CPM_CA_H_
#define _CPM_CA_H_

#define MAX_CHIP_NAME_LEN       16
#define MAX_MOD_NAME_LEN        5
#define MAX_CLK_NAME_LEN        32

#define CPM_MAX_DEVS                    8
#define CPM_MINOR                       0
#define CPM_DEVICE_NAME                 "cpm"
#define CPM_DEVICE_TAG                  "[cpm_driver] "

#define CPM_RES_CTX                     1
#define CPM_RES_SESSION                 2
#define CPM_RES_SHM_IN                  4
#define CPM_RES_SHM_OUT                 8

#define CPM_IN_SHM_SIZE                 64
#define CPM_OUT_SHM_SIZE                32

#define TA_CPM_UUID {0xea2c26f2, 0x9942, 0x11e3, {0xab, 0xa5, 0x00, 0x0c, 0x29, 0x1c, 0x86, 0x93}}

enum {
	CPM_INVALID,
	CPM_READ_LEAKAGE_ID,
	CPM_SET_STARTUP_CFG,
	CPM_CP_CONTROL,
	CPM_CORE_CONTROL,
	CPM_GET_MOD_NUM,
	CPM_GET_MOD_NAME,
	CPM_GET_MOD_STAT,
	CPM_GET_CORE_CLK_NUM,
	CPM_GET_CORE_CLK_FREQ,
	CPM_GET_MODULE_CLK_FREQ,
	CPM_GET_CLK_NUM,
	CPM_GET_CLK_STAT,
	CPM_GET_TEMP,
	CPM_READ_CP_MOD,
	CPM_WRITE_CP_MOD,
	CPM_CMD_MAX
};

struct cpm_core_voltage_tbl {
	unsigned int leakage;
	unsigned int low_voltage;
	unsigned int middle_voltage;
	unsigned int high_voltage;
};

struct cpm_core_mod_spec {
	char name[MAX_MOD_NAME_LEN];
	char clkname[MAX_CLK_NAME_LEN];
	unsigned int status;
	unsigned int request;
	unsigned int saved_req;	//saved request for active standby
	unsigned int ignore;
	unsigned int chg_gfx;	//change gfx clock
	unsigned int *clkfreq;
};

struct cpm_core_power_save_module {
	char name[MAX_MOD_NAME_LEN];
	unsigned int clk;
	unsigned int pwr;
};

struct cpm_core_ctrl_tbl {
	unsigned int num_modules;
	struct cpm_core_mod_spec *mod_spec;
	unsigned int num_act_sb_modules;
	struct cpm_core_power_save_module *act_sb_mod;
	unsigned int num_low_power_modules;
	struct cpm_core_power_save_module *low_power_mod;
};

#endif /* _CPM_CA_H_ */
