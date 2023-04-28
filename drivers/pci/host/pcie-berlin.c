/*
 * PCIe host controller driver for Marvell Berlin SoCs
 *
 * Copyright (C) 2015 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * Author: Jisheng Zhang <jszhang@marvell.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>

#include "pcie-designware.h"

#define to_berlin_pcie(x)	container_of(x, struct berlin_pcie, pp)

enum pcie_berlin_type {
	PCIE_BERLIN,
	PCIE_BERLIN4CDP,
};

struct berlin_pcie {
	int			reset_gpio;
	int			power_gpio;
	int			enable_gpio;
	int			variant;
	struct pcie_port	pp;
	void __iomem		*pad_base;
	struct clk		*clk;
	struct reset_control	*rc_rst;
};

/* PCIe Port Logic registers */
#define PL_OFFSET 0x700
#define PL_DEBUG_R0 (PL_OFFSET + 0x28)
#define PL_DEBUG_R1 (PL_OFFSET + 0x2c)
#define PL_DEBUG_XMLH_LINK_IN_TRAINING	(1 << 29)
#define PL_DEBUG_XMLH_LINK_UP		(1 << 4)

#define MAC_INTR_STATUS		0xA00C
#define IRQ_INTA_ASSERT		(0x1 << 17)
#define IRQ_INTB_ASSERT		(0x1 << 18)
#define IRQ_INTC_ASSERT		(0x1 << 19)
#define IRQ_INTD_ASSERT		(0x1 << 20)
#define MAC_INTR_MASK		0xA010
#define MAC_CTRL		0xA014

static int berlin_pcie_power_on_phy(struct pcie_port *pp)
{
	u32 val;
	int count;
	struct berlin_pcie *berlin_pcie = to_berlin_pcie(pp);

	/* Turn On PCIE REF CLOCK BUFFER */
	writel(0x4560, berlin_pcie->pad_base);
	msleep(1);
	writel(0x4568, berlin_pcie->pad_base);

	writel(0x21, pp->dbi_base + 0x8704);
	writel(0x40, pp->dbi_base + 0x870c);
	msleep(1);
	writel(0x400, pp->dbi_base + 0x8094);
	writel(0x911, pp->dbi_base + 0x8098);
	writel(0xfc62, pp->dbi_base + 0x8004);
	writel(0x2a52, pp->dbi_base + 0x803c);
	writel(0x1060, pp->dbi_base + 0x8120);
	if (berlin_pcie->variant == PCIE_BERLIN4CDP)
		writel(0x02c0, pp->dbi_base + 0x813c);
	else
		writel(0xa0dd, pp->dbi_base + 0x813c);
	writel(0x107, pp->dbi_base + 0x8740);
	writel(0x6430, pp->dbi_base + 0x8008);
	writel(0x20, pp->dbi_base + 0x8704);

	count = 1000;
	while (count--) {
		val = readl(pp->dbi_base + 0x860C);
		if (val & 1)
			break;
		usleep_range(100, 1000);
	}
	if (!count) {
		dev_err(pp->dev, "PCIe comphy init Fail: %x\n", val);
		return -ETIMEDOUT;
	}

	return 0;
}

static int berlin_pcie_link_up(struct pcie_port *pp)
{
	u32 debug_r1 = readl(pp->dbi_base + PL_DEBUG_R1);
	if ((debug_r1 & PL_DEBUG_XMLH_LINK_UP) &&
		(!(debug_r1 & PL_DEBUG_XMLH_LINK_IN_TRAINING)))
		return 1;

	return 0;
}

static void berlin_pcie_enable_irq_pulse(struct pcie_port *pp)
{
	u32 val;

	/* enable INTX interrupt */
	val = ~(IRQ_INTA_ASSERT | IRQ_INTB_ASSERT |
		IRQ_INTC_ASSERT | IRQ_INTD_ASSERT),
	writel(val, pp->dbi_base + MAC_INTR_MASK);
	return;
}

