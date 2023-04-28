#include <linux/cpufreq.h>
#include <linux/devfreq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

static LIST_HEAD(ppm_profiles);

struct berlin_ppm_priv {
	struct platform_device *ppm_pdev;

	int num_devfreqs;
	struct devfreq **devfreqs;

	struct berlin_ppm_profile *current_profile;
};

struct berlin_ppm_profile {
	struct list_head node;

	const char *name;
	struct berlin_ppm_priv *ppm_priv;

	u64 *dev_rates_normal;
	u64 cpu_rate_normal;
};

static int ppm_of_add_profiles(struct device *dev)
{
	struct device_node *np;
	struct device_node *node, *child;
	struct devfreq *devfreq;
	struct berlin_ppm_profile *profile;
	int i, size, ret;
	struct berlin_ppm_priv *ppm_priv = dev->driver_data;

	np = of_get_child_by_name(dev->of_node, "profiles");
	if (!np) {
		dev_warn(dev, "cannot find profiles\n");
		return 0;
	}

	for (i = 0;; i++) {
		node = of_parse_phandle(dev->of_node, "devfreq", i);
		if (!node)
			break;

		of_node_put(node);
	}
	ppm_priv->num_devfreqs = i;

	size = sizeof(struct devfreq *) * ppm_priv->num_devfreqs;
	ppm_priv->devfreqs = devm_kzalloc(dev, size, GFP_KERNEL);
	if (ppm_priv->devfreqs == NULL) {
		dev_err(dev, "cannot allocate memory for 'devfreqs'\n");
		return -ENOMEM;
	}

	for (i = 0;; i++) {
		devfreq = devfreq_get_devfreq_by_phandle(dev, i);
		if (IS_ERR(devfreq))
			break;

		ppm_priv->devfreqs[i] = devfreq;
	}

	for_each_child_of_node(np, child) {
		profile = devm_kzalloc(dev, sizeof(*profile), GFP_KERNEL);
		if (profile == NULL) {
			ret = -ENOMEM;
			dev_err(dev, "cannot allocate memory for 'devfreqs'\n");
			break;
		}

		profile->name = child->name;
		profile->ppm_priv = ppm_priv;

		ret = of_property_read_u64(child, "cpufreq-normal",
					   &profile->cpu_rate_normal);
		if (ret) {
			dev_err(dev, "fail to parse 'cpufreq-normal' (%d)\n",
				ret);
			profile->cpu_rate_normal = 0;
		}

		size = sizeof(profile->dev_rates_normal[0]) * ppm_priv->num_devfreqs;
		profile->dev_rates_normal = devm_kzalloc(dev, size, GFP_KERNEL);
		if (profile->dev_rates_normal == NULL) {
			dev_err(dev, "cannot allocate memory"
				" for 'dev_rates_normal'\n");
			break;
		}

		ret = of_property_read_u64_array(child,
						 "devfreq-normal",
						 profile->dev_rates_normal,
						 ppm_priv->num_devfreqs);
		if (ret) {
			dev_err(dev, "cannot read property 'devfreq-normal'"
				" (%d)\n", ret);
			break;
		}

		list_add_tail(&profile->node, &ppm_profiles);
	}

	return ret;
}

static void ppm_profile_update_devfreq(struct device *dev,
				       struct berlin_ppm_priv *ppm_priv,
				       struct berlin_ppm_profile *new_profile)
{
	int i, ret;

	if (new_profile->cpu_rate_normal) {
		ret = cpufreq_driver_target(cpufreq_cpu_get(0),
					    new_profile->cpu_rate_normal,
					    0);
		if (ret)
			dev_err(dev, "cannot update CPU frequency to"
				" %llu\n", new_profile->cpu_rate_normal);
	}

	for (i = 0; i < ppm_priv->num_devfreqs; i++) {
		struct devfreq *devfreq;
		unsigned long new_freq;
		u32 flags = 0;

		new_freq = new_profile->dev_rates_normal[i];
		if (new_freq == 0)
			continue;

		devfreq = ppm_priv->devfreqs[i];
		if (strncmp("userspace", devfreq->governor_name,
			    DEVFREQ_NAME_LEN) != 0)
			continue;

		mutex_lock(&devfreq->lock);

		ret = devfreq->profile->target(devfreq->dev.parent,
					       &new_freq, flags);
		if (ret)
			goto err_update_devfreq;

		devfreq->previous_freq = new_freq;

err_update_devfreq:
		mutex_unlock(&devfreq->lock);

		if (ret) {
			dev_err(dev, "cannot update devfreq\n");
			break;
		}
	}
}

static const struct of_device_id berlin_ppm_of_match[] = {
	{ .compatible = "marvell,berlin-ppm", },
	{},
};
MODULE_DEVICE_TABLE(of, berlin_ppm_of_match);

