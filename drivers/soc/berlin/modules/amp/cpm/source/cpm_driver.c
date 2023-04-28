/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*******************************************************************************
  System head files
  */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>

#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <asm/uaccess.h>
/*******************************************************************************
  Local head files
  */
#include "tee_client_api.h"

#include "cpm_ca.h"
#include "cpm_driver.h"
/*******************************************************************************
  Module API defined
  */

struct cpm_device_t {
	unsigned char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	struct file_operations *fops;
	int major;
	int minor;
};

typedef struct _cpm_ca {
	int inited;

	TEEC_Context ctx;
	TEEC_Session session;
	TEEC_SharedMemory shm_in;
	TEEC_SharedMemory shm_out;
	TEEC_Operation op;

	int res_alloc;
} cpm_ca_t;

typedef struct _cpm_timer {
	int inited;
	struct timer_list cpm_timer;
	struct task_struct *cpm_timer_handle;
	int res_alloc;
} cpm_timer_t;

typedef struct _cpm_update {
	int taskrunning;
	struct task_struct *cpm_update_handle;
} cpm_update_t;

typedef struct _cpm_temp {
	int taskrunning;
	int temp;
	struct task_struct *cpm_temp_handle;
} cpm_temp_t;

extern const struct cpm_core_ctrl_tbl *cur_core_tbl;

static int cpm_driver_open(struct inode *inode, struct file *filp);
static int cpm_driver_release(struct inode *inode, struct file *filp);
static long cpm_driver_ioctl(struct file *filp, unsigned int cmd,
			     unsigned long arg);
static int cpm_init_core_status(void);
static void cpm_get_core_regulator(void);
static struct cpm_core_mod_spec *cpm_get_core_module_by_name(const char *name);
static int cpm_reset_timer(void);

/*******************************************************************************
  Macro Defined
  */
//#define CPM_LOG_EN
#ifdef CPM_LOG_EN
/* default printk level is KERN_WARNING */
#define cpm_info(...)         printk(KERN_INFO CPM_DEVICE_TAG __VA_ARGS__)
#define cpm_error(...)        printk(KERN_ERR CPM_DEVICE_TAG __VA_ARGS__)
#else
#define cpm_info(...)
#define cpm_error(...)
#endif

#define CHECK_CORE_TABLE(_ret)                                   \
    do{                                                         \
        if (!cur_core_tbl) {                                     \
            cpm_error("ERROR: %s NO CHIP IS SPECIFIED!\n",__FUNCTION__); \
            return (_ret);                                      \
        }                                                       \
    }while(0)

#define CHECK_CORE_REGULATOR(_ret)                                   \
    do{                                                         \
        if (!vcore_reg) {                                     \
            cpm_error("ERROR: %s Did not get Core regulator!\n",__FUNCTION__); \
            return (_ret);                                      \
        }                                                       \
    }while(0)

#define CHECK_TEE_INIT(_ret)                                   \
    do{                                                         \
        if (!cpm_ca.inited) {                                     \
            cpm_error("%s TEE has not been initialized!\n", __FUNCTION__); \
            return (_ret);                       \
        }                                                       \
    }while(0)

#define CHECK_VALID_POINTER(_pt)                                   \
    do{                                                         \
        if (!_pt) {                                     \
            cpm_error("%s BAD PARAMETER!\n", __FUNCTION__); \
            return (TEEC_ERROR_BAD_PARAMETERS);                       \
        }                                                       \
    }while(0)

#define CHECK_RESULT(_res)                                   \
    do{                                                         \
        if (_res != 0) {                                     \
            cpm_error("ERROR: %s return fail!\n", __FUNCTION__);         \
            return (_res);                                      \
        }                                                       \
    }while(0)

#define CHECK_CORE_ADJUST_EN(_ret)                                   \
    do{                                                         \
        if (core_adjust_disable) {                                     \
            cpm_error("%s core regulater and clk adjusting is disabled!\n", __FUNCTION__); \
            return (_ret);                       \
        }                                                       \
    }while(0)

#define GET_PARAM0(_params)         (((unsigned int*)_params)[0])
#define GET_PARAM1(_params)         (((unsigned int*)_params)[1])
#define GET_PARAM2(_params)         (((unsigned int*)_params)[2])
#define GET_PARAM3(_params)         (((unsigned int*)_params)[3])
/*******************************************************************************
  Module Variable
  */

static struct file_operations cpm_ops = {
	.open = cpm_driver_open,
	.release = cpm_driver_release,
	.unlocked_ioctl = cpm_driver_ioctl,
#ifdef HAVE_COMPAT_IOCTL
	.compat_ioctl = cpm_driver_ioctl,
#endif
	.owner = THIS_MODULE,
};

struct cpm_device_t cpm_dev = {
	.dev_name = CPM_DEVICE_NAME,
	.minor = CPM_MINOR,
	.fops = &cpm_ops,
};

struct regulator *vcore_reg = NULL;

static const TEEC_UUID TACpm_UUID = TA_CPM_UUID;

static cpm_ca_t cpm_ca;

static cpm_timer_t cpm_timer;

static cpm_update_t cpm_update;

static cpm_temp_t cpm_temp;
static unsigned int cpm_overheat = false;

static struct cpm_core_voltage_tbl core_voltage_tbl[LEAKAGE_GROUP_NUM];
static unsigned int core_voltage_tbl_loaded = 0;
static unsigned int core_adjust_disable = 0;
static unsigned int core_adjust_ignore = 0;

static CPM_GFX_CALLBACK gfxcallback;
static CPM_GFX_LOADING gfxloading;

static unsigned int leakage = 0;
static unsigned int group = 0;
static unsigned int core_voltage = 0;
static unsigned int core_voltage_status = CPM_CORE_VOLTAGE_HIGH;

static struct cpm_core_mod_spec *request_module = NULL;

static DEFINE_MUTEX(cpm_mutex);
static DEFINE_MUTEX(cpm_request_mutex);
static DEFINE_SEMAPHORE(cpm_timer_sem);
static DEFINE_SEMAPHORE(cpm_update_sem);
static DEFINE_SEMAPHORE(cpm_request_sem);
/*******************************************************************************/
static int cpm_read_leakage_id(void)
{
	unsigned int leakageID;
	TEEC_Result result;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	mutex_lock(&cpm_mutex);

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE, TEEC_VALUE_OUTPUT);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_READ_LEAKAGE_ID, &cpm_ca.op, NULL);

	leakageID = cpm_ca.op.params[3].value.a;

	mutex_unlock(&cpm_mutex);

	CHECK_RESULT(result);

	leakage = leakageID;

	return result;
}