static irqreturn_t berlin_pcie_irq_handler(int irq, void *arg)
{
	u32 val;
	struct pcie_port *pp = arg;

	val = readl(pp->dbi_base + MAC_INTR_STATUS);
	writel(val, pp->dbi_base + MAC_INTR_STATUS);
	return IRQ_HANDLED;
}

static void berlin_pcie_host_init(struct pcie_port *pp)
{
	u32 val;
	int ret, count = 0;
	struct berlin_pcie *berlin_pcie = to_berlin_pcie(pp);

	/* reset RC */
	reset_control_assert(berlin_pcie->rc_rst);
	reset_control_deassert(berlin_pcie->rc_rst);

	/* power up EP */
	if (gpio_is_valid(berlin_pcie->power_gpio)) {
		gpio_set_value_cansleep(berlin_pcie->power_gpio, 1);
		msleep(10);
	}

	if (gpio_is_valid(berlin_pcie->enable_gpio)) {
		gpio_set_value_cansleep(berlin_pcie->enable_gpio, 1);
		msleep(10);
	}

	/* disable link training and bypass PIPE clock gating logic */
	val = readl(pp->dbi_base + MAC_CTRL);
	val &= ~(1 << 5);
	writel(val, pp->dbi_base + MAC_CTRL);

	/* power on phy */
	ret = berlin_pcie_power_on_phy(pp);
	if (ret < 0)
		return;

	/* reset EP */
	if (gpio_is_valid(berlin_pcie->reset_gpio)) {
		gpio_set_value_cansleep(berlin_pcie->reset_gpio, 0);
		msleep(100);
		gpio_set_value_cansleep(berlin_pcie->reset_gpio, 1);
		msleep(100);
	}

	dw_pcie_setup_rc(pp);

	/* enable link training */
	val = readl(pp->dbi_base + MAC_CTRL);
	val |= (1 << 5);
	writel(val, pp->dbi_base + MAC_CTRL);

	/* wait for link up */
	count = 100;
	while (!berlin_pcie_link_up(pp)) {
		usleep_range(100, 1000);
		if (--count)
			continue;

		dev_err(pp->dev, "PCIe Link Fail\n");
		dev_dbg(pp->dev, "DEBUG_R0: 0x%08x, DEBUG_R1: 0x%08x\n",
			readl(pp->dbi_base + PL_DEBUG_R0),
			readl(pp->dbi_base + PL_DEBUG_R1));
		return;
	}

	val = readl(pp->dbi_base + 0x8608);
	val &= 0x30;
	dev_info(pp->dev, "Link up, Gen=%d\n", (val == 0x30) ? 2 : 1);
	berlin_pcie_enable_irq_pulse(pp);
}

static struct pcie_host_ops berlin_pcie_host_ops = {
	.link_up = berlin_pcie_link_up,
	.host_init = berlin_pcie_host_init,
};

static int berlin_add_pcie_port(struct pcie_port *pp,
				struct platform_device *pdev)
{
	int ret;

	pp->irq = platform_get_irq(pdev, 0);
	if (!pp->irq) {
		dev_err(&pdev->dev, "failed to get irq\n");
		return -ENODEV;
	}
	ret = devm_request_irq(&pdev->dev, pp->irq, berlin_pcie_irq_handler,
				IRQF_SHARED, "berlin-pcie", pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to request irq\n");
		return ret;
	}

