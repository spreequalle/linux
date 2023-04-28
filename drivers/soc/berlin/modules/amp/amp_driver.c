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
#include <net/sock.h>
#include <linux/proc_fs.h>
#include <linux/io.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <asm/page.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <../drivers/staging/android/ion/ion.h>
#include <asm/uaccess.h>

/*******************************************************************************
  Local head files
  */

#include "galois_io.h"
#include "cinclude.h"

#include "api_avio_dhub.h"
#include "amp_driver.h"
#include "drv_debug.h"
#include "amp_memmap.h"
#include "tee_ca_dhub.h"
#include "hal_dhub_wrap.h"
#if defined(CONFIG_MV_AMP_COMPONENT_CPM_ENABLE)
#include "cpm_driver.h"
#endif
#include "amp_ioctl.h"
#include "drv_msg.h"

/* Sub Driver Modules */
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE)
#include "api_avif_dhub.h"
#include "avif_dhub_config.h"
#endif
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
#include "drv_avif.h"
#endif
#include "drv_aout.h"
#ifdef CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
#include "drv_app.h"
#endif
#ifdef KCONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
#include "drv_zsp.h"
#endif
#include "drv_vpu.h"
#include "drv_vpp.h"

#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
#include "drv_vip.h"
#endif

#if defined(CONFIG_MV_AMP_COMPONENT_OVP_ENABLE)
#include "drv_ovp.h"
#include "tee_dev_ca_ovp.h"
#endif
#include "drv_tsp.h"
/*******************************************************************************
  Static Variables
  */
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
static AVIF_CTX *hAvifCtx;
#endif

#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
static VIP_CTX *hVipCtx;
#endif

static AOUT_CTX *hAoutCtx;
#ifdef CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
static APP_CTX *hAppCtx;
#endif
#ifdef KCONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
static ZSP_CTX *hZspCtx;
#endif
static VPP_CTX *hVppCtx;
static VPU_CTX *hVpuCtx;
#if defined(CONFIG_MV_AMP_COMPONENT_OVP_ENABLE)
static OVP_CTX *hOvpCtx;
#endif
static TSP_CTX *hTspCtx;

static INT gIsrEnState[IRQ_MAX_MODULE];
unsigned int __read_mostly berlin_chip_revision;
extern DHUB_channel_config  VPP_config_A0[];
extern DHUB_channel_config  AG_config_A0[];
static DEFINE_SEMAPHORE(resume_sem);

#ifdef BERLIN_BOOTLOGO
#define MEMMAP_AVIO_BCM_REG_BASE			0xF7B50000
#define RA_AVIO_BCM_AUTOPUSH				0x0198
#endif

#define SPDIFRX_AUDIO_SOURCE				(1)
#define CONFIG_MV88DE3010_REGAREA_BASE          0xf6000000
#define CONFIG_MV88DE3010_REGAREA_SIZE          0x03000000

/*******************************************************************************
  Static Functions
  */
static INT amp_driver_open(struct inode *inode, struct file *filp);
static INT amp_driver_release(struct inode *inode, struct file *filp);
static LONG amp_driver_ioctl_unlocked(struct file *filp, UINT cmd, ULONG arg);
static INT amp_driver_mmap(struct file *file, struct vm_area_struct *vma);
static INT amp_device_init(struct amp_device_t *amp_dev, UINT user);
static INT amp_device_exit(struct amp_device_t *amp_dev, UINT user);
/*static INT amp_probe(struct platform_device *pdev);
static INT amp_remove(struct platform_device *pdev);*/
static VOID amp_shutdown(struct platform_device *pdev);
static INT amp_suspend(struct platform_device *pdev, pm_message_t state);
static INT amp_resume(struct platform_device *pdev);

static struct file_operations amp_ops = {
	.open = amp_driver_open,
	.release = amp_driver_release,
	.unlocked_ioctl = amp_driver_ioctl_unlocked,
	.compat_ioctl = amp_driver_ioctl_unlocked,
	.mmap = amp_driver_mmap,
	.owner = THIS_MODULE,
};

static struct amp_device_t legacy_pe_dev = {
	.dev_name = AMP_DEVICE_NAME,
	.minor = AMP_MINOR,
	.dev_init = amp_device_init,
	.dev_exit = amp_device_exit,
	.fops = &amp_ops,
};

static struct of_device_id amp_match[] = {
	{
	 .compatible = "marvell,berlin-amp",
	 },
	{},
};

static struct platform_driver amp_driver = {
	.shutdown = amp_shutdown,
	.suspend = amp_suspend,
	.resume = amp_resume,
	.driver = {
		   .name = AMP_DEVICE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = amp_match,
		   },
};

static char *amp_irq_match[IRQ_AMP_MAX] = {
	"/soc/amp/dHubIntrAvio0",	//IRQ_DHUBINTRAVIO0
	"/soc/amp/dHubIntrAvio1",	//IRQ_DHUBINTRAVIO1
	"/soc/amp/dHubIntrAvio2",	//IRQ_DHUBINTRAVIO2
	"/soc/amp/dHubIntrAvif0",	//IRQ_DHUBINTRAVIIF0
	"/soc/amp/zsp",		//IRQ_ZSP
	"/soc/amp/vmeta",	//IRQ_VMETA
#if (BERLIN_CHIP_VERSION >= BERLIN_BG4_CD)
	"/soc/amp/v4g",		//IRQ_V2G
#else
	"/soc/amp/v2g",		//IRQ_V2G
#endif
	"/soc/amp/h1",		//IRQ_H1
	"/soc/amp/ovp",		//IRQ_OVP
	"/soc/amp/figo",	//IRQ_FIGO
	"/soc/amp/cec",		//IRQ_SM_CEC
};

/*******************************************************************************
  Macro Defined
  */
static struct proc_dir_entry *amp_driver_config;
static struct proc_dir_entry *amp_driver_state;
static struct proc_dir_entry *amp_driver_detail;
static struct proc_dir_entry *amp_driver_aout;
/*******************************************************************************
  IO remap function
*/
/*AMP memory map table
 */
static struct amp_reg_map_desc amp_reg_map = {
	.vir_addr = 0,
	.phy_addr = CONFIG_MV88DE3010_REGAREA_BASE,
	.length = CONFIG_MV88DE3010_REGAREA_SIZE,
};

static INT amp_create_devioremap(VOID)
{
	amp_reg_map.vir_addr =
	    ioremap(amp_reg_map.phy_addr, amp_reg_map.length);
	if (!amp_reg_map.vir_addr) {
		amp_error("Fail to map address before it is used!\n");
		return -1;
	}
	amp_error("ioremap success: vir_addr: %p, size: 0x%zx, phy_addr: %p!\n",
		  amp_reg_map.vir_addr, amp_reg_map.length,
		  (void *)amp_reg_map.phy_addr);
	return S_OK;
}

static INT amp_destroy_deviounmap(VOID)
{
	if (amp_reg_map.vir_addr) {
		iounmap(amp_reg_map.vir_addr);
		amp_reg_map.vir_addr = 0;
	}
	return S_OK;
}

VOID *amp_memmap_phy_to_vir(UINT32 phyaddr)
{
	if (amp_reg_map.vir_addr &&
	    (phyaddr >= CONFIG_MV88DE3010_REGAREA_BASE) &&
	    (phyaddr <
	     CONFIG_MV88DE3010_REGAREA_BASE + CONFIG_MV88DE3010_REGAREA_SIZE)) {
		return (phyaddr - CONFIG_MV88DE3010_REGAREA_BASE +
			amp_reg_map.vir_addr);
	}
	amp_error("Fail to map phyaddr: 0x%x, Base vir: %p!\n", phyaddr,
		  amp_reg_map.vir_addr);

	return 0;
}

UINT32 amp_memmap_vir_to_phy(void *vir_addr)
{
	if (amp_reg_map.vir_addr &&
	    (vir_addr >= amp_reg_map.vir_addr) &&
	    (vir_addr < amp_reg_map.vir_addr + CONFIG_MV88DE3010_REGAREA_SIZE))
	{
		return (vir_addr - amp_reg_map.vir_addr +
			CONFIG_MV88DE3010_REGAREA_BASE);
	}

	return 0;
}

/*******************************************************************************
  Module Variable
  */
INT amp_irqs[IRQ_AMP_MAX];
static UINT8 gEnableVIP;
unsigned char gAmp_EnableTEE __read_mostly;
int avioDhub_channel_num = 0x7;
static INT amp_major;
static struct cdev amp_dev;
static INT debug_ctl;

static INT amp_device_init(struct amp_device_t *amp_dev, UINT user)
{
	hVppCtx = drv_vpp_init();
	if (unlikely(hVppCtx == NULL)) {
		amp_trace("drv_vpp_init: falied, unknown error\n");
		return -1;
	}
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
	if (gEnableVIP) {
		hAvifCtx = drv_avif_init();
		if (unlikely(hAvifCtx == NULL)) {
			amp_error("drv_avif_init: falied, unknown error\n");
			return -1;
		}
	}
#endif

#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
	if (gEnableVIP) {
		hVipCtx = drv_vip_init();
		if (unlikely(hVipCtx == NULL)) {
			amp_error("drv_vip_init: falied, unknown error\n");
			return -1;
		}
	}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
	hAppCtx = drv_app_init();
	if (unlikely(hAppCtx == NULL)) {
		amp_trace("drv_app_init init: failed\n");
		return -1;
	}
#endif

#if defined(CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE) || defined (CONFIG_MV_AMP_COMPONENT_V2G_ENABLE)
	hVpuCtx = drv_vpu_init();
	if (unlikely(hVpuCtx == NULL)) {
		amp_error("drv_vpu_init: falied, unknown error\n");
		return -1;
	}
#endif

	hAoutCtx = drv_aout_init();
	if (unlikely(hAoutCtx == NULL)) {
		amp_error("drv_aout_init: falied, unknown error\n");
		return -1;
	}
#ifdef KCONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
	hZspCtx = drv_zsp_init();
	if (unlikely(hZspCtx == NULL)) {
		amp_error("drv_zsp_init init: failed, unknown err\n");
		return -1;
	}
#endif
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
	if (hAvifCtx) {
		hAoutCtx->p_arc_fifo = &hAvifCtx->arc_fifo;
	}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
	//Initialize OVP driver.
	hOvpCtx = drv_ovp_init();
	if (unlikely(hOvpCtx == NULL)) {
		amp_trace("drv_vpp_init: falied, unknown error\n");
		return -1;
	}
#endif

	hTspCtx = drv_tsp_init();
	if (unlikely(hTspCtx == NULL)) {
		amp_error("drv_tsp_init init: failed, unknown err\n");
		return -1;
	}
	memset(gIsrEnState, 0, sizeof(gIsrEnState));

	amp_trace("amp_device_init ok\n");

	return S_OK;
}

