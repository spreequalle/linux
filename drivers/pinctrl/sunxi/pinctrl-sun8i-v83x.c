// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Icenowy Zheng <icenowy@aosc.io>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun8i_v83x_pins[] = {
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_VARIANT(0x3,
					 "mmc2",	/* DS */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x4, "spi0"),		/* CLK */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_VARIANT(0x3,
					 "mmc2",	/* RST */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x4, "spi0"),		/* CS0 */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_VARIANT(0x3,
					 "mmc2",	/* CLK */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x4, "spi0"),		/* MOSI */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_VARIANT(0x3,
					 "mmc2",	/* CMD */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x4, "spi0"),		/* MISO */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_VARIANT(0x3,
					 "mmc2",	/* WP */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x4, "spi0"),		/* D3 */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_VARIANT(0x3,
					 "mmc2",	/* HOLD */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x4, "spi0"),		/* D4 */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 5)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(C, 6),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x3, "mmc2"),		/* D0 */
			  SUNXI_FUNCTION(0x4, "spi0"),		/* CS1 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 6)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(C, 7),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x3, "mmc2"),		/* D5 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 7)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(C, 8),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x3, "mmc2"),		/* D1 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 8)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(C, 9),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x3, "mmc2"),		/* D6 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 9)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(C, 10),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x3, "mmc2"),		/* D2 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 10)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(C, 11),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x3, "mmc2"),		/* D7 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 11)),
	/* Hole */
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 0),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D2 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* D3 */
		  SUNXI_FUNCTION(0x3, "pwm"),		/* 0 */
		  SUNXI_FUNCTION(0x4, "vo"),		/* D0 */
		  SUNXI_FUNCTION(0x5, "emac"),		/* RXD1 */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* D4 */
		  SUNXI_FUNCTION(0x3, "pwm"),		/* 1 */
		  SUNXI_FUNCTION(0x4, "vo"),		/* D1 */
		  SUNXI_FUNCTION(0x5, "emac"),		/* RXD0 */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* D5 */
		  SUNXI_FUNCTION(0x3, "pwm"),		/* 2 */
		  SUNXI_FUNCTION(0x4, "vo"),		/* D2 */
		  SUNXI_FUNCTION(0x5, "emac"),		/* CRS_DV */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* D6 */
		  SUNXI_FUNCTION(0x3, "pwm"),		/* 3 */
		  SUNXI_FUNCTION(0x4, "vo"),		/* D3 */
		  SUNXI_FUNCTION(0x5, "emac"),		/* RXER */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* D7 */
		  SUNXI_FUNCTION(0x3, "pwm"),		/* 4 */
		  SUNXI_FUNCTION(0x4, "vo"),		/* D4 */
		  SUNXI_FUNCTION(0x5, "emac"),		/* TXD1 */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* D10 */
		  SUNXI_FUNCTION(0x3, "pwm"),		/* 5 */
		  SUNXI_FUNCTION(0x4, "vo"),		/* D5 */
		  SUNXI_FUNCTION(0x5, "emac"),		/* TXD0 */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* D11 */
		  SUNXI_FUNCTION(0x3, "pwm"),		/* 6 */
		  SUNXI_FUNCTION(0x4, "vo"),		/* D6 */
		  SUNXI_FUNCTION(0x5, "emac"),		/* TXCK */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* D12 */
		  SUNXI_FUNCTION(0x3, "pwm"),		/* 7 */
		  SUNXI_FUNCTION(0x4, "vo"),		/* D7 */
		  SUNXI_FUNCTION(0x5, "emac"),		/* TXEN */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 8)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 9),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D13 */
			  SUNXI_FUNCTION(0x3, "pwm"),		/* 8 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 9)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 10),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D14 */
			  SUNXI_FUNCTION(0x3, "i2s1"),		/* MCLK */
			  SUNXI_FUNCTION(0x4, "vo"),		/* D8 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 10)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 11),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D15 */
			  SUNXI_FUNCTION(0x3, "i2s1"),		/* BCLK */
			  SUNXI_FUNCTION(0x4, "vo"),		/* D9 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 11)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 12),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D18 */
			  SUNXI_FUNCTION(0x3, "i2s1"),		/* LRCK */
			  SUNXI_FUNCTION(0x4, "vo"),		/* D10 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 12)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 13),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D19 */
			  SUNXI_FUNCTION(0x3, "i2s1"),		/* DOUT0 */
			  SUNXI_FUNCTION(0x4, "vo"),		/* D11 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 13)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 14),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D20 */
			  SUNXI_FUNCTION(0x3, "i2s1_out"),	/* DOUT1 */
			  SUNXI_FUNCTION(0x4, "vo"),		/* D11 */
			  SUNXI_FUNCTION(0x5, "i2s1_in"),	/* DIN1 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 14)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 15),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D21 */
			  SUNXI_FUNCTION(0x3, "i2s1_out"),	/* DOUT2 */
			  SUNXI_FUNCTION(0x4, "vo"),		/* D13 */
			  SUNXI_FUNCTION(0x5, "i2s1_in"),	/* DIN2 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 15)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 16),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D22 */
			  SUNXI_FUNCTION(0x3, "i2s1_out"),	/* DOUT3 */
			  SUNXI_FUNCTION(0x4, "vo"),		/* D14 */
			  SUNXI_FUNCTION(0x5, "i2s1_in"),	/* DIN3 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 16)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 17),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "lcd"),		/* D23 */
			  SUNXI_FUNCTION(0x3, "i2s1"),		/* DIN0 */
			  SUNXI_FUNCTION(0x4, "vo"),		/* D15 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 17)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* CLK */
		  SUNXI_FUNCTION(0x4, "vo"),		/* CLK */
		  SUNXI_FUNCTION(0x5, "emac"),		/* EPHY_25M */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 18)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* DE */
		  SUNXI_FUNCTION(0x4, "vo"),		/* FIELD */
		  SUNXI_FUNCTION(0x5, "tcon_trig"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 19)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* HSYNC */
		  SUNXI_FUNCTION(0x4, "vo"),		/* HSYNC */
		  SUNXI_FUNCTION(0x5, "emac"),		/* MDC */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 20)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd"),		/* VSYNC */
		  SUNXI_FUNCTION(0x4, "vo"),		/* VSYNC */
		  SUNXI_FUNCTION(0x5, "emac"),		/* MDIO */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 21)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(D, 22),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "pwm"),		/* 9 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 22)),
	/* Hole */
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 0),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* PCLK */
			  SUNXI_FUNCTION(0x3, "emac"),		/* RXD3 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 0)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 1),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* MCLK */
			  SUNXI_FUNCTION(0x3, "emac"),		/* RXD2 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 1)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 2),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* HSYNC */
			  SUNXI_FUNCTION(0x3, "emac"),		/* RXD1 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 2)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 3),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* VSYNC */
			  SUNXI_FUNCTION(0x3, "emac"),		/* RXD0 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 3)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 4),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D0 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* RXCK */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 4)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 5),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D1 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* RXCTL */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 5)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 6),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D2 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* CLKIN */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 6)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 7),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D3 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* TXD3 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 7)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 8),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D4 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* TXD2 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 8)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 9),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D5 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* TXD1 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 9)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 10),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D6 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* TXD0 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 10)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 11),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D7 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* TXCK */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 11)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 12),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D8 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* TXCTL */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 12)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 13),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D9 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* MDC */
			  SUNXI_FUNCTION(0x5, "i2c1"),		/* SCK */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 13)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 14),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D10 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* MDIO */
			  SUNXI_FUNCTION(0x5, "i2c1"),		/* SDA */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 14)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 15),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1_data"),	/* D11 */
			  SUNXI_FUNCTION(0x3, "emac"),		/* EPHY_25M */
			  SUNXI_FUNCTION(0x3, "csi1_field"),
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 15)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 16),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_VARIANT(0x3,
					 "lcd",		/* D0 */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x5, "i2c0"),		/* SCK */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 16)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 17),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_VARIANT(0x3,
					 "lcd",		/* D1 */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x5, "i2c0"),		/* SDA */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 17)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 18),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D12 */
			  SUNXI_FUNCTION(0x3, "lcd"),		/* D8 */
			  SUNXI_FUNCTION(0x4, "spi2"),		/* CLK */
			  SUNXI_FUNCTION(0x5, "uart2"),		/* TX */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 18)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 19),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D13 */
			  SUNXI_FUNCTION(0x3, "lcd"),		/* D9 */
			  SUNXI_FUNCTION(0x4, "spi2"),		/* MOSI */
			  SUNXI_FUNCTION(0x5, "uart2"),		/* RX */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 19)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 20),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D14 */
			  SUNXI_FUNCTION(0x3, "lcd"),		/* D16 */
			  SUNXI_FUNCTION(0x4, "spi2"),		/* MISO */
			  SUNXI_FUNCTION(0x5, "uart2"),		/* RTS */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 20)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(E, 21),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x2, "csi1"),		/* D15 */
			  SUNXI_FUNCTION(0x3, "lcd"),		/* D17 */
			  SUNXI_FUNCTION(0x4, "spi2"),		/* CS0 */
			  SUNXI_FUNCTION(0x5, "uart2"),		/* CTS */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 21)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc0"),		/* D1 */
		  SUNXI_FUNCTION(0x3, "jtag"),		/* MS */
		  SUNXI_FUNCTION(0x5, "cpu_bist0"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc0"),		/* D0 */
		  SUNXI_FUNCTION(0x3, "jtag"),		/* DI */
		  SUNXI_FUNCTION(0x5, "cpu_bist1"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc0"),		/* CLK */
		  SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc0"),		/* CMD */
		  SUNXI_FUNCTION(0x3, "jtag"),		/* DO */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc0"),		/* D3 */
		  SUNXI_FUNCTION(0x3, "uart0"),		/* RX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc0"),		/* D2 */
		  SUNXI_FUNCTION(0x3, "jtag"),		/* CK */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 3, 6)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc1"),		/* CLK */
		  SUNXI_FUNCTION(0x5, "uart3"),		/* TX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc1"),		/* CMD */
		  SUNXI_FUNCTION(0x5, "uart3"),		/* RX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc1"),		/* D0 */
		  SUNXI_FUNCTION(0x5, "uart3"),		/* CTS */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc1"),		/* D1 */
		  SUNXI_FUNCTION(0x5, "uart3"),		/* RTS */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc1"),		/* D2 */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* RTS */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "mmc1"),		/* D3 */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* CTS */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x5, "uart1"),		/* TX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x5, "uart1"),		/* RX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 4, 7)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 0 */
		  SUNXI_FUNCTION(0x3, "i2s0"),		/* MCLK */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* CLK */
		  SUNXI_FUNCTION(0x5, "uart3"),		/* TX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 1 */
		  SUNXI_FUNCTION(0x3, "i2s0"),		/* BCLK */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* MOSI */
		  SUNXI_FUNCTION(0x5, "uart3"),		/* RX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 2 */
		  SUNXI_FUNCTION(0x3, "i2s0"),		/* LRCK */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* MISO */
		  SUNXI_FUNCTION(0x5, "uart3"),		/* CTS */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 3 */
		  SUNXI_FUNCTION(0x3, "i2s0"),		/* DOUT */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* CS0 */
		  SUNXI_FUNCTION(0x5, "uart3"),		/* RTS */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 4),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 4 */
		  SUNXI_FUNCTION(0x3, "i2s0"),		/* DIN */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* CS1 */
		  SUNXI_FUNCTION(0x5, "w1"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 5),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 5 */
		  SUNXI_FUNCTION(0x3, "emac"),		/* RXD1 */
		  SUNXI_FUNCTION(0x4, "i2c2"),		/* SCK */
		  SUNXI_FUNCTION(0x5, "uart2"),		/* TX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 6),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 6 */
		  SUNXI_FUNCTION(0x3, "emac"),		/* RXD0 */
		  SUNXI_FUNCTION(0x4, "i2c2"),		/* SDA */
		  SUNXI_FUNCTION(0x5, "uart2"),		/* RX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 7),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 7 */
		  SUNXI_FUNCTION(0x3, "emac"),		/* CRS_DV */
		  SUNXI_FUNCTION(0x4, "uart0"),		/* TX */
		  SUNXI_FUNCTION(0x5, "uart2"),		/* RTS */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 8),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 8 */
		  SUNXI_FUNCTION(0x3, "emac"),		/* RXER */
		  SUNXI_FUNCTION(0x4, "uart0"),		/* RX */
		  SUNXI_FUNCTION(0x5, "uart2"),		/* CTS */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 9),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "pwm"),		/* 9 */
		  SUNXI_FUNCTION(0x3, "emac"),		/* TXD1 */
		  SUNXI_FUNCTION(0x4, "i2c3"),		/* SCK */
		  SUNXI_FUNCTION(0x5, "uart0"),		/* TX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 10),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x3, "emac"),		/* TXD0 */
		  SUNXI_FUNCTION(0x4, "i2c3"),		/* SDA */
		  SUNXI_FUNCTION(0x5, "uart0"),		/* RX */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 11),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "jtag"),		/* MS */
		  SUNXI_FUNCTION(0x3, "emac"),		/* TXCK */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* CLK */
		  SUNXI_FUNCTION(0x5, "i2c2"),		/* SCK */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 12),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "jtag"),		/* CK */
		  SUNXI_FUNCTION(0x3, "emac"),		/* TXEN */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* MOSI */
		  SUNXI_FUNCTION(0x5, "i2c2"),		/* SDA */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 13),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "jtag"),		/* DO */
		  SUNXI_FUNCTION(0x3, "emac"),		/* MDC */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* MISO */
		  SUNXI_FUNCTION(0x5, "i2c3"),		/* SCK */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 14),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "jtag"),		/* DI */
		  SUNXI_FUNCTION(0x3, "emac"),		/* MDIO */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* CS0 */
		  SUNXI_FUNCTION(0x5, "i2c3"),		/* SDA */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 14)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(H, 15),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x3, "emac"),		/* EPHY_25M */
			  SUNXI_FUNCTION(0x4, "spi1"),		/* CS1 */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 5, 15)),
	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* MCLK */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* SM_HS */
		  SUNXI_FUNCTION_VARIANT(0x4,
					 "spi2",	/* CLK */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x5, "i2c1"),		/* SCK */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* SM_VS */
		  SUNXI_FUNCTION(0x3, "tcon_trig"),
		  SUNXI_FUNCTION_VARIANT(0x4,
					 "spi2",	/* MOSI */
					 PINCTRL_SUN8I_V833),
		  SUNXI_FUNCTION(0x5, "i2c1"),		/* SDA */
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 2)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(I, 3),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x4, "spi3"),		/* MISO */
			  SUNXI_FUNCTION(0x5, "i2c0"),		/* SCK */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 3)),
	SUNXI_PIN_VARIANT(SUNXI_PINCTRL_PIN(I, 4),
			  PINCTRL_SUN8I_V833,
			  SUNXI_FUNCTION(0x0, "gpio_in"),
			  SUNXI_FUNCTION(0x1, "gpio_out"),
			  SUNXI_FUNCTION(0x4, "spi3"),		/* CS0 */
			  SUNXI_FUNCTION(0x5, "i2c0"),		/* SDA */
			  SUNXI_FUNCTION_IRQ_BANK(0x6, 6, 4)),
};

