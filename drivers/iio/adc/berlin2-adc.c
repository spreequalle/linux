/*
 * Marvell Berlin2 ADC driver
 *
 * Copyright (C) 2015 Marvell Technology Group Ltd.
 *
 * Antoine Tenart <antoine.tenart@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/iio/iio.h>
#include <linux/iio/driver.h>
#include <linux/iio/machine.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/sched.h>
#include <linux/wait.h>

#define BERLIN2_SM_CTRL				0x14
#define  BERLIN2_SM_CTRL_SM_SOC_INT		BIT(1)
#define  BERLIN2_SM_CTRL_SOC_SM_INT		BIT(2)
#define  BERLIN2_SM_CTRL_ADC_SEL(x)		((x) << 5)	/* 0-15 */
#define  BERLIN2_SM_CTRL_ADC_SEL_MASK		GENMASK(8, 5)
#define  BERLIN2_SM_CTRL_ADC_POWER		BIT(9)
#define  BERLIN2_SM_CTRL_ADC_CLKSEL_DIV2	(0x0 << 10)
#define  BERLIN2_SM_CTRL_ADC_CLKSEL_DIV3	(0x1 << 10)
#define  BERLIN2_SM_CTRL_ADC_CLKSEL_DIV4	(0x2 << 10)
#define  BERLIN2_SM_CTRL_ADC_CLKSEL_DIV8	(0x3 << 10)
#define  BERLIN2_SM_CTRL_ADC_CLKSEL_MASK	GENMASK(11, 10)
#define  BERLIN2_SM_CTRL_ADC_START		BIT(12)
#define  BERLIN2_SM_CTRL_ADC_RESET		BIT(13)
#define  BERLIN2_SM_CTRL_ADC_BANDGAP_RDY	BIT(14)
#define  BERLIN2_SM_CTRL_ADC_CONT_SINGLE	(0x0 << 15)
#define  BERLIN2_SM_CTRL_ADC_CONT_CONTINUOUS	(0x1 << 15)
#define  BERLIN2_SM_CTRL_ADC_BUFFER_EN		BIT(16)
#define  BERLIN2_SM_CTRL_ADC_VREF_EXT		(0x0 << 17)
#define  BERLIN2_SM_CTRL_ADC_VREF_INT		(0x1 << 17)
#define  BERLIN2_SM_CTRL_ADC_ROTATE		BIT(19)
#define  BERLIN2_SM_CTRL_TSEN_EN		BIT(20)
#define  BERLIN2_SM_CTRL_TSEN_CLK_SEL_125	(0x0 << 21)	/* 1.25 MHz */
#define  BERLIN2_SM_CTRL_TSEN_CLK_SEL_250	(0x1 << 21)	/* 2.5 MHz */
#define  BERLIN2_SM_CTRL_TSEN_MODE_0_125	(0x0 << 22)	/* 0-125 C */
#define  BERLIN2_SM_CTRL_TSEN_MODE_10_50	(0x1 << 22)	/* 10-50 C */
#define  BERLIN2_SM_CTRL_TSEN_RESET		BIT(29)
#define BERLIN2_SM_ADC_DATA			0x20
#define  BERLIN2_SM_ADC_MASK			GENMASK(9, 0)
#define BERLIN2_SM_ADC_STATUS			0x1c
#define  BERLIN2_SM_ADC_STATUS_DATA_RDY(x)	BIT(x)		/* 0-15 */
#define  BERLIN2_SM_ADC_STATUS_DATA_RDY_MASK	GENMASK(15, 0)
#define  BERLIN2_SM_ADC_STATUS_INT_EN(x)	(BIT(x) << 16)	/* 0-15 */
#define  BERLIN2_SM_ADC_STATUS_INT_EN_MASK	GENMASK(31, 16)
#define BERLIN2_SM_TSEN_STATUS			0x24
#define  BERLIN2_SM_TSEN_STATUS_DATA_RDY	BIT(0)
#define  BERLIN2_SM_TSEN_STATUS_INT_EN		BIT(1)
#define BERLIN2_SM_TSEN_DATA			0x28
#define  BERLIN2_SM_TSEN_MASK			GENMASK(9, 0)
#define BERLIN2_SM_TSEN_CTRL			0x74
#define  BERLIN2_SM_TSEN_CTRL_START		BIT(8)
#define  BERLIN2_SM_TSEN_CTRL_SETTLING_4	(0x0 << 21)	/* 4 us */
#define  BERLIN2_SM_TSEN_CTRL_SETTLING_12	(0x1 << 21)	/* 12 us */
#define  BERLIN2_SM_TSEN_CTRL_SETTLING_MASK	BIT(21)
#define  BERLIN2_SM_TSEN_CTRL_TRIM(x)		((x) << 22)
#define  BERLIN2_SM_TSEN_CTRL_TRIM_MASK		GENMASK(25, 22)

