#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/pm.h>
#include <linux/platform_device.h>

#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/system_misc.h>

#include "sm.h"

#define w32smSysCtl_SM_CTRL { \
	unsigned int uSM_CTRL_ISO_EN		:  1; \
	unsigned int uSM_CTRL_SM2SOC_SW_INTR	:  1; \
	unsigned int uSM_CTRL_SOC2SM_SW_INTR	:  1; \
	unsigned int uSM_CTRL_REV_0		:  2; \
	unsigned int uSM_CTRL_ADC_SEL		:  4; \
	unsigned int uSM_CTRL_ADC_PU		:  1; \
	unsigned int uSM_CTRL_ADC_CKSEL		:  2; \
	unsigned int uSM_CTRL_ADC_START		:  1; \
	unsigned int uSM_CTRL_ADC_RESET		:  1; \
	unsigned int uSM_CTRL_ADC_BG_RDY	:  1; \
	unsigned int uSM_CTRL_ADC_CONT		:  1; \
	unsigned int uSM_CTRL_ADC_BUF_EN	:  1; \
	unsigned int uSM_CTRL_ADC_VREF_SEL	:  1; \
	unsigned int uSM_CTRL_ADC_TST_SAR	:  1; \
	unsigned int uSM_CTRL_ADC_ROTATE_SEL	:  1; \
	unsigned int uSM_CTRL_TSEN_EN		:  1; \
	unsigned int uSM_CTRL_TSEN_CLK_SEL	:  1; \
	unsigned int uSM_CTRL_TSEN_MODE_SEL	:  1; \
	unsigned int uSM_CTRL_TSEN_ADC_CAL	:  2; \
	unsigned int uSM_CTRL_TSEN_TST_SEL	:  2; \
	unsigned int RSVDx14_b27		:  5; \
}

static int sm_irq;
static void __iomem *sm_base;
static void __iomem *sm_ctrl;

typedef union {
	unsigned int u32;
	struct w32smSysCtl_SM_CTRL;
} T32smSysCtl_SM_CTRL;

#define SM_Q_PUSH( pSM_Q ) {				\
	pSM_Q->m_iWrite += SM_MSG_SIZE;			\
	if( pSM_Q->m_iWrite >= SM_MSGQ_SIZE )		\
		pSM_Q->m_iWrite -= SM_MSGQ_SIZE;	\
	pSM_Q->m_iWriteTotal += SM_MSG_SIZE; }

#define SM_Q_POP( pSM_Q ) {				\
	pSM_Q->m_iRead += SM_MSG_SIZE;			\
	if( pSM_Q->m_iRead >= SM_MSGQ_SIZE )		\
		pSM_Q->m_iRead -= SM_MSGQ_SIZE;		\
	pSM_Q->m_iReadTotal += SM_MSG_SIZE; }

static int bsm_link_msg(MV_SM_MsgQ *q, MV_SM_Message *m, spinlock_t *lock)
{
	unsigned long flags;
	MV_SM_Message *p;
	int ret = 0;

	if (lock)
		spin_lock_irqsave(lock, flags);

	if (q->m_iWrite < 0 || q->m_iWrite >= SM_MSGQ_SIZE) {
		/* buggy ? */
		ret = -EIO;
		goto out;
	}

	/* message queue full, ignore the newest message */
	if (q->m_iRead == q->m_iWrite && q->m_iReadTotal != q->m_iWriteTotal) {
		ret = -EBUSY;
		goto out;
	}

	p = (MV_SM_Message*)(&(q->m_Queue[q->m_iWrite]));
	memcpy(p, m, sizeof(*p));
	SM_Q_PUSH(q);
out:
	if (lock)
		spin_unlock_irqrestore(lock, flags);

	return ret;
}

