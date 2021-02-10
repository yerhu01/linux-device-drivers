// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/time.h>

static char *whom = "world";
module_param(whom, charp, 0644);
MODULE_PARM_DESC(whom, "Recipient of the hello message");

static unsigned long time;

static int __init hello_init(void)
{
	pr_alert("Hello %s. You are currently using Linux %s.\n", whom, init_uts_ns.name.release);
	time = get_seconds();
	
	return 0;
}

static void __exit hello_exit(void)
{
	pr_alert("Loaded for %lu seconds.\n", get_seconds() - time);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Greeting module");
MODULE_AUTHOR("Darian");