#define BERLIN4CDP_SM_CTRL			0x14
#define  BERLIN4CDP_SM_CTRL_SM_SOC_INT		BIT(1)
#define  BERLIN4CDP_SM_CTRL_SOC_SM_INT		BIT(2)
#define  BERLIN4CDP_SM_CTRL_ADC_SEL(x)		((x) << 5)	/* 0-7 */
#define  BERLIN4CDP_SM_CTRL_ADC_SEL_MASK	GENMASK(7, 5)
#define  BERLIN4CDP_SM_CTRL_ADC_POWER		BIT(8)
#define  BERLIN4CDP_SM_CTRL_ADC_CLKSEL_DIV2	(0x0 << 9)
#define  BERLIN4CDP_SM_CTRL_ADC_CLKSEL_DIV3	(0x1 << 9)
#define  BERLIN4CDP_SM_CTRL_ADC_CLKSEL_DIV4	(0x2 << 9)
#define  BERLIN4CDP_SM_CTRL_ADC_CLKSEL_DIV8	(0x3 << 9)
#define  BERLIN4CDP_SM_CTRL_ADC_CLKSEL_MASK	GENMASK(10, 9)
#define  BERLIN4CDP_SM_CTRL_ADC_START		BIT(11)
#define  BERLIN4CDP_SM_CTRL_ADC_RESET		BIT(12)
#define BERLIN4CDP_SM_ADC_DATA			0x20
#define  BERLIN4CDP_SM_ADC_MASK			GENMASK(9, 0)
#define BERLIN4CDP_SM_ADC_STATUS		0x1c
#define  BERLIN4CDP_SM_ADC_STATUS_DATA_RDY(x)	BIT(x)		/* 0-7 */
#define  BERLIN4CDP_SM_ADC_STATUS_DATA_RDY_MASK	GENMASK(7, 0)
#define  BERLIN4CDP_SM_ADC_STATUS_INT_EN(x)	(BIT(x) << 8)	/* 0-7 */
#define  BERLIN4CDP_SM_ADC_STATUS_INT_EN_MASK	GENMASK(15, 8)
#define BERLIN4CDP_SM_TSEN_STATUS		0x24
#define  BERLIN4CDP_SM_TSEN_STATUS_DATA_RDY	BIT(0)
#define  BERLIN4CDP_SM_TSEN_STATUS_INT_EN	BIT(1)
#define BERLIN4CDP_SM_TSEN_DATA			0x28
#define  BERLIN4CDP_SM_TSEN_MASK		GENMASK(11, 0)
#define BERLIN4CDP_SM_TSEN_CTRL			0x64
#define  BERLIN4CDP_SM_CTRL_TSEN_EN		BIT(0)
#define  BERLIN4CDP_SM_CTRL_TSEN_CLK_EN		BIT(1)
#define  BERLIN4CDP_SM_CTRL_TSEN_RESET		BIT(6)
#define  BERLIN4CDP_SM_CTRL_TSEN_ISO_EN		BIT(7)
#define  BERLIN4CDP_SM_TSEN_CTRL_START		BIT(8)
#define  BERLIN4CDP_SM_CTRL_BG_DTRM(x)		((x) << 15)
#define  BERLIN4CDP_SM_CTRL_BG_DTRM_MASK	GENMASK(18, 15)

#define STR(x) #x
#define CHANNEL_MILLI_DEG_C 6
#define CHANNEL_DEG_C 9

