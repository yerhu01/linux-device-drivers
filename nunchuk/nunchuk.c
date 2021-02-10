// SPDX-License-Identifier: GPL-2.0
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>

struct nunchuk_dev {
	struct i2c_client *i2c_client;
};

static int nunchuk_read_registers(struct i2c_client *client, u8 *recv)
{
	u8 buf[1];
	int ret;

	usleep_range(10000, 20000);

	buf[0] = 0x00;
	ret = i2c_master_send(client, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		dev_err(&client->dev, "i2c_master_send failed (%d)\n", ret);
		return ret;
	}

	usleep_range(10000, 20000);

	return i2c_master_recv(client, recv, 6);
}

static void nunchuk_poll(struct input_dev *input)
{
	struct nunchuk_dev *nunchuk = input_get_drvdata(input);
	bool zpressed, cpressed;
	u8 xaxis, yaxis;
	u8 recv[6];
	int ret;

	ret = nunchuk_read_registers(nunchuk->i2c_client, recv);
	if (ret < 0)
		return;

	zpressed = !(recv[5] & BIT(0));
	cpressed = !(recv[5] & BIT(1));
	xaxis = recv[0];
	yaxis = recv[1];

	input_event(input, EV_KEY, BTN_Z, zpressed);
	input_event(input, EV_KEY, BTN_C, cpressed);
	input_event(input, EV_ABS, ABS_X, xaxis);
	input_event(input, EV_ABS, ABS_Y, yaxis);
	input_sync(input);
}

static int nunchuk_probe(struct i2c_client *client)
{
	u8 recv[6];
	u8 buf[2];
	int ret;

	struct input_dev *input;
	struct nunchuk_dev *nunchuk;

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

	ret = nunchuk_read_registers(client, recv);
	if (ret < 0)
		return ret;

	nunchuk = devm_kzalloc(&client->dev, sizeof(*nunchuk), GFP_KERNEL);
	if (!nunchuk)
		return -ENOMEM;

	input = devm_input_allocate_device(&client->dev);
	if (!input)
		return -ENOMEM;

	nunchuk->i2c_client = client;
	input_set_drvdata(input, nunchuk);

	input->name = "Wii Nunchuk";
	input->id.bustype = BUS_I2C;

	set_bit(EV_KEY, input->evbit);
	set_bit(BTN_C, input->keybit);
	set_bit(BTN_Z, input->keybit);
	set_bit(ABS_X, input->absbit);
	set_bit(ABS_Y, input->absbit);
	input_set_abs_params(input, ABS_X, 30, 220, 4, 8);
	input_set_abs_params(input, ABS_Y, 40, 200, 4, 8);
	/* Classic buttons */
	set_bit(BTN_TL, input->keybit);
	set_bit(BTN_SELECT, input->keybit);
	set_bit(BTN_MODE, input->keybit);
	set_bit(BTN_START, input->keybit);
	set_bit(BTN_TR, input->keybit);
	set_bit(BTN_TL2, input->keybit);
	set_bit(BTN_B, input->keybit);
	set_bit(BTN_Y, input->keybit);
	set_bit(BTN_A, input->keybit);
	set_bit(BTN_X, input->keybit);
	set_bit(BTN_TR2, input->keybit);

	ret = input_setup_polling(input, nunchuk_poll);
	if (ret)
		return ret;

	input_set_poll_interval(input, 50);

	return input_register_device(input);
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
