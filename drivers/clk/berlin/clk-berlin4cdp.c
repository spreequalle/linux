/*
 * Copyright (c) 2016 Marvell Technology Group Ltd.
 *
 * Author: Jisheng Zhang <jszhang@marvell.com>
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
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>

#include "clk.h"

static DEFINE_SPINLOCK(lock);

static const struct gateclk_desc berlin4cdp_gateclk_desc[] __initconst = {
	{ "ahbapbcoreclk",	"perifclk",	0, CLK_IGNORE_UNUSED },
	{ "usb0coreclk",	"perifclk",	1 },
	{ "sdiocoreclk",	"perifclk",	2 },
	{ "tssssysclk",		"perifclk",	3, CLK_IGNORE_UNUSED },
	{ "nfcsysclk",		"perifclk",	4 },
	{ "pcieclk",		"perifclk",	5 },
	{ "ihb0sysclk",		"perifclk",	6 },
	{ "emmcsysclk",		"perifclk",	7 },
	{ "zspsysclk",		"perifclk",	8, CLK_IGNORE_UNUSED },
	{},
};

static void __init berlin4cdp_gateclk_setup(struct device_node *np)
{
	berlin_gateclk_setup(np, berlin4cdp_gateclk_desc, &lock);
}
CLK_OF_DECLARE(berlin4cdp_gateclk, "marvell,berlin4cdp-gateclk",
	       berlin4cdp_gateclk_setup);