struct adc_ops {
	void (*power_up)(struct regmap *regmap);
	void (*power_down)(struct regmap *regmap);
	void (*adc_start)(struct regmap *regmap, int channel);
	void (*adc_stop)(struct regmap *regmap, int channel);
	void (*tsen_start)(struct regmap *regmap);
	void (*tsen_stop)(struct regmap *regmap);
	int (*tsen_raw2mc)(int raw);
	irqreturn_t (*adc_tsen_irq)(int irq, void *private);
	irqreturn_t (*adc_irq)(int irq, void *private);
};

struct berlin2_adc_priv {
	struct regmap		*regmap;
	struct mutex		lock;
	const struct adc_ops	*ops;
	wait_queue_head_t	wq;
	bool			data_available;
	int			data;
};

static void berlin4cdp_power_up(struct regmap *regmap)
{
	/* Power up the ADC */
	regmap_update_bits(regmap, BERLIN4CDP_SM_CTRL,
			   BERLIN4CDP_SM_CTRL_ADC_POWER,
			   BERLIN4CDP_SM_CTRL_ADC_POWER);
}

static void berlin4cdp_power_down(struct regmap *regmap)
{
	/* Power down the ADC */
	regmap_update_bits(regmap, BERLIN4CDP_SM_CTRL,
			   BERLIN4CDP_SM_CTRL_ADC_POWER, 0);
}

static void berlin4cdp_adc_start(struct regmap *regmap, int channel)
{
	/* Enable the interrupts */
	regmap_write(regmap, BERLIN4CDP_SM_ADC_STATUS,
		     BERLIN4CDP_SM_ADC_STATUS_INT_EN(channel));

	/* Configure the ADC */
	regmap_update_bits(regmap, BERLIN4CDP_SM_CTRL,
			   BERLIN4CDP_SM_CTRL_ADC_RESET |
			   BERLIN4CDP_SM_CTRL_ADC_SEL_MASK |
			   BERLIN4CDP_SM_CTRL_ADC_CLKSEL_MASK |
			   BERLIN4CDP_SM_CTRL_ADC_START,
			   BERLIN4CDP_SM_CTRL_ADC_SEL(channel) |
			   BERLIN4CDP_SM_CTRL_ADC_CLKSEL_DIV2 |
			   BERLIN4CDP_SM_CTRL_ADC_START);
}

static void berlin4cdp_adc_stop(struct regmap *regmap, int channel)
{
	/* Disable the interrupts */
	regmap_update_bits(regmap, BERLIN4CDP_SM_ADC_STATUS,
			   BERLIN4CDP_SM_ADC_STATUS_INT_EN(channel), 0);
	regmap_update_bits(regmap, BERLIN4CDP_SM_CTRL,
			   BERLIN4CDP_SM_CTRL_ADC_START, 0);
}

static void berlin4cdp_tsen_start(struct regmap *regmap)
{
	/* Enable interrupts */
	regmap_write(regmap, BERLIN4CDP_SM_TSEN_STATUS,
		     BERLIN4CDP_SM_TSEN_STATUS_INT_EN);
	regmap_update_bits(regmap, BERLIN4CDP_SM_TSEN_CTRL,
			   BERLIN4CDP_SM_CTRL_TSEN_ISO_EN |
			   BERLIN4CDP_SM_CTRL_TSEN_RESET |
			   BERLIN4CDP_SM_CTRL_TSEN_CLK_EN,
			   BERLIN4CDP_SM_CTRL_TSEN_CLK_EN);
	/* Configure the temperature sensor */
	regmap_update_bits(regmap, BERLIN4CDP_SM_TSEN_CTRL,
			   BERLIN4CDP_SM_CTRL_BG_DTRM_MASK, BERLIN4CDP_SM_CTRL_BG_DTRM(8));
	regmap_update_bits(regmap, BERLIN4CDP_SM_TSEN_CTRL,
			   BERLIN4CDP_SM_CTRL_TSEN_EN, BERLIN4CDP_SM_CTRL_TSEN_EN);
	regmap_update_bits(regmap, BERLIN4CDP_SM_TSEN_CTRL,
			   BERLIN4CDP_SM_TSEN_CTRL_START, BERLIN4CDP_SM_TSEN_CTRL_START);
}

