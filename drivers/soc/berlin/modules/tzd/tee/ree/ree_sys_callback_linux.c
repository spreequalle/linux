/********************************************************************************
 * Marvell GPL License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED.  The GPL License provides additional details about this warranty
 * disclaimer.
 ******************************************************************************/

#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/export.h>
#include <linux/namei.h>
#include <asm-generic/errno-base.h>
#include "config.h"
#include "ree_sys_callback_cmd.h"
#include "ree_sys_callback.h"
#include "ree_sys_callback_logger.h"
#ifdef CONFIG_REE_SYS_MUTEX
#include "ree_sys_callback_mutex.h"
#endif
#ifdef CONFIG_TEE_SYS_MUTEX
#include "ree_sys_callback_mutex_ext.h"
#endif
#include "tee_comm.h"
#include "log.h"

/*
 * Command functions
 */

static TEE_Result REESysCmd_GetTime(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	struct timeval tv;

	if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_OUTPUT)
		return TEE_ERROR_BAD_PARAMETERS;

	do_gettimeofday(&tv);

	params[0].value.a = tv.tv_sec;
	params[0].value.b = tv.tv_usec;

	return TEE_SUCCESS;
}

#define LOGGER_BUF_SIZE		512
#define TZD_TAG "tzd"
static DEFINE_PER_CPU(char [LOGGER_BUF_SIZE], logger_buf);

static int default_linux_log_print(struct ree_logger_param *param)
{
	static const char *__kernel_log_head[] = {
		KERN_WARNING TZD_TAG "(%s) ",
		KERN_NOTICE TZD_TAG "(%s) ",
		KERN_INFO TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
		KERN_DEBUG TZD_TAG "(%s) ",
	};
	char *buf;
	size_t n;
	unsigned long flags;

	buf  = *this_cpu_ptr(&logger_buf);
	local_irq_save(flags);
	n = snprintf(buf, LOGGER_BUF_SIZE, __kernel_log_head[param->prio & 0x7], param->tag);
	if (likely(n < LOGGER_BUF_SIZE))
		strlcpy(buf+n, param->text, LOGGER_BUF_SIZE-n);
	buf[LOGGER_BUF_SIZE-1] = 0; // make sure null terminated
	printk((const char *)buf);
	local_irq_restore(flags);

	return 0;
}

static BLOCKING_NOTIFIER_HEAD(ree_logger_notifier_list);

int register_ree_logger_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&ree_logger_notifier_list, nb);
}
EXPORT_SYMBOL(register_ree_logger_notifier);

int unregister_ree_logger_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&ree_logger_notifier_list, nb);
}
EXPORT_SYMBOL(unregister_ree_logger_notifier);

TEE_Result REESysCmd_LogWrite(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	REE_Logger_t *logger = (REE_Logger_t *) (param_ext);
	uint32_t inParamTypes = TEE_PARAM_TYPES(
					TEE_PARAM_TYPE_VALUE_INPUT,
					TEE_PARAM_TYPE_VALUE_INPUT,
					TEE_PARAM_TYPE_VALUE_INPUT,
					TEE_PARAM_TYPE_VALUE_INPUT);
	struct ree_logger_param param;
	int nr_calls = 0;

	if (unlikely(paramTypes != inParamTypes))
		return TEE_ERROR_BAD_PARAMETERS;

	if (unlikely(params[0].value.a != REE_LOGGER_PARAM_BUFID))
		return TEE_ERROR_BAD_PARAMETERS;

	if (unlikely(params[1].value.a != REE_LOGGER_PARAM_PRIO))
		return TEE_ERROR_BAD_PARAMETERS;

	if (unlikely(params[2].value.a != REE_LOGGER_PARAM_TAG)
		|| unlikely(params[2].value.b > REE_LOGGER_TAG_SIZE))
		return TEE_ERROR_BAD_PARAMETERS;

	if (unlikely(params[3].value.a != REE_LOGGER_PARAM_TEXT)
		|| unlikely(params[3].value.b > REE_LOGGER_TEXT_SIZE))
		return TEE_ERROR_BAD_PARAMETERS;

	param.bufID = params[0].value.b;
	param.prio  = params[1].value.b;
	param.tag   = logger->buffer.tag;
	param.text  = logger->buffer.text;

	if (!in_interrupt())
		__blocking_notifier_call_chain(&ree_logger_notifier_list,
				0, (void *) (&param), -1, &nr_calls);

	if (!nr_calls)
		default_linux_log_print(&param);

	return TEE_SUCCESS;
}

