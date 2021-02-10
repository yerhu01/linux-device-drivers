// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <uapi/linux/serial_reg.h>

struct serial_dev {
	void __iomem *regs;
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

static int serial_probe(struct platform_device *pdev)
{
	unsigned int baud_divisor, uartclk;
	struct serial_dev *dev;
	struct resource *res;

	dev = devm_kzalloc(&pdev->dev, sizeof(struct serial_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pr_err("start %x\n", res->start);
	dev->regs = devm_ioremap_resource(&pdev->dev, res);
	if (!dev->regs) {
		dev_err(&pdev->dev, "Cannot remap registers\n");
		return -ENOMEM;
	}

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

	serial_write_char(dev, 'A');

	return 0;
}

static int serial_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
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