static void berlin4cdp_tsen_stop(struct regmap *regmap)
{
	/* Disable interrupts */
	regmap_update_bits(regmap, BERLIN4CDP_SM_TSEN_STATUS,
			   BERLIN4CDP_SM_TSEN_STATUS_INT_EN, 0);
	regmap_update_bits(regmap, BERLIN4CDP_SM_TSEN_CTRL,
			   BERLIN4CDP_SM_TSEN_CTRL_START, 0);
}

static int berlin4cdpz1_tsen_raw2mc(int raw)
{
	return (raw * 381 - 280000);
}

static int berlin4cdp_tsen_raw2mc(int raw)
{
	return (raw * 384 - 275000);
}

static irqreturn_t berlin4cdp_adc_irq(int irq, void *private)
{
	struct berlin2_adc_priv *priv = iio_priv(private);
	unsigned val;

	regmap_read(priv->regmap, BERLIN4CDP_SM_ADC_STATUS, &val);
	if (val & BERLIN4CDP_SM_ADC_STATUS_DATA_RDY_MASK) {
		regmap_read(priv->regmap, BERLIN4CDP_SM_ADC_DATA, &priv->data);
		priv->data &= BERLIN4CDP_SM_ADC_MASK;

		val &= ~BERLIN4CDP_SM_ADC_STATUS_DATA_RDY_MASK;
		regmap_write(priv->regmap, BERLIN4CDP_SM_ADC_STATUS, val);

		priv->data_available = true;
		wake_up_interruptible(&priv->wq);
	}

	return IRQ_HANDLED;
}

static irqreturn_t berlin4cdp_adc_tsen_irq(int irq, void *private)
{
	struct berlin2_adc_priv *priv = iio_priv(private);
	unsigned val;

	regmap_read(priv->regmap, BERLIN4CDP_SM_TSEN_STATUS, &val);
	if (val & BERLIN4CDP_SM_TSEN_STATUS_DATA_RDY) {
		regmap_read(priv->regmap, BERLIN4CDP_SM_TSEN_DATA, &priv->data);
		priv->data &= BERLIN4CDP_SM_TSEN_MASK;

		val &= ~BERLIN4CDP_SM_TSEN_STATUS_DATA_RDY;
		regmap_write(priv->regmap, BERLIN4CDP_SM_TSEN_STATUS, val);

		priv->data_available = true;
		wake_up_interruptible(&priv->wq);
	}

	return IRQ_HANDLED;
}

static const struct adc_ops berlin4cdpz1_ops = {
	.power_up = berlin4cdp_power_up,
	.power_down = berlin4cdp_power_down,
	.adc_start = berlin4cdp_adc_start,
	.adc_stop = berlin4cdp_adc_stop,
	.tsen_start = berlin4cdp_tsen_start,
	.tsen_stop = berlin4cdp_tsen_stop,
	.tsen_raw2mc = berlin4cdpz1_tsen_raw2mc,
	.adc_irq = berlin4cdp_adc_irq,
	.adc_tsen_irq = berlin4cdp_adc_tsen_irq,
};

static const struct adc_ops berlin4cdp_ops = {
	.power_up = berlin4cdp_power_up,
	.power_down = berlin4cdp_power_down,
	.adc_start = berlin4cdp_adc_start,
	.adc_stop = berlin4cdp_adc_stop,
	.tsen_start = berlin4cdp_tsen_start,
	.tsen_stop = berlin4cdp_tsen_stop,
	.tsen_raw2mc = berlin4cdp_tsen_raw2mc,
	.adc_irq = berlin4cdp_adc_irq,
	.adc_tsen_irq = berlin4cdp_adc_tsen_irq,
};

static void berlin2_power_up(struct regmap *regmap)
{
	/* Power up the ADC */
	regmap_update_bits(regmap, BERLIN2_SM_CTRL,
			   BERLIN2_SM_CTRL_ADC_POWER,
			   BERLIN2_SM_CTRL_ADC_POWER);
}

static void berlin2_power_down(struct regmap *regmap)
{
	/* Power down the ADC */
	regmap_update_bits(regmap, BERLIN2_SM_CTRL,
			   BERLIN2_SM_CTRL_ADC_POWER, 0);
}

