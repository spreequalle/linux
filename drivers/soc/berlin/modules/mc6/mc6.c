/*
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>

#define MC6_USER_COMMAND_1	0x0024
#define MC6_MRR_DATA		0x0370

struct mc6_priv {
	void __iomem *base;
	u32 mempll;
	struct delayed_work timeout_work;
	struct device *dev;
	struct clk *clk;
	u32 prev_refresh_rate;
};

struct derating {
	char *name;
	u32 normal_requirement_ns_x_1e3;
	u32 normal_requirement_nck;
	u32 derating_plus_ns_x_1e3;
	u32 reg_offset;
	u32 start_bit;
	u32 mask;
};

static const struct derating derating[] = {
	// name,   normal_ns, normal_nck, derating_plus, reg_offset, start_bit, mask
	{"tRCD",   18000,     4,          1875,          0x3ac,      8,         0x3f},
	{"tRC",    60000,     14,         3750,          0x3ac,      16,        0xff},
	{"tRAS",   42000,     3,          1875,          0x3ac,      0,         0x7f},
	{"tRPpb",  18000,     3,          1875,          0x3b0,      0,         0x3f},
	{"tRPab",  21000,     3,          1875,          0x3b0,      24,        0x3f},
	{"tRRD",   10000,     4,          1875,          0x3b8,      8,         0x1f},
};

static u32 get_mr4(struct mc6_priv *priv)
{
	u32 val;
	int retries;

	writel_relaxed(0x11010004, priv->base + MC6_USER_COMMAND_1);
	for (retries = 10; retries; --retries) {
		udelay(1);
		val = readl_relaxed(priv->base + MC6_MRR_DATA);
		if (val & (1 << 31))
			return val;
	}

	return val;
}

static void config_derating_timing(u32 *field,
				   u32 normal_requirement_ns_x_1e3,
				   u32 normal_requirement_nck,
				   u32 derating_plus_ns_x_1e3,
				   u32 mempll)
{
	u32 val;

	val = (normal_requirement_ns_x_1e3 * mempll + 2000000 - 1) / 2000000;
	if (val < normal_requirement_nck)
		val = normal_requirement_nck + (derating_plus_ns_x_1e3 * mempll + 2000000 - 1) / 2000000;
	else
		val = ((normal_requirement_ns_x_1e3 + derating_plus_ns_x_1e3) * mempll + 2000000 - 1) / 2000000;
	*field = val;
}

static void write_mc_reg_field(struct mc6_priv *priv, u32 offset,
			       u32 start_bit, u32 mask, u32 value)
{
	u32 regval;

	regval = readl_relaxed(priv->base + offset);
	regval &= ~(mask << start_bit);
	regval |= (value & mask) << start_bit;
	writel_relaxed(regval, priv->base + offset);
}

static u32 read_mc_reg_field(struct mc6_priv *priv, u32 offset,
			     u32 start_bit, u32 mask)
{
	u32 regval;

	regval = readl_relaxed(priv->base + offset);
	return (regval >> start_bit) & mask;
}

static void program_refi(struct mc6_priv *priv, int refresh_rate)
{
	u32 refi, offset = 0x394, start_bit = 0, mask = 0x3fff;
	struct device *dev = priv->dev;

	refi = read_mc_reg_field(priv, offset, start_bit, mask);
	dev_dbg(dev, "[tREFI]: 0x%x -> ", refi);
	switch (refresh_rate) {
	case 1:
		refi = 390;
		break;
	case 2:
		refi = 195;
		break;
	case 3:
		refi = 97;
		break;
	case 4:
		refi = 48;
		break;
	case 5:
	case 6:
	case 7:
		refi = 24;
		break;
	default:
		refi = 97;
		break;
	}
	write_mc_reg_field(priv, offset, start_bit, mask, refi);
	dev_dbg(dev, "0x%x\n", read_mc_reg_field(priv, offset, start_bit, mask));
}

static void program_timings(struct mc6_priv *priv, u32 mempll, u32 derating_on)
{

	u32 i, field;
	const struct derating *p;
	struct device *dev = priv->dev;

	for (i = 0, p = derating; i < ARRAY_SIZE(derating); i++, p++) {
		dev_dbg(dev, "[%s]: 0x%x -> ", p->name, read_mc_reg_field(priv, p->reg_offset, p->start_bit, p->mask));
		config_derating_timing(&field, p->normal_requirement_ns_x_1e3, p->normal_requirement_nck, p->derating_plus_ns_x_1e3 * derating_on, mempll);
		write_mc_reg_field(priv, p->reg_offset, p->start_bit, p->mask, field);
		dev_dbg(dev, "0x%x\n", read_mc_reg_field(priv, p->reg_offset, p->start_bit, p->mask));
	}
}

static void program_refi_and_other_timings(struct mc6_priv *priv,
					   int refresh_rate, u32 mempll)
{
	program_refi(priv, refresh_rate);
	program_timings(priv, mempll, refresh_rate == 6);
}

static void mc6_work(struct work_struct *work)
{
	struct delayed_work *d = container_of(work, struct delayed_work, work);
	struct mc6_priv *priv = container_of(d, struct mc6_priv, timeout_work);
	u32 mr4 = get_mr4(priv);

	mr4 &= 7;
	if (mr4 > 6 || mr4 == 0)
		dev_warn(priv->dev, "may not operate properly\n");

	if (mr4 != priv->prev_refresh_rate) {
		program_refi_and_other_timings(priv, mr4, priv->mempll);
	}
	priv->prev_refresh_rate = mr4;

	schedule_delayed_work(&priv->timeout_work, HZ);
}

static const struct of_device_id mc6_of_match[] = {
	{ .compatible = "marvell,berlin-mc6", },
	{},
};
MODULE_DEVICE_TABLE(of, mc6_of_match);

static int mc6_probe(struct platform_device *pdev)
{
	u32 mr4;
	struct mc6_priv *priv;
	struct resource *res;
	struct device *dev = &pdev->dev;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		dev_err(dev, "couldn't get clock\n");
		return PTR_ERR(priv->clk);
	}
	clk_prepare_enable(priv->clk);

	INIT_DELAYED_WORK(&priv->timeout_work, mc6_work);

	priv->dev = dev;
	priv->mempll = clk_get_rate(priv->clk) / 1000000;
	priv->prev_refresh_rate = (uint32_t) -1;
	platform_set_drvdata(pdev, priv);

	/* program refi once during init */
	mr4 = get_mr4(priv);
	program_refi_and_other_timings(priv, mr4, priv->mempll);

	schedule_delayed_work(&priv->timeout_work, HZ);

	return 0;
}

static int mc6_remove(struct platform_device *pdev)
{
	struct mc6_priv *priv = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&priv->timeout_work);
	clk_disable_unprepare(priv->clk);

	return 0;
}

static struct platform_driver mc6_driver = {
	.probe		= mc6_probe,
	.remove		= mc6_remove,
	.driver		= {
		.name	= "berlin-mc6",
		.of_match_table = mc6_of_match,
	},
};
module_platform_driver(mc6_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jisheng Zhang <jszhang@marvell.com>");
MODULE_DESCRIPTION("Marvell Berlin MC6 DDR Controller");