static int cpm_init_core_state(void)
{
	unsigned int i, result;

	result = 0;

	if (cur_core_tbl != NULL) {
		if (core_voltage_tbl_loaded) {
			for (i = 0; i < LEAKAGE_GROUP_NUM; i++) {
				if (leakage <= core_voltage_tbl[i].leakage) {
					break;
				}
			}
			if (i == LEAKAGE_GROUP_NUM) {
				group = LEAKAGE_GROUP_NUM - 1;	//select the last one
			} else {
				group = i;
			}
			if ((core_voltage_tbl[group].low_voltage == 0)
			    || (core_voltage_tbl[group].middle_voltage == 0)
			    || (core_voltage_tbl[group].high_voltage == 0)) {
				core_adjust_disable = 1;
			}
		} else {
			core_adjust_disable = 1;
		}

		cpm_get_core_regulator();

		result = cpm_init_core_status();

		gfxloading.gfx3d = cpm_get_core_module_by_name("GFX3");
	}
	return result;
}

static int cpm_set_startup_cfg(unsigned int *cfg)
{
	TEEC_Result result;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	if (!cfg || ((cfg[0] + sizeof(unsigned int)) >= cpm_ca.shm_in.size)) {
		cpm_info("%s BAD PARAMETER!\n", __FUNCTION__);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	mutex_lock(&cpm_mutex);

	memcpy(cpm_ca.shm_in.buffer, cfg, cfg[0] + sizeof(unsigned int));

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
						TEEC_NONE,
						TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_SET_STARTUP_CFG, &cpm_ca.op, NULL);

	mutex_unlock(&cpm_mutex);

	return result;
}

static int cpm_cp_control(unsigned int *control)
{
	TEEC_Result result;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(control);

	mutex_lock(&cpm_mutex);

	cpm_ca.op.params[2].value.a = control[0];
	cpm_ca.op.params[2].value.b = control[1];
	cpm_ca.op.params[3].value.a = control[2];
	cpm_ca.op.params[3].value.b = control[3];

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_NONE,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_CP_CONTROL, &cpm_ca.op, NULL);

	mutex_unlock(&cpm_mutex);

	return result;
}

static int cpm_get_cp_mod_num(unsigned int *num)
{
	TEEC_Result result;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(num);

	mutex_lock(&cpm_mutex);

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE, TEEC_VALUE_OUTPUT);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_MOD_NUM, &cpm_ca.op, NULL);

	*num = cpm_ca.op.params[3].value.a;

	mutex_unlock(&cpm_mutex);

	return result;
}

static int cpm_get_cp_mod_name(char *name)
{
	TEEC_Result result;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(name);

	mutex_lock(&cpm_mutex);

	cpm_ca.op.params[2].value.a = GET_PARAM0(name);

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_MEMREF_PARTIAL_OUTPUT,
						TEEC_VALUE_INPUT, TEEC_NONE);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_MOD_NAME, &cpm_ca.op, NULL);
	strcpy(name, (char *)cpm_ca.shm_out.buffer);

	mutex_unlock(&cpm_mutex);

	return result;
}

static int cpm_get_cp_mod_status(unsigned int *status)
{
	TEEC_Result result;
	unsigned int len;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(status);

	len = GET_PARAM1(status);

	mutex_lock(&cpm_mutex);

	cpm_ca.op.params[2].value.a = GET_PARAM0(status);

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_MEMREF_PARTIAL_OUTPUT,
						TEEC_VALUE_INPUT, TEEC_NONE);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_MOD_STAT, &cpm_ca.op, NULL);
	memcpy(status, (unsigned int *)cpm_ca.shm_out.buffer, len);

	mutex_unlock(&cpm_mutex);

	return result;
}

static int cpm_read_cp_mod(struct cpm_core_power_save_module *status)
{
	TEEC_Result result;
	TEEC_Operation operation;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(status);

	mutex_lock(&cpm_mutex);

	strncpy((char *)&operation.params[0].value.a, status->name, 4);
	/* clear result */
	operation.params[3].value.a = 0xdeadbeef;

	operation.started = 1;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT,
						TEEC_NONE, TEEC_VALUE_OUTPUT);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_READ_CP_MOD, &operation, NULL);

	mutex_unlock(&cpm_mutex);

	if (result == 0) {
		if (operation.params[3].value.a == 0) {
			status->clk = operation.params[1].value.a;
			status->pwr = operation.params[1].value.b;
		}
		return operation.params[3].value.a;
	}
	return result;
}

static int cpm_write_cp_mod(struct cpm_core_power_save_module *ctrl,
			    unsigned int type)
{
	TEEC_Result result;
	TEEC_Operation operation;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(ctrl);

	mutex_lock(&cpm_mutex);

	strncpy((char *)&operation.params[0].value.a, ctrl->name, 4);
	operation.params[1].value.a = ctrl->clk;
	operation.params[1].value.b = ctrl->pwr;
	operation.params[2].value.a = type;
	/* clear result */
	operation.params[3].value.a = 0xdeadbeef;

	operation.started = 1;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_OUTPUT);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_WRITE_CP_MOD, &operation, NULL);

	mutex_unlock(&cpm_mutex);

	if (result == 0) {
		return operation.params[3].value.a;
	}
	return result;
}

int cpm_cp_mod_save(unsigned int mode)
{
	int result;
	unsigned int i, num;
	struct cpm_core_power_save_module *module;

	CHECK_CORE_TABLE(0);

	if (mode == CPM_ACTIVE_STANDBY_MODE) {
		num = cur_core_tbl->num_act_sb_modules;
		module = cur_core_tbl->act_sb_mod;
	} else if (mode == CPM_LOW_POWER_MODE) {
		num = cur_core_tbl->num_low_power_modules;
		module = cur_core_tbl->low_power_mod;
	} else {
		num = 0;
		module = NULL;
	}
	for (i = 0; i < num; i++) {
		result = cpm_read_cp_mod(&module[i]);
		CHECK_RESULT(result);
	}

	for (i = 0; i < cur_core_tbl->num_modules; i++) {
		cur_core_tbl->mod_spec[i].saved_req =
		    cur_core_tbl->mod_spec[i].request;
	}
	return 0;
}

int cpm_cp_mod_disable(unsigned int mode, unsigned int type)
{
	int result;
	unsigned int i, num;
	struct cpm_core_power_save_module ctrl;
	struct cpm_core_power_save_module *module;

	CHECK_CORE_TABLE(0);

	if (mode == CPM_ACTIVE_STANDBY_MODE) {
		num = cur_core_tbl->num_act_sb_modules;
		module = cur_core_tbl->act_sb_mod;
	} else if (mode == CPM_LOW_POWER_MODE) {
		num = cur_core_tbl->num_low_power_modules;
		module = cur_core_tbl->low_power_mod;
	} else {
		num = 0;
		module = NULL;
	}
	for (i = 0; i < num; i++) {
		strncpy((char *)&ctrl.name, module[i].name, MAX_MOD_NAME_LEN);
		ctrl.clk = 0;
		ctrl.pwr = 0;
		result = cpm_write_cp_mod(&ctrl, type);
		CHECK_RESULT(result);
	}

	for (i = 0; i < cur_core_tbl->num_modules; i++) {
		cur_core_tbl->mod_spec[i].request = CPM_REQUEST_CORE_LOW;
	}
	core_adjust_ignore = true;
	up(&cpm_update_sem);
	return 0;
}