#ifdef CONFIG_REE_SYS_WAIT
static TEE_Result REESysCmd_Wait(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	unsigned long delay_ms, jiffies;

	if (TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT)
		return TEE_ERROR_BAD_PARAMETERS;

	delay_ms = params[0].value.a;
	jiffies = HZ * delay_ms / 1000;

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(jiffies);

	return TEE_SUCCESS;
}
#endif

static TEE_Result REESysCmd_FileOpen(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	REE_FileOpenParam *param = param_ext;
	mm_segment_t oldfs;
	struct file *filp;
	int flags = 0;
	TEE_Result res = TEE_SUCCESS;
	uint32_t tee_flags = params[0].value.a;
	uint32_t size = 0;

	if (param_ext_size != sizeof(REE_FileOpenParam))
		return TEE_ERROR_BAD_PARAMETERS;

	switch (tee_flags & (TEE_DATA_FLAG_ACCESS_READ |
				TEE_DATA_FLAG_ACCESS_WRITE)) {
	case TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE:
		flags |= O_RDWR;
		break;
	case TEE_DATA_FLAG_ACCESS_READ:
		flags |= O_RDONLY;
		break;
	case TEE_DATA_FLAG_ACCESS_WRITE:
		flags |= O_WRONLY;
		break;
	}

	if ((tee_flags & TEE_DATA_FLAG_CREATE) == TEE_DATA_FLAG_CREATE)
		flags |= O_CREAT | O_TRUNC;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(param->fileName, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (IS_ERR(filp)) {
		res = TEE_ERROR_GENERIC;
		error("fileName: %s, flags: %d\n", param->fileName, flags);
		goto done;
	}

	size = (uint32_t)vfs_llseek(filp, 0, SEEK_END);
	vfs_llseek(filp, 0, SEEK_SET);
	trace("filp: %p, size: %u\n", filp, size);

	params[1].value.a = (uint32_t)((unsigned long)filp >> 32);
	params[1].value.b = (uint32_t)((unsigned long)filp & 0xffffffff);
	/* FIXME: the assignment should be modified on 64-bit system. */
	params[0].value.b = size;

done:
	set_fs(oldfs);
	return res;
}

static TEE_Result REESysCmd_FileClose(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	struct file **filp = param_ext;
	mm_segment_t oldfs;

	if (param_ext_size != sizeof(struct file*))
		return TEE_ERROR_BAD_PARAMETERS;

	oldfs = get_fs();
	set_fs(get_ds());
	filp_close(*filp, NULL);
	set_fs(oldfs);

	return TEE_SUCCESS;
}

static TEE_Result REESysCmd_FileRead(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	REE_FileReadWriteParam *param = param_ext;
	mm_segment_t oldfs;
	loff_t offset;
	TEE_Result res = TEE_SUCCESS;
	struct file *filp = (struct file*)param->filp;

	if (param_ext_size != sizeof(REE_FileReadWriteParam))
		return TEE_ERROR_BAD_PARAMETERS;

	oldfs = get_fs();
	set_fs(get_ds());
	offset = filp->f_pos;
	param->ret = vfs_read(filp, param->buff, param->size, &offset);
	filp->f_pos = offset;
	if (param->ret < 0) {
		error("FileRead param->ret: %d\n", param->ret);
		res = TEE_ERROR_GENERIC;
	}
	param->offset = (int32_t)offset;
	trace("FileRead filp: %p, buff: %p, size: %lu, offset:%d\n",
		filp, param->buff, param->size, param->offset);

	set_fs(oldfs);
	return res;
}

static TEE_Result REESysCmd_FileWrite(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	REE_FileReadWriteParam *param = param_ext;
	mm_segment_t oldfs;
	loff_t offset;
	TEE_Result res = TEE_SUCCESS;
	struct file *filp = (struct file*)param->filp;

	if (param_ext_size != sizeof(REE_FileReadWriteParam) + param->size)
		return TEE_ERROR_BAD_PARAMETERS;

	oldfs = get_fs();
	set_fs(get_ds());
	offset = filp->f_pos;
	param->ret = vfs_write(filp, param->buff, param->size, &offset);
	filp->f_pos = offset;

	if (param->ret < 0) {
		error("FileWrite param->ret: %d\n", param->ret);
		res = TEE_ERROR_GENERIC;
	}
	param->offset = (int32_t)offset;
	trace("FileWrite filp: %p, buff: %p, size: %lu, offset: %d\n",
		filp, param->buff, param->size, param->offset);

	set_fs(oldfs);
	return res;
}

static TEE_Result REESysCmd_FileSeek(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	REE_FileSeekParam *param = param_ext;
	mm_segment_t oldfs;
	int origin;
	struct file *filp = (struct file *)param->filp;

	if (param_ext_size != sizeof(REE_FileSeekParam))
		return TEE_ERROR_BAD_PARAMETERS;

	switch (param->whence) {
	case TEE_DATA_SEEK_SET:
		origin = SEEK_SET;
		break;
	case TEE_DATA_SEEK_CUR:
		origin = SEEK_CUR;
		break;
	case TEE_DATA_SEEK_END:
		origin = SEEK_END;
		break;
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}

	trace("filp: %p, param->pos: %d, filp->f_pos:%llu\n",
		filp, param->pos, filp->f_pos);

	oldfs = get_fs();
	set_fs(get_ds());
	param->pos = (uint32_t)vfs_llseek(filp, param->offset, origin);
	set_fs(oldfs);

	return TEE_SUCCESS;
}

static int lnx_mkdir(const char *pathname, umode_t mode)
{
	struct dentry *dentry;
	struct path path;
	int error;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(get_ds());

	dentry = user_path_create(AT_FDCWD, pathname, &path, 1);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	if (!IS_POSIXACL(path.dentry->d_inode))
		mode &= ~current_umask();

	error = vfs_mkdir(path.dentry->d_inode, dentry, mode);

	dput(dentry);
	mutex_unlock(&path.dentry->d_inode->i_mutex);
	path_put(&path);

	set_fs(oldfs);

	return error;
}

static TEE_Result REESysCmd_MakeDir(
		void*			userdata,
		uint32_t		paramTypes,
		TEE_Param		params[4],
		void*			param_ext,
		uint32_t		param_ext_size)
{
	TEE_Result res = TEE_SUCCESS;
	REE_FolderMakeParam *param = param_ext;
	uint32_t flags = params[0].value.a;

	if (param_ext_size != sizeof(REE_FolderMakeParam))
		return TEE_ERROR_BAD_PARAMETERS;

	res = lnx_mkdir(param->folderPath, flags);

	if (!res || res == -EEXIST)
		return TEE_SUCCESS;
	else
		return res;
}


TEE_Result REE_InvokeSysCommandEntryPoint(
		void		*sessionContext,
		uint32_t	commandID,
		uint32_t	paramTypes,
		TEE_Param	params[4],
		void		*param_ext,
		uint32_t	param_ext_size)
{
	TEE_Result res = TEE_ERROR_NOT_SUPPORTED;

	trace("commandID=%d, paramTypes=0x%08x\n", commandID, paramTypes);

	switch (commandID) {
	case REE_CMD_TIME_GET:
		res = REESysCmd_GetTime(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_LOG_WRITE:
		res = REESysCmd_LogWrite(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
#ifdef CONFIG_REE_SYS_WAIT
	case REE_CMD_TIME_WAIT:
		res = REESysCmd_Wait(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
#endif
#ifdef CONFIG_REE_SYS_MUTEX
	case REE_CMD_MUTEX_CREATE:
		res = REESysCmd_MutexCreate(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_MUTEX_DESTROY:
		res = REESysCmd_MutexDestroy(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_MUTEX_LOCK:
		res = REESysCmd_MutexLock(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_MUTEX_TRYLOCK:
		res = REESysCmd_MutexTryLock(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_MUTEX_UNLOCK:
		res = REESysCmd_MutexUnlock(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
#endif

#ifdef CONFIG_TEE_SYS_MUTEX
	case REE_CMD_MUTEX_EXT_WAIT:
		res = REESysCmd_MutexWait(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_MUTEX_EXT_WAKE:
		res = REESysCmd_MutexWake(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_MUTEX_EXT_DEL:
		res = REESysCmd_MutexDel(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
#endif
#ifdef CONFIG_REE_SYS_SEMAPHORE
	case REE_CMD_SEMAPHORE_CREATE:
		res = REESysCmd_SemaphoreCreate(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_SEMAPHORE_DESTROY:
		res = REESysCmd_SemaphoreDestroy(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_SEMAPHORE_WAIT:
		res = REESysCmd_SemaphoreWait(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_SEMAPHORE_TIMEDWAIT:
		res = REESysCmd_SemaphoreTimedWait(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_SEMAPHORE_TRYWAIT:
		res = REESysCmd_SemaphoreTryWait(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_SEMAPHORE_POST:
		res = REESysCmd_SemaphorePost(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
#endif
	case REE_CMD_FILE_OPEN:
		res = REESysCmd_FileOpen(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_FILE_CLOSE:
		res = REESysCmd_FileClose(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_FILE_READ:
		res = REESysCmd_FileRead(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_FILE_WRITE:
		res = REESysCmd_FileWrite(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_FILE_SEEK:
		res = REESysCmd_FileSeek(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	case REE_CMD_FOLDER_MAKE:
		res = REESysCmd_MakeDir(sessionContext, paramTypes,
				params, param_ext, param_ext_size);
		break;
	}

	return res;
}