static INT amp_device_exit(struct amp_device_t *amp_dev, UINT user)
{
#if defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE)
	if (gEnableVIP)
		drv_avif_exit(hAvifCtx);
#endif

#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
	if (gEnableVIP)
		drv_vip_exit(hVipCtx);
#endif
	drv_vpp_exit(hVppCtx);

#ifdef CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
	drv_app_exit(hAppCtx);
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
	drv_vpu_exit(hVpuCtx);
#endif
	drv_aout_exit(hAoutCtx);

#ifdef KCONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
	drv_zsp_exit(hZspCtx);
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
	drv_ovp_exit(hOvpCtx);
#endif
	drv_tsp_exit(hTspCtx);
	amp_trace("amp_device_exit ok");

	return S_OK;
}

/*******************************************************************************
  Module API
  */

static atomic_t amp_dev_refcnt = ATOMIC_INIT(0);

static INT amp_driver_open(struct inode *inode, struct file *filp)
{
	UINT vec_num;
	INT err = 0;

	amp_trace("Start open amp driver!\n");

	if (atomic_inc_return(&amp_dev_refcnt) > 1) {
		amp_trace("amp driver reference count %d!\n",
			  atomic_read(&amp_dev_refcnt));
		return 0;
	}
#ifdef CONFIG_MV88DE3100_FASTLOGO
	fastlogo_stop();
#endif

#ifdef BERLIN_BOOTLOGO
#if (BERLIN_CHIP_VERSION < BERLIN_BG4_CD)
	GA_REG_WORD32_WRITE(MEMMAP_AVIO_BCM_REG_BASE + RA_AVIO_BCM_AUTOPUSH,
			    0x0);
#elif(BERLIN_CHIP_VERSION >= BERLIN_BG4_CD)
	GA_REG_WORD32_WRITE(MEMMAP_AVIO_REG_BASE +
			    AVIO_MEMMAP_AVIO_BCM_REG_BASE +
			    RA_AVIO_BCM_AUTOPUSH, 0x0);
#endif
#endif

	/* initialize dhub */
	if(gAmp_EnableTEE)
		DhubInitialize();

	if (berlin_chip_revision == BERLIN_BG4CDP_A0_EXT) {
		wrap_DhubInitialization(CPUINDEX, VPP_DHUB_BASE_A0,
				VPP_HBO_SRAM_BASE_A0,
				&VPP_dhubHandle, VPP_config_A0,
				VPP_NUM_OF_CHANNELS_A0);
		DhubInitialization(CPUINDEX, AG_DHUB_BASE_A0,
				AG_HBO_SRAM_BASE_A0,
				&AG_dhubHandle, AG_config_A0,
				AG_NUM_OF_CHANNELS_A0);
	} else {
		wrap_DhubInitialization(CPUINDEX, VPP_DHUB_BASE,
					VPP_HBO_SRAM_BASE,
					&VPP_dhubHandle, VPP_config,
					VPP_NUM_OF_CHANNELS);
		DhubInitialization(CPUINDEX, AG_DHUB_BASE,
					AG_HBO_SRAM_BASE,
					&AG_dhubHandle, AG_config,
					AG_NUM_OF_CHANNELS);
	}
#ifdef CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE
	DhubInitialization(CPUINDEX, AVIF_DHUB_BASE, AVIF_HBO_SRAM_BASE,
			   &AVIF_dhubHandle, AVIF_config, AVIF_NUM_OF_CHANNELS);
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE
	/* register and enable cec interrupt */
	vec_num = amp_irqs[IRQ_SM_CEC];
	err = request_irq(vec_num, amp_devices_vpp_cec_isr, 0,
			  "amp_module_vpp_cec", (VOID *) hVppCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	gIsrEnState[IRQ_VPP_MODULE] |= VPP_CEC_INT_EN;
#endif
	/* register and enable VPP ISR */
	vec_num = amp_irqs[IRQ_DHUBINTRAVIO0];
	err = request_irq(vec_num, amp_devices_vpp_isr, 0,
			  "amp_module_vpp", (VOID *) hVppCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	gIsrEnState[IRQ_VPP_MODULE] |= VPP_INT_EN;

#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
	if (gEnableVIP) {
		/* register and enable VIP ISR */
		vec_num = amp_irqs[IRQ_DHUBINTRAVIO2];

		err =
			request_irq(vec_num,
			amp_devices_vip_isr, IRQF_SHARED,
			"amp_module_vip", (VOID *)hVipCtx);
		if (unlikely(err < 0)) {
			amp_trace("vec_num:%5d, err:%8x\n",
				vec_num, err);
			return err;
		}

		gIsrEnState[IRQ_VIP_MODULE] |= VIP_INT_EN;
	}
#endif
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	/* register and enable AVIF ISR */
	vec_num = amp_irqs[IRQ_DHUBINTRAVIIF0];
	err = request_irq(vec_num, amp_devices_avif_isr, 0,
			  "amp_module_avif", (VOID *) hAvifCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	gIsrEnState[IRQ_AVIN_MODULE] = 1;
#endif
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
	/* register and enable VDEC VMETA ISR */
	vec_num = amp_irqs[IRQ_VMETA];
	/*
	 * when amp service is killed there may be pending interrupts
	 * do not enable VDEC interrupts automatically here unless requested by VDEC module
	 */
	irq_modify_status(vec_num, 0, IRQ_NOAUTOEN);
	err = request_irq(vec_num, amp_devices_vdec_isr, IRQF_ONESHOT,
			  "amp_module_vdec", (VOID *) hVpuCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	disable_irq(amp_irqs[IRQ_VMETA]);
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
	/* register and enable VDEC V2G ISR */
	vec_num = amp_irqs[IRQ_V2G];
	err = request_irq(vec_num, amp_devices_vdec_isr, IRQF_ONESHOT,
			  "amp_module_vdec", (VOID *) hVpuCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	disable_irq(amp_irqs[IRQ_V2G]);
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_H1_ENABLE
	/* register and enable VDEC H1 ISR */
	vec_num = amp_irqs[IRQ_H1];
	err = request_irq(vec_num, amp_devices_vdec_isr, IRQF_ONESHOT,
			  "amp_module_venc", (VOID *) hVpuCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	disable_irq(amp_irqs[IRQ_H1]);
#endif

	/* register and enable audio out ISR */
	vec_num = amp_irqs[IRQ_DHUBINTRAVIO1];
	err = request_irq(vec_num, amp_devices_aout_isr, 0,
			  "amp_module_aout", (VOID *) hAoutCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	gIsrEnState[IRQ_AOUT_MODULE] = 1;

#ifdef KCONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
	/* register and enable ZSP ISR */
	vec_num = amp_irqs[IRQ_ZSP];
	err = request_irq(vec_num, amp_devices_zsp_isr, 0,
			  "amp_module_zsp", (VOID *) hZspCtx);
	if (unlikely(err < 0)) {
		amp_error("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	gIsrEnState[IRQ_ZSP_MODULE] = 1;
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
	/* register and enable OVP ISR  */
	vec_num = amp_irqs[IRQ_OVP];
	err = request_irq(vec_num, amp_devices_ovp_isr, 0,
			  "amp_module_ovp", (VOID *) hOvpCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	gIsrEnState[IRQ_OVP_MODULE] = 1;
#endif

	vec_num = amp_irqs[IRQ_FIGO];
	err = request_irq(vec_num, amp_devices_tsp_isr, 0,
			  "amp_module_tsp", (VOID *) hTspCtx);
	if (unlikely(err < 0)) {
		amp_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		return err;
	}
	gIsrEnState[IRQ_TSP_MODULE] = 1;

	amp_trace("amp_driver_open ok\n");

	return 0;
}

static INT amp_driver_release(struct inode *inode, struct file *filp)
{
	ULONG aoutirq;

	if (atomic_read(&amp_dev_refcnt) == 0) {
		amp_trace("amp driver already released!\n");
		return 0;
	}

	if (atomic_dec_return(&amp_dev_refcnt)) {
		amp_trace("amp dev ref cnt after this release: %d!\n",
			  atomic_read(&amp_dev_refcnt));
		return 0;
	}
#ifdef CONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE
	/* unregister cec interrupt */
	free_irq(amp_irqs[IRQ_SM_CEC], (VOID *) hVppCtx);
#endif
	/* unregister VPP interrupt */
	free_irq(amp_irqs[IRQ_DHUBINTRAVIO0], (VOID *) hVppCtx);
	gIsrEnState[IRQ_VPP_MODULE] = 0;

#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
	/* unregister VDEC vmeta/v2g interrupt */
	free_irq(amp_irqs[IRQ_VMETA], (VOID *) hVpuCtx);
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
	free_irq(amp_irqs[IRQ_V2G], (VOID *) hVpuCtx);
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_H1_ENABLE
	free_irq(amp_irqs[IRQ_H1], (VOID *) hVpuCtx);
#endif
	gIsrEnState[IRQ_VDEC_MODULE] = 0;

	/* unregister audio out interrupt */
	free_irq(amp_irqs[IRQ_DHUBINTRAVIO1], (VOID *) hAoutCtx);
	gIsrEnState[IRQ_AOUT_MODULE] = 0;
#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
	if (gEnableVIP) {
		/* unregister hdmirx vip interrupt */
		free_irq(amp_irqs[IRQ_DHUBINTRAVIO2], (VOID *)hVipCtx);
		gIsrEnState[IRQ_VIP_MODULE] = 0;
	}
#endif
#ifdef KCONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
	/* unregister ZSP interrupt */
	free_irq(amp_irqs[IRQ_ZSP], (VOID *) hZspCtx);
#endif
	gIsrEnState[IRQ_ZSP_MODULE] = 0;

#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	/* unregister AVIF interrupt */
	free_irq(amp_irqs[IRQ_DHUBINTRAVIIF0], (VOID *) hAvifCtx);
#endif
#endif
	gIsrEnState[IRQ_AVIN_MODULE] = 0;

#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
	/* unregister OVP interrupt */
	free_irq(amp_irqs[IRQ_OVP], (VOID *) hOvpCtx);
#endif
	gIsrEnState[IRQ_OVP_MODULE] = 0;

	free_irq(amp_irqs[IRQ_FIGO], (VOID *) hTspCtx);
	gIsrEnState[IRQ_TSP_MODULE] = 0;

	if (gAmp_EnableTEE)
		DhubFinalize();

	/* Stop all commands */
	spin_lock_irqsave(&hAoutCtx->aout_spinlock, aoutirq);
	aout_stop_cmd(hAoutCtx, MULTI_PATH);
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_20_ENABLE
	aout_stop_cmd(hAoutCtx, LoRo_PATH);
#endif
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE
	aout_stop_cmd(hAoutCtx, SPDIF_PATH);
#endif
#ifdef KCONFIG_MV_AMP_AUDIO_PATH_HDMI_ENABLE
	aout_stop_cmd(hAoutCtx, HDMI_PATH);
#endif
	spin_unlock_irqrestore(&hAoutCtx->aout_spinlock, aoutirq);

	amp_trace("amp_driver_release ok\n");

	return 0;
}

static INT amp_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long addr =
	    CONFIG_MV88DE3010_REGAREA_BASE + (vma->vm_pgoff << PAGE_SHIFT);

	if (!(addr >= CONFIG_MV88DE3010_REGAREA_BASE &&
	      (addr + size) <=
	      CONFIG_MV88DE3010_REGAREA_BASE +
	      CONFIG_MV88DE3010_REGAREA_SIZE)) {
		amp_error("Invalid address, start=0x%lx, end=0x%lx\n", addr,
			  addr + size);
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	ret = remap_pfn_range(vma,
			      vma->vm_start,
			      (CONFIG_MV88DE3010_REGAREA_BASE >> PAGE_SHIFT) +
			      vma->vm_pgoff, size, vma->vm_page_prot);
	if (ret)
		amp_trace("amp_driver_mmap failed, ret = %d\n", ret);
	else
		amp_trace("amp_driver_mmap ok\n");
	return ret;
}

static LONG amp_driver_ioctl_unlocked(struct file *filp, UINT cmd, ULONG arg)
{
	INT aout_info[2];
	ULONG aoutirq = 0;
#ifdef CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
	ULONG appirq = 0;
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_AIP_ENABLE
	ULONG aipirq = 0;
	INT aip_info[3];
#endif
	UINT bcm_sche_cmd_info[3], q_id;
	INT shm_mem = 0, gid = 0;
	PVOID param = NULL;
	UINT64 amptimevsync;
	struct ion_handle *ionh = NULL;
	struct ion_client *ionc = legacy_pe_dev.ionClient;
	switch (cmd) {
	case VPP_IOCTL_BCM_SCHE_CMD:
		if (copy_from_user
		    (bcm_sche_cmd_info, (VOID __user *) arg, 3 * sizeof(UINT)))
			return -EFAULT;
		q_id = bcm_sche_cmd_info[2];
		if (q_id > BCM_SCHED_Q5) {
			amp_trace("error BCM queue ID = %d\n", q_id);
			return -EFAULT;
		}
		hVppCtx->bcm_sche_cmd[q_id][0] = bcm_sche_cmd_info[0];
		hVppCtx->bcm_sche_cmd[q_id][1] = bcm_sche_cmd_info[1];
		amp_trace("vpp bcm shed qid:%d info:%x %x\n",
			  q_id, bcm_sche_cmd_info[0], bcm_sche_cmd_info[0]);
		break;

	case VPP_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&hVppCtx->vpp_sem);
			if (rc < 0)
				return rc;

#ifdef CONFIG_IRQ_LATENCY_PROFILE
			hVppCtx->amp_irq_profiler.vpp_task_sched_last =
			    cpu_clock(smp_processor_id());
#endif
			// check fullness, clear message queue once.
			// only send latest message to task.
			if (AMPMsgQ_Fullness(&hVppCtx->hVPPMsgQ) <= 0) {
				//amp_trace(" E/[vpp isr task]  message queue empty\n");
				return -EFAULT;
			}

			AMPMsgQ_DequeueRead(&hVppCtx->hVPPMsgQ, &msg);

			if (!atomic_read(&hVppCtx->vpp_isr_msg_err_cnt)) {
				atomic_set(&hVppCtx->vpp_isr_msg_err_cnt, 0);
			}
			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}
	case CEC_IOCTL_RX_MSG_BUF_MSG:	// copy cec rx message to user space buffer
		if (copy_to_user
		    ((VOID __user *) arg, &hVppCtx->rx_buf,
		     sizeof(VPP_CEC_RX_MSG_BUF)))
			return -EFAULT;

		return S_OK;
		break;
	case VPP_IOCTL_START_BCM_TRANSACTION:
		{
#if ((BERLIN_CHIP_VERSION != BERLIN_BG4_CD) && (BERLIN_CHIP_VERSION != BERLIN_BG4_CT) && \
     (BERLIN_CHIP_VERSION != BERLIN_BG4_CT_A0) && (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP))
			ULONG irqstat = 0;
			INT bcmbuf_info[2];
			if (copy_from_user
			    (bcmbuf_info, (VOID __user *) arg, 2 * sizeof(INT)))
				return -EFAULT;
			spin_lock_irqsave(&hVppCtx->bcm_spinlock, irqstat);
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_DTV)
			wrap_dhub_channel_write_cmd(&(VPP_dhubHandle.dhub),
						    avioDhubChMap_vpp_BCM_R,
						    bcmbuf_info[0],
						    bcmbuf_info[1], 0, 0, 0, 1,
						    0, 0);
#else
			dhub_channel_write_cmd(&(AG_dhubHandle.dhub),
					       avioDhubChMap_aio64b_BCM_R,
					       bcmbuf_info[0], bcmbuf_info[1],
					       0, 0, 0, 1, 0, 0);
#endif
			spin_unlock_irqrestore(&hVppCtx->bcm_spinlock, irqstat);
#endif
			break;
		}

	case VPP_IOCTL_INTR_MSG:
		//get VPP INTR MASK info
		{
			INTR_MSG vpp_intr_info = { 0, 0 };

			if (copy_from_user
			    (&vpp_intr_info, (VOID __user *) arg,
			     sizeof(INTR_MSG)))
				return -EFAULT;

			if (vpp_intr_info.DhubSemMap < MAX_INTR_NUM)
				hVppCtx->vpp_intr_status[vpp_intr_info.
							 DhubSemMap] =
				    vpp_intr_info.Enable;
			else
				return -EFAULT;

			break;
		}

	case VPP_IOCTL_GET_VSYNC:
		{
			HRESULT rc = S_OK;
			rc = down_interruptible(&hVppCtx->vsync_sem);
			if (rc < 0)
				return rc;
			if (!arg) {
				amp_error("IOCTL arg NULL!\n");
				return -EFAULT;
			}
			amptimevsync =
			    (UINT64) atomic64_read(&hVppCtx->vsynctime);

			if (put_user(amptimevsync, (UINT64 __user *) arg))
				return -EFAULT;
			break;
		}

#ifdef CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE
	/**************************************
	 * AVIF IOCTL
	 **************************************/
	case AVIF_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&hAvifCtx->avif_sem);
			if (rc < 0)
				return rc;

			rc = AMPMsgQ_ReadTry(&hAvifCtx->hPEAVIFMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("AVIF read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hAvifCtx->hPEAVIFMsgQ);
			if (!atomic_read(&hAvifCtx->avif_isr_msg_err_cnt)) {
				atomic_set(&hAvifCtx->avif_isr_msg_err_cnt, 0);
			}
			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}
	case AVIF_IOCTL_VDE_CFGQ:
		break;
	case AVIF_IOCTL_VBI_CFGQ:
		break;
	case AVIF_IOCTL_SD_WRE_CFGQ:
		break;
	case AVIF_IOCTL_SD_RDE_CFGQ:
		break;
	case AVIF_IOCTL_SEND_MSG:
		{
			//get msg from AVIF
			INT avif_msg = 0;
			if (copy_from_user
			    (&avif_msg, (VOID __user *) arg, sizeof(INT)))
				return -EFAULT;

			if (avif_msg == AVIF_MSG_DESTROY_ISR_TASK) {
				//force one more INT to AVIF to destroy ISR task
				up(&hAvifCtx->avif_sem);
			}
			break;
		}
	case AVIF_IOCTL_INTR_MSG:
		{
			//get AVIF INTR MASK info
			INTR_MSG avif_intr_info = { 0, 0 };

			if (copy_from_user
			    (&avif_intr_info, (VOID __user *) arg,
			     sizeof(INTR_MSG)))
				return -EFAULT;

			if (avif_intr_info.DhubSemMap < MAX_INTR_NUM)
				hAvifCtx->avif_intr_status[avif_intr_info.
							   DhubSemMap] =
				    avif_intr_info.Enable;
			else
				return -EFAULT;
			break;
		}
	case AVIF_HRX_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;
			rc = down_interruptible(&hAvifCtx->avif_hrx_sem);
			if (rc < 0)
				return rc;

			rc = AMPMsgQ_ReadTry(&hAvifCtx->hPEAVIFHRXMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("HRX read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hAvifCtx->hPEAVIFHRXMsgQ);
			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}
	case AVIF_HRX_IOCTL_SEND_MSG:	/* get msg from User Space */
		{
			INT hrx_msg = 0;
			if (copy_from_user
			    (&hrx_msg, (VOID __user *) arg, sizeof(INT)))
				return -EFAULT;

			if (hrx_msg == AVIF_HRX_DESTROY_ISR_TASK) {
				/* force one more INT to HRX to destroy ISR task */
				up(&hAvifCtx->avif_hrx_sem);
			}

			break;
		}

	case AVIF_HDCP_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;
			rc = down_interruptible(&hAvifCtx->avif_hdcp_sem);
			if (rc < 0)
				return rc;

			rc = AMPMsgQ_ReadTry(&hAvifCtx->hPEAVIFHDCPMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("HRX read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hAvifCtx->hPEAVIFHDCPMsgQ);
			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}

	case AVIF_HDCP_IOCTL_SEND_MSG:	/* get msg from User Space */
		{
			INT hdcp_msg = 0;
			if (copy_from_user
			    (&hdcp_msg, (VOID __user *) arg, sizeof(INT)))
				return -EFAULT;

			if (hdcp_msg == AVIF_HDCP_DESTROY_ISR_TASK) {
				/* force one more INT to HDCP to destroy ISR task */
				up(&hAvifCtx->avif_hdcp_sem);
			}
			break;
		}

#ifdef KCONFIG_MV_AMP_AUDIO_PATH_SPDIF_ENABLE
	case AVIF_HRX_IOCTL_ARC_SET:
		{
			INT hrx_msg = 0;
			static INT count = 0;
			MV_CC_MSG_t msg = { 0, 0, 0 };

			if (copy_from_user
			    (&hrx_msg, (VOID __user *) arg, sizeof(INT)))
				return -EFAULT;
			if (count != 0)
				break;
			count++;
			amp_trace("set arc enable value to %d\n", hrx_msg);
			//issue a dhub interrupt for avif arc or aout
			arc_copy_spdiftx_data(hAvifCtx);
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
			msg.m_MsgID = 1 << avioDhubChMap_aio64b_SPDIF_RX_W;
#else
			msg.m_MsgID = 1 << avioDhubChMap_ag_SPDIF_R;
#endif
			AMPMsgQ_Add(&hAoutCtx->hAoutMsgQ, &msg);
			up(&hAoutCtx->aout_sem);

			break;
		}
#endif

	case AVIF_VDEC_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;
			rc = down_interruptible(&hAvifCtx->avif_vdec_sem);
			if (rc < 0)
				return rc;

			rc = AMPMsgQ_ReadTry(&hAvifCtx->hPEAVIFVDECMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("VDEC read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hAvifCtx->hPEAVIFVDECMsgQ);
			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}

	case AVIF_VDEC_IOCTL_SEND_MSG:	/* get msg from User Space */
		{
			INT hrx_msg = 0;
			if (copy_from_user
			    (&hrx_msg, (VOID __user *) arg, sizeof(INT)))
				return -EFAULT;

			if (hrx_msg == AVIF_VDEC_DESTROY_ISR_TASK) {
				/* force one more INT to HRX to destroy ISR task */
				up(&hAvifCtx->avif_vdec_sem);
			}

			break;
		}
#endif

	/**************************************
	 * VDEC IOCTL
	 **************************************/
	case VDEC_IOCTL_ENABLE_INT:
		{
			HRESULT rc = S_OK;
			INT vpu_id = (INT) arg;

#if CONFIG_VDEC_IRQ_CHECKER
			if ((vpu_id == VPU_VMETA) || (vpu_id == VPU_G1)) {
				hVpuCtx->vdec_enable_int_cnt++;
			}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
			if ((vpu_id == VPU_VMETA) || (vpu_id == VPU_G1)) {
				MV_CC_MSG_t msg = { 0 };
				enable_irq(amp_irqs[IRQ_VMETA]);
				gIsrEnState[IRQ_VDEC_MODULE] |= VMETA_INT_EN;
				rc = down_interruptible(&hVpuCtx->
							vdec_vmeta_sem);
				if (rc < 0)
					return rc;

				// check fullness, clear message queue once.
				// only send latest message to task.
				if (AMPMsgQ_Fullness(&hVpuCtx->hVPUMsgQ) <= 0) {
					//amp_trace(" E/[vdec isr task]  message queue empty\n");
					return -EFAULT;
				}

				AMPMsgQ_DequeueRead(&hVpuCtx->hVPUMsgQ, &msg);

				if (!atomic_read
				    (&hVpuCtx->vdec_isr_msg_err_cnt)) {
					atomic_set(&hVpuCtx->
						   vdec_isr_msg_err_cnt, 0);
				}
			}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
			if (vpu_id == VPU_V2G) {
				enable_irq(amp_irqs[IRQ_V2G]);
				gIsrEnState[IRQ_VDEC_MODULE] |= V2G_INT_EN;
				rc = down_interruptible(&hVpuCtx->vdec_v2g_sem);
				if (rc < 0)
					return rc;
			}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_H1_ENABLE
			if (vpu_id == VPU_H1_0) {
				enable_irq(amp_irqs[IRQ_H1]);
				gIsrEnState[IRQ_VDEC_MODULE] |= H1_INT_EN;
				rc = down_interruptible(&hVpuCtx->vdec_v2g_sem);
				if (rc < 0)
					return rc;
			}
#endif
			break;
		}

    /**************************************
     * AOUT IOCTL
     **************************************/
	case AOUT_IOCTL_START_CMD:
		{
			UINT32 output_port;
			if (copy_from_user
			    (aout_info, (VOID __user *) arg, 2 * sizeof(INT)))
				return -EFAULT;
			output_port = aout_info[0];
			if (output_port >= MAX_OUTPUT_AUDIO) {
				printk("output %d out of range\r\n",
				       output_port);
				break;
			}
			gid = aout_info[1];
			if (ionc) {
				ionh = ion_gethandle(ionc, gid);
			}
			if (ionh) {
				hAoutCtx->gstAoutIon.ionh[output_port] = ionh;
				param = ion_map_kernel(ionc, ionh);
				if (debug_ctl > 0) {
					ion_phys_addr_t addr = 0;
					size_t len = 0;
					ion_phys(ionc, ionh, &addr, &len);
					amp_trace
					    ("aout start path:0x%x gid:0x%x ion:%p pfifo:0x%lx phyadd:%p len:%zu\n",
					     (UINT) output_port, (UINT) gid,
					     ionh, (ULONG) param, (void *)addr,
					     len);
				}
			}
			if (!param) {
				amp_trace("ctl:%x bad shm mem offset:%x\n",
					  AOUT_IOCTL_START_CMD, shm_mem);
				return -EFAULT;
			}
			//MV_SHM_Takeover(shm_mem);
			spin_lock_irqsave(&hAoutCtx->aout_spinlock, aoutirq);
			aout_start_cmd(hAoutCtx, aout_info, param);
			spin_unlock_irqrestore(&hAoutCtx->aout_spinlock,
					       aoutirq);
			break;
		}
	case AOUT_IOCTL_STOP_CMD:
		{
			UINT32 output_port;
			if (copy_from_user
			    (aout_info, (VOID __user *) arg, 2 * sizeof(INT)))
				return -EFAULT;

			output_port = aout_info[0];
			if (output_port >= MAX_OUTPUT_AUDIO) {
				printk("output %d out of range\r\n",
				       output_port);
				break;
			}
			spin_lock_irqsave(&hAoutCtx->aout_spinlock, aoutirq);
			aout_stop_cmd(hAoutCtx, output_port);
			spin_unlock_irqrestore(&hAoutCtx->aout_spinlock,
					       aoutirq);
			if (ionc && hAoutCtx->gstAoutIon.ionh[output_port]) {
				if (debug_ctl > 0) {
					amp_trace("aout stop path:%x ion:%p\n",
						  (UINT) output_port,
						  hAoutCtx->gstAoutIon.
						  ionh[output_port]);
				}
				ion_unmap_kernel(ionc,
						 hAoutCtx->gstAoutIon.
						 ionh[output_port]);
				ion_free(ionc,
					 hAoutCtx->gstAoutIon.
					 ionh[output_port]);
			}
			break;
		}

#ifdef CONFIG_MV_AMP_COMPONENT_AIP_ENABLE
	/**************************************
	 * AIP IOCTL
	 **************************************/
	case AIP_IOCTL_START_CMD:
		if (copy_from_user
		    (aip_info, (VOID __user *) arg, 3 * sizeof(INT))) {
			return -EFAULT;
		}
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
		if ((SPDIFRX_AUDIO_SOURCE == aip_info[2]) && hAoutCtx) {
			hAoutCtx->is_spdifrx_enabled = 1;
		}
#else
		if ((SPDIFRX_AUDIO_SOURCE == aip_info[2]) && hVppCtx) {
			hVppCtx->is_spdifrx_enabled = 1;
		}
#endif
		gid = aip_info[1];
		if (ionc) {
			ionh = ion_gethandle(ionc, gid);
		}
		if (ionh) {
			hAvifCtx->gstAipIon.ionh = ionh;
			param = ion_map_kernel(ionc, ionh);
		}
		if (!param) {
			amp_trace("ctl:%x bad shm mem offset:%x\n",
				  AIP_IOCTL_START_CMD, shm_mem);
			return -EFAULT;
		}
		//MV_SHM_Takeover(shm_mem);
		spin_lock_irqsave(&hAvifCtx->aip_spinlock, aipirq);
		aip_start_cmd(hAvifCtx, aip_info, param);
		spin_unlock_irqrestore(&hAvifCtx->aip_spinlock, aipirq);
		break;

	case AIP_IOCTL_STOP_CMD:
		spin_lock_irqsave(&hAvifCtx->aip_spinlock, aipirq);
		aip_stop_cmd(hAvifCtx);
		spin_unlock_irqrestore(&hAvifCtx->aip_spinlock, aipirq);

		if (copy_from_user
		    (aip_info, (VOID __user *) arg, 2 * sizeof(INT))) {
			return -EFAULT;
		}
		gid = aip_info[1];
		if (ionc && hAvifCtx->gstAipIon.ionh) {
			ion_unmap_kernel(ionc, hAvifCtx->gstAipIon.ionh);
			ion_free(ionc, hAvifCtx->gstAipIon.ionh);
		}
		break;

#ifdef CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE
	/**************************************
	 * AIP AVIF IOCTL
	 **************************************/
	case AIP_AVIF_IOCTL_START_CMD:
		if (copy_from_user
		    (aip_info, (VOID __user *) arg, 2 * sizeof(INT))) {
			return -EFAULT;
		}
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_DTV)
		if ((SPDIFRX_AUDIO_SOURCE == aip_info[2]) && hAoutCtx) {
			hAoutCtx->is_spdifrx_enabled = 1;
		}
#else
		if ((SPDIFRX_AUDIO_SOURCE == aip_info[2]) && hVppCtx) {
			hVppCtx->is_spdifrx_enabled = 1;
		}
#endif
		gid = aip_info[1];
		if (ionc) {
			ionh = ion_gethandle(ionc, gid);
		}
		if (ionh) {
			hAvifCtx->gstAipIon.ionh = ionh;
			param = ion_map_kernel(ionc, ionh);
		}
		if (!param) {
			amp_trace("ctl:%x bad shm mem offset:%x\n",
				  AIP_AVIF_IOCTL_START_CMD, shm_mem);
			return -EFAULT;
		}
		spin_lock_irqsave(&hAvifCtx->aip_spinlock, aipirq);
		aip_avif_start_cmd(hAvifCtx, aip_info, param);
		spin_unlock_irqrestore(&hAvifCtx->aip_spinlock, aipirq);
		break;
	/* MAIN/PIP Audio */
	case AIP_AVIF_IOCTL_SET_MODE:
		{
			INT audio_mode = 0;
			if (copy_from_user
			    (&audio_mode, (VOID __user *) arg, sizeof(INT)))
				return -EFAULT;
			amp_trace("Audio switch mode = 0x%x\r\n", audio_mode);
			hAvifCtx->IsPIPAudio = audio_mode;
			break;
		}
	case AIP_AVIF_IOCTL_STOP_CMD:
		{
			aip_avif_stop_cmd(hAvifCtx);

			if (copy_from_user
			    (aip_info, (VOID __user *) arg, 2 * sizeof(INT))) {
				return -EFAULT;
			}
			gid = aip_info[1];
			if (ionc && hAvifCtx->gstAipIon.ionh) {
				ion_unmap_kernel(ionc,
						 hAvifCtx->gstAipIon.ionh);
				ion_free(ionc, hAvifCtx->gstAipIon.ionh);
			}

			break;
		}
#endif

	case AIP_IOCTL_SemUp_CMD:
		{
			up(&hAvifCtx->aip_sem);
			break;
		}

	case AIP_IOCTL_GET_MSG_CMD:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&hAvifCtx->aip_sem);
			if (rc < 0) {
				return rc;
			}

			rc = AMPMsgQ_ReadTry(&hAvifCtx->hAIPMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("AIP read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hAvifCtx->hAIPMsgQ);

			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t))) {
				return -EFAULT;
			}
			break;
		}
#endif /* CONFIG_MV_AMP_COMPONENT_AIP_ENABLE */

#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
	case VIP_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&hVipCtx->vip_sem);
			if (rc < 0)
				return rc;

			/* check fullness, clear message queue once.
			* only send latest message to task. */
			if (AMPMsgQ_Fullness(&hVipCtx->hVIPMsgQ) <= 0) {
				amp_error(
					" E/[vip isr task]  message queue empty\n");
				return -EFAULT;
			}

			AMPMsgQ_DequeueRead(&hVipCtx->hVIPMsgQ, &msg);

			if (!atomic_read(&hVipCtx->vip_isr_msg_err_cnt)) {
				/* msgQ get full, if isr task
				* can run here, reset msgQ */
				atomic_set(&hVipCtx->vip_isr_msg_err_cnt, 0);
			}

			if (copy_to_user
				((VOID __user *)arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}
	case VIP_IOCTL_SEND_MSG:
		{
			/* get msg from VIP */
			INT vip_msg = 0;

			if (copy_from_user(&vip_msg,
				(VOID __user *)arg, sizeof(INT)))
				return -EFAULT;

			if (vip_msg == VIP_MSG_DESTROY_ISR_TASK) {
				/* force one more INT to VIP to
				* destroy ISR task */
				amp_trace("Destroy ISR Task...\r\n");
				up(&hVipCtx->vip_sem);
			}

			amp_trace("Destroyed ISR Task...\r\n");
			break;
		}
	case VIP_HRX_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&hVipCtx->vip_hrx_sem);
			if (rc < 0)
				return rc;

			rc = AMPMsgQ_ReadTry(&hVipCtx->hVIPHRXMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("HRX read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hVipCtx->hVIPHRXMsgQ);
			if (copy_to_user
						((VOID __user *)arg,
						&msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}
	case VIP_HRX_IOCTL_SEND_MSG: /* get msg from User Space */
		{
			INT hrx_msg = 0;

			if (copy_from_user
					(&hrx_msg,
					(VOID __user *)arg, sizeof(INT)))
				return -EFAULT;

			if (hrx_msg == HRX_MSG_DESTROY_ISR_TASK) {
				/* force one more
				* INT to HRX to destroy ISR task */
				up(&hVipCtx->vip_hrx_sem);
			}
			break;
		}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_HWAPP_ENABLE
	/**************************************
	 * APP IOCTL
	 **************************************/
	case APP_IOCTL_INIT_CMD:
		if (copy_from_user(&gid, (VOID __user *) arg, sizeof(INT))) {
			return -EFAULT;
		}
		if (ionc) {
			ionh = ion_gethandle(ionc, gid);
		}
		if (ionh) {
			hAppCtx->gstAppIon.ionh = ionh;
			hAppCtx->p_app_fifo =
			    (HWAPP_CMD_FIFO *) ion_map_kernel(ionc, ionh);
		}
		break;

	case APP_IOCTL_DEINIT_CMD:
		{
			if (copy_from_user
			    (&gid, (VOID __user *) arg, sizeof(INT))) {
				return -EFAULT;
			}
			if (ionc && hAppCtx->gstAppIon.ionh) {
				ion_unmap_kernel(ionc, hAppCtx->gstAppIon.ionh);
				ion_free(ionc, hAppCtx->gstAppIon.ionh);
			}
			break;
		}

	case APP_IOCTL_START_CMD:
		spin_lock_irqsave(&hAppCtx->app_spinlock, appirq);
		if (hAppCtx->p_app_fifo) {
			app_start_cmd(hAppCtx, hAppCtx->p_app_fifo);
		}
		spin_unlock_irqrestore(&hAppCtx->app_spinlock, appirq);
		break;

	case APP_IOCTL_GET_MSG_CMD:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&hAppCtx->app_sem);
			if (rc < 0)
				return rc;

			rc = AMPMsgQ_ReadTry(&hAppCtx->hAPPMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("APP read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hAppCtx->hAPPMsgQ);
			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}
#endif

	case AOUT_IOCTL_GET_MSG_CMD:
		{
			HRESULT rc = S_OK;
			MV_CC_MSG_t msg = { 0 };
			rc = down_interruptible(&hAoutCtx->aout_sem);
			if (rc < 0) {
				return rc;
			}
			rc = AMPMsgQ_ReadTry(&hAoutCtx->hAoutMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("AOUT read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hAoutCtx->hAoutMsgQ);
			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t))) {
				return -EFAULT;
			}
			break;
		}

#ifdef KCONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
	case ZSP_IOCTL_GET_MSG_CMD:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;
			rc = down_interruptible(&hZspCtx->zsp_sem);
			if (rc < 0) {
				return rc;
			}
			rc = AMPMsgQ_ReadTry(&hZspCtx->hZSPMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("ZSP read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hZspCtx->hZSPMsgQ);
			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t))) {
				return -EFAULT;
			}
			break;
		}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
		/*  OVP IOCTL commands  */
	case OVP_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;

			rc = down_interruptible(&hOvpCtx->ovp_sem);
			if (rc < 0)
				return rc;

			// check fullness, clear message queue once.
			// only send latest message to task.
			if (AMPMsgQ_Fullness(&hOvpCtx->hOVPMsgQ) <= 0) {
				//amp_trace(" E/[vpp isr task]  message queue empty\n");
				return -EFAULT;
			}

			AMPMsgQ_DequeueRead(&hOvpCtx->hOVPMsgQ, &msg);

			if (copy_to_user
			    ((VOID __user *) arg, &msg, sizeof(MV_CC_MSG_t)))
				return -EFAULT;
			break;
		}

	case OVP_IOCTL_INTR:
		{
			INTR_MSG ovp_intr_info = { 0, 0 };

			if (copy_from_user
			    (&ovp_intr_info, (VOID __user *) arg,
			     sizeof(INTR_MSG)))
				return -EFAULT;

			hOvpCtx->ovp_intr_status[0] = ovp_intr_info.Enable;

			break;
		}
	case OVP_IOCTL_DUMMY_INTR:
		{
			up(&hOvpCtx->ovp_sem);

			break;
		}

	case OVP_INIT_TA_SESSION:
		{
			HRESULT rc = S_OK;

			rc = OvpInitialize();

			if (rc) {
				amp_error("OvpInitialize: failed,0x%x\n", rc);
			}

			break;

		}

	case OVP_KILL_TA_SESSION:
		{
			OvpCloseSession();

			break;
		}
#endif
	case POWER_IOCTL_DISABLE_INT:
		{
			INT module_name = (INT) (arg & 0xFFFF);
			INT module_mask = (INT) (arg >> 16);

			switch (module_name) {
			case IRQ_VPP_MODULE:
#ifdef CONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE
				if (gIsrEnState[IRQ_VPP_MODULE] & VPP_CEC_INT_EN
				    & module_mask) {
					/* disable CEC interrupt */
					disable_irq(amp_irqs[IRQ_SM_CEC]);
				}
#endif
				if (gIsrEnState[IRQ_VPP_MODULE] & VPP_INT_EN &
				    module_mask) {
					/* disable VPP interrupt */
					disable_irq(amp_irqs
						    [IRQ_DHUBINTRAVIO0]);
				}
				break;

			case IRQ_AVIN_MODULE:
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
				if (gIsrEnState[IRQ_AVIN_MODULE]) {
					/* disable AVIF interrupt */
					disable_irq(amp_irqs
						    [IRQ_DHUBINTRAVIIF0]);
				}
#endif
#endif
				break;

			case IRQ_VDEC_MODULE:
#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
				if (gIsrEnState[IRQ_VDEC_MODULE] & VMETA_INT_EN
				    & module_mask) {
					/* disable VMETA interrupt */
					disable_irq(amp_irqs[IRQ_VMETA]);
				}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
				if (gIsrEnState[IRQ_VDEC_MODULE] & V2G_INT_EN &
				    module_mask) {
					/* disable V2G interrupt */
					disable_irq(amp_irqs[IRQ_V2G]);
				}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_H1_ENABLE
				if (gIsrEnState[IRQ_VDEC_MODULE] & H1_INT_EN &
				    module_mask) {
					/* disable H1 interrupt */
					disable_irq(amp_irqs[IRQ_H1]);
				}
#endif
				break;

			case IRQ_AOUT_MODULE:
				if (gIsrEnState[IRQ_AOUT_MODULE]) {
					/* disable audio out interrupt */
					disable_irq(amp_irqs
						    [IRQ_DHUBINTRAVIO1]);
				}
				break;
#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
			case IRQ_VIP_MODULE:
				if (gIsrEnState[IRQ_VIP_MODULE]) {
					/* disable vip interrupt */
					disable_irq(
						amp_irqs[IRQ_DHUBINTRAVIO2]);
				}
				break;
#endif

			case IRQ_ZSP_MODULE:
#ifdef CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
				if (gIsrEnState[IRQ_ZSP_MODULE]) {
					/* disable ZSP interrupt */
					disable_irq(amp_irqs[IRQ_ZSP]);
				}
#endif
				break;
			case IRQ_OVP_MODULE:
#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
				if (gIsrEnState[IRQ_OVP_MODULE]) {
					/* disable OVP interrupt */
					disable_irq(amp_irqs[IRQ_OVP]);
				}
#endif
				break;
			case IRQ_TSP_MODULE:
				if (gIsrEnState[IRQ_TSP_MODULE]) {
					/* disable TSP interrupt */
					disable_irq(amp_irqs[IRQ_FIGO]);
				}
				break;
			default:
				break;
			}
			break;
		}

	case POWER_IOCTL_ENABLE_INT:
		{
			INT module_name = (INT) (arg & 0xFFFF);
			INT module_mask = (INT) (arg >> 16);

			switch (module_name) {
			case IRQ_VPP_MODULE:
#ifdef CONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE
				if (gIsrEnState[IRQ_VPP_MODULE] & VPP_CEC_INT_EN
				    & module_mask) {
					/* enable CEC interrupt */
					enable_irq(amp_irqs[IRQ_SM_CEC]);
				}
#endif
				if (gIsrEnState[IRQ_VPP_MODULE] & VPP_INT_EN &
				    module_mask) {
					/* enable VPP interrupt */
					enable_irq(amp_irqs[IRQ_DHUBINTRAVIO0]);
				}
				break;

			case IRQ_AVIN_MODULE:
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
				if (gIsrEnState[IRQ_AVIN_MODULE]) {
					/* enable AVIF interrupt */
					enable_irq(amp_irqs
						   [IRQ_DHUBINTRAVIIF0]);
				}
#endif
#endif
				break;

			case IRQ_VDEC_MODULE:
#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
				if (gIsrEnState[IRQ_VDEC_MODULE] & VMETA_INT_EN
				    & module_mask) {
					/* enable VMETA interrupt */
					enable_irq(amp_irqs[IRQ_VMETA]);
				}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
				if (gIsrEnState[IRQ_VDEC_MODULE] & V2G_INT_EN &
				    module_mask) {
					/* enable V2G interrupt */
					enable_irq(amp_irqs[IRQ_V2G]);
				}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_H1_ENABLE
				if (gIsrEnState[IRQ_VDEC_MODULE] & H1_INT_EN &
				    module_mask) {
					/* enable H1 interrupt */
					enable_irq(amp_irqs[IRQ_H1]);
				}
#endif
				break;

			case IRQ_AOUT_MODULE:
				if (gIsrEnState[IRQ_AOUT_MODULE]) {
					/* enable audio out interrupt */
					enable_irq(amp_irqs[IRQ_DHUBINTRAVIO1]);
				}
				break;

#if defined(CONFIG_MV_AMP_COMPONENT_VIP_ENABLE)
			case IRQ_VIP_MODULE:
				if (gIsrEnState[IRQ_VIP_MODULE]) {
					/* disable vip interrupt */
					enable_irq(amp_irqs[IRQ_DHUBINTRAVIO2]);
				}
				break;
#endif
			case IRQ_ZSP_MODULE:
#ifdef CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
				if (gIsrEnState[IRQ_ZSP_MODULE]) {
					/* enable ZSP interrupt */
					enable_irq(amp_irqs[IRQ_ZSP]);
				}
#endif
				break;
			case IRQ_OVP_MODULE:
#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
				if (gIsrEnState[IRQ_OVP_MODULE]) {
					/* enable OVP interrupt */
					enable_irq(amp_irqs[IRQ_OVP]);
				}
#endif
				break;
			case IRQ_TSP_MODULE:
				if (gIsrEnState[IRQ_TSP_MODULE]) {
					/* enable TSP interrupt */
					enable_irq(amp_irqs[IRQ_FIGO]);
				}
				break;

			default:
				break;
			}
			break;
		}

	case POWER_IOCTL_WAIT_RESUME:
		{
			HRESULT rc = S_OK;
			sema_init(&resume_sem, 0);
			rc = down_interruptible(&resume_sem);
			if (rc < 0)
				return rc;
			break;
		}

	case POWER_IOCTL_START_RESUME:
		{
			up(&resume_sem);
			break;
		}

	case TSP_IOCTL_CMD_MSG:
		{
			INT io_info[2];
			if (unlikely(copy_from_user
				     (io_info, (VOID __user *) arg,
				      2 * sizeof(INT)) > 0)) {
				return -EFAULT;
			}
			switch (io_info[0]) {
			case TSP_ISR_START:
				drv_tsp_open(hTspCtx, io_info[1]);
				break;
			case TSP_ISR_STOP:
				drv_tsp_close(hTspCtx);
				break;
			case TSP_ISR_WAKEUP:
				up(&hTspCtx->tsp_sem);
				break;
			default:
				break;
			}
			break;
		}

	case TSP_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			HRESULT rc = S_OK;
			rc = down_interruptible(&hTspCtx->tsp_sem);
			if (unlikely(rc < 0)) {
				return rc;
			}
			rc = AMPMsgQ_ReadTry(&hTspCtx->hTSPMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				amp_trace("TSP read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&hTspCtx->hTSPMsgQ);
			if (unlikely(copy_to_user
				     ((VOID __user *) arg, &msg,
				      sizeof(MV_CC_MSG_t)) > 0)) {
				return -EFAULT;
			}
			break;
		}
	default:
		break;
	}
	return 0;
}

static ssize_t amp_config_proc_write(struct file *file,
				     const char __user * buffer, size_t count,
				     loff_t * f_pos)
{
	UINT32 config = 0;
	UINT size = count > sizeof(UINT32) ? sizeof(UINT32) : count;

	if (copy_from_user(&config, buffer, size)) {
		return EFAULT;
	}

	if (config & AMP_CONFIG_VSYNC_MEASURE) {
		if (hVppCtx->is_vsync_measure_enabled == 0) {
			hVppCtx->lastvsynctime = 0;
			hVppCtx->vsync_period_max = 0;
			hVppCtx->vsync_period_min = 0xFFFFFFFF;
			hVppCtx->is_vsync_measure_enabled = 1;
		}
	} else {
		hVppCtx->is_vsync_measure_enabled = 0;
	}

	return size;
}

static int amp_config_seq_show(struct seq_file *m, void *v)
{
	UINT32 config = 0;
	if (hVppCtx->is_vsync_measure_enabled) {
		config |= AMP_CONFIG_VSYNC_MEASURE;
	}

	seq_printf(m, "--------------------------------------\n");
	seq_printf(m, "| CONFIG |0x%08x                 |\n", config);
	seq_printf(m, "|        |BIT0: vsync measure enable |\n");
	seq_printf(m, "--------------------------------------\n");

	return 0;
}

static int amp_config_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, amp_config_seq_show, NULL);
}

static int amp_status_seq_show(struct seq_file *m, void *v)
{
	seq_printf(m, "-----------------------------------\n");
	seq_printf(m, "| VPP | IRQ:%-10d            |\n",
		   hVppCtx->vpp_cpcb0_vbi_int_cnt);
	seq_printf(m, "-----------------------------------\n");

	return 0;
}

static int amp_status_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, amp_status_seq_show, NULL);
}

