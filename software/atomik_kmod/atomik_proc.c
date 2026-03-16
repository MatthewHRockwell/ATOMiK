// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_proc.c — /proc/atomik interface
 *
 * Provides machine-readable key=value output for easy scripting:
 *   cat /proc/atomik          → all metrics
 *   cat /proc/atomik/cow      → COW metrics only
 *   cat /proc/atomik/net      → network metrics only
 *   cat /proc/atomik/summary  → one-line summary for monitoring
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "atomik_audit.h"
#include "atomik_cow.h"
#include "atomik_hw.h"
#include "atomik_net.h"
#include "atomik_stats.h"
#include "include/uapi/atomik.h"

extern int atomik_license_check(void);
extern int atomik_license_days_remaining(void);

static struct proc_dir_entry *proc_dir;

/* --- /proc/atomik (all metrics) --- */

static int atomik_proc_show(struct seq_file *m, void *v)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);

	/* Version & backend */
	seq_printf(m, "version=%d.%d.%d\n",
		   ATOMIK_VERSION_MAJOR, ATOMIK_VERSION_MINOR,
		   ATOMIK_VERSION_PATCH);
	seq_printf(m, "backend=%s\n",
		   atomik_hw_available() ? "hardware" : "software");
	if (atomik_hw_available())
		seq_printf(m, "hw_banks=%d\n", atomik_hw_dev->n_banks);
	seq_printf(m, "license=%s\n",
		   atomik_license_check() ? "active" : "trial");

	/* Core operations */
	seq_printf(m, "ops_load=%llu\n", agg.ops_load);
	seq_printf(m, "ops_accum=%llu\n", agg.ops_accum);
	seq_printf(m, "ops_read=%llu\n", agg.ops_read);
	seq_printf(m, "ops_swap=%llu\n", agg.ops_swap);
	seq_printf(m, "ops_total=%llu\n",
		   agg.ops_load + agg.ops_accum +
		   agg.ops_read + agg.ops_swap);

	/* Fingerprinting */
	seq_printf(m, "fp_checks=%llu\n", agg.fp_checks_total);
	seq_printf(m, "fp_unchanged=%llu\n", agg.fp_checks_unchanged);
	seq_printf(m, "fp_bytes=%llu\n", agg.fp_bytes_fingerprinted);

	/* COW */
	seq_printf(m, "cow_faults=%llu\n", agg.cow_faults_total);
	seq_printf(m, "cow_redundant=%llu\n", agg.cow_copies_redundant);
	seq_printf(m, "cow_bytes_copied=%llu\n", agg.cow_bytes_copied);
	seq_printf(m, "cow_bytes_saved=%llu\n", agg.cow_bytes_saved);

	/* Network */
	seq_printf(m, "net_sends=%llu\n", agg.net_sends_total);
	seq_printf(m, "net_redundant=%llu\n", agg.net_sends_redundant);
	seq_printf(m, "net_bytes=%llu\n", agg.net_bytes_total);
	seq_printf(m, "net_bytes_redundant=%llu\n", agg.net_bytes_redundant);

	return 0;
}

static int atomik_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, atomik_proc_show, NULL);
}

static const struct proc_ops atomik_proc_ops = {
	.proc_open    = atomik_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* --- /proc/atomik/cow --- */

static int cow_proc_show(struct seq_file *m, void *v)
{
	struct atomik_stats agg;
	u64 rate;

	atomik_stats_aggregate(&agg);

	seq_printf(m, "faults=%llu\n", agg.cow_faults_total);
	seq_printf(m, "copies=%llu\n", agg.cow_copies_performed);
	seq_printf(m, "redundant=%llu\n", agg.cow_copies_redundant);
	seq_printf(m, "bytes_copied=%llu\n", agg.cow_bytes_copied);
	seq_printf(m, "bytes_saved=%llu\n", agg.cow_bytes_saved);

	if (agg.cow_copies_performed > 0) {
		rate = agg.cow_copies_redundant * 1000 /
		       agg.cow_copies_performed;
		seq_printf(m, "redundancy_rate=%llu.%llu%%\n",
			   rate / 10, rate % 10);
	} else {
		seq_puts(m, "redundancy_rate=0.0%\n");
	}

	return 0;
}

static int cow_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cow_proc_show, NULL);
}

static const struct proc_ops cow_proc_ops = {
	.proc_open    = cow_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* --- /proc/atomik/net --- */

static int net_proc_show(struct seq_file *m, void *v)
{
	struct atomik_stats agg;
	u64 rate;

	atomik_stats_aggregate(&agg);

	seq_printf(m, "sends=%llu\n", agg.net_sends_total);
	seq_printf(m, "redundant=%llu\n", agg.net_sends_redundant);
	seq_printf(m, "bytes=%llu\n", agg.net_bytes_total);
	seq_printf(m, "bytes_redundant=%llu\n", agg.net_bytes_redundant);

	if (agg.net_sends_total > 0) {
		rate = agg.net_sends_redundant * 1000 /
		       agg.net_sends_total;
		seq_printf(m, "redundancy_rate=%llu.%llu%%\n",
			   rate / 10, rate % 10);
	} else {
		seq_puts(m, "redundancy_rate=0.0%\n");
	}

	return 0;
}

static int net_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, net_proc_show, NULL);
}

static const struct proc_ops net_proc_ops = {
	.proc_open    = net_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* --- /proc/atomik/summary (one-line for monitoring) --- */

static int summary_proc_show(struct seq_file *m, void *v)
{
	struct atomik_stats agg;
	u64 ops_total;

	atomik_stats_aggregate(&agg);
	ops_total = agg.ops_load + agg.ops_accum +
		    agg.ops_read + agg.ops_swap;

	seq_printf(m, "ops=%llu cow_saved=%llu net_saved=%llu\n",
		   ops_total, agg.cow_bytes_saved,
		   agg.net_bytes_redundant);

	return 0;
}

static int summary_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, summary_proc_show, NULL);
}

static const struct proc_ops summary_proc_ops = {
	.proc_open    = summary_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* --- /proc/atomik/audit --- */

static int audit_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, atomik_audit_read, NULL);
}

static const struct proc_ops audit_proc_ops = {
	.proc_open    = audit_proc_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

/* --- Init / Exit --- */

int atomik_proc_init(void)
{
	proc_dir = proc_mkdir("atomik", NULL);
	if (!proc_dir)
		return -ENOMEM;

	if (!proc_create("atomik", 0444, NULL, &atomik_proc_ops))
		goto err;
	if (!proc_create("cow", 0444, proc_dir, &cow_proc_ops))
		goto err;
	if (!proc_create("net", 0444, proc_dir, &net_proc_ops))
		goto err;
	if (!proc_create("summary", 0444, proc_dir, &summary_proc_ops))
		goto err;
	if (!proc_create("audit", 0444, proc_dir, &audit_proc_ops))
		goto err;

	return 0;

err:
	proc_remove(proc_dir);
	remove_proc_entry("atomik", NULL);
	return -ENOMEM;
}

void atomik_proc_exit(void)
{
	remove_proc_entry("atomik", NULL);
	proc_remove(proc_dir);
}
