/*
 * Copyright (c) 2015 Marvell Technology Group Ltd.
 *		http://www.marvell.com
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
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>

#include "clk.h"

#define PLL_CTRL0	0x0
#define PLL_CTRL1	0x4
#define PLL_CTRL2	0x8
#define PLL_CTRL3	0xC
#define PLL_CTRL4	0x10
#define PLL_CTRL5	0x14
#define PLL_STATUS	0x18

static unsigned long berlin4cdp_pll_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	u32 val, fbdiv, rfdiv, vcodivsel;
	struct berlin_pll *pll = to_berlin_pll(hw);

	val = readl_relaxed(pll->bypass);
	if (unlikely(val & (1 << pll->bypass_shift)))
		return 400000000;

	val = readl_relaxed(pll->ctrl + PLL_CTRL0);
	fbdiv = (val >> 12) & 0x1FF;
	rfdiv = (val >> 3) & 0x1FF;
	val = readl_relaxed(pll->ctrl + PLL_CTRL1);
	vcodivsel = (val >> 6) & 0x1FF;
	vcodivsel = (vcodivsel == 0) ? 1 : vcodivsel;
	return parent_rate * fbdiv * 4 / rfdiv / vcodivsel;
}

static long berlin4cdp_pll_round_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long *parent_rate)
{
	int vcodivsel;
	u32 vco, rfdiv, fbdiv, ctrl0;
	struct berlin_pll *pll = to_berlin_pll(hw);
	long ret;

	if (unlikely(rate == 400000000))
		return rate;

	rate /= 1000000;

	vcodivsel = 999 / rate + 1;
	if (vcodivsel > 511)
		return berlin4cdp_pll_recalc_rate(hw, *parent_rate);

	vco = rate * vcodivsel;
	if (vco > 4000 || vco < 1000)
		return berlin4cdp_pll_recalc_rate(hw, *parent_rate);

	ctrl0 = readl_relaxed(pll->ctrl);
	rfdiv = (ctrl0 >> 3) & 0x1FF;
	fbdiv = (vco * rfdiv) / (4 * *parent_rate / 1000000);
	fbdiv &= 0x1FF;
	ret = *parent_rate * fbdiv * 4 / rfdiv / vcodivsel;
	return ret;
}

static int berlin4cdp_pll_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	int vcodivsel, pllintpi, kvco;
	u32 vco, rfdiv, fbdiv, ctrl0, ctrl1, ctrl2, bypass;
	struct berlin_pll *pll = to_berlin_pll(hw);

	if (unlikely(rate == 400000000)) {
		bypass = readl_relaxed(pll->bypass);
		bypass |= (1 << pll->bypass_shift);
		writel_relaxed(bypass, pll->bypass);
		ctrl0 = readl_relaxed(pll->ctrl + PLL_CTRL0);
		ctrl0 &= ~(1 << 0);
		writel_relaxed(ctrl0, pll->ctrl + PLL_CTRL0);
		return 0;
	}

	bypass = readl_relaxed(pll->bypass);
	if (unlikely(bypass & (1 << pll->bypass_shift))) {
		ctrl0 = readl_relaxed(pll->ctrl + PLL_CTRL0);
		ctrl0 |= (1 << 0);
		writel_relaxed(ctrl0, pll->ctrl + PLL_CTRL0);
	}

	rate /= 1000000;

	vcodivsel = 999 / rate + 1;
	if (vcodivsel > 511)
		return -EPERM;

	vco = rate * vcodivsel;
	if (vco >= 1000 && vco <= 1150)
		kvco = 1;
	else if (vco <= 1250)
		kvco = 2;
	else if (vco <= 1400)
		kvco = 3;
	else if (vco <= 1550)
		kvco = 4;
	else if (vco <= 1650)
		kvco = 5;
	else if (vco <= 1800)
		kvco = 6;
	else if (vco <= 1950)
		kvco = 7;
	else if (vco <= 2170)
		kvco = 8;
	else if (vco <= 2500)
		kvco = 9;
	else if (vco <= 2800)
		kvco = 10;
	else if (vco <= 3100)
		kvco = 11;
	else if (vco <= 3450)
		kvco = 12;
	else if (vco <= 3750)
		kvco = 13;
	else if (vco <= 4000)
		kvco = 14;
	else
		return -EPERM;

	if (vco >= 2000 && vco < 2750)
		pllintpi = 3;
	else if (vco < 3250)
		pllintpi = 5;
	else if (vco <= 4000)
		pllintpi = 7;
	else
		return -EPERM;

	ctrl0 = readl_relaxed(pll->ctrl + PLL_CTRL0);
	ctrl1 = readl_relaxed(pll->ctrl + PLL_CTRL1);
	ctrl2 = readl_relaxed(pll->ctrl + PLL_CTRL2);

	rfdiv = (ctrl0 >> 3) & 0x1FF;
	fbdiv = (vco * rfdiv) / (4 * parent_rate / 1000000);
	fbdiv &= 0x1FF;
	ctrl0 &= ~(0x1FF << 12);
	ctrl0 |= (fbdiv << 12);

	ctrl1 &= ~(0xf << 0);
	ctrl1 |= (kvco << 0);
	ctrl1 &= ~(0x1FF << 6);
	ctrl1 |= (vcodivsel << 6);
	ctrl1 &= ~(0x1FF << 15);
	ctrl1 |= (vcodivsel << 15);

	ctrl2 &= ~(0xf << 9);
	ctrl2 |= (pllintpi << 9);

	/* Pll bypass enable */
	local_irq_disable();
	bypass = readl_relaxed(pll->bypass);
	bypass |= (1 << pll->bypass_shift);
	writel_relaxed(bypass, pll->bypass);
	ctrl1 |= (1 << 26);
	writel_relaxed(ctrl1, pll->ctrl + PLL_CTRL1);

	/* reset on */
	ctrl0 |= (1 << 1);
	writel_relaxed(ctrl0, pll->ctrl + PLL_CTRL0);

	writel_relaxed(ctrl2, pll->ctrl + PLL_CTRL2);

	/* make sure RESET is high for at least 2us */
	udelay(2);

	/* clear reset */
	ctrl0 &= ~(1 << 1);
	writel_relaxed(ctrl0, pll->ctrl + PLL_CTRL0);

	/* wait 50us */
	udelay(50);

	/* make sure pll locked */
	while (!(readl_relaxed(pll->ctrl + PLL_STATUS) & (1 << 0)))
		cpu_relax();

	/* pll bypass disable */
	ctrl1 &= ~(1 << 26);
	writel_relaxed(ctrl1, pll->ctrl + PLL_CTRL1);
	bypass &= ~(1 << pll->bypass_shift);
	writel_relaxed(bypass, pll->bypass);
	local_irq_enable();

	return 0;
}

