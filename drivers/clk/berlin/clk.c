/*
 * Copyright (c) 2013 Marvell Technology Group Ltd.
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
#include <linux/of_address.h>
#include <linux/slab.h>

#include "clk.h"

#define CLKEN		(1 << 0)
#define CLKPLLSEL_MASK	7
#define CLKPLLSEL_SHIFT	1
#define CLKPLLSWITCH	(1 << 4)
#define CLKSWITCH	(1 << 5)
#define CLKD3SWITCH	(1 << 6)
#define CLKSEL_MASK	7
#define CLKSEL_SHIFT	7

struct berlin_clk {
	struct clk_hw hw;
	void __iomem *base;
};

#define to_berlin_clk(hw)	container_of(hw, struct berlin_clk, hw)

static u8 clk_div[] = { 0, 2, 4, 6, 8, 12, 0, 0 };

static unsigned long berlin_clkdiv_find_rate_floor(struct clk_hw *hw,
						   unsigned long rate,
						   unsigned long parent_rate)
{
	int i;

	/* Check divider 1 */
	if (rate >= parent_rate)
		return parent_rate;

	/* Check other dividers */
	for (i = 0; i < ARRAY_SIZE(clk_div); i++) {
		if (clk_div[i] == 0)
			continue;

		if (rate >= parent_rate / clk_div[i]) {
			return parent_rate / clk_div[i];
		}

		/* Check divider 3  */
		if (clk_div[i] == 2) {
			if (rate >= parent_rate / 3) {
				return parent_rate / 3;
			}
		}
	}

	return 0;
}

static int berlin_clkdiv_set_rate_precise(struct clk_hw *hw,
					  unsigned long rate, unsigned long parent_rate,
					  unsigned int clk_pllsel)
{
	int i;
	u32 val;
	struct berlin_clk *clk = to_berlin_clk(hw);

	if (parent_rate == 0)
		return -EINVAL;

	val = readl_relaxed(clk->base);
	val &= CLKEN | (CLKSEL_MASK << CLKSEL_SHIFT); /* keep clken and clksel bits */

	/* Select clock source */
	if (clk_pllsel > 4) {
		pr_err("%s: %s: invalid clk_pllsel(%u)\n", __func__,
		       __clk_get_name(hw->clk), clk_pllsel);
		return -EINVAL;
	}

	val |= (clk_pllsel & CLKPLLSEL_MASK) << CLKPLLSEL_SHIFT;
	if (clk_pllsel < 4)
		val |= CLKPLLSWITCH;

	/* Try divider 1 */
	if (rate == parent_rate) {
		writel_relaxed(val, clk->base);
		return 0;
	}

	/* Try other dividers */
	val |= CLKSWITCH;
	for (i = 0; i < ARRAY_SIZE(clk_div); i++) {
		if (clk_div[i] == 0)
			continue;

		if (rate == parent_rate / clk_div[i]) {
			val &= ~(CLKSEL_MASK << CLKSEL_SHIFT);
			val |= (i << CLKSEL_SHIFT);
			writel_relaxed(val, clk->base);
			return 0;
		}

		/* Try divider 3 */
		if (rate == parent_rate / 3) {
			val |= CLKD3SWITCH;
			writel_relaxed(val, clk->base);
			return 0;
		}
	}

	return -EINVAL;
}

static unsigned long berlin_clk_recalc_rate(struct clk_hw *hw,
					    unsigned long _parent_rate)
{
	u32 val, divider, clk_pllsel, clk_pllswitch;
	struct berlin_clk *clk = to_berlin_clk(hw);
	struct clk *parent = NULL;
	unsigned long parent_rate;
	u8 index;

	val = readl_relaxed(clk->base);
	clk_pllsel = (val >> CLKPLLSEL_SHIFT) & CLKPLLSEL_MASK;
	clk_pllswitch = val & CLKPLLSWITCH;

	if (clk_pllsel == 4)
		index = 0;
	else if (clk_pllsel < 4)
		index = clk_pllsel + 1;
	else
		return 0;

	parent = clk_get_parent_by_index(hw->clk, index);
	if (!parent)
		pr_err("%s: %s: cannot find parent (index=%u)\n", __func__,
		       __clk_get_name(hw->clk), index);
	parent_rate = __clk_get_rate(parent);
	if (parent_rate != _parent_rate && !parent_rate)
		pr_err("%s: %s: true parent rate (%ld) does not match"
		       " input parent rate (%ld)\n", __func__,
		       __clk_get_name(hw->clk),
		       parent_rate, _parent_rate);

	if (val & CLKD3SWITCH)
		divider = 3;
	else {
		if (val & CLKSWITCH) {
			val >>= CLKSEL_SHIFT;
			val &= CLKSEL_MASK;
			divider = clk_div[val];
		} else
			divider = 1;
	}

	if (divider == 0) {
		pr_err("%s: %s: invalid divider\n", __func__, __clk_get_name(hw->clk));
		return 0;
	}

	return parent_rate / divider;
}

