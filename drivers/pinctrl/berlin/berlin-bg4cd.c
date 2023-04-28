/*
 * Marvell Berlin BG4CD pinctrl driver
 *
 * Copyright (C) 2015 Marvell Technology Group Ltd.
 *
 * Jisheng Zhang <jszhang@marvell.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "berlin.h"

static const struct berlin_desc_group berlin4cd_soc_pinctrl_groups[] = {
	BERLIN_PINCTRL_GROUP("USB0_DRV_VBUS", 0x0, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "usb0"), /* VBUS */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO39 */
	BERLIN_PINCTRL_GROUP("SD0_CDn", 0x0, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO37 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* CDn */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm"), /* DI0 */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm"), /* pwm0 */
			BERLIN_PINCTRL_FUNCTION(0x5, "i2s2")), /* DI0 */
	BERLIN_PINCTRL_GROUP("SD0_WP", 0x0, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO38 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* WP */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm"), /* DI1 */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x5, "i2s2"), /* DI1 */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s1"), /* DO */
	BERLIN_PINCTRL_GROUP("SD0_CLK", 0x0, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO31 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0")),
	BERLIN_PINCTRL_GROUP("SD0_DAT0", 0x0, 0x3, 0x0c,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO32 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0")),
	BERLIN_PINCTRL_GROUP("SD0_DAT1", 0x0, 0x3, 0x0f,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO33 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0")),
	BERLIN_PINCTRL_GROUP("SD0_DAT2", 0x0, 0x3, 0x12,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO34 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0")),
	BERLIN_PINCTRL_GROUP("SD0_DAT3", 0x0, 0x3, 0x15,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO35 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0")),
	BERLIN_PINCTRL_GROUP("SD0_CMD", 0x0, 0x3, 0x18,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO36 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0")),
	BERLIN_PINCTRL_GROUP("GPIO_D", 0x0, 0x3, 0x1b,
			BERLIN_PINCTRL_FUNCTION(0x0, "tw2"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")),
	BERLIN_PINCTRL_GROUP("GPIO_E", 0x4, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "tw2"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")),
	BERLIN_PINCTRL_GROUP("GPIO_F", 0x4, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"),
			BERLIN_PINCTRL_FUNCTION(0x1, "urt0")), /* RXD1 */
	BERLIN_PINCTRL_GROUP("GPIO_G", 0x4, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"),
			BERLIN_PINCTRL_FUNCTION(0x1, "urt0")), /* TXD1 */
	BERLIN_PINCTRL_GROUP("NAND_IO0", 0x4, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* DATA0 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO16 */
	BERLIN_PINCTRL_GROUP("NAND_IO1", 0x4, 0x3, 0x0c,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* DATA1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO17 */
	BERLIN_PINCTRL_GROUP("NAND_IO2", 0x4, 0x3, 0x0f,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* DATA2 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO18 */
	BERLIN_PINCTRL_GROUP("NAND_IO3", 0x4, 0x3, 0x12,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* DATA3 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO19 */
	BERLIN_PINCTRL_GROUP("NAND_IO4", 0x4, 0x3, 0x15,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* DATA4 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO20 */
	BERLIN_PINCTRL_GROUP("NAND_IO5", 0x4, 0x3, 0x18,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* DATA5 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO21 */
	BERLIN_PINCTRL_GROUP("NAND_IO6", 0x4, 0x3, 0x1b,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* DATA6 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO22 */
	BERLIN_PINCTRL_GROUP("NAND_IO7", 0x8, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* DATA7 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO23 */
	BERLIN_PINCTRL_GROUP("NAND_ALE", 0x8, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO24 */
	BERLIN_PINCTRL_GROUP("NAND_CLE", 0x8, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO25 */
	BERLIN_PINCTRL_GROUP("NAND_WEn", 0x8, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO26 */
	BERLIN_PINCTRL_GROUP("NAND_REn", 0x8, 0x3, 0x0c,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO27 */
	BERLIN_PINCTRL_GROUP("NAND_WPn", 0x8, 0x3, 0x0f,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO28 */
	BERLIN_PINCTRL_GROUP("NAND_CEn", 0x8, 0x3, 0x12,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* RSTn */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO29 */
	BERLIN_PINCTRL_GROUP("NAND_RDY", 0x8, 0x3, 0x15,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"),
			BERLIN_PINCTRL_FUNCTION(0x1, "emmc"), /* CMD */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO30 */
	BERLIN_PINCTRL_GROUP("SPI1_SS0n", 0x8, 0x3, 0x18,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO5 */
	BERLIN_PINCTRL_GROUP("SPI1_SS1n", 0x8, 0x3, 0x1b,
			BERLIN_PINCTRL_FUNCTION(0x0, "urt0"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"),
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO6 */
			BERLIN_PINCTRL_FUNCTION(0x4, "urt1"), /* RXD2 */
			BERLIN_PINCTRL_FUNCTION(0x5, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x6, "tw0")), /* SCL */
	BERLIN_PINCTRL_GROUP("SPI1_SS2n", 0xc, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "urt0"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"),
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO7 */
			BERLIN_PINCTRL_FUNCTION(0x4, "urt1"), /* TXD2 */
			BERLIN_PINCTRL_FUNCTION(0x5, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x6, "tw0")), /* SDA */
	BERLIN_PINCTRL_GROUP("SPI1_SCLK", 0xc, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO8 */
			BERLIN_PINCTRL_FUNCTION(0x5, "pwm")), /* PWM2 */
	BERLIN_PINCTRL_GROUP("SPI1_SDO", 0xc, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO9 */
			BERLIN_PINCTRL_FUNCTION(0x2, "urt1"), /* TXD1 */
			BERLIN_PINCTRL_FUNCTION(0x5, "pwm")), /* PWM3 */
	BERLIN_PINCTRL_GROUP("SPI1_SDI", 0xc, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO10 */
			BERLIN_PINCTRL_FUNCTION(0x2, "urt1")), /* RXD1 */
	BERLIN_PINCTRL_GROUP("TW1_SCL", 0x10, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "hdmi"), /* TX_EDDC */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw1"),
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio")), /* AV GPIO0 */
	BERLIN_PINCTRL_GROUP("TW1_SDA", 0x10, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "hdmi"), /* TX_EDDC */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw1"),
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio")), /* AV GPIO1 */
	BERLIN_PINCTRL_GROUP("HDMI_CEC", 0x10, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "hdmi"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* AV GPIO2 */
	BERLIN_PINCTRL_GROUP("HDMI_HPD", 0x10, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "hdmi"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* AV GPIO3 */
};

static const struct berlin_desc_group berlin4cd_sysmgr_pinctrl_groups[] = {
	BERLIN_PINCTRL_GROUP("SM_TRSTn", 0x0, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "sm"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO3 */
			BERLIN_PINCTRL_FUNCTION(0x2, "usb0")), /* VBUS */
	BERLIN_PINCTRL_GROUP("SM_TMS", 0x0, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "sm"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO0 */
			BERLIN_PINCTRL_FUNCTION(0x3, "urt1"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x5, "i2s2"), /* BCLKI */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s1")), /* BCLKO */
	BERLIN_PINCTRL_GROUP("SM_TDI", 0x0, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "sm"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "urt1"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x5, "i2s2"), /* LRCKI */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s1")), /* LRCKO */
	BERLIN_PINCTRL_GROUP("SM_TDO", 0x0, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "sm"),
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO2 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pdm"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x5, "i2s2"), /* MCLK */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s1")), /* MCLK */
};

static const struct berlin_pinctrl_desc berlin4cd_soc_pinctrl_data = {
	.groups = berlin4cd_soc_pinctrl_groups,
	.ngroups = ARRAY_SIZE(berlin4cd_soc_pinctrl_groups),
};

static const struct berlin_pinctrl_desc berlin4cd_sysmgr_pinctrl_data = {
	.groups = berlin4cd_sysmgr_pinctrl_groups,
	.ngroups = ARRAY_SIZE(berlin4cd_sysmgr_pinctrl_groups),
};

static const struct of_device_id berlin4cd_pinctrl_match[] = {
	{
		.compatible = "marvell,berlin4cd-pinctrl",
		.data = &berlin4cd_soc_pinctrl_data,
	},
	{
		.compatible = "marvell,berlin4cd-sm-pinctrl",
		.data = &berlin4cd_sysmgr_pinctrl_data,
	},
	{}
};
MODULE_DEVICE_TABLE(of, berlin4cd_pinctrl_match);

static int berlin4cd_pinctrl_probe(struct platform_device *pdev)
{
	const struct of_device_id *match =
		of_match_device(berlin4cd_pinctrl_match, &pdev->dev);
	struct regmap_config *rmconfig;
	struct regmap *regmap;
	struct resource *res;
	void __iomem *base;

	rmconfig = devm_kzalloc(&pdev->dev, sizeof(*rmconfig), GFP_KERNEL);
	if (!rmconfig)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	rmconfig->reg_bits = 32,
	rmconfig->val_bits = 32,
	rmconfig->reg_stride = 4,
	rmconfig->max_register = resource_size(res);

	regmap = devm_regmap_init_mmio(&pdev->dev, base, rmconfig);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	return berlin_pinctrl_probe(pdev, match->data);
}

static struct platform_driver berlin4cd_pinctrl_driver = {
	.probe	= berlin4cd_pinctrl_probe,
	.driver	= {
		.name = "berlin-bg4cd-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = berlin4cd_pinctrl_match,
	},
};
module_platform_driver(berlin4cd_pinctrl_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@marvell.com>");
MODULE_DESCRIPTION("Marvell Berlin BG4CD pinctrl driver");
MODULE_LICENSE("GPL");