int cpm_cp_mod_restore(unsigned int mode, unsigned int type)
{
	int result;
	unsigned int i, num;
	struct cpm_core_power_save_module *module;

	CHECK_CORE_TABLE(0);

	if (mode == CPM_ACTIVE_STANDBY_MODE) {
		num = cur_core_tbl->num_act_sb_modules;
		module = cur_core_tbl->act_sb_mod;
	} else if (mode == CPM_LOW_POWER_MODE) {
		num = cur_core_tbl->num_low_power_modules;
		module = cur_core_tbl->low_power_mod;
	} else {
		num = 0;
		module = NULL;
	}
	for (i = 0; i < num; i++) {
		result = cpm_write_cp_mod(&module[num - 1 - i], type);
		CHECK_RESULT(result);
	}

	for (i = 0; i < cur_core_tbl->num_modules; i++) {
		cur_core_tbl->mod_spec[i].request =
		    cur_core_tbl->mod_spec[i].saved_req;
	}
	if (gfxloading.gfx3d) {
		gfxloading.gfx3d->request = CPM_REQUEST_CORE_LOW;
	}
	core_adjust_ignore = false;
	up(&cpm_update_sem);
	return 0;
}

static int cpm_get_core_mod_num(void)
{
	CHECK_CORE_TABLE(0);
	CHECK_CORE_REGULATOR(0);
	CHECK_TEE_INIT(0);

	return cur_core_tbl->num_modules;
}

static int cpm_get_core_mod_status(int id, struct cpm_core_mod_spec *mod_stat)
{

	CHECK_CORE_TABLE(-EFAULT);

	CHECK_VALID_POINTER(mod_stat);

	memcpy(mod_stat, &cur_core_tbl->mod_spec[id],
	       sizeof(struct cpm_core_mod_spec));

	return 0;
}

static int cpm_get_core_clk_num(unsigned int *num)
{
	TEEC_Result result;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(num);

	mutex_lock(&cpm_mutex);

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT,
						TEEC_NONE,
						TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_CORE_CLK_NUM, &cpm_ca.op, NULL);

	*num = cpm_ca.op.params[0].value.a;

	mutex_unlock(&cpm_mutex);

	return result;
}

static int cpm_get_core_clk_freq(unsigned int *status)
{
	TEEC_Result result;
	unsigned int len;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(status);

	len = GET_PARAM1(status);

	mutex_lock(&cpm_mutex);

	cpm_ca.op.params[0].value.a = GET_PARAM0(status);

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_MEMREF_PARTIAL_OUTPUT,
						TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_CORE_CLK_FREQ, &cpm_ca.op, NULL);
	memcpy(status, (unsigned int *)cpm_ca.shm_out.buffer, len);

	mutex_unlock(&cpm_mutex);

	return result;
}

static int cpm_get_core_status(unsigned int *status)
{
	CHECK_VALID_POINTER(status);

	status[0] = leakage;
	status[1] = group;
	status[2] = core_voltage / 1000;
	status[3] = core_voltage_status;
	status[4] = cpm_temp.temp;

	return 0;
}

static void cpm_get_core_regulator(void)
{
	struct regulator *vcore;

	vcore = regulator_get(NULL, "vcore");
	if (IS_ERR(vcore)) {
		cpm_info("%s: failed to get vcore regulator\n", __func__);
		return;
	}
	vcore_reg = vcore;
	core_voltage = regulator_get_voltage(vcore_reg);
}

static int cpm_get_clk_num(unsigned int *num)
{
	TEEC_Result result;
	TEEC_Operation operation;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(num);

	mutex_lock(&cpm_mutex);

	/* clear result */
	operation.params[3].value.a = 0xdeadbeef;

	operation.started = 1;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE, TEEC_VALUE_OUTPUT);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_CLK_NUM, &operation, NULL);

	mutex_unlock(&cpm_mutex);

	if (result == 0) {
		*num = operation.params[3].value.a;
	}

	return result;
}

static int cpm_get_clk_status(char *status)
{
	TEEC_Result result;
	TEEC_Operation operation;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(status);

	mutex_lock(&cpm_mutex);

	memset(cpm_ca.shm_out.buffer, 0, cpm_ca.shm_out.size);
	operation.params[1].memref.parent = &cpm_ca.shm_out;
	operation.params[1].memref.size = cpm_ca.shm_out.size;
	operation.params[1].memref.offset = 0;

	operation.params[2].value.a = GET_PARAM0(status);

	operation.started = 1;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_MEMREF_PARTIAL_OUTPUT,
						TEEC_VALUE_INPUT, TEEC_NONE);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_CLK_STAT, &operation, NULL);

	mutex_unlock(&cpm_mutex);

	if (result == 0) {
		strcpy(status, (char *)cpm_ca.shm_out.buffer);
	}

	return result;
}

static int cpm_get_temp(int *temp)
{
	TEEC_Result result;
	TEEC_Operation operation;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	CHECK_VALID_POINTER(temp);

	mutex_lock(&cpm_mutex);

	operation.params[3].value.a = 0;
	operation.started = 1;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE, TEEC_VALUE_OUTPUT);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_TEMP, &operation, NULL);

	if (result == 0) {
		*temp = operation.params[3].value.a;
	}

	mutex_unlock(&cpm_mutex);

	return result;
}

static int cpm_update_core_voltage(unsigned int request)
{
	int voltage, res, i;

	CHECK_CORE_TABLE(-EFAULT);
	CHECK_CORE_REGULATOR(-EFAULT);
	CHECK_CORE_ADJUST_EN(0);

	if (request == core_voltage_status) {
		return 0;
	}

	cpm_error("set core voltage to %s\n", request ?
		  request == CPM_CORE_VOLTAGE_MIDDLE ? "M" : "H" : "L");

	mutex_lock(&cpm_mutex);

	if (request == CPM_CORE_VOLTAGE_LOW) {
		voltage = core_voltage_tbl[group].low_voltage;
	} else if (request == CPM_CORE_VOLTAGE_MIDDLE) {
		voltage = core_voltage_tbl[group].middle_voltage;
	} else {
		voltage = core_voltage_tbl[group].high_voltage;
	}

	voltage *= 1000;

	for (i = 0; i < REGULATOR_ADJ_CNT_MAX; i++) {
		if (core_voltage == voltage) {
			break;
		}
		if (core_voltage > voltage) {
			core_voltage -= REGULATOR_STEP;
		} else {
			core_voltage += REGULATOR_STEP;
		}
		res =
		    regulator_set_voltage(vcore_reg, core_voltage,
					  core_voltage);
		if (res != 0) {
			break;
		}
	}
	mutex_unlock(&cpm_mutex);
	if (core_voltage != voltage) {
		cpm_info("Set Core regulator failed!\n");
	}

	if (res == 0) {
		core_voltage_status = request;
	}
	return res;
}

