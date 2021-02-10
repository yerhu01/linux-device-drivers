// SPDX-License-Identifier: GPL-2.0
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>


static int nunchuk_probe(struct i2c_client *client)
{
	u8 buf[2];
	int ret;

	buf[0] = 0xf0;
	buf[1] = 0x55;
	ret = i2c_master_send(client, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		dev_err(&client->dev, "i2c_master_send failed (%d)\n", ret);
		return ret;
	}

	udelay(1000);

	buf[0] = 0xfb;
	buf[1] = 0x00;
	ret = i2c_master_send(client, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		dev_err(&client->dev, "i2c_master_send failed (%d)\n", ret);
		return ret;
	}

	dev_err(&client->dev, "%s called\n", __func__);

	return 0;
}

static int nunchuk_remove(struct i2c_client *client)
{
	pr_err("%s called\n", __func__);

	return 0;
}

static const struct of_device_id nunchuk_dt_ids[] = {
	{ .compatible = "nintendo,nunchuk" },
	{ },
};

static struct i2c_driver nunchuk_driver = {
	.driver = {
		.name = "nunchuk",
		.of_match_table = nunchuk_dt_ids,
	},
	.probe_new = nunchuk_probe,
	.remove = nunchuk_remove,
};

module_i2c_driver(nunchuk_driver);
MODULE_LICENSE("GPL");
