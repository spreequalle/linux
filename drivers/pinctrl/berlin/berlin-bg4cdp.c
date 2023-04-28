/*
 * Marvell Berlin BG4CDP pinctrl driver
 *
 * Copyright (C) 2016 Marvell Technology Group Ltd.
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

static const struct berlin_desc_group berlin4cdp_soc_pinctrl_groups[] = {
	BERLIN_PINCTRL_GROUP("SPI1_SDI", 0x0, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SDI */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO0 */
	BERLIN_PINCTRL_GROUP("SPI1_SDO", 0x0, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SDO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO1 */
			BERLIN_PINCTRL_FUNCTION(0x5, "pwm")), /* PWM3 */
	BERLIN_PINCTRL_GROUP("SPI1_SCLK", 0x0, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SCLK */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO2 */
			BERLIN_PINCTRL_FUNCTION(0x5, "pwm")), /* PWM1 */
	BERLIN_PINCTRL_GROUP("SPI1_SS2n", 0x0, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "urt0"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"), /* SS2n */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio"), /* GPIO3 */
			BERLIN_PINCTRL_FUNCTION(0x5, "pwm"), /* PWM2 */
			BERLIN_PINCTRL_FUNCTION(0x6, "tw0")), /* SDA */
	BERLIN_PINCTRL_GROUP("SPI1_SS1n", 0x0, 0x3, 0x0c,
			BERLIN_PINCTRL_FUNCTION(0x0, "urt0"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x1, "spi1"), /* SS2n */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio"), /* GPIO4 */
			BERLIN_PINCTRL_FUNCTION(0x5, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x6, "tw0")), /* SCL */
	BERLIN_PINCTRL_GROUP("SPI1_SS0n", 0x0, 0x3, 0x0f,
			BERLIN_PINCTRL_FUNCTION(0x0, "spi1"), /* SS0n */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* GPIO5 */
	BERLIN_PINCTRL_GROUP("NAND_RDY", 0x0, 0x3, 0x12,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* RDY */
			BERLIN_PINCTRL_FUNCTION(0x2, "i2s1a"), /* DO */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO6 */
	BERLIN_PINCTRL_GROUP("NAND_CEn", 0x0, 0x3, 0x15,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* CEn */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO7 */
			BERLIN_PINCTRL_FUNCTION(0x4, "avio")), /* DBG3 */
	BERLIN_PINCTRL_GROUP("NAND_WPn", 0x0, 0x3, 0x18,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* WPn */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO8 */
			BERLIN_PINCTRL_FUNCTION(0x4, "avio")), /* DBG2 */
	BERLIN_PINCTRL_GROUP("NAND_REn", 0x0, 0x3, 0x1b,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* REn */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw2a"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO9 */
			BERLIN_PINCTRL_FUNCTION(0x4, "avio")), /*DBG1 */
	BERLIN_PINCTRL_GROUP("NAND_WEn", 0x4, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* WEn */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw2a"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO10 */
			BERLIN_PINCTRL_FUNCTION(0x4, "avio"), /* DBG0 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS14 */
	BERLIN_PINCTRL_GROUP("NAND_CLE", 0x4, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* CLE */
			BERLIN_PINCTRL_FUNCTION(0x1, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio")), /* GPIO11 */
	BERLIN_PINCTRL_GROUP("NAND_ALE", 0x4, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* ALE */
			BERLIN_PINCTRL_FUNCTION(0x1, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO12 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS13 */
	BERLIN_PINCTRL_GROUP("NAND_IO7", 0x4, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* IO7 */
			BERLIN_PINCTRL_FUNCTION(0x1, "urt1a"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO13 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS12 */
	BERLIN_PINCTRL_GROUP("NAND_IO6", 0x4, 0x3, 0x0c,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* IO6 */
			BERLIN_PINCTRL_FUNCTION(0x1, "urt1a"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO14 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS11 */
	BERLIN_PINCTRL_GROUP("NAND_IO5", 0x4, 0x3, 0x0f,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* IO5 */
			BERLIN_PINCTRL_FUNCTION(0x2, "hdmi"), /* FBCLK */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO15 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg"), /* BUS10 */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s2a"), /* DI0 */
			BERLIN_PINCTRL_FUNCTION(0x7, "pdma")), /* CLKO */
	BERLIN_PINCTRL_GROUP("NAND_IO4", 0x4, 0x3, 0x12,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* IO4 */
			BERLIN_PINCTRL_FUNCTION(0x2, "arc_test"), /* OUT */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO16 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg"), /* BUS9 */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s2a"), /* DI1 */
			BERLIN_PINCTRL_FUNCTION(0x7, "pdma")), /* DI1 */
	BERLIN_PINCTRL_GROUP("NAND_IO3", 0x4, 0x3, 0x15,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* IO3 */
			BERLIN_PINCTRL_FUNCTION(0x2, "arc_test"), /* IN */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO17 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg"), /* BUS8 */
			BERLIN_PINCTRL_FUNCTION(0x7, "pdma")), /* DI0 */
	BERLIN_PINCTRL_GROUP("NAND_IO2", 0x4, 0x3, 0x18,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* IO2 */
			BERLIN_PINCTRL_FUNCTION(0x2, "i2s1a"), /* LRCKO */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO18 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg"), /* BUS7 */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s2a")), /* LRCKI */
	BERLIN_PINCTRL_GROUP("NAND_IO1", 0x4, 0x3, 0x1b,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* IO1 */
			BERLIN_PINCTRL_FUNCTION(0x2, "i2s1a"), /* BCLKO */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO19 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg"), /* BUS6 */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s2a")), /* BCLKI */
	BERLIN_PINCTRL_GROUP("NAND_IO0", 0x8, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "nand"), /* IO0 */
			BERLIN_PINCTRL_FUNCTION(0x2, "i2s1a"), /* MCLK */
			BERLIN_PINCTRL_FUNCTION(0x3, "gpio"), /* GPIO20 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg"), /* BUS15 */
			BERLIN_PINCTRL_FUNCTION(0x6, "i2s2a")), /* MCLK */
	BERLIN_PINCTRL_GROUP("EMMC_RSTn", 0x8, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* RSTn */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO32 */
			BERLIN_PINCTRL_FUNCTION(0x3, "i2s1b"), /* MCLK */
			BERLIN_PINCTRL_FUNCTION(0x4, "i2s2b")), /* MCLK */
	BERLIN_PINCTRL_GROUP("EMMC_DATA0", 0x8, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA0 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO21 */
			BERLIN_PINCTRL_FUNCTION(0x2, "tw2b")), /* SCL */
	BERLIN_PINCTRL_GROUP("EMMC_DATA1", 0x8, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA1 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO22 */
			BERLIN_PINCTRL_FUNCTION(0x2, "tw2b")), /* SDA */
	BERLIN_PINCTRL_GROUP("EMMC_DATA2", 0x8, 0x3, 0x0c,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA2 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO23 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "i2s2b"), /* DI0 */
			BERLIN_PINCTRL_FUNCTION(0x4, "pdmb")), /* DI0 */
	BERLIN_PINCTRL_GROUP("EMMC_DATA3", 0x8, 0x3, 0x0f,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA3 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO24 */
			BERLIN_PINCTRL_FUNCTION(0x2, "pwm"), /* PWM0 */
			BERLIN_PINCTRL_FUNCTION(0x3, "i2s2b"), /* DI1 */
			BERLIN_PINCTRL_FUNCTION(0x4, "pdmb")), /* DI1 */
	BERLIN_PINCTRL_GROUP("EMMC_STRB", 0x8, 0x3, 0x12,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* STRB */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO25 */
			BERLIN_PINCTRL_FUNCTION(0x2, "urt0a"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x4, "pdmb"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x5, "pdmc")), /* CLKO */
	BERLIN_PINCTRL_GROUP("EMMC_CLK", 0x8, 0x3, 0x15,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO26 */
			BERLIN_PINCTRL_FUNCTION(0x2, "urt0a")), /* TXD */
	BERLIN_PINCTRL_GROUP("EMMC_CMD", 0x8, 0x3, 0x18,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* CMD */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO27 */
			BERLIN_PINCTRL_FUNCTION(0x3, "i2s1b")), /* DO */
	BERLIN_PINCTRL_GROUP("EMMC_DATA4", 0x8, 0x3, 0x1b,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA4 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO28 */
			BERLIN_PINCTRL_FUNCTION(0x2, "tw0a"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x5, "pdmc")), /* DI0 */
	BERLIN_PINCTRL_GROUP("EMMC_DATA5", 0xc, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA5 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO29 */
			BERLIN_PINCTRL_FUNCTION(0x2, "tw0a"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x5, "pdmc")), /* DI1 */
	BERLIN_PINCTRL_GROUP("EMMC_DATA6", 0xc, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA6 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO30 */
			BERLIN_PINCTRL_FUNCTION(0x3, "i2s1b"), /* LRCKO */
			BERLIN_PINCTRL_FUNCTION(0x4, "i2s2b")), /* LRCKI */
	BERLIN_PINCTRL_GROUP("EMMC_DATA7", 0xc, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "emmc"), /* DATA7 */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* GPIO31 */
			BERLIN_PINCTRL_FUNCTION(0x3, "i2s1b"), /* BCLKO */
			BERLIN_PINCTRL_FUNCTION(0x4, "i2s2b")), /* BCLKI */
	BERLIN_PINCTRL_GROUP("SD0_CLK", 0xc, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "por_b_vout"), /* 1p05 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio"), /* GPIO33 */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS0 */
	BERLIN_PINCTRL_GROUP("SD0_DAT0", 0xc, 0x3, 0x0c,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO34 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* DAT0 */
			BERLIN_PINCTRL_FUNCTION(0x3, "cpupll"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS1 */
	BERLIN_PINCTRL_GROUP("SD0_DAT1", 0xc, 0x3, 0x0f,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO35 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* DAT1 */
			BERLIN_PINCTRL_FUNCTION(0x3, "mempll"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS2 */
	BERLIN_PINCTRL_GROUP("SD0_DAT2", 0xc, 0x3, 0x12,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO36 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* DAT2 */
			BERLIN_PINCTRL_FUNCTION(0x2, "sif"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x3, "syspll"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS3 */
	BERLIN_PINCTRL_GROUP("SD0_DAT3", 0xc, 0x3, 0x15,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO37 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* DAT3 */
			BERLIN_PINCTRL_FUNCTION(0x2, "sif"), /* DEN */
			BERLIN_PINCTRL_FUNCTION(0x3, "avplla"), /* CLKO */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS4 */
	BERLIN_PINCTRL_GROUP("SD0_CMD", 0xc, 0x3, 0x18,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO38 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* CMD */
			BERLIN_PINCTRL_FUNCTION(0x2, "sif"), /* DIO */
			BERLIN_PINCTRL_FUNCTION(0x5, "dbg")), /* BUS5 */
	BERLIN_PINCTRL_GROUP("USB0_DRV_VBUS", 0xc, 0x3, 0x1b,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO39 */
			BERLIN_PINCTRL_FUNCTION(0x1, "usb0"), /* VBUS */
			BERLIN_PINCTRL_FUNCTION(0x2, "sts0")), /* SD */
	BERLIN_PINCTRL_GROUP("SD0_CDn", 0x10, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO40 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* CDn */
			BERLIN_PINCTRL_FUNCTION(0x4, "sts0")), /* SOP */
	BERLIN_PINCTRL_GROUP("SD0_WP", 0x10, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "gpio"), /* GPIO41 */
			BERLIN_PINCTRL_FUNCTION(0x1, "sd0"), /* WP */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm")), /* PWM1 */
};

static const struct berlin_desc_group berlin4cdp_avio_pinctrl_groups[] = {
	BERLIN_PINCTRL_GROUP("RX_EDDC_SCL", 0x0, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "rx_eddc"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* AVIO GPIO4 */
	BERLIN_PINCTRL_GROUP("RX_EDDC_SDA", 0x0, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "rx_eddc"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* AVIO GPIO5 */
	BERLIN_PINCTRL_GROUP("HDMIRX_HPD", 0x0, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "hdmirx"), /* PWR5V */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio")), /* AVIO GPIO6 */
	BERLIN_PINCTRL_GROUP("TW1_SCL", 0x0, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "tx_eddc"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw1"), /* SCL */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio")), /* AVIO GPIO0 */
	BERLIN_PINCTRL_GROUP("TW1_SDA", 0x0, 0x3, 0x0c,
			BERLIN_PINCTRL_FUNCTION(0x0, "tx_eddc"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x1, "tw1"), /* SDA */
			BERLIN_PINCTRL_FUNCTION(0x2, "gpio")), /* AVIO GPIO1 */
	BERLIN_PINCTRL_GROUP("HDMI_CEC", 0x0, 0x3, 0x0f,
			BERLIN_PINCTRL_FUNCTION(0x0, "hdmi"), /* CEC */
			BERLIN_PINCTRL_FUNCTION(0x1, "avio")), /* AVIO GPIO2 */
};

static const struct berlin_desc_group berlin4cdp_sysmgr_pinctrl_groups[] = {
	BERLIN_PINCTRL_GROUP("SM_TRSTn", 0x0, 0x3, 0x00,
			BERLIN_PINCTRL_FUNCTION(0x0, "sm"), /* TRSTn */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO0 */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm")), /* PWM2 */
	BERLIN_PINCTRL_GROUP("SM_TMS", 0x0, 0x3, 0x03,
			BERLIN_PINCTRL_FUNCTION(0x0, "sm"), /* TMS */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO1 */
			BERLIN_PINCTRL_FUNCTION(0x2, "sts0"), /* CLK */
			BERLIN_PINCTRL_FUNCTION(0x3, "urt1"), /* RXD */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm")), /* PWM0 */
	BERLIN_PINCTRL_GROUP("SM_TDI", 0x0, 0x3, 0x06,
			BERLIN_PINCTRL_FUNCTION(0x0, "sm"), /* TDI */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO2 */
			BERLIN_PINCTRL_FUNCTION(0x2, "sts0"), /* VALD */
			BERLIN_PINCTRL_FUNCTION(0x3, "urt1"), /* TXD */
			BERLIN_PINCTRL_FUNCTION(0x4, "pwm")), /* PWM1 */
	BERLIN_PINCTRL_GROUP("SM_TDO", 0x0, 0x3, 0x09,
			BERLIN_PINCTRL_FUNCTION(0x0, "sm"), /* TDO */
			BERLIN_PINCTRL_FUNCTION(0x1, "gpio"), /* SM GPIO3 */
			BERLIN_PINCTRL_FUNCTION(0x2, "SPDIFO")),
};

static const struct berlin_pinctrl_desc berlin4cdp_soc_pinctrl_data = {
	.groups = berlin4cdp_soc_pinctrl_groups,
	.ngroups = ARRAY_SIZE(berlin4cdp_soc_pinctrl_groups),
};

static const struct berlin_pinctrl_desc berlin4cdp_avio_pinctrl_data = {
	.groups = berlin4cdp_avio_pinctrl_groups,
	.ngroups = ARRAY_SIZE(berlin4cdp_avio_pinctrl_groups),
};

static const struct berlin_pinctrl_desc berlin4cdp_sysmgr_pinctrl_data = {
	.groups = berlin4cdp_sysmgr_pinctrl_groups,
	.ngroups = ARRAY_SIZE(berlin4cdp_sysmgr_pinctrl_groups),
};

static const struct of_device_id berlin4cdp_pinctrl_match[] = {
	{
		.compatible = "marvell,berlin4cdp-soc-pinctrl",
		.data = &berlin4cdp_soc_pinctrl_data,
	},
	{
		.compatible = "marvell,berlin4cdp-avio-pinctrl",
		.data = &berlin4cdp_avio_pinctrl_data,
	},
	{
		.compatible = "marvell,berlin4cdp-sm-pinctrl",
		.data = &berlin4cdp_sysmgr_pinctrl_data,
	},
	{}
};
MODULE_DEVICE_TABLE(of, berlin4cdp_pinctrl_match);

static int berlin4cdp_pinctrl_probe(struct platform_device *pdev)
{
	const struct of_device_id *match =
		of_match_device(berlin4cdp_pinctrl_match, &pdev->dev);
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

static struct platform_driver berlin4cdp_pinctrl_driver = {
	.probe	= berlin4cdp_pinctrl_probe,
	.driver	= {
		.name = "berlin-bg4cdp-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = berlin4cdp_pinctrl_match,
	},
};
module_platform_driver(berlin4cdp_pinctrl_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@marvell.com>");
MODULE_DESCRIPTION("Marvell Berlin BG4CDP pinctrl driver");
MODULE_LICENSE("GPL");