static int cpm_update_core_clk(struct cpm_core_mod_spec *module)
{
	TEEC_Result result;
	int res;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);
	CHECK_CORE_ADJUST_EN(0);

	if (module->status == module->request) {
		return 0;
	}

	cpm_error("set %s clock to %s\n", module->name,
		  module->request ? "H" : "L");

	if (module->chg_gfx == CPM_CHANGE_GFX2_CLK_SRC) {
		if (gfxcallback.before_change != NULL) {
			res =
			    gfxcallback.before_change(gfxcallback.gfx,
						      CPM_GFX2D_CALLBACK);
			CHECK_RESULT(res);
		} else {
			cpm_info("No GFX2 before change callback");
		}
	}
	if (module->chg_gfx == CPM_CHANGE_GFX3_CLK_SRC) {
		if (gfxcallback.before_change != NULL) {
			res =
			    gfxcallback.before_change(gfxcallback.gfx,
						      CPM_GFX3D_CALLBACK);
			CHECK_RESULT(res);
		} else {
			cpm_info("No GFX3 before change callback");
		}
	}

	mutex_lock(&cpm_mutex);

	strncpy((char *)&cpm_ca.op.params[2].value.a, module->name, 4);
	cpm_ca.op.params[3].value.a = module->request;

	cpm_ca.op.started = 1;
	cpm_ca.op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_NONE,
						TEEC_VALUE_INPUT,
						TEEC_VALUE_INPUT);

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_CORE_CONTROL, &cpm_ca.op, NULL);

	mutex_unlock(&cpm_mutex);

	if (result == 0) {
		module->status = module->request;
	}

	if (module->chg_gfx == CPM_CHANGE_GFX2_CLK_SRC) {
		if (gfxcallback.after_change != NULL) {
			res =
			    gfxcallback.after_change(gfxcallback.gfx,
						     CPM_GFX2D_CALLBACK);
			CHECK_RESULT(res);
		} else {
			cpm_info("No GFX2 after change callback");
		}
	}
	if (module->chg_gfx == CPM_CHANGE_GFX3_CLK_SRC) {
		if (gfxcallback.after_change != NULL) {
			res =
			    gfxcallback.after_change(gfxcallback.gfx,
						     CPM_GFX3D_CALLBACK);
			CHECK_RESULT(res);
		} else {
			cpm_info("No GFX3 after change callback");
		}
	}
	return result;
}

static int cpm_update_core_clk_value(struct cpm_core_mod_spec *module)
{
	TEEC_Result result;
	TEEC_Operation operation;

	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	mutex_lock(&cpm_mutex);

	operation.params[0].value.a = strlen(module->clkname);
	memset(cpm_ca.shm_in.buffer, 0, cpm_ca.shm_in.size);
	strncpy((char *)cpm_ca.shm_in.buffer, module->clkname,
		operation.params[0].value.a);

	operation.params[1].memref.parent = &cpm_ca.shm_in;
	operation.params[1].memref.size = cpm_ca.shm_in.size;
	operation.params[1].memref.offset = 0;

	/* clear result */
	operation.params[3].value.b = 0xdeadbeef;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
						TEEC_MEMREF_PARTIAL_INPUT,
						TEEC_NONE, TEEC_VALUE_OUTPUT);
	operation.started = 1;

	result = TEEC_InvokeCommand(&cpm_ca.session,
				    CPM_GET_MODULE_CLK_FREQ, &operation, NULL);

	mutex_unlock(&cpm_mutex);

	if (result == 0) {
		*module->clkfreq = operation.params[3].value.a;
	}

	return result;
}

static int cpm_get_core_voltage_request(void)
{
	unsigned int i;
	unsigned int request = CPM_CORE_VOLTAGE_LOW;

	for (i = 0; i < cur_core_tbl->num_modules; i++) {
		if (cur_core_tbl->mod_spec[i].ignore == CPM_REQUEST_IGNORE_OFF) {
			if (cur_core_tbl->mod_spec[i].request !=
			    CPM_REQUEST_CORE_LOW) {
				if (strcmp
				    ("GFX2",
				     cur_core_tbl->mod_spec[i].name) == 0) {
					continue;
				}
#ifdef GFX3D_V4G_CHECK
				if (strcmp
				    ("V4G",
				     cur_core_tbl->mod_spec[i].name) == 0) {
					request = CPM_CORE_VOLTAGE_MIDDLE;
					break;
				}
#endif
				if (strcmp
				    ("GFX3",
				     cur_core_tbl->mod_spec[i].name) == 0) {
					request = CPM_CORE_VOLTAGE_HIGH;
					break;
				} else {
					request = CPM_CORE_VOLTAGE_MIDDLE;
				}
			}
		}
	}
	return request;
}

static int cpm_check_core_request(void)
{
	unsigned int request, res, i;

	CHECK_CORE_TABLE(-EFAULT);
	CHECK_CORE_REGULATOR(-EFAULT);
	CHECK_TEE_INIT(-EFAULT);
	request = cpm_get_core_voltage_request();

	if (request > core_voltage_status) {
		res = cpm_update_core_voltage(request);
		CHECK_RESULT(res);
		for (i = 0; i < cur_core_tbl->num_modules; i++) {
			if (cur_core_tbl->mod_spec[i].ignore ==
			    CPM_REQUEST_IGNORE_OFF) {
				res =
				    cpm_update_core_clk(&cur_core_tbl->
							mod_spec[i]);
				CHECK_RESULT(res);
				cpm_update_core_clk_value(&cur_core_tbl->
							  mod_spec[i]);
			}
		}
	} else {
		for (i = 0; i < cur_core_tbl->num_modules; i++) {
			if (cur_core_tbl->mod_spec[i].ignore ==
			    CPM_REQUEST_IGNORE_OFF) {
				res =
				    cpm_update_core_clk(&cur_core_tbl->
							mod_spec[i]);
				CHECK_RESULT(res);
				cpm_update_core_clk_value(&cur_core_tbl->
							  mod_spec[i]);
			}
		}
		res = cpm_update_core_voltage(request);
		CHECK_RESULT(res);
	}

	return 0;
}

