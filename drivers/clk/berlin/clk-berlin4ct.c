/*
 * Copyright (c) 2015 Marvell Technology Group Ltd.
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

static const struct gateclk_desc berlin4ct_gateclk_desc[] __initconst = {
	{ "tspsysclk",		"perifsysclk",	0, CLK_IGNORE_UNUSED },
	{ "usb0coreclk",	"perifsysclk",	1 },
	{ "zspsysclk",		"perifsysclk",	2, CLK_IGNORE_UNUSED },
	{ "sdiosysclk",		"perifsysclk",	3 },
	{ "ethcoreclk",		"perifsysclk",	4 },
	{ "pcie0sys",		"perifsysclk",	6 },
	{ "sata0core",		"perifsysclk",	7 },
	{ "nfcsysclk",		"perifsysclk",	8 },
	{ "emmcsysclk",		"perifsysclk",	9 },
	{},
};

static void __init berlin4ct_gateclk_setup(struct device_node *np)
{
	berlin_gateclk_setup(np, berlin4ct_gateclk_desc, &lock);
}
CLK_OF_DECLARE(berlin4ct_gateclk, "marvell,berlin4ct-gateclk",
	       berlin4ct_gateclk_setup);
