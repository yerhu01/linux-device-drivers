// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>

/* Add your code here */

static int serial_probe(struct platform_device *pdev)
{
	pr_info("Called %s\n", __func__);
	return 0;
}

static int serial_remove(struct platform_device *pdev)
{
	pr_info("Called %s\n", __func__);
        return 0;
}

static struct of_device_id serial_dt_ids[] = {
	{ .compatible = "bootlin,serial" },
	{ },
};
MODULE_DEVICE_TABLE(of, serial_dt_ids);

static struct platform_driver serial_driver = {
	.driver = {
		.name = "serial",
		.owner = THIS_MODULE,
		.of_match_table = serial_dt_ids,
        },
        .probe = serial_probe,
        .remove = serial_remove,
};

module_platform_driver(serial_driver);
MODULE_LICENSE("GPL");