static struct cpm_core_mod_spec *cpm_get_core_module_by_name(const char *name)
{
	unsigned int i;

	CHECK_CORE_TABLE(NULL);

	for (i = 0; i < cur_core_tbl->num_modules; i++) {
		if (strcmp(name, cur_core_tbl->mod_spec[i].name) == 0) {
			return &cur_core_tbl->mod_spec[i];
		}
	}

	return NULL;
}

int cpm_set_core_mode(char *name, unsigned int request)
{
	struct cpm_core_mod_spec *module = NULL;

	/* return OK and do nothing before init or not support */
	CHECK_CORE_TABLE(0);
	CHECK_CORE_REGULATOR(0);
	CHECK_TEE_INIT(0);

	if ((name == NULL) || (request > CPM_REQUEST_MAX)) {
		return -EFAULT;
	}

	if (core_adjust_ignore == true) {
		return 0;
	}

	mutex_lock(&cpm_request_mutex);

	if (request == CPM_REQUEST_MAX) {
		request = CPM_REQUEST_CORE_HIGH;
	}

	name[4] = 0;

	module = cpm_get_core_module_by_name(name);

#ifdef GFX3D_V4G_CHECK
	if ((strcmp(name, "V4G") == 0)
	    && (request == CPM_REQUEST_CORE_HIGH)
	    && (gfxloading.gfx3d != NULL)) {
		gfxloading.gfx3d->request = CPM_REQUEST_CORE_LOW;
	}
#endif

	if (module) {
		cpm_info("%s(%s, request core to %s\n)", __FUNCTION__,
			 name, request ? "H" : "L");

		if (module->request != request) {
			module->request = request;
			request_module = module;
			up(&cpm_update_sem);
			down_interruptible(&cpm_request_sem);
		}
		mutex_unlock(&cpm_request_mutex);
		if (module->request != module->status) {
			return -EFAULT;
		}
		return 0;
	}
	cpm_info("%s Module not found! [%s]", __FUNCTION__, name);

	mutex_unlock(&cpm_request_mutex);
	return -EFAULT;
}

EXPORT_SYMBOL(cpm_set_core_mode);

void cpm_register_gfx_callback(CPM_GFX_CALLBACK * callback)
{
	gfxcallback.gfx = callback->gfx;
	gfxcallback.before_change = callback->before_change;
	gfxcallback.after_change = callback->after_change;
}

EXPORT_SYMBOL(cpm_register_gfx_callback);

void cpm_unregister_gfx_callback(void)
{
	gfxcallback.gfx = NULL;
	gfxcallback.before_change = NULL;
	gfxcallback.after_change = NULL;
}

EXPORT_SYMBOL(cpm_unregister_gfx_callback);

int cpm_set_gfx3d_loading(unsigned int loading)
{
#ifdef GFX3D_V4G_CHECK
	struct cpm_core_mod_spec *v4g;
#endif

	/* return OK and do nothing before init or not support */
	if ((!cur_core_tbl) || (!vcore_reg) || (!cpm_ca.inited)) {
		return 0;
	}

	if (gfxloading.gfx3d == NULL) {
		return 0;
	}

	if ((loading == GFX_CLK_GATTING_VAL)
	    || (cpm_overheat == true)
	    || (core_adjust_ignore == true)) {
		return 0;
	}
#ifdef GFX3D_V4G_CHECK
	v4g = cpm_get_core_module_by_name("V4G");
	if (v4g->request == CPM_REQUEST_CORE_HIGH)
		return 0;
#endif

	if (loading) {
		cpm_reset_timer();
		if (gfxloading.gfx3d->request == CPM_REQUEST_CORE_LOW) {
			gfxloading.gfx3d->request = CPM_REQUEST_CORE_HIGH;
			up(&cpm_update_sem);
		}
	}
	return 0;
}

EXPORT_SYMBOL(cpm_set_gfx3d_loading);

static int cpm_freeze_gfx(unsigned int freeze)
{
	int res = 0;

	if (freeze) {
		if (gfxcallback.before_change != NULL) {
			res =
			    gfxcallback.before_change(gfxcallback.gfx,
						      CPM_GFX2D_CALLBACK);
			CHECK_RESULT(res);
			res =
			    gfxcallback.before_change(gfxcallback.gfx,
						      CPM_GFX3D_CALLBACK);
			CHECK_RESULT(res);
		} else {
			cpm_info("No GFX before change callback");
		}
	} else {
		if (gfxcallback.after_change != NULL) {
			res =
			    gfxcallback.after_change(gfxcallback.gfx,
						     CPM_GFX2D_CALLBACK);
			CHECK_RESULT(res);
			res =
			    gfxcallback.after_change(gfxcallback.gfx,
						     CPM_GFX3D_CALLBACK);
			CHECK_RESULT(res);
		} else {
			cpm_info("No GFX after change callback");
		}
	}
	return res;
}

static int cpm_ignore_request(char *name, unsigned int ignore)
{
	struct cpm_core_mod_spec *module = NULL;

	if ((name == NULL) || (ignore >= CPM_REQUEST_IGNORE_MAX)) {
		return -EFAULT;
	}

	name[4] = 0;

	module = cpm_get_core_module_by_name(name);

	if (module) {

		module->ignore = ignore;
		up(&cpm_update_sem);
		return 0;
	}

	cpm_info("%s Module not found! [%s]", __FUNCTION__, name);
	return -EFAULT;
}

static int cpm_init_core_status(void)
{
	unsigned int i, res, request;

	CHECK_CORE_TABLE(-EFAULT);
	CHECK_CORE_REGULATOR(-EFAULT);
	CHECK_TEE_INIT(TEEC_ERROR_BAD_STATE);

	/* bsp set core voltage to high */
	for (i = 0; i < cur_core_tbl->num_modules; i++) {
		if (cur_core_tbl->mod_spec[i].ignore == CPM_REQUEST_IGNORE_OFF) {
			res = cpm_update_core_clk(&cur_core_tbl->mod_spec[i]);
			CHECK_RESULT(res);
			cpm_update_core_clk_value(&cur_core_tbl->mod_spec[i]);
		}
	}

	request = cpm_get_core_voltage_request();
	res = cpm_update_core_voltage(request);
	CHECK_RESULT(res);
	return 0;
}

static int cpm_timeout_task(void *unused)
{
	int rc;
	while (!kthread_should_stop()) {
		rc = down_interruptible(&cpm_timer_sem);
		if (rc < 0) {
			continue;
		}
		if (gfxloading.gfx3d != NULL) {
			gfxloading.gfx3d->request = CPM_REQUEST_CORE_LOW;
			up(&cpm_update_sem);
		}
	}
	cpm_timer.res_alloc &= ~CPM_RES_TIMIER_TASK;
	return 0;
}

