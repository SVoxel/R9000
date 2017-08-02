/*
 * Annapurna Labs thermal driver.
 *
 * Copyright (C) 2013 Annapurna Labs
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/err.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>

#include "al_hal_thermal_sensor.h"

#define TIMEOUT_MS	1000

struct al_thermal_dev {
	struct al_thermal_sensor_handle handle;
};

static inline int thermal_enable(
	struct al_thermal_sensor_handle *handle)
{
	int timeout;
	
	al_thermal_sensor_enable_set(handle, 1);

	for (timeout = 0; timeout < TIMEOUT_MS; timeout++) {
		if (al_thermal_sensor_is_ready(handle))
			break;
		udelay(1000);
	}
	if (timeout == TIMEOUT_MS) {
		pr_err("%s: al_thermal_sensor_is_ready timed out!\n", __func__);
		return -ETIME;
	}

	al_thermal_sensor_trigger_continuous(handle);

	return 0;
}

static inline int thermal_get_temp(struct thermal_zone_device *thermal,
				unsigned long *temp)
{
	struct al_thermal_dev *al_dev = thermal->devdata;
	int timeout;

	for (timeout = 0; timeout < TIMEOUT_MS; timeout++) {
		if (al_thermal_sensor_readout_is_valid(&al_dev->handle))
			break;
		udelay(1000);
	}
	if (timeout == TIMEOUT_MS) {
		pr_err("%s: al_thermal_sensor_readout_is_valid timed out!\n",
				__func__);
		return -ETIME;
	}

	*temp = al_thermal_sensor_readout_get(&al_dev->handle);

	return 0;
}

static struct thermal_zone_device_ops ops = {
	.get_temp = thermal_get_temp,
};

#ifdef CONFIG_PM
static int al_thermal_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct thermal_zone_device *al_thermal = platform_get_drvdata(pdev);
	struct al_thermal_dev *al_dev = al_thermal->devdata;

	/* Disable Annapurna Labs Thermal Sensor */
	al_thermal_sensor_enable_set(&al_dev->handle, 0);

	pr_info("%s: Suspended.\n", __func__);

	return 0;
}

static int al_thermal_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct thermal_zone_device *al_thermal = platform_get_drvdata(pdev);
	struct al_thermal_dev *al_dev = al_thermal->devdata;
	int err = 0;

	/* Enable Annapurna Labs Thermal Sensor */
	err = thermal_enable(&al_dev->handle);
	if (err) {
		pr_err("%s: thermal_enable failed!\n", __func__);
		return err;
	}

	pr_info("%s: Resumed.\n", __func__);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(al_thermal_pm_ops, al_thermal_suspend,
		al_thermal_resume);

static int al_thermal_probe(struct platform_device *pdev)
{
	struct thermal_zone_device *al_thermal = NULL;
	struct al_thermal_dev *al_dev;
	struct device_node *np = pdev->dev.of_node;
	struct resource *al_res =
		platform_get_resource(pdev, IORESOURCE_MEM, 0);
	void __iomem *thermal_base;
	int err;

	if (!np) {
		pr_err("%s: Failed: DT data not available\n", __func__);
		return -EINVAL;
	}

	if (!al_res) {
		pr_err("%s: memory resource missing\n", __func__);
		return -ENODEV;
	}

	al_dev = devm_kzalloc(&pdev->dev, sizeof(*al_dev), GFP_KERNEL);
	if (!al_dev) {
		pr_err("%s: kzalloc fail\n", __func__);
		return -ENOMEM;
	}

	thermal_base = devm_ioremap(&pdev->dev, al_res->start,
			resource_size(al_res));
	if (!thermal_base) {
		pr_err("%s: ioremap failed\n", __func__);
		return -ENOMEM;
	}

	err = al_thermal_sensor_handle_init(&al_dev->handle,
			thermal_base);
	if (err) {
		pr_err("%s: al_thermal_sensor_init failed!\n", __func__);
		return err;
	}

	err = thermal_enable(&al_dev->handle);
	if (err) {
		pr_err("%s: thermal_enable failed!\n", __func__);
		return err;
	}

	al_thermal = thermal_zone_device_register("al_thermal", 0, 0,
				al_dev, &ops, NULL, 0, 0);
	if (IS_ERR(al_thermal)) {
		pr_err("%s: thermal zone device is NULL\n", __func__);
		err = PTR_ERR(al_thermal);
		return err;
	}

	platform_set_drvdata(pdev, al_thermal);

	pr_info("%s: Thermal Sensor Loaded at: 0x%p.\n",
			__func__, thermal_base);

	return 0;
}

static int al_thermal_exit(struct platform_device *pdev)
{
	struct thermal_zone_device *al_thermal = platform_get_drvdata(pdev);
	struct al_thermal_dev *al_dev = al_thermal->devdata;

	thermal_zone_device_unregister(al_thermal);
	platform_set_drvdata(pdev, NULL);

	al_thermal_sensor_enable_set(&al_dev->handle, 0);

	return 0;
}

static const struct of_device_id al_thermal_id_table[] = {
	{ .compatible = "annapurna-labs,al-thermal" },
	{}
};
MODULE_DEVICE_TABLE(of, al_thermal_id_table);

static struct platform_driver al_thermal_driver = {
	.probe = al_thermal_probe,
	.remove = al_thermal_exit,
	.driver = {
		.name = "al_thermal",
		.owner = THIS_MODULE,
		.pm = &al_thermal_pm_ops,
		.of_match_table = of_match_ptr(al_thermal_id_table),
	},
};

module_platform_driver(al_thermal_driver);

MODULE_DESCRIPTION("Annapurna Labs thermal driver");
MODULE_LICENSE("GPL");
