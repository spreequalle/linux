/*
 * Copyright (C) 2014 Marvell Technology Group Ltd.
 *
 * Author:	Benson Gui <benson@marvell.com>
 * 		Dejin Zheng <devinzh@marvell.com>
 *		Jisheng Zhang <jszhang@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

struct phy_berlin4_usb2_priv {
	void __iomem		*base;
};

#define USB2_PLL_REG0		0x0
#define USB2_PLL_REG1		0x4
#define USB2_CAL_CTRL		0x8
#define USB2_TX_CH_CTRL0	0xc
#define USB2_DIG_REG0		0x1c
#define USB2_DIG_REG1		0x20
#define USB2_OTG_REG0		0x34
#define USB2_CHARGER_REG0	0x38

#define PU_PLL_BY_REG 		BIT(1)
#define PU_PLL			BIT(0)

#define OTG_CONTROL_BY_PIN	BIT(5)
#define PU_OTG			BIT(4)

static int phy_berlin4_usb2_power_on(struct phy *phy)
{
	u32 data, timeout;
	struct phy_berlin4_usb2_priv *priv = phy_get_drvdata(phy);

	/* powering up OTG */
	data = readl(priv->base + USB2_OTG_REG0);
	data |= 1<<4;
	writel(data, priv->base + USB2_OTG_REG0);

	/* powering Charger detector circuit */
	data = readl(priv->base + USB2_CHARGER_REG0);
	data |= 1<<5;
	writel(data, priv->base + USB2_CHARGER_REG0);

	data = readl(priv->base + USB2_DIG_REG1);
	data |= 1<<10;
	writel(data, priv->base + USB2_DIG_REG1);

	/* Power up analog port */
	data = 0x03BE7F6F;
	writel(data, priv->base + USB2_TX_CH_CTRL0);

	/* Squelch setting */
	data = 0xC39816CE;
	writel(data, priv->base + USB2_DIG_REG0);

	/* Impedance calibration */
	data = 0xf5930488;
	writel(data, priv->base + USB2_CAL_CTRL);

	data = readl(priv->base + USB2_DIG_REG1);
	data &= 0xfffffbff;
	writel(data, priv->base + USB2_DIG_REG1);

	/* Configuring FBDIV for SOC Clk 25 Mhz */
	data = readl(priv->base + USB2_PLL_REG0);
	data &= 0xce00ff80;
	data |= 5 | (0x60 << 16) | (0 << 28);
	writel(data, priv->base + USB2_PLL_REG0);

	/* Power up PLL, Reset, Suspen_en disable */
	writel(0x407, priv->base + USB2_PLL_REG1);
	udelay(100);

	/* Deassert Reset */
	writel(0x403, priv->base + USB2_PLL_REG1);

	/* Wait for PLL Lock */
	timeout = 0x1000000;
	do {
		data = readl(priv->base + USB2_PLL_REG0);
		if (!--timeout)
			break;
	} while (!(data&0x80000000));

	if (!timeout)
		printk(KERN_ERR "ERROR: USB PHY PLL NOT LOCKED!\n");

	return 0;
}

static int phy_berlin4_usb2_power_down(struct phy *phy)
{
	u32 data;
	struct phy_berlin4_usb2_priv *priv = phy_get_drvdata(phy);

	/* powering down OTG */
	data = readl(priv->base + USB2_OTG_REG0);
	data &= ~OTG_CONTROL_BY_PIN;
	data &= ~PU_OTG;
	writel(data, priv->base + USB2_OTG_REG0);

	data = readl(priv->base + USB2_PLL_REG1);
	data |= PU_PLL_BY_REG;
	data &= ~PU_PLL;
	writel(data, priv->base + USB2_PLL_REG1);
	return 0;
}

static const struct phy_ops phy_berlin4_usb2_ops = {
	.power_on	= phy_berlin4_usb2_power_on,
	.power_off	= phy_berlin4_usb2_power_down,
	.owner		= THIS_MODULE,
};

static const struct of_device_id phy_berlin4_usb2_of_match[] = {
	{
		.compatible = "marvell,berlin4ct-usb2-phy",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, phy_berlin4_usb2_of_match);

static int phy_berlin4_usb2_probe(struct platform_device *pdev)
{
	struct phy_berlin4_usb2_priv *priv;
	struct resource *res;
	struct phy *phy;
	struct phy_provider *phy_provider;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	phy = devm_phy_create(&pdev->dev, NULL, &phy_berlin4_usb2_ops);
	if (IS_ERR(phy)) {
		dev_err(&pdev->dev, "failed to create PHY\n");
		return PTR_ERR(phy);
	}

	phy_set_drvdata(phy, priv);

	phy_provider =
		devm_of_phy_provider_register(&pdev->dev, of_phy_simple_xlate);
	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct platform_driver phy_berlin4_usb2_driver = {
	.probe	= phy_berlin4_usb2_probe,
	.driver	= {
		.name		= "phy-berlin4-usb2",
		.of_match_table	= phy_berlin4_usb2_of_match,
	},
};
module_platform_driver(phy_berlin4_usb2_driver);

MODULE_AUTHOR("Benson Gui <benson@marvell.com>");
MODULE_AUTHOR("Jisheng Zhang <jszhang@marvell.com>");
MODULE_DESCRIPTION("Marvell Berlin4 PHY driver for USB2.0");
MODULE_LICENSE("GPL v2");