static const struct clk_ops berlin4cdp_pll_ops = {
	.recalc_rate	= berlin4cdp_pll_recalc_rate,
	.round_rate	= berlin4cdp_pll_round_rate,
	.set_rate	= berlin4cdp_pll_set_rate,
};

static void __init berlin4cdp_pll_setup(struct device_node *np)
{
	berlin_pll_setup(np, &berlin4cdp_pll_ops);
}
CLK_OF_DECLARE(berlin4cdp_pll, "marvell,berlin4cdp-pll", berlin4cdp_pll_setup);

static unsigned long berlin4cdp_avpll_recalc_rate(struct clk_hw *hw,
						  unsigned long parent_rate)
{
	return 0;
}

static unsigned long berlin4cdp_avpllch_recalc_rate(struct clk_hw *hw,
						    unsigned long parent_rate)
{
	unsigned long rate = 0;
	struct berlin_avpllch *avpllch = to_berlin_avpllch(hw);

	rate = avpllch->cur_rate;
	return rate;
}

static long berlin4cdp_avpllch_round_rate(struct clk_hw *hw, unsigned long rate,
					  unsigned long *parent_rate)
{
	return rate;
}

static int berlin4cdp_avpllch_set_rate(struct clk_hw *hw, unsigned long rate,
				       unsigned long parent_rate)
{
	struct berlin_avpllch *avpllch = to_berlin_avpllch(hw);

	avpllch->cur_rate = rate;
	return 0;
}

static const struct clk_ops berlin4cdp_avpll_ops = {
	.recalc_rate	= berlin4cdp_avpll_recalc_rate,
};

static const struct clk_ops berlin4cdp_avpllch_ops = {
	.recalc_rate	= berlin4cdp_avpllch_recalc_rate,
	.round_rate	= berlin4cdp_avpllch_round_rate,
	.set_rate	= berlin4cdp_avpllch_set_rate,
};

static struct berlin_avplldata berlin4cdp_avpll_data = {
	.avpll_ops	= &berlin4cdp_avpll_ops,
	.avpllch_ops	= &berlin4cdp_avpllch_ops,
};

static void __init berlin4cdp_avpll_setup(struct device_node *np)
{
	berlin_avpll_setup(np, &berlin4cdp_avpll_data);
}
CLK_OF_DECLARE(berlin4cdp_avpll, "marvell,berlin4cdp-avpll", berlin4cdp_avpll_setup);

static int __init berlin4cdp_avpll_sysfs_init(void)
{
	berlin_avpll_sysfs_init(&berlin4cdp_avpll_data);
	return 0;
}
late_initcall(berlin4cdp_avpll_sysfs_init);