static void cpm_timeout_call(unsigned long cpm)
{
	up(&cpm_timer_sem);
}

static int cpm_reset_timer(void)
{
	mod_timer(&cpm_timer.cpm_timer, jiffies + GFX_LOW_TIME_OUT);
	return 0;
}

static int cpm_update_task(void *unused)
{
	int rc;
	while (!kthread_should_stop()) {
		rc = down_interruptible(&cpm_update_sem);
		if (rc < 0) {
			continue;
		}

		rc = cpm_check_core_request();

		if (request_module) {
			if ((rc != 0)
			    || (request_module->ignore == CPM_REQUEST_IGNORE_ON)
			    || (request_module->request ==
				request_module->status)) {
				request_module = NULL;
				up(&cpm_request_sem);
			}
		}
	}
	cpm_update.taskrunning = 0;
	return 0;
}

static int cpm_temp_task(void *unused)
{
	int temp, res;
	while (!kthread_should_stop()) {
		res = cpm_get_temp(&temp);
		if (res == 0) {
			cpm_temp.temp = temp;
			if ((cpm_overheat == true) && (temp < TEMP_LOW_THR)) {
				cpm_overheat = false;
			}
			if ((cpm_overheat == false) && (temp > TEMP_UP_THR)) {
				cpm_overheat = true;
			}
		}
		msleep_interruptible(TEMP_CHECK_CYCLE);
	}
	cpm_temp.taskrunning = 0;
	return 0;
}

static void cpm_tee_finalize(void)
{
	if (!cpm_ca.inited) {
		cpm_info("CPM has not been initialized!\n");
		return;
	}
	cpm_ca.inited = 0;

	if (cpm_ca.res_alloc & CPM_RES_SHM_IN) {
		TEEC_ReleaseSharedMemory(&cpm_ca.shm_in);
	}

	if (cpm_ca.res_alloc & CPM_RES_SHM_OUT) {
		TEEC_ReleaseSharedMemory(&cpm_ca.shm_out);
	}

	if (cpm_ca.res_alloc & CPM_RES_SESSION) {
		TEEC_CloseSession(&cpm_ca.session);
	}

	if (cpm_ca.res_alloc & CPM_RES_CTX) {
		TEEC_FinalizeContext(&cpm_ca.ctx);
	}

	cpm_ca.res_alloc = 0;

}

static TEEC_Result cpm_tee_initialize(void)
{
	TEEC_Result result;

	if (cpm_ca.inited) {
		cpm_info("CPM ca has already been initialized!\n");
		return TEEC_SUCCESS;
	}

	cpm_ca.inited = 1;

	/* ========================================================================
	   TA need be loaded before here
	   [1] Connect to TEE
	   ======================================================================== */
	result = TEEC_InitializeContext(NULL, &cpm_ca.ctx);
	if (result != TEEC_SUCCESS) {
		cpm_info("TEEC_InitializeContext ret=0x%08x\n", result);
		goto _err_out;
	}
	cpm_ca.res_alloc |= CPM_RES_CTX;

	/* ========================================================================
	   [2] Open session with TEE application
	   ======================================================================== */
	/* Open a Session with the TEE application. */
	result = TEEC_OpenSession(&cpm_ca.ctx, &cpm_ca.session, &TACpm_UUID, TEEC_LOGIN_USER, NULL,	/* No connection data needed for TEEC_LOGIN_USER. */
				  NULL,	/* No payload, and do not want cancellation. */
				  NULL);
	if (result != TEEC_SUCCESS) {
		cpm_info("TEEC_OpenSession ret=0x%08x\n", result);
		goto _err_out;
	}
	cpm_ca.res_alloc |= CPM_RES_SESSION;

	/* ========================================================================
	   [2] Create default input/output buffers for TZ API calls
	   ======================================================================== */
	cpm_ca.shm_in.size = CPM_IN_SHM_SIZE;
	cpm_ca.shm_in.flags = TEEC_MEM_INPUT;
	result = TEEC_AllocateSharedMemory(&cpm_ca.ctx, &cpm_ca.shm_in);
	if (result != TEEC_SUCCESS) {
		cpm_info("AllocateShareMemory failed\n");
		goto _err_out;
	}
	cpm_ca.res_alloc |= CPM_RES_SHM_IN;

	cpm_ca.shm_out.size = CPM_OUT_SHM_SIZE;
	cpm_ca.shm_out.flags = TEEC_MEM_OUTPUT;
	result = TEEC_AllocateSharedMemory(&cpm_ca.ctx, &cpm_ca.shm_out);
	if (result != TEEC_SUCCESS) {
		cpm_info("AllocateShareMemory failed\n");
		goto _err_out;
	}
	cpm_ca.res_alloc |= CPM_RES_SHM_OUT;

	cpm_ca.op.params[0].memref.parent = &cpm_ca.shm_in;
	cpm_ca.op.params[0].memref.size = cpm_ca.shm_in.size;
	cpm_ca.op.params[0].memref.offset = 0;

	cpm_ca.op.params[1].memref.parent = &cpm_ca.shm_out;
	cpm_ca.op.params[1].memref.size = cpm_ca.shm_out.size;
	cpm_ca.op.params[1].memref.offset = 0;

	return TEEC_SUCCESS;
_err_out:
	cpm_tee_finalize();
	cpm_info("cpm_tee_initialize fail\n");
	return result;
}

static void cpm_timer_finalize(void)
{
	if (!cpm_timer.inited) {
		cpm_info("CPM timer has not been initialized!\n");
		return;
	}
	cpm_timer.inited = 0;
	if (cpm_timer.res_alloc & CPM_RES_TIMIER) {
		del_timer(&cpm_timer.cpm_timer);
	}
	if (cpm_timer.res_alloc & CPM_RES_TIMIER_TASK) {
		if (cpm_timer.cpm_timer_handle != NULL) {
			kthread_stop(cpm_timer.cpm_timer_handle);
			cpm_timer.cpm_timer_handle = NULL;
		}
	}
	cpm_timer.res_alloc = 0;
}

static int cpm_timer_initialize(void)
{
	if (cpm_timer.inited) {
		cpm_info("CPM timer has already been initialized!\n");
		return 0;
	}

	cpm_timer.inited = 1;
	init_timer(&cpm_timer.cpm_timer);
	cpm_timer.cpm_timer.function = cpm_timeout_call;
	cpm_timer.res_alloc |= CPM_RES_TIMIER;

	sema_init(&cpm_timer_sem, 0);

	cpm_timer.cpm_timer_handle = kthread_create(cpm_timeout_task,
						    NULL, "CPM Timer");
	if (IS_ERR(cpm_timer.cpm_timer_handle)) {
		cpm_info("cpm create timer task failed\n");
		goto _err_out;
	}
	wake_up_process(cpm_timer.cpm_timer_handle);
	cpm_timer.res_alloc |= CPM_RES_TIMIER_TASK;
	return 0;
_err_out:
	cpm_timer_finalize();
	return -EFAULT;
}