static void berlin2_adc_start(struct regmap *regmap, int channel)
{
	/* Enable the interrupts */
	regmap_write(regmap, BERLIN2_SM_ADC_STATUS,
		     BERLIN2_SM_ADC_STATUS_INT_EN(channel));

	/* Configure the ADC */
	regmap_update_bits(regmap, BERLIN2_SM_CTRL,
			   BERLIN2_SM_CTRL_ADC_RESET |
			   BERLIN2_SM_CTRL_ADC_SEL_MASK |
			   BERLIN2_SM_CTRL_ADC_CLKSEL_MASK |
			   BERLIN2_SM_CTRL_ADC_START,
			   BERLIN2_SM_CTRL_ADC_SEL(channel) |
			   BERLIN2_SM_CTRL_ADC_CLKSEL_DIV2 |
			   BERLIN2_SM_CTRL_ADC_START);
}

static void berlin2_adc_stop(struct regmap *regmap, int channel)
{
	/* Disable the interrupts */
	regmap_update_bits(regmap, BERLIN2_SM_ADC_STATUS,
			   BERLIN2_SM_ADC_STATUS_INT_EN(channel), 0);
	regmap_update_bits(regmap, BERLIN2_SM_CTRL,
			   BERLIN2_SM_CTRL_ADC_START, 0);
}

static void berlin2_tsen_start(struct regmap *regmap)
{
	/* Enable interrupts */
	regmap_write(regmap, BERLIN2_SM_TSEN_STATUS,
		     BERLIN2_SM_TSEN_STATUS_INT_EN);
	regmap_update_bits(regmap, BERLIN2_SM_CTRL,
			   BERLIN2_SM_CTRL_TSEN_RESET |
			   BERLIN2_SM_CTRL_ADC_ROTATE,
			   BERLIN2_SM_CTRL_ADC_ROTATE);

	/* Configure the temperature sensor */
	regmap_update_bits(regmap, BERLIN2_SM_TSEN_CTRL,
			   BERLIN2_SM_TSEN_CTRL_TRIM_MASK |
			   BERLIN2_SM_TSEN_CTRL_SETTLING_MASK |
			   BERLIN2_SM_TSEN_CTRL_START,
			   BERLIN2_SM_TSEN_CTRL_TRIM(3) |
			   BERLIN2_SM_TSEN_CTRL_SETTLING_12 |
			   BERLIN2_SM_TSEN_CTRL_START);
}

static void berlin2_tsen_stop(struct regmap *regmap)
{
	/* Disable interrupts */
	regmap_update_bits(regmap, BERLIN2_SM_TSEN_STATUS,
			   BERLIN2_SM_TSEN_STATUS_INT_EN, 0);
	regmap_update_bits(regmap, BERLIN2_SM_TSEN_CTRL,
			   BERLIN2_SM_TSEN_CTRL_START, 0);
}

static int berlin2_tsen_raw2mc(int raw)
{
	return ((raw * 100000) / 264 - 270000);
}

static irqreturn_t berlin2_adc_irq(int irq, void *private)
{
	struct berlin2_adc_priv *priv = iio_priv(private);
	unsigned val;

	regmap_read(priv->regmap, BERLIN2_SM_ADC_STATUS, &val);
	if (val & BERLIN2_SM_ADC_STATUS_DATA_RDY_MASK) {
		regmap_read(priv->regmap, BERLIN2_SM_ADC_DATA, &priv->data);
		priv->data &= BERLIN2_SM_ADC_MASK;

		val &= ~BERLIN2_SM_ADC_STATUS_DATA_RDY_MASK;
		regmap_write(priv->regmap, BERLIN2_SM_ADC_STATUS, val);

		priv->data_available = true;
		wake_up_interruptible(&priv->wq);
	}

	return IRQ_HANDLED;
}

static irqreturn_t berlin2_adc_tsen_irq(int irq, void *private)
{
	struct berlin2_adc_priv *priv = iio_priv(private);
	unsigned val;

	regmap_read(priv->regmap, BERLIN2_SM_TSEN_STATUS, &val);
	if (val & BERLIN2_SM_TSEN_STATUS_DATA_RDY) {
		regmap_read(priv->regmap, BERLIN2_SM_TSEN_DATA, &priv->data);
		priv->data &= BERLIN2_SM_TSEN_MASK;

		val &= ~BERLIN2_SM_TSEN_STATUS_DATA_RDY;
		regmap_write(priv->regmap, BERLIN2_SM_TSEN_STATUS, val);

		priv->data_available = true;
		wake_up_interruptible(&priv->wq);
	}

	return IRQ_HANDLED;
}