	pp->root_bus_nr = 0;
	pp->ops = &berlin_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id berlin_pcie_of_match[] = {
	{
		.compatible = "marvell,berlin-pcie",
		.data = (void *)PCIE_BERLIN
	},
	{
		.compatible = "marvell,berlin4cdp-pcie",
		.data = (void *)PCIE_BERLIN4CDP
	},
	{},
};
MODULE_DEVICE_TABLE(of, berlin_pcie_of_match);

static int berlin_pcie_probe(struct platform_device *pdev)
{
	struct berlin_pcie *berlin_pcie;
	struct pcie_port *pp;
	struct device_node *np = pdev->dev.of_node;
	struct resource *dbi_base;
	struct resource *pad_base;
	const struct of_device_id *of_id;
	int ret;

	berlin_pcie = devm_kzalloc(&pdev->dev, sizeof(*berlin_pcie),
				GFP_KERNEL);
	if (!berlin_pcie)
		return -ENOMEM;

	of_id = of_match_device(berlin_pcie_of_match, &pdev->dev);
	if (of_id)
		berlin_pcie->variant = (enum pcie_berlin_type)of_id->data;
	else
		berlin_pcie->variant = PCIE_BERLIN;

	pp = &berlin_pcie->pp;

	pp->dev = &pdev->dev;

	berlin_pcie->rc_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(berlin_pcie->rc_rst))
		return PTR_ERR(berlin_pcie->rc_rst);

	berlin_pcie->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);
	berlin_pcie->power_gpio = of_get_named_gpio(np, "power-gpio", 0);
	berlin_pcie->enable_gpio = of_get_named_gpio(np, "enable-gpio", 0);
	if (berlin_pcie->reset_gpio == -EPROBE_DEFER ||
			berlin_pcie->power_gpio == -EPROBE_DEFER ||
			berlin_pcie->enable_gpio == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (gpio_is_valid(berlin_pcie->reset_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev,
					    berlin_pcie->reset_gpio,
					    GPIOF_DIR_OUT, "PCIe reset");
		if (ret) {
			dev_err(&pdev->dev, "unable to get reset gpio\n");
			return ret;
		}
	}

	if (gpio_is_valid(berlin_pcie->power_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev,
					    berlin_pcie->power_gpio,
					    GPIOF_DIR_OUT, "PCIe EP Power");
		if (ret) {
			dev_err(&pdev->dev, "unable to get power gpio\n");
			return ret;
		}
	}

	berlin_pcie->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(berlin_pcie->clk)) {
		dev_err(&pdev->dev, "Failed to get pcie rc clock\n");
		return PTR_ERR(berlin_pcie->clk);
	}
	ret = clk_prepare_enable(berlin_pcie->clk);
	if (ret)
		return ret;

	dbi_base = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	pp->dbi_base = devm_ioremap_resource(&pdev->dev, dbi_base);
	if (IS_ERR(pp->dbi_base)) {
		ret = PTR_ERR(pp->dbi_base);
		goto fail_clk;
	}

	pad_base = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pad");
	berlin_pcie->pad_base = devm_ioremap_resource(&pdev->dev, pad_base);
	if (IS_ERR(berlin_pcie->pad_base)) {
		ret = PTR_ERR(berlin_pcie->pad_base);
		goto fail_clk;
	}

	ret = berlin_add_pcie_port(pp, pdev);
	if (ret < 0)
		goto fail_clk;

	platform_set_drvdata(pdev, berlin_pcie);
	return 0;

fail_clk:
	clk_disable_unprepare(berlin_pcie->clk);
	return ret;
}

static int berlin_pcie_remove(struct platform_device *pdev)
{
	struct berlin_pcie *berlin_pcie = platform_get_drvdata(pdev);

	clk_disable_unprepare(berlin_pcie->clk);

	return 0;
}

static struct platform_driver berlin_pcie_driver = {
	.probe = berlin_pcie_probe,
	.remove = berlin_pcie_remove,
	.driver = {
		.name = "berlin-pcie",
		.owner = THIS_MODULE,
		.of_match_table = berlin_pcie_of_match,
	},
};
module_platform_driver(berlin_pcie_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@marvell.com>");
MODULE_DESCRIPTION("Marvell Berlin PCIe host controller driver");
MODULE_LICENSE("GPL v2");