static void cpm_update_finalize(void)
{
	if (cpm_update.taskrunning != 1) {
		cpm_info("CPM update task is not running!\n");
		return;
	}

	if (cpm_update.cpm_update_handle != NULL) {
		kthread_stop(cpm_update.cpm_update_handle);
		cpm_update.cpm_update_handle = NULL;
		cpm_update.taskrunning = 0;
	}
}

static int cpm_update_initialize(void)
{
	if (cpm_update.taskrunning == 1) {
		cpm_info("CPM update task is already running!\n");
		return 0;
	}

	cpm_update.cpm_update_handle = kthread_create(cpm_update_task,
						      NULL, "CPM Update");
	if (IS_ERR(cpm_update.cpm_update_handle)) {
		cpm_info("cpm create update task failed\n");
		return -EFAULT;
	}
	wake_up_process(cpm_update.cpm_update_handle);
	cpm_update.taskrunning = 1;
	return 0;
}

static void cpm_temp_finalize(void)
{
	if (cpm_temp.taskrunning != 1) {
		cpm_info("CPM temperature task is not running!\n");
		return;
	}

	if (cpm_temp.cpm_temp_handle != NULL) {
		kthread_stop(cpm_temp.cpm_temp_handle);
		cpm_temp.cpm_temp_handle = NULL;
		cpm_temp.taskrunning = 0;
	}
}

static int cpm_temp_initialize(void)
{
	if (cpm_temp.taskrunning == 1) {
		cpm_info("CPM temperature task is already running!\n");
		return 0;
	}

	cpm_temp.cpm_temp_handle = kthread_create(cpm_temp_task,
						  NULL, "CPM Temperature");
	if (IS_ERR(cpm_temp.cpm_temp_handle)) {
		cpm_info("cpm create temperature task failed\n");
		return -EFAULT;
	}
	wake_up_process(cpm_temp.cpm_temp_handle);
	cpm_temp.taskrunning = 1;
	return 0;
}

/*******************************************************************************
  Module API
  */
static int cpm_driver_open(struct inode *inode, struct file *filp)
{
	int result;

	result = cpm_tee_initialize();
	if (result != TEEC_SUCCESS) {
		cpm_info("cpm_tee_initialize failed\n");
		return result;
	}
	cpm_read_leakage_id();

	result = cpm_timer_initialize();
	if (result != 0) {
		cpm_info("cpm_timer_initialize failed\n");
		return result;
	}

	result = cpm_update_initialize();
	if (result != 0) {
		cpm_info("cpm_update_initialize failed\n");
		return result;
	}

	result = cpm_temp_initialize();
	if (result != 0) {
		cpm_info("cpm_temp_initialize failed\n");
		return result;
	}

	sema_init(&cpm_request_sem, 0);
	return 0;
}

static int cpm_driver_release(struct inode *inode, struct file *filp)
{
	cpm_tee_finalize();
	cpm_timer_finalize();
	cpm_update_finalize();
	cpm_temp_finalize();
	cpm_info("cpm_driver_release ok\n");

	return 0;
}