static const struct adc_ops berlin2_ops = {
	.power_up = berlin2_power_up,
	.power_down = berlin2_power_down,
	.adc_start = berlin2_adc_start,
	.adc_stop = berlin2_adc_stop,
	.tsen_start = berlin2_tsen_start,
	.tsen_stop = berlin2_tsen_stop,
	.tsen_raw2mc = berlin2_tsen_raw2mc,
	.adc_irq = berlin2_adc_irq,
	.adc_tsen_irq = berlin2_adc_tsen_irq,
};

#define BERLIN2_ADC_CHANNEL(n, t)					\
	{								\
		.channel		= n,				\
		.datasheet_name		= "channel"#n,			\
		.type			= t,				\
		.indexed		= 1,				\
		.info_mask_separate	= BIT(IIO_CHAN_INFO_RAW),	\
	}

static const struct iio_chan_spec berlin2_adc_channels[] = {
	BERLIN2_ADC_CHANNEL(0, IIO_VOLTAGE),	/* external input */
	BERLIN2_ADC_CHANNEL(1, IIO_VOLTAGE),	/* external input */
	BERLIN2_ADC_CHANNEL(2, IIO_VOLTAGE),	/* external input */
	BERLIN2_ADC_CHANNEL(3, IIO_VOLTAGE),	/* external input */
	BERLIN2_ADC_CHANNEL(4, IIO_VOLTAGE),	/* reserved */
	BERLIN2_ADC_CHANNEL(5, IIO_VOLTAGE),	/* reserved */
	{				/* temperature sensor. Milli Celcius */
		.channel		= CHANNEL_MILLI_DEG_C,
		.datasheet_name		= "channel" STR(CHANNEL_MILLI_DEG_C),
		.type			= IIO_TEMP,
		.indexed		= 1,
		.info_mask_separate	= BIT(IIO_CHAN_INFO_PROCESSED),
	},
	BERLIN2_ADC_CHANNEL(7, IIO_VOLTAGE),	/* reserved */
	IIO_CHAN_SOFT_TIMESTAMP(8),		/* timestamp */
	{				/* temperature sensor. Celcius */
		.channel		= CHANNEL_DEG_C,
		.datasheet_name		= "channel" STR(CHANNEL_DEG_C),
		.type			= IIO_TEMP,
		.indexed		= 1,
		.info_mask_separate	= BIT(IIO_CHAN_INFO_PROCESSED),
	},
};

static int berlin2_adc_read(struct iio_dev *indio_dev, int channel)
{
	struct berlin2_adc_priv *priv = iio_priv(indio_dev);
	int data, ret;

	mutex_lock(&priv->lock);

	priv->ops->adc_start(priv->regmap, channel);
	ret = wait_event_interruptible_timeout(priv->wq, priv->data_available,
					       msecs_to_jiffies(1000));

	priv->ops->adc_stop(priv->regmap, channel);
	if (ret == 0)
		ret = -ETIMEDOUT;
	if (ret < 0) {
		mutex_unlock(&priv->lock);
		return ret;
	}

	data = priv->data;
	priv->data_available = false;

	mutex_unlock(&priv->lock);

	return data;
}

static int berlin2_adc_tsen_read(struct iio_dev *indio_dev)
{
	struct berlin2_adc_priv *priv = iio_priv(indio_dev);
	int data, ret;

	mutex_lock(&priv->lock);

	priv->ops->tsen_start(priv->regmap);

	ret = wait_event_interruptible_timeout(priv->wq, priv->data_available,
					       msecs_to_jiffies(1000));

	priv->ops->tsen_stop(priv->regmap);
	if (ret == 0)
		ret = -ETIMEDOUT;
	if (ret < 0) {
		mutex_unlock(&priv->lock);
		return ret;
	}

	data = priv->data;
	priv->data_available = false;

	mutex_unlock(&priv->lock);

	return data;
}

