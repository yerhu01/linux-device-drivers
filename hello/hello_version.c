// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>

static int __init hello_init(void)
{
	pr_alert("Hello Master. You are currently using Linux %s.\n", init_uts_ns.name.release);
	return 0;
}

static void __exit hello_exit(void)
{
	pr_alert("Goodbye.\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Greeting module");
MODULE_AUTHOR("Darian");
