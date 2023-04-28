
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/devfreq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pm_opp.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include "governor.h"

struct berlin_devfreq_priv {
	struct platform_device *devfreq_pdev;
	struct devfreq *devfreq;
	struct regulator *dev_rgltr;

	struct mutex lock;
	struct clk *dev_clk;
	unsigned long max_freq;
	unsigned long cur_freq;

	struct berlin_devfreq_nb *clk_parent_nbs;
};

struct berlin_devfreq_nb {
	struct berlin_devfreq_priv *priv;
	struct notifier_block nb;
	struct clk *clk;
};

#define to_berlin_devfreq_nb(nb) \
	container_of(nb, struct berlin_devfreq_nb, nb)

static int berlin_devfreq_target(struct device *dev, unsigned long *_freq,
				 u32 flags)
{
	return dev_pm_opp_set_rate(dev, *_freq);
}

static int berlin_devfreq_get_dev_status(struct device *dev,
					 struct devfreq_dev_status *stat)
{
	return 0;
}

static struct devfreq_dev_profile berlin_devfreq_profile = {
	.polling_ms	= 0,
	.target		= berlin_devfreq_target,
	.get_dev_status = berlin_devfreq_get_dev_status,
};

static const struct of_device_id berlin_devfreq_of_match[] = {
	{ .compatible = "marvell,berlin-devfreq", },
	{},
};
MODULE_DEVICE_TABLE(of, berlin_devfreq_of_match);

static int berlin_devfreq_clk_parent_notify(struct notifier_block *nb,
					    unsigned long event, void *data)
{
	struct berlin_devfreq_priv *devfreq_priv = to_berlin_devfreq_nb(nb)->priv;

	if (event == POST_RATE_CHANGE) {
		mutex_lock(&devfreq_priv->devfreq->lock);
		update_devfreq(devfreq_priv->devfreq);
		mutex_unlock(&devfreq_priv->devfreq->lock);
	}

	return 0;
}

static int berlin_devfreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct berlin_devfreq_priv *devfreq_priv;
	int i, size, num_clk_parents, err;

	devfreq_priv = devm_kzalloc(dev, sizeof(*devfreq_priv), GFP_KERNEL);
	if (!devfreq_priv)
		return -ENOMEM;

	mutex_init(&devfreq_priv->lock);

	dev_pm_opp_of_add_table(dev);

	devfreq_priv->dev_rgltr = devm_regulator_get(dev, "dev");
	if (IS_ERR(devfreq_priv->dev_rgltr)) {
		dev_err(dev, "Cannot get the regulator \"dev_rgltr\"\n");
		return PTR_ERR(devfreq_priv->dev_rgltr);
	}

	devfreq_priv->devfreq_pdev = pdev;
	devfreq_priv->dev_clk = devm_clk_get(dev, NULL);
	if (IS_ERR(devfreq_priv->dev_clk)) {
		dev_err(dev, "can't get device clock.\n");
		return PTR_ERR(devfreq_priv->dev_clk);
	}

	devfreq_priv->max_freq = clk_round_rate(devfreq_priv->dev_clk, ULONG_MAX);
	devfreq_priv->cur_freq = clk_get_rate(devfreq_priv->dev_clk);

	num_clk_parents = __clk_get_num_parents(devfreq_priv->dev_clk);
	size = sizeof(devfreq_priv->clk_parent_nbs[0]) * num_clk_parents;
	devfreq_priv->clk_parent_nbs = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!devfreq_priv->clk_parent_nbs)
		return -ENOMEM;

	for (i = 0; i < num_clk_parents; i++) {
		struct clk *clk_parent;
		struct notifier_block *nb;

		clk_parent = clk_get_parent_by_index(devfreq_priv->dev_clk, i);
		if (!clk_parent)
			continue;

		devfreq_priv->clk_parent_nbs[i].priv = devfreq_priv;
		devfreq_priv->clk_parent_nbs[i].clk = clk_parent;
		nb = &(devfreq_priv->clk_parent_nbs[i].nb);
		nb->notifier_call = berlin_devfreq_clk_parent_notify;
		err = clk_notifier_register(clk_parent, nb);
		if (err)
			dev_warn(dev, "fail to register notifier\n");
	}

	platform_set_drvdata(pdev, devfreq_priv);

	berlin_devfreq_profile.initial_freq = clk_get_rate(devfreq_priv->dev_clk);
	devfreq_priv->devfreq = devm_devfreq_add_device(dev, &berlin_devfreq_profile,
							"userspace", NULL);
	if (IS_ERR(devfreq_priv->devfreq)) {
		dev_err(dev, "Failed to add devfreq device\n");
		return PTR_ERR(devfreq_priv->devfreq);
	}

	err = devm_devfreq_register_opp_notifier(dev, devfreq_priv->devfreq);
	if (err < 0) {
		dev_err(dev, "Failed to register opp notifier\n");
		return err;
	}

	return 0;
}

static int berlin_devfreq_remove(struct platform_device *pdev)
{
	int i, num_clk_parents;
	struct berlin_devfreq_priv *priv = platform_get_drvdata(pdev);

	num_clk_parents = __clk_get_num_parents(priv->dev_clk);
	for (i = 0; i < num_clk_parents; i++)
		clk_notifier_unregister(priv->clk_parent_nbs[i].clk,
					&priv->clk_parent_nbs[i].nb);

	return 0;
}

static struct platform_driver berlin_devfreq_driver = {
	.probe		= berlin_devfreq_probe,
	.remove		= berlin_devfreq_remove,
	.driver	= {
		.name	= "berlin-devfreq",
		.owner	= THIS_MODULE,
		.of_match_table = berlin_devfreq_of_match,
	},
};
module_platform_driver(berlin_devfreq_driver);

MODULE_AUTHOR("Hongjun Yuan <hyuan@marvell.com>");
MODULE_DESCRIPTION("Berlin PPM Driver");
MODULE_LICENSE("GPL");