static int berlin2_adc_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan, int *val,
				int *val2, long mask)
{
	int temp;
	struct berlin2_adc_priv *priv = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (chan->type != IIO_VOLTAGE)
			return -EINVAL;

		*val = berlin2_adc_read(indio_dev, chan->channel);
		if (*val < 0)
			return *val;

		return IIO_VAL_INT;
	case IIO_CHAN_INFO_PROCESSED:
		if (chan->type != IIO_TEMP)
			return -EINVAL;

		temp = berlin2_adc_tsen_read(indio_dev);
		if (temp < 0)
			return temp;

		if (temp > 2047)
			temp -= 4096;

		/* Convert to milli Celsius */
		*val = priv->ops->tsen_raw2mc(temp);
		if (chan->channel != CHANNEL_MILLI_DEG_C) {
			// Round to Celsius
			*val = (*val + 500) / 1000;
		}
		return IIO_VAL_INT;
	default:
		break;
	}

	return -EINVAL;
}

static const struct iio_info berlin2_adc_info = {
	.driver_module	= THIS_MODULE,
	.read_raw	= berlin2_adc_read_raw,
};

static const struct of_device_id berlin2_adc_match[] = {
	{
		.compatible = "marvell,berlin2-adc",
		.data = &berlin2_ops,
	},
	{
		.compatible = "marvell,berlin4cdpz1-adc",
		.data = &berlin4cdpz1_ops,
	},
	{
		.compatible = "marvell,berlin4cdp-adc",
		.data = &berlin4cdp_ops,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, berlin2_adc_match);

static int berlin2_adc_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	struct berlin2_adc_priv *priv;
	void __iomem *base;
	struct regmap_config adc_regmap_config = {
		.reg_bits = 32,
		.val_bits = 32,
		.reg_stride = 4,
	};
	struct resource *res;
	const struct of_device_id *of_id;
	int irq, tsen_irq;
	int ret;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*priv));
	if (!indio_dev)
		return -ENOMEM;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*priv));
	priv = iio_priv(indio_dev);
	of_id = of_match_device(berlin2_adc_match, &pdev->dev);
	if (of_id)
		priv->ops = (struct adc_ops*)of_id->data;
	else
		priv->ops = &berlin2_ops;

	platform_set_drvdata(pdev, indio_dev);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);
	priv->regmap = regmap_init_mmio(NULL, base, &adc_regmap_config);

	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	irq = platform_get_irq_byname(pdev, "adc");
	if (irq < 0)
		return irq;

	tsen_irq = platform_get_irq_byname(pdev, "tsen");
	if (tsen_irq < 0)
		return tsen_irq;

	ret = devm_request_irq(&pdev->dev, irq, priv->ops->adc_irq, 0,
			       pdev->dev.driver->name, indio_dev);
	if (ret)
		return ret;

	ret = devm_request_irq(&pdev->dev, tsen_irq, priv->ops->adc_tsen_irq,
			       0, pdev->dev.driver->name, indio_dev);
	if (ret)
		return ret;

	init_waitqueue_head(&priv->wq);
	mutex_init(&priv->lock);

	indio_dev->dev.parent = &pdev->dev;
	indio_dev->name = dev_name(&pdev->dev);
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &berlin2_adc_info;

	indio_dev->channels = berlin2_adc_channels;
	indio_dev->num_channels = ARRAY_SIZE(berlin2_adc_channels);

	priv->ops->power_up(priv->regmap);
	ret = iio_device_register(indio_dev);
	if (ret) {
		priv->ops->power_down(priv->regmap);
		return ret;
	}

	return 0;
}

static int berlin2_adc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct berlin2_adc_priv *priv = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);

	priv->ops->power_down(priv->regmap);

	return 0;
}

static struct platform_driver berlin2_adc_driver = {
	.driver	= {
		.name		= "berlin2-adc",
		.of_match_table	= berlin2_adc_match,
	},
	.probe	= berlin2_adc_probe,
	.remove	= berlin2_adc_remove,
};
module_platform_driver(berlin2_adc_driver);

MODULE_AUTHOR("Antoine Tenart <antoine.tenart@free-electrons.com>");
MODULE_DESCRIPTION("Marvell Berlin2 ADC driver");
MODULE_LICENSE("GPL v2");