static const unsigned int sun8i_v83x_pinctrl_irq_bank_map[] = { 2, 3, 4, 5, 6, 7, 8 };

static const struct sunxi_pinctrl_desc sun8i_v83x_pinctrl_data = {
	.pins = sun8i_v83x_pins,
	.npins = ARRAY_SIZE(sun8i_v83x_pins),
	.irq_banks = 7,
	.irq_bank_map = sun8i_v83x_pinctrl_irq_bank_map,
	.io_bias_cfg_variant = BIAS_VOLTAGE_PIO_POW_MODE_SEL,
};

static int sun8i_v83x_pinctrl_probe(struct platform_device *pdev)
{
	unsigned long variant = (unsigned long)of_device_get_match_data(&pdev->dev);

	return sunxi_pinctrl_init_with_variant(pdev, &sun8i_v83x_pinctrl_data,
					       variant);
}

static const struct of_device_id sun8i_v83x_pinctrl_match[] = {
	{
		.compatible = "allwinner,sun8i-v831-pinctrl",
		.data = (void *)PINCTRL_SUN8I_V831
	},
	{
		.compatible = "allwinner,sun8i-v833-pinctrl",
		.data = (void *)PINCTRL_SUN8I_V833
	},
	{ },
};

static struct platform_driver sun8i_v83x_pinctrl_driver = {
	.probe	= sun8i_v83x_pinctrl_probe,
	.driver	= {
		.name		= "sun8i-v83x-pinctrl",
		.of_match_table	= sun8i_v83x_pinctrl_match,
	},
};
builtin_platform_driver(sun8i_v83x_pinctrl_driver);