static long cpm_driver_ioctl(struct file *filp, unsigned int cmd,
			     unsigned long arg)
{
	switch (cmd) {
	case CPM_IOCTL_CMD_LOAD_CORE_VOLTAGE_TABLE:
		{
			unsigned int length;
			if (copy_from_user
			    (&length, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;
			if (length > VOLTAGE_TABLE_SIZE) {
				return -EFAULT;
			}
			if (copy_from_user
			    (core_voltage_tbl,
			     (void __user *)arg + sizeof(unsigned int),
			     length * sizeof(unsigned int))) {
				return -EFAULT;
			}
			core_voltage_tbl_loaded = 1;
			return 0;
		}

	case CPM_IOCTL_CMD_INIT_CORE_STATE:
		{
			return cpm_init_core_state();
		}

	case CPM_IOCTL_CMD_SET_STARTUP_CFG:
		{
			unsigned int cfg[cpm_ca.shm_in.size];

			if (copy_from_user
			    (cfg, (void __user *)arg, sizeof(unsigned int))) {
				return -EFAULT;
			}
			if (copy_from_user
			    (&cfg[1], (void __user *)arg + sizeof(unsigned int),
			     cfg[0])) {
				return -EFAULT;
			}
			return cpm_set_startup_cfg(cfg);
		}

	case CPM_IOCTL_CMD_CP_CONTROL:
		{
			unsigned int buf[4];

			if (copy_from_user
			    (buf, (void __user *)arg, 4 * sizeof(unsigned int)))
				return -EFAULT;
			return cpm_cp_control(buf);
		}

	case CPM_IOCTL_CMD_CP_MOD_SAVE:
		{
			return cpm_cp_mod_save(CPM_LOW_POWER_MODE);
		}

	case CPM_IOCTL_CMD_CP_MOD_DISABLE:
		{
			unsigned int type;
			if (copy_from_user
			    (&type, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;
			return cpm_cp_mod_disable(CPM_LOW_POWER_MODE, type);
		}

	case CPM_IOCTL_CMD_CP_MOD_RESTORE:
		{
			unsigned int type;
			if (copy_from_user
			    (&type, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;
			return cpm_cp_mod_restore(CPM_LOW_POWER_MODE, type);
		}

	case CPM_IOCTL_CMD_GET_CP_MOD_NUM:
		{
			unsigned int num, res;

			res = cpm_get_cp_mod_num(&num);

			CHECK_RESULT(res);

			if (copy_to_user
			    ((void __user *)arg, &num, sizeof(unsigned int))) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_GET_CP_MOD_NAME:
		{
			char name[MAX_MOD_NAME_LEN];
			unsigned int res;

			if (copy_from_user
			    (name, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;

			res = cpm_get_cp_mod_name(name);

			CHECK_RESULT(res);

			if (copy_to_user
			    ((void __user *)arg, name,
			     MAX_MOD_NAME_LEN * sizeof(char))) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_GET_CP_MOD_STATUS:
		{
			unsigned int status[cpm_ca.shm_in.size];
			unsigned int res, len;

			if (copy_from_user
			    (status, (void __user *)arg,
			     2 * sizeof(unsigned int)))
				return -EFAULT;

			len = GET_PARAM1(status);
			res = cpm_get_cp_mod_status(status);

			CHECK_RESULT(res);

			if (copy_to_user((void __user *)arg, status, len)) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_GET_CORE_MOD_NUM:
		{
			unsigned int num;

			num = cpm_get_core_mod_num();

			if (copy_to_user
			    ((void __user *)arg, &num, sizeof(unsigned int))) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_GET_CORE_MOD_STATUS:
		{
			unsigned int status[cpm_ca.shm_in.size];
			unsigned int len, res;

			if (copy_from_user
			    (status, (void __user *)arg,
			     sizeof(unsigned int) * 2))
				return -EFAULT;

			len = GET_PARAM1(status);
			res =
			    cpm_get_core_mod_status(GET_PARAM0(status),
						    (struct cpm_core_mod_spec *)
						    status);
			CHECK_RESULT(res);

			if (copy_to_user((void __user *)arg, status, len)) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_GET_CORE_CLK_NUM:
		{
			unsigned int num, res;

			res = cpm_get_core_clk_num(&num);

			CHECK_RESULT(res);

			if (copy_to_user
			    ((void __user *)arg, &num, sizeof(unsigned int))) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_GET_CORE_CLK_FREQ:
		{
			unsigned int status[cpm_ca.shm_in.size];
			unsigned int res, len;

			if (copy_from_user
			    (status, (void __user *)arg,
			     2 * sizeof(unsigned int)))
				return -EFAULT;

			len = GET_PARAM1(status);
			res = cpm_get_core_clk_freq(status);
			if (res) {
				return res;
			}

			if (copy_to_user((void __user *)arg, status, len)) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_GET_CORE_STATUS:
		{
			unsigned int status[cpm_ca.shm_in.size];
			unsigned int res, len;

			if (copy_from_user
			    (status, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;

			len = GET_PARAM0(status);
			res = cpm_get_core_status(status);

			CHECK_RESULT(res);

			if (copy_to_user((void __user *)arg, status, len)) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_REQUEST_CORE_LOW:
		{
			char name[MAX_MOD_NAME_LEN];
			if (copy_from_user
			    (name, (void __user *)arg,
			     MAX_MOD_NAME_LEN * sizeof(char))) {
				return -EFAULT;
			}
			name[MAX_MOD_NAME_LEN - 1] = 0;
			return cpm_set_core_mode(name, CPM_REQUEST_CORE_LOW);
		}

	case CPM_IOCTL_CMD_REQUEST_CORE_HIGH:
		{
			char name[MAX_MOD_NAME_LEN];
			if (copy_from_user
			    (name, (void __user *)arg,
			     MAX_MOD_NAME_LEN * sizeof(char))) {
				return -EFAULT;
			}
			name[MAX_MOD_NAME_LEN - 1] = 0;
			return cpm_set_core_mode(name, CPM_REQUEST_CORE_HIGH);
		}

	case CPM_IOCTL_CMD_REQUEST_IGNORE_OFF:
		{
			char name[MAX_MOD_NAME_LEN];
			if (copy_from_user
			    (name, (void __user *)arg,
			     MAX_MOD_NAME_LEN * sizeof(char))) {
				return -EFAULT;
			}

			return cpm_ignore_request(name, CPM_REQUEST_IGNORE_OFF);
		}

	case CPM_IOCTL_CMD_REQUEST_IGNORE_ON:
		{
			char name[MAX_MOD_NAME_LEN];
			if (copy_from_user
			    (name, (void __user *)arg,
			     MAX_MOD_NAME_LEN * sizeof(char))) {
				return -EFAULT;
			}

			return cpm_ignore_request(name, CPM_REQUEST_IGNORE_ON);
		}

	case CPM_IOCTL_CMD_GET_CLK_NUM:
		{
			unsigned int num, res;

			res = cpm_get_clk_num(&num);

			CHECK_RESULT(res);

			if (copy_to_user
			    ((void __user *)arg, &num, sizeof(unsigned int))) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_GET_CLK_STATUS:
		{
			char status[MAX_CLK_NAME_LEN];
			unsigned int res;

			if (copy_from_user
			    (status, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;

			res = cpm_get_clk_status(status);

			CHECK_RESULT(res);

			if (copy_to_user
			    ((void __user *)arg, status,
			     MAX_CLK_NAME_LEN * sizeof(char))) {
				return -EFAULT;
			}
			break;
		}

	case CPM_IOCTL_CMD_FREEZE_GFX:
		{
			unsigned int freeze;

			if (copy_from_user
			    (&freeze, (void __user *)arg,
			     sizeof(unsigned int))) {
				return -EFAULT;
			}

			return cpm_freeze_gfx(freeze);
		}

	default:
		return -EFAULT;
	}

	return 0;
}

/*******************************************************************************
  Module Register API
  */

int cpm_module_init(void)
{
	int res;
	dev_t pedev;

	cpm_unregister_gfx_callback();

	res = alloc_chrdev_region(&pedev, 0, CPM_MAX_DEVS, CPM_DEVICE_NAME);
	if (res < 0) {
		cpm_info("alloc_chrdev_region() failed for cpm\n");
		goto err_reg_device;
	}
	cpm_dev.major = MAJOR(pedev);

	cdev_init(&cpm_dev.cdev, cpm_dev.fops);
	cpm_dev.cdev.owner = THIS_MODULE;
	cpm_dev.cdev.ops = cpm_dev.fops;
	res = cdev_add(&cpm_dev.cdev, MKDEV(cpm_dev.major, cpm_dev.minor), 1);
	if (res) {
		cpm_info("cdev_add() failed.\n");
		res = -ENODEV;
		goto err_reg_device;
	}

	/* add PE devices to sysfs */
	cpm_dev.dev_class = class_create(THIS_MODULE, cpm_dev.dev_name);
	if (IS_ERR(cpm_dev.dev_class)) {
		cpm_info("class_create failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	device_create(cpm_dev.dev_class, NULL,
		      MKDEV(cpm_dev.major, cpm_dev.minor), NULL,
		      cpm_dev.dev_name);

	return 0;
err_add_device:

	if (cpm_dev.dev_class) {
		device_destroy(cpm_dev.dev_class,
			       MKDEV(cpm_dev.major, cpm_dev.minor));
		class_destroy(cpm_dev.dev_class);
	}
	cdev_del(&cpm_dev.cdev);

err_reg_device:
	unregister_chrdev_region(MKDEV(cpm_dev.major, 0), CPM_MAX_DEVS);

	cpm_info("cpm_module_init failed !!! (%d)\n", res);

	return res;
}

void cpm_module_exit(void)
{
	/* del sysfs entries */
	if (cpm_dev.dev_class) {
		device_destroy(cpm_dev.dev_class,
			       MKDEV(cpm_dev.major, cpm_dev.minor));
		class_destroy(cpm_dev.dev_class);
	}

	/* del cdev */
	cdev_del(&cpm_dev.cdev);

	unregister_chrdev_region(MKDEV(cpm_dev.major, 0), CPM_MAX_DEVS);

	cpm_info("cpm_module_exit OK\n");
}