static long berlin_clk_determine_rate(struct clk_hw *hw,
				      unsigned long rate,
				      unsigned long min_rate,
				      unsigned long max_rate,
				      unsigned long *best_parent_rate,
				      struct clk_hw **best_parent_hw)
{
	struct clk *parent = NULL;
	unsigned long parent_rate, best_rate = 0;
	int i;

	*best_parent_rate = 0;
	for (i = 0; i < __clk_get_num_parents(hw->clk); i++) {
		unsigned long new_rate;

		parent = clk_get_parent_by_index(hw->clk, i);
		if (!parent)
			continue;

		parent_rate = __clk_get_rate(parent);
		new_rate = berlin_clkdiv_find_rate_floor(hw, rate, parent_rate);

		if (new_rate > best_rate) {
			best_rate = new_rate;
			*best_parent_rate = parent_rate;
			*best_parent_hw = __clk_get_hw(parent);
		}
	}

	if (best_rate == 0) {
		parent_rate = __clk_get_rate(clk_get_parent(hw->clk));
		best_rate = berlin_clk_recalc_rate(hw,  parent_rate);
	}

	return best_rate;
}

static int berlin_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long best_parent_rate)
{
	struct clk *parent = NULL;
	unsigned long parent_rate = 0;
	unsigned int i;

	if (unlikely(rate == 0)) {
		pr_err("%s: %s: invalid input rate\n", __func__,
		       __clk_get_name(hw->clk));
		return -EINVAL;
	}

	for (i = 0; i < __clk_get_num_parents(hw->clk); i++) {
		parent = clk_get_parent_by_index(hw->clk, i);
		parent_rate = __clk_get_rate(parent);

		if (parent_rate == best_parent_rate)
			break;
	}
	if (i >= __clk_get_num_parents(hw->clk))
		return -EINVAL;

	if (parent_rate == 0)
		return -EINVAL;

	if (i == 0)
		return berlin_clkdiv_set_rate_precise(hw, rate, parent_rate, 4);
	else if (i <= 4)
		return berlin_clkdiv_set_rate_precise(hw, rate, parent_rate, i - 1);
	else
		return -EINVAL;
}

static u8 berlin_clk_get_parent(struct clk_hw *hw)
{
	u32 val;
	struct berlin_clk *clk = to_berlin_clk(hw);

	val = readl_relaxed(clk->base);
	if (val & CLKPLLSWITCH) {
		val >>= CLKPLLSEL_SHIFT;
		val &= CLKPLLSEL_MASK;
		return val;
	}

	return 0;
}

static int berlin_clk_enable(struct clk_hw *hw)
{
	u32 val;
	struct berlin_clk *clk = to_berlin_clk(hw);

	val = readl_relaxed(clk->base);
	val |= CLKEN;
	writel_relaxed(val, clk->base);

	return 0;
}

static void berlin_clk_disable(struct clk_hw *hw)
{
	u32 val;
	struct berlin_clk *clk = to_berlin_clk(hw);

	val = readl_relaxed(clk->base);
	val &= ~CLKEN;
	writel_relaxed(val, clk->base);
}

static int berlin_clk_is_enabled(struct clk_hw *hw)
{
	u32 val;
	struct berlin_clk *clk = to_berlin_clk(hw);

	val = readl_relaxed(clk->base);
	val &= CLKEN;

	return val ? 1 : 0;
}

static const struct clk_ops berlin_clk_ops = {
	.recalc_rate	= berlin_clk_recalc_rate,
	.determine_rate = berlin_clk_determine_rate,
	.set_rate	= berlin_clk_set_rate,
	.get_parent	= berlin_clk_get_parent,
	.enable		= berlin_clk_enable,
	.disable	= berlin_clk_disable,
	.is_enabled	= berlin_clk_is_enabled,
};

static void __init berlin_clk_setup(struct device_node *np)
{
	struct clk_init_data init;
	struct berlin_clk *bclk;
	struct clk *clk;
	const char *parent_names[5];
	int i, ret;

	bclk = kzalloc(sizeof(*bclk), GFP_KERNEL);
	if (WARN_ON(!bclk))
		return;

	bclk->base = of_iomap(np, 0);
	if (WARN_ON(!bclk->base))
		return;

	if (of_property_read_bool(np, "ignore-unused"))
		init.flags = CLK_IGNORE_UNUSED;
	else
		init.flags = 0;

	if (of_property_read_bool(np, "get-rate-nocache"))
		init.flags |= CLK_GET_RATE_NOCACHE;

	init.name = np->name;
	init.ops = &berlin_clk_ops;
	for (i = 0; i < ARRAY_SIZE(parent_names); i++) {
		parent_names[i] = of_clk_get_parent_name(np, i);
		if (!parent_names[i])
			break;
	}
	init.parent_names = parent_names;
	init.num_parents = i;

	bclk->hw.init = &init;

	clk = clk_register(NULL, &bclk->hw);
	if (WARN_ON(IS_ERR(clk)))
		return;

	ret = of_clk_add_provider(np, of_clk_src_simple_get, clk);
	if (WARN_ON(ret))
		return;
}
CLK_OF_DECLARE(berlin_clk, "marvell,berlin-clk", berlin_clk_setup);