static ssize_t ppm_show_available_profiles(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct berlin_ppm_priv *ppm_priv = dev->driver_data;
	struct berlin_ppm_profile *profile;
	ssize_t offset = 0;

	if (!ppm_priv) {
		dev_warn(dev, "Unavailable PPM data\n");
		goto out;
	}

	list_for_each_entry(profile, &ppm_profiles, node) {
		offset += scnprintf(&buf[offset],
				    PAGE_SIZE - offset - 2,
				    "%s ", profile->name);

		if (offset >= PAGE_SIZE - 2)
			break;
	}

out:
	offset += sprintf(&buf[offset], "\n");
	return offset;
}

static ssize_t ppm_show_current_profile(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct berlin_ppm_priv *ppm_priv = dev->driver_data;
	ssize_t offset = 0;

	if (!ppm_priv) {
		dev_warn(dev, "Unavailable PPM data\n");
		goto out;
	}

	if (ppm_priv->current_profile == NULL) {
		offset += scnprintf(&buf[offset],
				    PAGE_SIZE - offset - 2,
				    "(none)");
	} else {
		offset += scnprintf(&buf[offset],
				    PAGE_SIZE - offset - 2,
				    ppm_priv->current_profile->name);
	}

out:
	offset += sprintf(&buf[offset], "\n");
	return offset;
}

static ssize_t ppm_store_change_profile(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct berlin_ppm_priv *ppm_priv = dev->driver_data;
	struct berlin_ppm_profile *profile, *new_profile = NULL;
	char str_profile[32];
	int ret;

	if (!ppm_priv) {
		dev_err(dev, "Unavailable PPM data\n");
		return -EINVAL;
	}

	ret = sscanf(buf, "%31s", str_profile);
	if (ret != 1)
		return -EINVAL;

	list_for_each_entry(profile, &ppm_profiles, node) {
		ret = strncmp(str_profile, profile->name,
			      sizeof(str_profile));
		if (ret == 0) {
			new_profile = profile;
			break;
		}
	}

	if (new_profile == NULL) {
		dev_err(dev, "cannot find profile '%s'\n",
			str_profile);
		return -EINVAL;
	}

	ppm_profile_update_devfreq(dev, ppm_priv, new_profile);
	ppm_priv->current_profile = new_profile;

	return count;
}

#define BERLIN_PPM_DEV_ATTR_RO(_name) \
	static DEVICE_ATTR(_name, S_IRUGO, \
			   ppm_show_##_name, \
			   NULL)

#define BERLIN_PPM_DEV_ATTR_WR(_name) \
	static DEVICE_ATTR(_name, S_IWUSR, \
			   NULL, \
			   ppm_store_##_name)

BERLIN_PPM_DEV_ATTR_RO(available_profiles);
BERLIN_PPM_DEV_ATTR_RO(current_profile);
BERLIN_PPM_DEV_ATTR_WR(change_profile);

static struct attribute *ppm_sysfs_entries[] = {
	&dev_attr_available_profiles.attr,
	&dev_attr_current_profile.attr,
	&dev_attr_change_profile.attr,
	NULL
};

static struct attribute_group ppm_attr_group = {
	.attrs	= ppm_sysfs_entries,
};

static int berlin_ppm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct berlin_ppm_priv *ppm_priv;
	int ret;

	ppm_priv = devm_kzalloc(dev, sizeof(*ppm_priv), GFP_KERNEL);
	if (!ppm_priv)
		return -ENOMEM;

	ppm_priv->ppm_pdev = pdev;
	dev_set_drvdata(dev, ppm_priv);

	ppm_of_add_profiles(dev);
	ret = sysfs_create_group(&dev->kobj, &ppm_attr_group);
	if (ret)
		dev_err(dev, "unable to create sysfs group (%d)\n", ret);

	return 0;
}

static int berlin_ppm_remove(struct platform_device *pdev)
{
	INIT_LIST_HEAD(&ppm_profiles);
	sysfs_remove_group(&pdev->dev.kobj, &ppm_attr_group);

	return 0;
}

static struct platform_driver berlin_ppm_driver = {
	.probe			= berlin_ppm_probe,
	.remove			= berlin_ppm_remove,
	.driver	= {
		.name		= "berlin-ppm",
		.owner		= THIS_MODULE,
		.of_match_table = berlin_ppm_of_match,
	},
};

#ifdef MODULE
module_platform_driver(berlin_ppm_driver);
#else
static int berlin_ppm_init(void)
{
	return platform_driver_register(&berlin_ppm_driver);
}
device_initcall_sync(berlin_ppm_init);
#endif


MODULE_AUTHOR("Marvell-Berlin");
MODULE_DESCRIPTION("Berlin PPM Driver");
MODULE_LICENSE("GPL");