static int amp_detail_seq_show(struct seq_file *m, void *v)
{

	seq_printf(m, "------------------------------------\n");

	if (hVppCtx->is_vsync_measure_enabled) {
		seq_printf(m, "|  VPP  | vsync | max:%-8d  ns |\n",
			   hVppCtx->vsync_period_max);
		seq_printf(m, "|       |       | min:%-8d  ns |\n",
			   hVppCtx->vsync_period_min);
		seq_printf(m, "------------------------------------\n");
	}
#if CONFIG_VDEC_IRQ_CHECKER
	seq_printf(m, "| VMETA | int    cnt | %-10d  |\n",
		   hVpuCtx->vdec_int_cnt);
	seq_printf(m, "|       | enable cnt | %-10d  |\n",
		   hVpuCtx->vdec_enable_int_cnt);
	seq_printf(m, "------------------------------------\n");
#endif

	return 0;
}

static int amp_detail_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, amp_detail_seq_show, NULL);
}

static int aout_status_seq_show(struct seq_file *m, void *v)
{
	if (!hAoutCtx)
		return 0;

	seq_printf(m,
		   "----------------------------------------------------------------------\n");
	seq_printf(m,
		   "| Path   | FIFO Info                   | Zero Buffer     | Underflow |\n");
	seq_printf(m,
		   "|        | <Size> < RD > < WR > <RD-K> | < Addr > <Size> | Counter   |\n");
	seq_printf(m,
		   "----------------------------------------------------------------------\n");
	if (hAoutCtx->p_ma_fifo) {
		seq_printf(m,
			   "| MULTI  |  %4u   %4u   %4u   %4u  | %08X  %4u  | %7u %u |\n",
			   hAoutCtx->p_ma_fifo->size,
			   hAoutCtx->p_ma_fifo->rd_offset,
			   hAoutCtx->p_ma_fifo->wr_offset,
			   hAoutCtx->p_ma_fifo->kernel_rd_offset,
			   hAoutCtx->p_ma_fifo->zero_buffer,
			   hAoutCtx->p_ma_fifo->zero_buffer_size,
			   hAoutCtx->p_ma_fifo->underflow_cnt,
			   hAoutCtx->p_ma_fifo->fifo_underflow);
	} else {
		seq_printf(m,
			   "| MULTI  |                          CLOSED                           |\n");
	}

	if (hAoutCtx->p_sa_fifo) {
		seq_printf(m,
			   "| STEREO |  %4u   %4u   %4u   %4u  | %08X  %4u  | %7u %u |\n",
			   hAoutCtx->p_sa_fifo->size,
			   hAoutCtx->p_sa_fifo->rd_offset,
			   hAoutCtx->p_sa_fifo->wr_offset,
			   hAoutCtx->p_sa_fifo->kernel_rd_offset,
			   hAoutCtx->p_sa_fifo->zero_buffer,
			   hAoutCtx->p_sa_fifo->zero_buffer_size,
			   hAoutCtx->p_sa_fifo->underflow_cnt,
			   hAoutCtx->p_sa_fifo->fifo_underflow);
	} else {
		seq_printf(m,
			   "| STEREO |                          CLOSED                           |\n");
	}

	if (hAoutCtx->p_spdif_fifo) {
		seq_printf(m,
			   "| S/PDIF |  %4u   %4u   %4u   %4u  | %08X  %4u  | %7u %u |\n",
			   hAoutCtx->p_spdif_fifo->size,
			   hAoutCtx->p_spdif_fifo->rd_offset,
			   hAoutCtx->p_spdif_fifo->wr_offset,
			   hAoutCtx->p_spdif_fifo->kernel_rd_offset,
			   hAoutCtx->p_spdif_fifo->zero_buffer,
			   hAoutCtx->p_spdif_fifo->zero_buffer_size,
			   hAoutCtx->p_spdif_fifo->underflow_cnt,
			   hAoutCtx->p_spdif_fifo->fifo_underflow);
	} else {
		seq_printf(m,
			   "| S/PDIF |                          CLOSED                           |\n");
	}

	if (hAoutCtx->p_hdmi_fifo) {
		seq_printf(m,
			   "| HDMI   |  %4u   %4u   %4u   %4u  | %08X  %4u  | %7u %u |\n",
			   hAoutCtx->p_hdmi_fifo->size,
			   hAoutCtx->p_hdmi_fifo->rd_offset,
			   hAoutCtx->p_hdmi_fifo->wr_offset,
			   hAoutCtx->p_hdmi_fifo->kernel_rd_offset,
			   hAoutCtx->p_hdmi_fifo->zero_buffer,
			   hAoutCtx->p_hdmi_fifo->zero_buffer_size,
			   hAoutCtx->p_hdmi_fifo->underflow_cnt,
			   hAoutCtx->p_hdmi_fifo->fifo_underflow);
	} else {
		seq_printf(m,
			   "| HDMI   |                          CLOSED                           |\n");
	}

	if (hAoutCtx->p_arc_fifo) {
		seq_printf(m,
			   "| ARC    |  %4u   %4u   %4u   %4u  | %08X  %4u  | %7u %u |\n",
			   hAoutCtx->p_arc_fifo->size,
			   hAoutCtx->p_arc_fifo->rd_offset,
			   hAoutCtx->p_arc_fifo->wr_offset,
			   hAoutCtx->p_arc_fifo->kernel_rd_offset,
			   hAoutCtx->p_arc_fifo->zero_buffer,
			   hAoutCtx->p_arc_fifo->zero_buffer_size,
			   hAoutCtx->p_arc_fifo->underflow_cnt,
			   hAoutCtx->p_arc_fifo->fifo_underflow);
	} else {
		seq_printf(m,
			   "| ARC    |                          CLOSED                           |\n");
	}
	seq_printf(m,
		   "----------------------------------------------------------------------\n");

	return 0;
}

