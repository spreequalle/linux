/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef OVP_IN_TZ
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include "tee_client_api.h"
#include "tee_dev_ca_ovp.h"
#include "dev_ovp_cmd.h"

const TEEC_UUID TAOvp_UUID = TAOVP_UUID;
static bool initialized = false;
TEEC_Context ovpcontext;

#define MAX_TAOVP_INSTANCE_NUM  1
#define LIBOVP_TA_STEM           "libovp"
#define LIBOVP_TA_EXT            "ta"

static TEEC_Context context;
static TEEC_Session session;

int OvpInitialize(void)
{
	int index;
	int fd = -1;
	TEEC_Result result;
	TEEC_SharedMemory ovp_tee_shm;
	TEEC_Parameter parameter;
	char ta_file_path[64];
	char *ta_folder_path;

	if (initialized)
		return TEEC_SUCCESS;

	initialized = true;

	/* ========================================================================
	   [1] Connect to TEE
	   ======================================================================== */
	result = TEEC_InitializeContext(NULL, &ovpcontext);

	if (result != TEEC_SUCCESS) {
		//printk("ret=0x%08x\n", result);
		goto cleanup1;
	}

	/* ========================================================================
	   [3] Open session with TEE application
	   ======================================================================== */

	/* Open a Session with the TEE application. */
	result = TEEC_OpenSession(&context, &session, &TAOvp_UUID, TEEC_LOGIN_USER, NULL,	/* No connection data needed for TEEC_LOGIN_USER. */
				  NULL,	/* No payload, and do not want cancellation. */
				  NULL);
	if (result != TEEC_SUCCESS) {
		//printk("ret=0x%08x\n", result);
		goto cleanup2;
	}

	return TEEC_SUCCESS;

cleanup2:
	TEEC_FinalizeContext(&ovpcontext);
cleanup1:
	return result;
}

void OvpCloseSession(void)
{
	int index;
	TEEC_Result result;
	if (initialized) {
		TEEC_CloseSession(&session);
		initialized = false;
	}
}

int OvpGet_Clr_IntrSts(unsigned int *puiIntrSts)
{
	int index;
	TEEC_Result result;
	TEEC_Operation operation;

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
						TEEC_NONE,
						TEEC_NONE, TEEC_NONE);

	operation.params[0].value.a = 0xdeadbeef;
	operation.params[0].value.b = 0xdeadbeef;

	operation.started = 1;
	result = TEEC_InvokeCommand(&session,
				    OVP_GET_CLR_INTR_STS, &operation, NULL);

	*puiIntrSts = operation.params[0].value.b;

	return operation.params[0].value.a;
}
#endif
