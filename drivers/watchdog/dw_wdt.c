/*
 * Copyright 2010-2011 Picochip Ltd., Jamie Iles
 * http://www.picochip.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This file implements a driver for the Synopsys DesignWare watchdog device
 * in the many subsystems. The watchdog has 16 different timeout periods
 * and these are a function of the input clock frequency.
 *
 * The DesignWare watchdog cannot be stopped once it has been started so we
 * use a software timer to implement a ping that will keep the watchdog alive.
 * If we receive an expected close for the watchdog then we keep the timer
 * running, otherwise the timer is stopped and the watchdog will expire.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/timer.h>
#include <linux/watchdog.h>

#define WDOG_CONTROL_REG_OFFSET		    0x00
#define WDOG_CONTROL_REG_WDT_EN_MASK	    0x01
#define WDOG_TIMEOUT_RANGE_REG_OFFSET	    0x04
#define WDOG_TIMEOUT_RANGE_TOPINIT_SHIFT    4
#define WDOG_CURRENT_COUNT_REG_OFFSET	    0x08
#define WDOG_COUNTER_RESTART_REG_OFFSET     0x0c
#define WDOG_COUNTER_RESTART_KICK_VALUE	    0x76

/* The maximum TOP (timeout period) value that can be set in the watchdog. */
#define DW_WDT_MAX_TOP		15

#define DW_WDT_DEFAULT_SECONDS	30

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started "
		 "(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

#define WDT_TIMEOUT		(HZ / 2)

struct dw_wdt_device {
	void __iomem		*regs;
	struct clk		*clk;
	struct timer_list	timer;
	struct watchdog_device	wdog;
	struct notifier_block	restart_handler;
};

static inline bool dw_wdt_is_enabled(struct dw_wdt_device *wdev)
{
	return readl(wdev->regs + WDOG_CONTROL_REG_OFFSET) &
		WDOG_CONTROL_REG_WDT_EN_MASK;
}

static inline unsigned int dw_wdt_top_in_seconds(struct dw_wdt_device *wdev,
						 unsigned int top)
{
	/*
	 * There are 16 possible timeout values in 0..15 where the number of
	 * cycles is 2 ^ (16 + i) and the watchdog counts down.
	 */
	return (1U << (16 + top)) / clk_get_rate(wdev->clk);
}

static unsigned int dw_wdt_get_timeleft(struct watchdog_device *wdog)
{
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	return readl(wdev->regs + WDOG_CURRENT_COUNT_REG_OFFSET) /
		clk_get_rate(wdev->clk);
}

static int dw_wdt_ping(struct watchdog_device *wdog)
{
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	writel(WDOG_COUNTER_RESTART_KICK_VALUE, wdev->regs +
	       WDOG_COUNTER_RESTART_REG_OFFSET);

	return 0;
}

static int dw_wdt_set_timeout(struct watchdog_device *wdog,
			      unsigned int new_timeout)
{
	unsigned int i, top_val = DW_WDT_MAX_TOP;
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	/*
	 * Iterate over the timeout values until we find the closest match. We
	 * always look for >=.
	 */
	for (i = 0; i <= DW_WDT_MAX_TOP; ++i)
		if (dw_wdt_top_in_seconds(wdev, i) >= new_timeout) {
			top_val = i;
			break;
		}

	/*
	 * Set the new value in the watchdog.  Some versions of dw_wdt
	 * have have TOPINIT in the TIMEOUT_RANGE register (as per
	 * CP_WDT_DUAL_TOP in WDT_COMP_PARAMS_1).  On those we
	 * effectively get a pat of the watchdog right here.
	 */
	writel(top_val | top_val << WDOG_TIMEOUT_RANGE_TOPINIT_SHIFT,
		wdev->regs + WDOG_TIMEOUT_RANGE_REG_OFFSET);

	/*
	 * Add an explicit pat to handle versions of the watchdog that
	 * don't have TOPINIT.  This won't hurt on versions that have
	 * it.
	 */
	dw_wdt_ping(wdog);

	return 0;
}

static int dw_wdt_restart_handle(struct notifier_block *this,
				unsigned long mode, void *cmd)
{
	u32 val;
	struct dw_wdt_device *wdev = container_of(this,
						  struct dw_wdt_device,
						  restart_handler);

	writel(0, wdev->regs + WDOG_TIMEOUT_RANGE_REG_OFFSET);
	val = readl(wdev->regs + WDOG_CONTROL_REG_OFFSET);
	if (val & WDOG_CONTROL_REG_WDT_EN_MASK)
		writel(WDOG_COUNTER_RESTART_KICK_VALUE, wdev->regs +
			WDOG_COUNTER_RESTART_REG_OFFSET);
	else
		writel(WDOG_CONTROL_REG_WDT_EN_MASK,
		       wdev->regs + WDOG_CONTROL_REG_OFFSET);

	/* wait for reset to assert... */
	mdelay(500);

	return NOTIFY_DONE;
}

static void dw_wdt_timer_ping(unsigned long arg)
{
	struct watchdog_device *wdog = (struct watchdog_device *)arg;
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	/* ping it every wdog->timeout / 2 seconds to prevent reboot */
	dw_wdt_ping(wdog);
	mod_timer(&wdev->timer, jiffies + wdog->timeout * HZ / 2);
}