static int bsm_unlink_msg(MV_SM_MsgQ *q, MV_SM_Message *m, spinlock_t *lock)
{
	unsigned long flags;
	MV_SM_Message *p;
	int ret = -EAGAIN; /* means no data */

	if (lock)
		spin_lock_irqsave(lock, flags);

	if (q->m_iRead < 0 || q->m_iRead >= SM_MSGQ_SIZE ||
			q->m_iReadTotal > q->m_iWriteTotal) {
		/* buggy ? */
		ret = -EIO;
		goto out;
	}

	/* if buffer was overflow written, only the last messages are
	 * saved in queue. move read pointer into the same position of
	 * write pointer and keep buffer full.
	 */
	if (q->m_iWriteTotal - q->m_iReadTotal > SM_MSGQ_SIZE) {
		int iTotalDataSize = q->m_iWriteTotal - q->m_iReadTotal;

		q->m_iReadTotal += iTotalDataSize - SM_MSGQ_SIZE;
		q->m_iRead += iTotalDataSize % SM_MSGQ_SIZE;
		if (q->m_iRead >= SM_MSGQ_SIZE)
			q->m_iRead -= SM_MSGQ_SIZE;
	}

	if (q->m_iReadTotal < q->m_iWriteTotal) {
		/* alright get one message */
		p = (MV_SM_Message*)(&(q->m_Queue[q->m_iRead]));
		memcpy(m, p, sizeof(*m));
		SM_Q_POP(q);
		ret = 0;
	}
out:
	if (lock)
		spin_unlock_irqrestore(lock, flags);

	return ret;
}

static spinlock_t sm_lock;

static inline int bsm_link_msg_to_sm(MV_SM_Message *m)
{
	MV_SM_MsgQ *q = (MV_SM_MsgQ *)(SOC_DTCM(SM_CPU1_INPUT_QUEUE_ADDR));

	return bsm_link_msg(q, m, &sm_lock);
}

static inline int bsm_unlink_msg_from_sm(MV_SM_Message *m)
{
	MV_SM_MsgQ *q = (MV_SM_MsgQ *)(SOC_DTCM(SM_CPU0_OUTPUT_QUEUE_ADDR));

	return bsm_unlink_msg(q, m, &sm_lock);
}

#define DEFINE_SM_MODULES(id)				\
	{						\
		.m_iModuleID  = id,			\
		.m_bWaitQueue = false,			\
	}

typedef struct {
	int m_iModuleID;
	bool m_bWaitQueue;
	wait_queue_head_t m_Queue;
	spinlock_t m_Lock;
	MV_SM_MsgQ m_MsgQ;
	struct mutex m_Mutex;
} MV_SM_Module;

static MV_SM_Module SMModules[MAX_MSG_TYPE] = {
	DEFINE_SM_MODULES(MV_SM_ID_SYS),
	DEFINE_SM_MODULES(MV_SM_ID_COMM),
	DEFINE_SM_MODULES(MV_SM_ID_IR),
	DEFINE_SM_MODULES(MV_SM_ID_KEY),
	DEFINE_SM_MODULES(MV_SM_ID_POWER),
	DEFINE_SM_MODULES(MV_SM_ID_WD),
	DEFINE_SM_MODULES(MV_SM_ID_TEMP),
	DEFINE_SM_MODULES(MV_SM_ID_VFD),
	DEFINE_SM_MODULES(MV_SM_ID_SPI),
	DEFINE_SM_MODULES(MV_SM_ID_I2C),
	DEFINE_SM_MODULES(MV_SM_ID_UART),
	DEFINE_SM_MODULES(MV_SM_ID_CEC),
};

static inline MV_SM_Module *bsm_search_module(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(SMModules); i++)
		if (SMModules[i].m_iModuleID == id)
			return &(SMModules[i]);

	return NULL;
}

static int bsm_link_msg_to_module(MV_SM_Message *m)
{
	MV_SM_Module *module;
	int ret;

	module = bsm_search_module(m->m_iModuleID);
	if (!module)
		return -EINVAL;

	ret = bsm_link_msg(&(module->m_MsgQ), m, &(module->m_Lock));
	if (ret == 0) {
		/* wake up any process pending on wait-queue */
		wake_up_interruptible(&(module->m_Queue));
	}

	return ret;
}

static int bsm_unlink_msg_from_module(MV_SM_Message *m)
{
	MV_SM_Module *module;
	DEFINE_WAIT(__wait);
	unsigned long flags;
	int ret;

	module = bsm_search_module(m->m_iModuleID);
	if (!module)
		return -EINVAL;

repeat:
	spin_lock_irqsave(&(module->m_Lock), flags);

	if (!in_interrupt())
		prepare_to_wait(&(module->m_Queue), &__wait,
						TASK_INTERRUPTIBLE);

	ret = bsm_unlink_msg(&(module->m_MsgQ), m, NULL);
	if (ret == -EAGAIN && module->m_bWaitQueue && !in_interrupt()) {
		spin_unlock_irqrestore(&(module->m_Lock), flags);

		if (!signal_pending(current))
			schedule();
		else {
			finish_wait(&(module->m_Queue), &__wait);
			return -ERESTARTSYS;
		}

		goto repeat;
	}

	if (!in_interrupt())
		finish_wait(&(module->m_Queue), &__wait);

	spin_unlock_irqrestore(&(module->m_Lock), flags);

	return ret;
}

