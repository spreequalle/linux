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
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/clk-provider.h>
#include <linux/fs.h>
#include <linux/device.h>

#include "clk.h"

void __init berlin_pll_setup(struct device_node *np,
			const struct clk_ops *ops)
{
	struct clk_init_data init;
	struct berlin_pll *pll;
	const char *parent_name;
	struct clk *clk;
	int ret;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (WARN_ON(!pll))
		return;

	pll->ctrl = of_iomap(np, 0);
	pll->bypass = of_iomap(np, 1);
	ret = of_property_read_u8(np, "bypass-shift", &pll->bypass_shift);
	if (WARN_ON(!pll->ctrl || !pll->bypass || ret))
		return;

	init.name = np->name;
	init.ops = ops;
	parent_name = of_clk_get_parent_name(np, 0);
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = 0;

	pll->hw.init = &init;

	clk = clk_register(NULL, &pll->hw);
	if (WARN_ON(IS_ERR(clk)))
		return;

	ret = of_clk_add_provider(np, of_clk_src_simple_get, clk);
	if (WARN_ON(ret))
		return;
}

static struct clk * __init berlin_avpllch_setup(struct device_node *np,
						u8 which, void __iomem *base,
						const struct clk_ops *ops)
{
	struct berlin_avpllch *avpllch;
	struct clk_init_data init;
	struct clk *clk;
	int err;

	err = of_property_read_string_index(np, "clock-output-names",
					    which, &init.name);
	if (WARN_ON(err))
		goto err_read_output_name;

	avpllch = kzalloc(sizeof(*avpllch), GFP_KERNEL);
	if (!avpllch)
		goto err_read_output_name;

	avpllch->base = base;
	avpllch->which = which;

	init.ops = ops;
	init.parent_names = &np->name;
	init.num_parents = 1;
	init.flags = 0;

	if (of_property_read_bool(np, "get-rate-nocache"))
		init.flags |= CLK_GET_RATE_NOCACHE;

	avpllch->hw.init = &init;

	clk = clk_register(NULL, &avpllch->hw);
	if (WARN_ON(IS_ERR(clk)))
		goto err_clk_register;

	return clk;

err_clk_register:
	kfree(avpllch);
err_read_output_name:
	return ERR_PTR(-EINVAL);
}

static ssize_t berlin_avpll_show_channel_freqs(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct berlin_avpll *avpll = (struct berlin_avpll*)dev->driver_data;
	ssize_t offset = 0;
	int i;

	if (!avpll) {
		pr_warn("Unavailable AVPLL data\n");
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(avpll->chs); i++) {
		offset += scnprintf(&buf[offset],
				    PAGE_SIZE - offset - 1,
				    "%lu ",
				    clk_get_rate(avpll->chs[i]));
	}

out:
	offset += sprintf(&buf[offset], "\n");
	return offset;
}

static ssize_t berlin_avpll_store_channel_freqs(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct berlin_avpll *avpll = (struct berlin_avpll*)dev->driver_data;
	u64 freqs[ARRAY_SIZE(avpll->chs)];
	const char *ptr = buf, *nptr;
	int i;

	if (!avpll) {
		pr_warn("Unavailable AVPLL data\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(avpll->chs); i++) {
		if (ptr == NULL)
			return -EINVAL;

		freqs[i] = simple_strtoul(ptr, (char**)&nptr, 10);
		if (ptr == nptr)
			return -EINVAL;

		ptr = skip_spaces(nptr);
	}

	for (i = 0; i < ARRAY_SIZE(avpll->chs); i++) {
		clk_set_rate(avpll->chs[i], freqs[i]);
	}

	return count;
}

static DEVICE_ATTR(cur_channel_freqs, S_IRUGO,
		   berlin_avpll_show_channel_freqs, NULL);
static DEVICE_ATTR(set_channel_freqs, S_IWUSR, NULL,
		   berlin_avpll_store_channel_freqs);

static struct attribute *avpll_sysfs_entries[] = {
	&dev_attr_cur_channel_freqs.attr,
	&dev_attr_set_channel_freqs.attr,
	NULL
};

static const struct attribute_group avpll_attr_group = {
	.attrs	= avpll_sysfs_entries,
};

void __init berlin_avpll_sysfs_init(struct berlin_avplldata *data)
{
	struct berlin_avpll *avpll;
	struct class *pll_class;
	struct device *dev;
	int i, ret;

	pll_class = class_create(THIS_MODULE, "pll");
	if (!pll_class) {
		pr_err("unable to create class pll\n");
		goto err_out;
	}

	for (i = 0; i < ARRAY_SIZE(data->avpll_table); i++) {
		avpll = data->avpll_table[i];
		if (!avpll)
			continue;

		dev = device_create(pll_class, NULL, MKDEV(0, 0), NULL,
				    __clk_get_name(avpll->hw.clk));
		if (!dev) {
			pr_warn("%s:%d unable to create device %s\n",
				__func__, __LINE__,
				__clk_get_name(avpll->hw.clk));
			continue;
		}

		dev_set_drvdata(dev, avpll);
		ret = sysfs_create_group(&dev->kobj, &avpll_attr_group);
		if (ret)
			pr_warn("%s:%d unable to create sysfs (%d)\n",
				__func__, __LINE__, ret);
	}

err_out:
	return;
}

void __init berlin_avpll_setup(struct device_node *np,
			struct berlin_avplldata *data)
{
	struct clk_init_data init;
	struct berlin_avpll *avpll;
	const char *parent_name;
	struct clk *clk;
	int i, index, ret;

	for (index = 0; index < ARRAY_SIZE(data->avpll_table); index++) {
		if (data->avpll_table[index] == NULL)
			break;
	}
	if (index >= ARRAY_SIZE(data->avpll_table)) {
		pr_err("AVPLL table is full\n");
		return;
	}

	avpll = kzalloc(sizeof(*avpll), GFP_KERNEL);
	if (WARN_ON(!avpll))
		return;

	data->avpll_table[index] = avpll;

	avpll->base = of_iomap(np, 0);
	if (WARN_ON(!avpll->base))
		return;

	avpll->data = data;
	init.name = np->name;
	init.ops = data->avpll_ops;
	parent_name = of_clk_get_parent_name(np, 0);
	init.parent_names = &parent_name;
	init.num_parents = 1;
	init.flags = 0;

	avpll->hw.init = &init;

	clk = clk_register(NULL, &avpll->hw);
	if (WARN_ON(IS_ERR(clk)))
		return;

	ret = of_clk_add_provider(np, of_clk_src_simple_get, clk);
	if (WARN_ON(ret))
		return;

	for (i = 0; i < ARRAY_SIZE(avpll->chs); i++) {
		avpll->chs[i] = berlin_avpllch_setup(np, i, avpll->base,
						     data->avpllch_ops);
		if (WARN_ON(IS_ERR(avpll->chs[i])))
			return;
	}

	avpll->onecell_data.clks = avpll->chs;
	avpll->onecell_data.clk_num = i;

	ret = of_clk_add_provider(np, of_clk_src_onecell_get,
				  &avpll->onecell_data);
	if (WARN_ON(ret))
		return;
}
