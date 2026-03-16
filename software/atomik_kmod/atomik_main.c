// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_main.c — ATOMiK Delta-State Accelerator kernel module
 *
 * Provides /dev/atomik character device and sysfs interface.
 * Automatically detects hardware (Zynq AXI4-Lite / ASIC MMIO)
 * via device tree. Falls back to software emulation on x86_64.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#include "atomik_cgroup.h"
#include "atomik_core_kern.h"
#include "atomik_cow.h"
#include "atomik_hw.h"
#include "atomik_net.h"
#include "atomik_stats.h"
#include "include/uapi/atomik.h"

/* Per-CPU stats definition */
DEFINE_PER_CPU(struct atomik_stats, atomik_pcpu_stats);

/* License key module parameter */
static char *license_key = "";
module_param(license_key, charp, 0400);
MODULE_PARM_DESC(license_key, "ATOMiK license key (ATOMIK-XXXX-XXXX-XXXX-XXXX)");

/* Forward declarations */
extern const struct file_operations atomik_fops;
extern int atomik_sysfs_init(void);
extern void atomik_sysfs_exit(void);
extern int atomik_license_init(const char *key);
extern void atomik_license_exit(void);
extern int atomik_license_check(void);
extern int atomik_proc_init(void);
extern void atomik_proc_exit(void);

static struct miscdevice atomik_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "atomik",
	.fops  = &atomik_fops,
	.mode  = 0660,
};

static int __init atomik_init(void)
{
	int ret;

	/* Initialize license subsystem */
	ret = atomik_license_init(license_key);
	if (ret) {
		pr_err("atomik: license init failed (%d)\n", ret);
		return ret;
	}

	/* Register hardware platform driver (probes via device tree) */
	ret = atomik_hw_driver_register();
	if (ret) {
		pr_warn("atomik: hw driver registration failed (%d), "
			"continuing with software backend\n", ret);
		/* Non-fatal: software backend still works */
	}

	/* Register /dev/atomik */
	ret = misc_register(&atomik_misc);
	if (ret) {
		pr_err("atomik: misc_register failed (%d)\n", ret);
		goto err_hw;
	}

	/* Create sysfs attributes */
	ret = atomik_sysfs_init();
	if (ret) {
		pr_err("atomik: sysfs init failed (%d)\n", ret);
		goto err_misc;
	}

	/* Initialize COW optimization (non-fatal if fails) */
	ret = atomik_cow_init();
	if (ret) {
		pr_warn("atomik: COW optimization unavailable (%d)\n", ret);
		/* Non-fatal: module works without COW interception */
	}

	/* Initialize network monitoring (non-fatal if fails) */
	ret = atomik_net_init();
	if (ret) {
		pr_warn("atomik: network monitoring unavailable (%d)\n", ret);
	}

	/* Initialize cgroup tracking (non-fatal if fails) */
	ret = atomik_cgroup_init();
	if (ret) {
		pr_warn("atomik: cgroup tracking unavailable (%d)\n", ret);
	}

	/* Create /proc/atomik interface (non-fatal if fails) */
	ret = atomik_proc_init();
	if (ret) {
		pr_warn("atomik: proc interface unavailable (%d)\n", ret);
	}

	pr_info("ATOMiK v%d.%d.%d loaded [%s backend%s] — %s\n",
		ATOMIK_VERSION_MAJOR, ATOMIK_VERSION_MINOR, ATOMIK_VERSION_PATCH,
		atomik_hw_available() ? "hardware" : "software",
		atomik_hw_dev ? "" : "",
		atomik_license_check() ? "licensed" : "trial");

	if (atomik_hw_available())
		pr_info("ATOMiK hardware: %d bank(s), %d max contexts\n",
			atomik_hw_dev->n_banks, ATOMIK_HW_MAX_CONTEXTS);

	return 0;

err_misc:
	misc_deregister(&atomik_misc);
err_hw:
	atomik_hw_driver_unregister();
	atomik_license_exit();
	return ret;
}

static void __exit atomik_exit(void)
{
	atomik_proc_exit();
	atomik_cgroup_exit();
	atomik_net_exit();
	atomik_cow_exit();
	atomik_sysfs_exit();
	misc_deregister(&atomik_misc);
	atomik_hw_driver_unregister();
	atomik_license_exit();
	pr_info("ATOMiK unloaded\n");
}

module_init(atomik_init);
module_exit(atomik_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matthew Rockwell <mrockwell@atomik.tech>");
MODULE_DESCRIPTION("ATOMiK Delta-State Accelerator");
MODULE_VERSION("0.5.0");
