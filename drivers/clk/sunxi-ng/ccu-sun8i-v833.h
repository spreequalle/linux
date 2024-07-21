/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2020 Icenowy Zheng <icenowy@aosc.io>
 */

#ifndef _CCU_SUN8I_V833_H_
#define _CCU_SUN8I_V833_H_

#include <dt-bindings/clock/sun8i-v833-ccu.h>
#include <dt-bindings/reset/sun8i-v833-ccu.h>

#define CLK_OSC12M		0
#define CLK_PLL_CPUX		1
#define CLK_PLL_DDR0		2
#define CLK_PLL_PERIPH0		3
#define CLK_PLL_PERIPH0_2X	4
#define CLK_PLL_UNI		5
#define CLK_PLL_UNI_2X		6
#define CLK_PLL_VIDEO0		7
#define CLK_PLL_VIDEO0_4X	8
#define CLK_PLL_AUDIO_BASE	9
#define CLK_PLL_AUDIO		10
#define CLK_PLL_AUDIO_2X	11
#define CLK_PLL_AUDIO_4X	12
#define CLK_PLL_CSI		13

/* CPUX clock exported for DVFS */

#define CLK_AXI			15
#define CLK_CPUX_APB		16
#define CLK_PSI_AHB1_AHB2	17
#define CLK_AHB3		18

/* APB1 clock exported for PIO */

#define CLK_APB2		20

/* All module clocks and bus gates are exported except DRAM */

#define CLK_DRAM		39

#define CLK_BUS_DRAM		48

#define CLK_NUMBER		(CLK_BUS_DSPO + 1)

#endif /* _CCU_SUN8I_V833_H_ */