static int dw_wdt_start(struct watchdog_device *wdog)
{
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	if (!dw_wdt_is_enabled(wdev)) {
		dw_wdt_set_timeout(wdog, wdog->timeout);
		writel(WDOG_CONTROL_REG_WDT_EN_MASK,
		       wdev->regs + WDOG_CONTROL_REG_OFFSET);
	} else {
		/* delete the timer that pings the watchdog after close */
		del_timer_sync(&wdev->timer);
		dw_wdt_set_timeout(wdog, wdog->timeout);
	}

	return 0;
}

static int dw_wdt_stop(struct watchdog_device *wdog)
{
	/*
	 * We don't need a clk_disable_unprepare, the wdt cannot be disabled
	 * once started. We use a timer to ping the watchdog while the
	 * /dev/watchdog is closed
	 */
	dw_wdt_timer_ping((unsigned long)wdog);
	return 0;
}

static inline void dw_wdt_ping_if_active(struct watchdog_device *wdog)
{
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	if (dw_wdt_is_enabled(wdev)) {
		dw_wdt_set_timeout(wdog, wdog->timeout);
		dw_wdt_timer_ping((unsigned long)wdog);
	}
}

static const struct watchdog_info dw_wdt_ident = {
	.options	= WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT |
			  WDIOF_MAGICCLOSE,
	.identity	= "Synopsys DesignWare Watchdog",
};

static const struct watchdog_ops dw_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= dw_wdt_start,
	.stop		= dw_wdt_stop,
	.ping		= dw_wdt_ping,
	.set_timeout	= dw_wdt_set_timeout,
	.get_timeleft	= dw_wdt_get_timeleft,
};

#ifdef CONFIG_PM_SLEEP
static int dw_wdt_suspend(struct device *dev)
{
	struct watchdog_device *wdog = dev_get_drvdata(dev);
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	clk_disable_unprepare(wdev->clk);

	return 0;
}

static int dw_wdt_resume(struct device *dev)
{
	struct watchdog_device *wdog = dev_get_drvdata(dev);
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	int err = clk_prepare_enable(wdev->clk);

	if (err)
		return err;

	dw_wdt_ping(wdog);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(dw_wdt_pm_ops, dw_wdt_suspend, dw_wdt_resume);

static int dw_wdt_drv_probe(struct platform_device *pdev)
{
	int ret;
	struct dw_wdt_device *wdev;
	struct watchdog_device *wdog;
	struct resource *mem;

	wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	wdev->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(wdev->regs))
		return PTR_ERR(wdev->regs);

	wdev->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(wdev->clk))
		return PTR_ERR(wdev->clk);

	wdog			= &wdev->wdog;
	wdog->info		= &dw_wdt_ident;
	wdog->ops		= &dw_wdt_ops;
	wdog->min_timeout	= dw_wdt_top_in_seconds(wdev, 0);
	wdog->max_timeout	= dw_wdt_top_in_seconds(wdev, DW_WDT_MAX_TOP);

	ret = clk_prepare_enable(wdev->clk);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, wdog);
	watchdog_set_drvdata(wdog, wdev);
	watchdog_set_nowayout(wdog, nowayout);
	watchdog_init_timeout(wdog, wdog->max_timeout, &pdev->dev);

	wdev->restart_handler.notifier_call = dw_wdt_restart_handle;
	wdev->restart_handler.priority = 128;
	ret = register_restart_handler(&wdev->restart_handler);
	if (ret)
		dev_warn(&pdev->dev, "cannot register restart handler\n");

	setup_timer(&wdev->timer, dw_wdt_timer_ping, (unsigned long)wdog);

	dw_wdt_ping_if_active(wdog);

	ret = watchdog_register_device(wdog);
	if (ret) {
		dev_err(&pdev->dev, "cannot register watchdog device\n");
		goto out_disable_clk;
	}

	return 0;

out_disable_clk:
	clk_disable_unprepare(wdev->clk);

	return ret;
}

static int dw_wdt_drv_remove(struct platform_device *pdev)
{
	struct watchdog_device *wdog = platform_get_drvdata(pdev);
	struct dw_wdt_device *wdev = watchdog_get_drvdata(wdog);

	unregister_restart_handler(&wdev->restart_handler);

	watchdog_unregister_device(wdog);

	clk_disable_unprepare(wdev->clk);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dw_wdt_of_match[] = {
	{ .compatible = "snps,dw-wdt", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, dw_wdt_of_match);
#endif

static struct platform_driver dw_wdt_driver = {
	.probe		= dw_wdt_drv_probe,
	.remove		= dw_wdt_drv_remove,
	.driver		= {
		.name	= "dw_wdt",
		.of_match_table = of_match_ptr(dw_wdt_of_match),
		.pm	= &dw_wdt_pm_ops,
	},
};

module_platform_driver(dw_wdt_driver);

MODULE_AUTHOR("Jamie Iles");
MODULE_DESCRIPTION("Synopsys DesignWare Watchdog Driver");
MODULE_LICENSE("GPL");