static void null_int(void)
{
}

void (*ir_sm_int)(void) = null_int;

int bsm_msg_send(int id, void *msg, int len)
{
	MV_SM_Message m;
	int ret;

	if (unlikely(len < 4) || unlikely(len > SM_MSG_BODY_SIZE))
		return -EINVAL;

	m.m_iModuleID = id;
	m.m_iMsgLen   = len;
	memcpy(m.m_pucMsgBody, msg, len);
	for (;;) {
		ret = bsm_link_msg_to_sm(&m);
		if (ret != -EBUSY)
			break;
		mdelay(10);
	}

	return ret;
}
EXPORT_SYMBOL(bsm_msg_send);

int bsm_msg_recv(int id, void *msg, int *len)
{
	MV_SM_Message m;
	int ret;

	m.m_iModuleID = id;
	ret = bsm_unlink_msg_from_module(&m);
	if (ret)
		return ret;

	if (msg)
		memcpy(msg, m.m_pucMsgBody, m.m_iMsgLen);

	if (len)
		*len = m.m_iMsgLen;

	return 0;
}
EXPORT_SYMBOL(bsm_msg_recv);

static void bsm_msg_dispatch(void)
{
	MV_SM_Message m;
	int ret;

	/* read all messages from SM buffers and dispatch them */
	for (;;) {
		ret = bsm_unlink_msg_from_sm(&m);
		if (ret)
			break;

		/* try best to dispatch received message */
		ret = bsm_link_msg_to_module(&m);
		if (ret != 0) {
			printk(KERN_ERR "Drop SM message\n");
			continue;
		}

		/* special case for IR events */
		if (m.m_iModuleID == MV_SM_ID_IR)
			ir_sm_int();
	}
}

static irqreturn_t bsm_intr(int irq, void *dev_id)
{
	void __iomem *addr;
	T32smSysCtl_SM_CTRL reg;

	addr = sm_ctrl;
	reg.u32 = readl_relaxed(addr);
	reg.uSM_CTRL_SM2SOC_SW_INTR = 0;
	writel_relaxed(reg.u32, addr);

	bsm_msg_dispatch();

	return IRQ_HANDLED;
}

static ssize_t berlin_sm_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	MV_SM_Message m;
	int id = (int)(*ppos);
	int ret;

	if (count < SM_MSG_SIZE)
		return -EINVAL;

	m.m_iModuleID = id;
	ret = bsm_unlink_msg_from_module(&m);
	if (!ret) {
		if (copy_to_user(buf, (void *)&m, SM_MSG_SIZE))
			return -EFAULT;
		return SM_MSG_SIZE;
	}

	return 0;
}

static ssize_t berlin_sm_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	MV_SM_Message SM_Msg;
	int ret;
	int id = (int)(*ppos);

	if (count < 4 || count > SM_MSG_BODY_SIZE)
		return -EINVAL;

	if (copy_from_user(SM_Msg.m_pucMsgBody, buf, count))
		return -EFAULT;

	ret = bsm_msg_send(id, SM_Msg.m_pucMsgBody, count);
	if (ret < 0)
		return -EFAULT;
	else
		return count;
}

static long berlin_sm_unlocked_ioctl(struct file *file,
			unsigned int cmd, unsigned long arg)
{
	MV_SM_Module *module;
	MV_SM_Message m;
	int ret = 0, id;

	if (cmd == SM_Enable_WaitQueue || cmd == SM_Disable_WaitQueue)
		id = (int)arg;
	else {
		if (copy_from_user(&m, (void __user *)arg, SM_MSG_SIZE))
			return -EFAULT;
		id = m.m_iModuleID;
	}

	module = bsm_search_module(id);
	if (!module)
		return -EINVAL;

	mutex_lock(&(module->m_Mutex));

	switch (cmd) {
	case SM_Enable_WaitQueue:
		module->m_bWaitQueue = true;
		break;
	case SM_Disable_WaitQueue:
		module->m_bWaitQueue = false;
		wake_up_interruptible(&(module->m_Queue));
		break;
	case SM_READ:
		ret = bsm_unlink_msg_from_module(&m);
		if (!ret) {
			if (copy_to_user((void __user *)arg, &m, SM_MSG_SIZE))
				ret = -EFAULT;
		}
		break;
	case SM_WRITE:
		ret = bsm_msg_send(m.m_iModuleID, m.m_pucMsgBody, m.m_iMsgLen);
		break;
	case SM_RDWR:
		ret = bsm_msg_send(m.m_iModuleID, m.m_pucMsgBody, m.m_iMsgLen);
		if (ret)
			break;
		ret = bsm_unlink_msg_from_module(&m);
		if (!ret) {
			if (copy_to_user((void __user *)arg, &m, SM_MSG_SIZE))
				ret = -EFAULT;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&(module->m_Mutex));

	return ret;
}

static struct file_operations berlin_sm_fops = {
	.owner		= THIS_MODULE,
	.write		= berlin_sm_write,
	.read		= berlin_sm_read,
	.unlocked_ioctl	= berlin_sm_unlocked_ioctl,
	.compat_ioctl	= berlin_sm_unlocked_ioctl,
	.llseek		= default_llseek,
};

static struct miscdevice sm_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "bsm",
	.fops	= &berlin_sm_fops,
};