static int aout_status_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, aout_status_seq_show, NULL);
}

/*******************************************************************************
  Module Register API
  */
static INT
amp_driver_setup_cdev(struct cdev *dev, INT major, INT minor,
		      struct file_operations *fops)
{
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	return cdev_add(dev, MKDEV(major, minor), 1);
}

static const struct file_operations amp_driver_config_fops = {
	.owner = THIS_MODULE,
	.open = amp_config_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = amp_config_proc_write,
};

static const struct file_operations amp_driver_status_fops = {
	.owner = THIS_MODULE,
	.open = amp_status_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations amp_driver_detail_fops = {
	.owner = THIS_MODULE,
	.open = amp_detail_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations amp_driver_aout_fops = {
	.open = aout_status_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


static INT amp_drv_init(struct amp_device_t *amp_device)
{
	INT res;

	/* Now setup cdevs. */
	res =
	    amp_driver_setup_cdev(&amp_device->cdev, amp_device->major,
				  amp_device->minor, amp_device->fops);
	if (res) {
		amp_error("amp_driver_setup_cdev failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}
	amp_trace("setup cdevs device minor [%d]\n", amp_device->minor);

	/* add PE devices to sysfs */
	amp_device->dev_class = class_create(THIS_MODULE, amp_device->dev_name);
	if (IS_ERR(amp_device->dev_class)) {
		amp_error("class_create failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	device_create(amp_device->dev_class, NULL,
		      MKDEV(amp_device->major, amp_device->minor), NULL,
		      amp_device->dev_name);
	amp_trace("create device sysfs [%s]\n", amp_device->dev_name);

	/* create hw device */
	if (amp_device->dev_init) {
		res = amp_device->dev_init(amp_device, 0);
		if (res != 0) {
			amp_error("amp_int_init failed !!! res = 0x%08X\n",
				  res);
			res = -ENODEV;
			goto err_add_device;
		}
	}

	/* create PE device proc file */
	amp_device->dev_procdir = proc_mkdir(amp_device->dev_name, NULL);
	amp_driver_config = proc_create(AMP_DEVICE_PROCFILE_CONFIG, 0644,
					amp_device->dev_procdir,
					&amp_driver_config_fops);
	amp_driver_state = proc_create(AMP_DEVICE_PROCFILE_STATUS, 0644,
				       amp_device->dev_procdir,
				       &amp_driver_status_fops);
	amp_driver_detail =
	    proc_create(AMP_DEVICE_PROCFILE_DETAIL, 0644,
			amp_device->dev_procdir, &amp_driver_detail_fops);
	amp_driver_aout =
	    proc_create(AMP_DEVICE_PROCFILE_AOUT, 0644, amp_device->dev_procdir,
			&amp_driver_aout_fops);

	return 0;

err_add_device:

	if (amp_device->dev_procdir) {
		remove_proc_entry(AMP_DEVICE_PROCFILE_AOUT,
				  amp_device->dev_procdir);
		remove_proc_entry(AMP_DEVICE_PROCFILE_DETAIL,
				  amp_device->dev_procdir);
		remove_proc_entry(AMP_DEVICE_PROCFILE_STATUS,
				  amp_device->dev_procdir);
		remove_proc_entry(AMP_DEVICE_PROCFILE_CONFIG,
				  amp_device->dev_procdir);
		remove_proc_entry(amp_device->dev_name, NULL);
	}

	if (amp_device->dev_class) {
		device_destroy(amp_device->dev_class,
			       MKDEV(amp_device->major, amp_device->minor));
		class_destroy(amp_device->dev_class);
	}

	cdev_del(&amp_device->cdev);

	return res;
}

static INT amp_drv_exit(struct amp_device_t *amp_device)
{
	INT res;

	amp_trace("amp_drv_exit [%s] enter\n", amp_device->dev_name);

	/* destroy kernel API */
	if (amp_device->dev_exit) {
		res = amp_device->dev_exit(amp_device, 0);
		if (res != 0)
			amp_error("dev_exit failed !!! res = 0x%08X\n", res);
	}

	if (amp_device->dev_procdir) {
		/* remove PE device proc file */
		remove_proc_entry(AMP_DEVICE_PROCFILE_AOUT,
				  amp_device->dev_procdir);
		remove_proc_entry(AMP_DEVICE_PROCFILE_DETAIL,
				  amp_device->dev_procdir);
		remove_proc_entry(AMP_DEVICE_PROCFILE_STATUS,
				  amp_device->dev_procdir);
		remove_proc_entry(AMP_DEVICE_PROCFILE_CONFIG,
				  amp_device->dev_procdir);
		remove_proc_entry(amp_device->dev_name, NULL);
	}

	if (amp_device->dev_class) {
		/* del sysfs entries */
		device_destroy(amp_device->dev_class,
			       MKDEV(amp_device->major, amp_device->minor));
		amp_trace("delete device sysfs [%s]\n", amp_device->dev_name);

		class_destroy(amp_device->dev_class);
	}
	/* del cdev */
	cdev_del(&amp_device->cdev);

	return 0;
}

static INT find_irq(char *name)
{
	INT i;

	for (i = 0; i < IRQ_AMP_MAX; i++) {
		if (strcmp(amp_irq_match[i], name) == 0)
			return i;
	}
	return IRQ_AMP_MAX;
}

static INT __init amp_module_init(VOID)
{
	INT i, res;
	dev_t pedev;
	struct device_node *np, *iter;
	struct resource r;
	u32 channel_num, chip_ext;
	const CHAR *vipstatus;
	const CHAR *teestatus;

	res = platform_driver_register(&amp_driver);
	if (res)
		return res;

	np = of_find_compatible_node(NULL, NULL, "marvell,berlin-amp");
	//get irq from DTS
	for_each_child_of_node(np, iter) {
		of_irq_to_resource(iter, 0, &r);
		i = find_irq((char *)r.name);
		if (i < IRQ_AMP_MAX) {
			amp_irqs[i] = r.start;
		}
	}
	for_each_child_of_node(np, iter) {
#ifdef CONFIG_MV_AMP_COMPONENT_SM_CEC_ENABLE
		if (of_device_is_compatible(iter, "marvell,berlin-cec")) {
			of_irq_to_resource(iter, 0, &r);
			amp_irqs[IRQ_SM_CEC] = r.start;
		}
#endif
#if (BERLIN_CHIP_VERSION == BERLIN_BG4_CDP)
	if (of_device_is_compatible(iter, "marvell,berlin-chip-ext")) {
		if (!of_property_read_u32(iter, "revision", &chip_ext))
			berlin_chip_revision = chip_ext;
	}
#endif
		if (of_device_is_compatible(iter, "marvell,berlin-avio")) {
			if (!of_property_read_u32(iter, "num", &channel_num))
				avioDhub_channel_num = channel_num;
		}

		if (of_device_is_compatible(iter, "marvell,berlin-vip")) {
			if (!of_property_read_string(iter, "status",
				&vipstatus)) {
				if (strcmp(vipstatus, "okay") == 0)
					gEnableVIP = 1;
			}
		}

		if (of_device_is_compatible(iter, "marvell,berlin-tee")) {
			if (!of_property_read_string(iter, "status",
				&teestatus)) {
				if (strcmp(teestatus, "enabled") == 0)
					gAmp_EnableTEE = 1;
			}
		}

	}
	of_node_put(np);

	res = alloc_chrdev_region(&pedev, 0, AMP_MAX_DEVS, AMP_DEVICE_NAME);
	amp_major = MAJOR(pedev);
	if (res < 0) {
		amp_error("alloc_chrdev_region() failed for amp\n");
		goto err_reg_device;
	}
	amp_trace("register cdev device major [%d]\n", amp_major);

	legacy_pe_dev.major = amp_major;
	res = amp_drv_init(&legacy_pe_dev);
	legacy_pe_dev.ionClient =
	    ion_client_create(ion_get_dev(), "amp_ion_client");
	amp_dev = legacy_pe_dev.cdev;

	res = amp_create_devioremap();
	if (res < 0) {
		amp_error("AMP ioremap fail!\n");
		goto err_reg_device;
	}
#if defined(CONFIG_MV_AMP_COMPONENT_CPM_ENABLE)
	cpm_module_init();
#endif
	amp_trace("amp_probe OK\n");
	return 0;
err_reg_device:
	amp_drv_exit(&legacy_pe_dev);

	unregister_chrdev_region(MKDEV(amp_major, 0), AMP_MAX_DEVS);

	platform_driver_unregister(&amp_driver);

	amp_trace("amp_probe failed !!! (%d)\n", res);

	return res;
}

static VOID __exit amp_module_exit(VOID)
{
	amp_trace("amp_remove\n");
	platform_driver_unregister(&amp_driver);

	amp_destroy_deviounmap();
	ion_client_destroy(legacy_pe_dev.ionClient);
	legacy_pe_dev.ionClient = NULL;
	amp_drv_exit(&legacy_pe_dev);

	unregister_chrdev_region(MKDEV(amp_major, 0), AMP_MAX_DEVS);
	amp_trace("unregister cdev device major [%d]\n", amp_major);
	amp_major = 0;

#if defined(CONFIG_MV_AMP_COMPONENT_CPM_ENABLE)
	cpm_module_exit();
#endif

	amp_trace("amp_remove OK\n");
}

static VOID amp_disable_irq(VOID)
{
	/* disable all enabled interrupt */

	if (gIsrEnState[IRQ_VPP_MODULE] & VPP_CEC_INT_EN) {
		/* disable CEC interrupt */
		disable_irq_nosync(amp_irqs[IRQ_SM_CEC]);
	}
	if (gIsrEnState[IRQ_VPP_MODULE] & VPP_INT_EN) {
		/* disable VPP interrupt */
		disable_irq_nosync(amp_irqs[IRQ_DHUBINTRAVIO0]);
	}
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	if (gIsrEnState[IRQ_AVIN_MODULE]) {
		/* disable AVIF interrupt */
		disable_irq_nosync(amp_irqs[IRQ_DHUBINTRAVIIF0]);
	}
#endif
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_VIP_ENABLE
	if (gIsrEnState[IRQ_VIP_MODULE]) {
		/* disable VIP interrupt */
		disable_irq_nosync(amp_irqs[IRQ_DHUBINTRAVIO2]);
	}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
	if (gIsrEnState[IRQ_VDEC_MODULE] & VMETA_INT_EN) {
		/* disable VMETA interrupt */
		disable_irq_nosync(amp_irqs[IRQ_VMETA]);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
	if (gIsrEnState[IRQ_VDEC_MODULE] & V2G_INT_EN) {
		/* disable V2G interrupt */
		disable_irq_nosync(amp_irqs[IRQ_V2G]);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_H1_ENABLE
	if (gIsrEnState[IRQ_VDEC_MODULE] & H1_INT_EN) {
		/* disable H1 interrupt */
		disable_irq_nosync(amp_irqs[IRQ_H1]);
	}
#endif
	if (gIsrEnState[IRQ_AOUT_MODULE]) {
		/* disable audio out interrupt */
		disable_irq_nosync(amp_irqs[IRQ_DHUBINTRAVIO1]);
	}
#ifdef CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
	if (gIsrEnState[IRQ_ZSP_MODULE]) {
		/* disable ZSP interrupt */
		disable_irq_nosync(amp_irqs[IRQ_ZSP]);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
	if (gIsrEnState[IRQ_OVP_MODULE]) {
		/* disable OVP interrupt */
		disable_irq_nosync(amp_irqs[IRQ_OVP]);
	}
#endif
	if (gIsrEnState[IRQ_TSP_MODULE]) {
		/* disable TSP interrupt */
		disable_irq_nosync(amp_irqs[IRQ_FIGO]);
	}
}

#ifdef AMP_ACTIVE_STANDBY
static VOID amp_enable_irq(VOID)
{
	/* disable all enabled interrupt */

	if (gIsrEnState[IRQ_VPP_MODULE] & VPP_CEC_INT_EN) {
		/* disable CEC interrupt */
		enable_irq(amp_irqs[IRQ_SM_CEC]);
	}
	if (gIsrEnState[IRQ_VPP_MODULE] & VPP_INT_EN) {
		/* disable VPP interrupt */
		enable_irq(amp_irqs[IRQ_DHUBINTRAVIO0]);
	}
#if defined(CONFIG_MV_AMP_COMPONENT_AVIN_ENABLE) || \
	defined(CONFIG_MV_AMP_COMPONENT_AIP_ENABLE)
#if (BERLIN_CHIP_VERSION != BERLIN_BG4_CDP)
	if (gIsrEnState[IRQ_AVIN_MODULE]) {
		/* disable AVIF interrupt */
		enable_irq(amp_irqs[IRQ_DHUBINTRAVIIF0]);
	}
#endif
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_VIP_ENABLE
	if (gIsrEnState[IRQ_VIP_MODULE]) {
		/* enable VIP interrupt */
		enable_irq(amp_irqs[IRQ_DHUBINTRAVIO2]);
	}
#endif

#ifdef CONFIG_MV_AMP_COMPONENT_VMETA_ENABLE
	if (gIsrEnState[IRQ_VDEC_MODULE] & VMETA_INT_EN) {
		/* disable VMETA interrupt */
		enable_irq(amp_irqs[IRQ_VMETA]);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_V2G_ENABLE
	if (gIsrEnState[IRQ_VDEC_MODULE] & V2G_INT_EN) {
		/* disable V2G interrupt */
		enable_irq(amp_irqs[IRQ_V2G]);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_H1_ENABLE
	if (gIsrEnState[IRQ_VDEC_MODULE] & H1_INT_EN) {
		/* disable H1 interrupt */
		enable_irq(amp_irqs[IRQ_H1]);
	}
#endif
	if (gIsrEnState[IRQ_AOUT_MODULE]) {
		/* disable audio out interrupt */
		enable_irq(amp_irqs[IRQ_DHUBINTRAVIO1]);
	}
#ifdef CONFIG_MV_AMP_COMPONENT_ZSP_ENABLE
	if (gIsrEnState[IRQ_ZSP_MODULE]) {
		/* disable ZSP interrupt */
		enable_irq(amp_irqs[IRQ_ZSP]);
	}
#endif
#ifdef CONFIG_MV_AMP_COMPONENT_OVP_ENABLE
	if (gIsrEnState[IRQ_OVP_MODULE]) {
		/* disable OVP interrupt */
		enable_irq(amp_irqs[IRQ_OVP]);
	}
#endif
	if (gIsrEnState[IRQ_TSP_MODULE]) {
		/* disable TSP interrupt */
		enable_irq(amp_irqs[IRQ_FIGO]);
	}
}
#endif

#ifdef AMP_ACTIVE_STANDBY
static VOID amp_cp_mod_control(UCHAR ctrl)
{
	if (ctrl) {
		cpm_cp_mod_restore(CPM_ACTIVE_STANDBY_MODE, CPM_CTRL_BOTH);
	} else {
		cpm_cp_mod_save(CPM_ACTIVE_STANDBY_MODE);
		cpm_cp_mod_disable(CPM_ACTIVE_STANDBY_MODE, CPM_CTRL_BOTH);
	}
}
#endif

int amp_get_vipenable_status(void)
{
	return gEnableVIP;
}

static VOID amp_shutdown(struct platform_device *pdev)
{
	amp_error("amp_shutdown\n");

	amp_disable_irq();
}

static INT amp_suspend(struct platform_device *pdev, pm_message_t state)
{
	amp_error("amp_suspend\n");

#ifdef AMP_ACTIVE_STANDBY
	amp_disable_irq();

	amp_vpp_cec_save_state(hVppCtx);

	amp_cp_mod_control(false);
#endif
	return 0;
}

static INT amp_resume(struct platform_device *pdev)
{
	amp_error("amp_resume\n");

#ifdef AMP_ACTIVE_STANDBY
	amp_cp_mod_control(true);

	amp_vpp_cec_restore_state(hVppCtx);

	amp_enable_irq();
#endif
	return 0;
}

#ifdef MODULE
module_init(amp_module_init);
module_exit(amp_module_exit);
#else
late_initcall_sync(amp_module_init);
#endif
/*******************************************************************************
    Module Descrption
*/
MODULE_AUTHOR("marvell");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("pe module template");
