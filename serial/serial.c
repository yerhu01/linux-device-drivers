// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <uapi/linux/serial_reg.h>

#define SERIAL_RESET_COUNTER 0
#define SERIAL_GET_COUNTER 1

struct serial_dev {
	struct miscdevice miscdev;
	void __iomem *regs;
	u32 count;
	int irq;
};

static u32 reg_read(struct serial_dev *dev, u32 offset)
{
	return readl(dev->regs + (offset << 2));
}

static void reg_write(struct serial_dev *dev, int value, u32 offset)
{
	writel(value, dev->regs + (offset << 2));
}

static void serial_write_char(struct serial_dev *dev, char c)
{
	while ((reg_read(dev, UART_LSR) & UART_LSR_THRE) == 0)
		cpu_relax();

	reg_write(dev, c, UART_TX);
}

static ssize_t serial_read(struct file *file, char __user *buf, size_t sz, loff_t *pos)
{
	return -EINVAL;
}

static ssize_t serial_write(struct file *file, const char __user *buf, size_t sz, loff_t *pos)
{
	struct serial_dev *dev = container_of(file->private_data, struct serial_dev, miscdev);
	int i;

	for (i = 0; i < sz; i++) {
		unsigned char c;

		if (get_user(c, buf + i))
			return -EFAULT;
		serial_write_char(dev, c);
		dev->count++;

		if (c == '\n')
			serial_write_char(dev, '\r');
	}

	return sz;
}

static long serial_ioctl(struct file *file, unsigned int cmd, unsigned long val)
{
	struct serial_dev *dev = container_of(file->private_data, struct serial_dev, miscdev);
	unsigned int __user *valp = (unsigned int __user *)val;

	switch(cmd) {
		case SERIAL_RESET_COUNTER:
			dev->count = 0;
			break;

		case SERIAL_GET_COUNTER:
			if(put_user(dev->count, valp))
				return -EFAULT;

			break;

		default:
			return -ENOTTY;
	}

	return 0;
}

static const struct file_operations serial_fops = {
	.read = serial_read,
	.write = serial_write,
	.unlocked_ioctl = serial_ioctl,
	.owner = THIS_MODULE,
};

static irqreturn_t serial_irq(int irq, void *data)
{
	pr_info("Called %s\n", __func__);

	return IRQ_HANDLED;
}

static int serial_probe(struct platform_device *pdev)
{
	unsigned int baud_divisor, uartclk;
	struct serial_dev *dev;
	struct resource *res;
	int ret;

	dev = devm_kzalloc(&pdev->dev, sizeof(struct serial_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->regs = devm_ioremap_resource(&pdev->dev, res);
	if (!dev->regs) {
		dev_err(&pdev->dev, "Cannot remap registers\n");
		return -ENOMEM;
	}

	dev->irq = platform_get_irq(pdev, 0);

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	of_property_read_u32(pdev->dev.of_node, "clock-frequency", &uartclk);
	baud_divisor = uartclk / 16 / 115200;
	reg_write(dev, 0x07, UART_OMAP_MDR1);
	reg_write(dev, 0x00, UART_LCR);
	reg_write(dev, UART_LCR_DLAB, UART_LCR);
	reg_write(dev, baud_divisor & 0xff, UART_DLL);
	reg_write(dev, (baud_divisor >> 8) & 0xff, UART_DLM);
	reg_write(dev, UART_LCR_WLEN8, UART_LCR);

	/* Soft reset */
	reg_write(dev, UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, UART_FCR);
	reg_write(dev, 0x00, UART_OMAP_MDR1);

	pr_info("Called %s\n", __func__);

	dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev->miscdev.name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "serial-%x", res->start);

	dev->miscdev.fops = &serial_fops;
	dev->miscdev.parent = &pdev->dev;

	platform_set_drvdata(pdev, dev);

	ret = devm_request_irq(&pdev->dev, dev->irq, serial_irq, 0, dev->miscdev.name, dev);
	if (ret < 0)
		goto err_pm;

	reg_write(dev, UART_IER_RDI, UART_IER);

	ret = misc_register(&dev->miscdev);
	if (ret < 0) {
		goto err_pm;
	}

	return 0;

err_pm:
	pm_runtime_disable(&pdev->dev);

	return ret;
}

static int serial_remove(struct platform_device *pdev)
{
	struct serial_dev *dev = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	misc_deregister(&dev->miscdev);
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