static void berlin_restart(enum reboot_mode mode, const char *cmd)
{
	int len = 0;
	int msg = MV_SM_POWER_SYS_RESET;
	void *p = SM_MSG_EXTRA_BUF_ADDR;

	if (cmd && (len = strlen(cmd))) {
		len += 1;
		if (len > SM_MSG_EXTRA_BUF_SIZE - sizeof(len)) {
			len = SM_MSG_EXTRA_BUF_SIZE - sizeof(len);
			printk("cut off too long reboot args to %d bytes\n", len);
		}
		printk("reboot cmd is %s@%d\n", cmd, len);
		memcpy(p, &len, sizeof(len));
		memcpy(p + sizeof(len), cmd, len);
	} else
		memset(p, 0, sizeof(int));

	flush_cache_all();

	bsm_msg_send(MV_SM_ID_POWER, &msg, sizeof(int));
	for (;;);
}

static int berlin_sm_probe(struct platform_device *pdev)
{
	int i;
	struct resource *r;
	struct resource *r_sm_ctrl;
	resource_size_t size;
	const char *name;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!r || resource_type(r) != IORESOURCE_MEM) {
		dev_err(&pdev->dev, "invalid resource\n");
		return -EINVAL;
	}

	size = resource_size(r);
	name = r->name ?: dev_name(&pdev->dev);

	if (!devm_request_mem_region(&pdev->dev, r->start, size, name)) {
		dev_err(&pdev->dev, "can't request region for resource %pR\n", r);
		return -EBUSY;
	}

	sm_base = devm_ioremap_wc(&pdev->dev, r->start, size);
	if (IS_ERR(sm_base))
		return PTR_ERR(sm_base);

	r_sm_ctrl = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	sm_ctrl = devm_ioremap_resource(&pdev->dev, r_sm_ctrl);
	if (IS_ERR(sm_ctrl))
		return PTR_ERR(sm_ctrl);

	sm_irq = platform_get_irq(pdev, 0);
	if (sm_irq < 0)
		return -ENXIO;

	for (i = 0; i < ARRAY_SIZE(SMModules); i++) {
		init_waitqueue_head(&(SMModules[i].m_Queue));
		spin_lock_init(&(SMModules[i].m_Lock));
		mutex_init(&(SMModules[i].m_Mutex));
		memset(&(SMModules[i].m_MsgQ), 0, sizeof(MV_SM_MsgQ));
	}

	if (misc_register(&sm_dev))
		return -ENODEV;

	arm_pm_restart = berlin_restart;
	spin_lock_init(&sm_lock);

	return request_irq(sm_irq, bsm_intr, 0, "bsm", &sm_dev);
}

static int berlin_sm_remove(struct platform_device *pdev)
{
	misc_deregister(&sm_dev);
	free_irq(sm_irq, &sm_dev);

	return 0;
}

static struct of_device_id berlin_sm_match[] = {
	{ .compatible = "marvell,berlin-sm", },
	{},
};
MODULE_DEVICE_TABLE(of, berlin_sm_match);

static struct platform_driver berlin_sm_driver = {
	.probe		= berlin_sm_probe,
	.remove		= berlin_sm_remove,
	.driver = {
		.name	= "marvell,berlin-sm",
		.owner	= THIS_MODULE,
		.of_match_table = berlin_sm_match,
	},
};

module_platform_driver(berlin_sm_driver);

MODULE_AUTHOR("Marvell-Galois");
MODULE_DESCRIPTION("System Manager Driver");
MODULE_LICENSE("GPL");
