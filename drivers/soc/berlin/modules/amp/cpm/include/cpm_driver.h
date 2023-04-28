/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _CPM_DRIVER_H_
#define _CPM_DRIVER_H_

#define LEAKAGE_GROUP_NUM               10
#define VOLTAGE_TABLE_SIZE              (LEAKAGE_GROUP_NUM*4)	//group*(leakage,low,middle,high)

#define GFX_CLK_GATTING_VAL             0xFFFFFFFF
#define GFX_LOW_TIME_OUT                (2*HZ)	//2s

#define TEMP_CHECK_CYCLE                20000
#define TEMP_UP_THR                     100	//overheat up threshold
#define TEMP_LOW_THR                    95	//overheat low threshold

#define CPM_RES_TIMIER                  1
#define CPM_RES_TIMIER_TASK             2

#define REGULATOR_STEP                  25000	//25mV
#define REGULATOR_ADJ_CNT_MAX           (1250000/REGULATOR_STEP)	//1.25V/25mV

typedef int (*GFX_CALLBACK) (void *gfx, int gfx_mod);

typedef struct {
	void *gfx;
	GFX_CALLBACK before_change;
	GFX_CALLBACK after_change;
} CPM_GFX_CALLBACK;

typedef struct {
	struct cpm_core_mod_spec *gfx3d;
} CPM_GFX_LOADING;

enum {
	CPM_GFX2D_CALLBACK,
	CPM_GFX3D_CALLBACK,
};

enum {
	CPM_IOCTL_CMD_LOAD_CORE_VOLTAGE_TABLE = 100,
	CPM_IOCTL_CMD_INIT_CORE_STATE,
	CPM_IOCTL_CMD_SET_STARTUP_CFG,
	CPM_IOCTL_CMD_CP_CONTROL,
	CPM_IOCTL_CMD_CP_MOD_SAVE,
	CPM_IOCTL_CMD_CP_MOD_DISABLE,
	CPM_IOCTL_CMD_CP_MOD_RESTORE,
	CPM_IOCTL_CMD_GET_CP_MOD_NUM,
	CPM_IOCTL_CMD_GET_CP_MOD_NAME,
	CPM_IOCTL_CMD_GET_CP_MOD_STATUS,
	CPM_IOCTL_CMD_GET_CORE_MOD_NUM,
	CPM_IOCTL_CMD_GET_CORE_MOD_STATUS,
	CPM_IOCTL_CMD_GET_CORE_CLK_NUM,
	CPM_IOCTL_CMD_GET_CORE_CLK_FREQ,
	CPM_IOCTL_CMD_GET_CORE_STATUS,
	CPM_IOCTL_CMD_REQUEST_CORE_LOW,
	CPM_IOCTL_CMD_REQUEST_CORE_HIGH,
	CPM_IOCTL_CMD_REQUEST_IGNORE_OFF,
	CPM_IOCTL_CMD_REQUEST_IGNORE_ON,
	CPM_IOCTL_CMD_GET_CLK_NUM,
	CPM_IOCTL_CMD_GET_CLK_STATUS,
	CPM_IOCTL_CMD_FREEZE_GFX,
	CPM_IOCTL_CMD_MAX,
};

enum {
	CPM_REQUEST_CORE_LOW,
	CPM_REQUEST_CORE_HIGH,
	CPM_REQUEST_MAX,
};

enum {
	CPM_STATUS_CORE_LOW = CPM_REQUEST_CORE_LOW,
	CPM_STATUS_CORE_HIGH = CPM_REQUEST_CORE_HIGH,
};

enum {
	CPM_REQUEST_IGNORE_OFF,
	CPM_REQUEST_IGNORE_ON,
	CPM_REQUEST_IGNORE_MAX,
};

enum {
	CPM_CORE_VOLTAGE_LOW,
	CPM_CORE_VOLTAGE_MIDDLE,
	CPM_CORE_VOLTAGE_HIGH,
	CPM_CORE_VOLTAGE_MAX,
};

enum {
	CPM_NOT_CHANGE_GFX_CLK_SRC,
	CPM_CHANGE_GFX2_CLK_SRC,
	CPM_CHANGE_GFX3_CLK_SRC,
};

enum {
	CPM_CTRL_CLOCK = 1,
	CPM_CTRL_POWER = 2,
	CPM_CTRL_BOTH = 3,
	CPM_CTRL_MASK = 7,
};

enum {
	CPM_ACTIVE_STANDBY_MODE,
	CPM_LOW_POWER_MODE,
	CPM_POWER_SAVE_MAX,
};

int cpm_set_core_mode(char *name, unsigned int request);
void cpm_register_gfx_callback(CPM_GFX_CALLBACK * callback);
void cpm_unregister_gfx_callback(void);
int cpm_set_gfx3d_loading(unsigned int loading);
int cpm_cp_mod_save(unsigned int mode);
int cpm_cp_mod_disable(unsigned int mode, unsigned int type);
int cpm_cp_mod_restore(unsigned int mode, unsigned int type);
int cpm_module_init(void);
void cpm_module_exit(void);
#endif
