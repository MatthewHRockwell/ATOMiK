// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_sysfs.c — /sys/class/atomik/ attributes
 *
 * Exposes module status and statistics via sysfs.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>

#include "atomik_cgroup.h"
#include "atomik_core_kern.h"
#include "atomik_cow.h"
#include "atomik_hw.h"
#include "atomik_net.h"
#include "atomik_stats.h"
#include "include/uapi/atomik.h"

extern int atomik_license_check(void);
extern int atomik_license_days_remaining(void);

static struct class *atomik_class;
static struct device *atomik_dev;

/* --- sysfs show functions --- */

static ssize_t version_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%d.%d.%d\n",
			  ATOMIK_VERSION_MAJOR, ATOMIK_VERSION_MINOR,
			  ATOMIK_VERSION_PATCH);
}
static DEVICE_ATTR_RO(version);

static ssize_t backend_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	if (atomik_hw_available())
		return sysfs_emit(buf, "hardware (%d banks)\n",
				  atomik_hw_dev->n_banks);
	return sysfs_emit(buf, "software\n");
}
static DEVICE_ATTR_RO(backend);

static ssize_t ops_total_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n",
			  agg.ops_load + agg.ops_accum +
			  agg.ops_read + agg.ops_swap);
}
static DEVICE_ATTR_RO(ops_total);

static ssize_t ops_load_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.ops_load);
}
static DEVICE_ATTR_RO(ops_load);

static ssize_t ops_accum_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.ops_accum);
}
static DEVICE_ATTR_RO(ops_accum);

static ssize_t ops_read_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.ops_read);
}
static DEVICE_ATTR_RO(ops_read);

static ssize_t ops_swap_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.ops_swap);
}
static DEVICE_ATTR_RO(ops_swap);

static ssize_t fp_checks_total_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.fp_checks_total);
}
static DEVICE_ATTR_RO(fp_checks_total);

static ssize_t fp_checks_unchanged_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.fp_checks_unchanged);
}
static DEVICE_ATTR_RO(fp_checks_unchanged);

static ssize_t fp_hit_rate_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;
	u64 pct = 0;

	atomik_stats_aggregate(&agg);
	if (agg.fp_checks_total > 0)
		pct = div64_u64(agg.fp_checks_unchanged * 100,
				agg.fp_checks_total);
	return sysfs_emit(buf, "%llu\n", pct);
}
static DEVICE_ATTR_RO(fp_hit_rate);

static ssize_t fp_bytes_saved_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.fp_bytes_skipped);
}
static DEVICE_ATTR_RO(fp_bytes_saved);

static ssize_t license_status_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int days = atomik_license_days_remaining();

	if (atomik_license_check())
		return sysfs_emit(buf, "active\n");
	if (days > 0)
		return sysfs_emit(buf, "trial (%d days remaining)\n", days);
	return sysfs_emit(buf, "expired\n");
}
static DEVICE_ATTR_RO(license_status);

/* --- COW optimization attributes (v0.3) --- */

static ssize_t cow_enabled_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%d\n", atomik_cow_is_enabled() ? 1 : 0);
}

static ssize_t cow_enabled_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	bool val;
	int ret;

	ret = kstrtobool(buf, &val);
	if (ret)
		return ret;

	atomik_cow_set_enabled(val);
	return count;
}
static DEVICE_ATTR_RW(cow_enabled);

static ssize_t cow_faults_total_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.cow_faults_total);
}
static DEVICE_ATTR_RO(cow_faults_total);

static ssize_t cow_copies_performed_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.cow_copies_performed);
}
static DEVICE_ATTR_RO(cow_copies_performed);

static ssize_t cow_bytes_copied_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.cow_bytes_copied);
}
static DEVICE_ATTR_RO(cow_bytes_copied);

static ssize_t cow_copies_redundant_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.cow_copies_redundant);
}
static DEVICE_ATTR_RO(cow_copies_redundant);

static ssize_t cow_bytes_saved_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.cow_bytes_saved);
}
static DEVICE_ATTR_RO(cow_bytes_saved);

static ssize_t cow_redundancy_rate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct atomik_stats agg;
	u64 pct = 0;

	atomik_stats_aggregate(&agg);
	if (agg.cow_copies_performed > 0)
		pct = div64_u64(agg.cow_copies_redundant * 100,
				agg.cow_copies_performed);
	return sysfs_emit(buf, "%llu\n", pct);
}
static DEVICE_ATTR_RO(cow_redundancy_rate);

static ssize_t cow_top_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return atomik_cow_top(buf, PAGE_SIZE, 10);
}
static DEVICE_ATTR_RO(cow_top);

/* --- Network monitoring attributes (v0.4) --- */

static ssize_t net_enabled_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%d\n", atomik_net_is_enabled() ? 1 : 0);
}

static ssize_t net_enabled_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	bool val;
	int ret;

	ret = kstrtobool(buf, &val);
	if (ret)
		return ret;

	atomik_net_set_enabled(val);
	return count;
}
static DEVICE_ATTR_RW(net_enabled);

static ssize_t net_sends_total_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.net_sends_total);
}
static DEVICE_ATTR_RO(net_sends_total);

static ssize_t net_sends_redundant_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.net_sends_redundant);
}
static DEVICE_ATTR_RO(net_sends_redundant);

static ssize_t net_bytes_total_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.net_bytes_total);
}
static DEVICE_ATTR_RO(net_bytes_total);

static ssize_t net_bytes_redundant_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct atomik_stats agg;

	atomik_stats_aggregate(&agg);
	return sysfs_emit(buf, "%llu\n", agg.net_bytes_redundant);
}
static DEVICE_ATTR_RO(net_bytes_redundant);

static ssize_t net_redundancy_rate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct atomik_stats agg;
	u64 pct = 0;

	atomik_stats_aggregate(&agg);
	if (agg.net_sends_total > 0)
		pct = div64_u64(agg.net_sends_redundant * 100,
				agg.net_sends_total);
	return sysfs_emit(buf, "%llu\n", pct);
}
static DEVICE_ATTR_RO(net_redundancy_rate);

/* --- Cgroup/container tracking (v0.4.1) --- */

static ssize_t cgroup_top_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return atomik_cgroup_top(buf, PAGE_SIZE, 10);
}
static DEVICE_ATTR_RO(cgroup_top);

static struct attribute *atomik_attrs[] = {
	&dev_attr_version.attr,
	&dev_attr_backend.attr,
	&dev_attr_ops_total.attr,
	&dev_attr_ops_load.attr,
	&dev_attr_ops_accum.attr,
	&dev_attr_ops_read.attr,
	&dev_attr_ops_swap.attr,
	&dev_attr_fp_checks_total.attr,
	&dev_attr_fp_checks_unchanged.attr,
	&dev_attr_fp_hit_rate.attr,
	&dev_attr_fp_bytes_saved.attr,
	&dev_attr_license_status.attr,
	/* COW optimization (v0.3) */
	&dev_attr_cow_enabled.attr,
	&dev_attr_cow_faults_total.attr,
	&dev_attr_cow_copies_performed.attr,
	&dev_attr_cow_bytes_copied.attr,
	&dev_attr_cow_copies_redundant.attr,
	&dev_attr_cow_bytes_saved.attr,
	&dev_attr_cow_redundancy_rate.attr,
	&dev_attr_cow_top.attr,
	/* Network monitoring (v0.4) */
	&dev_attr_net_enabled.attr,
	&dev_attr_net_sends_total.attr,
	&dev_attr_net_sends_redundant.attr,
	&dev_attr_net_bytes_total.attr,
	&dev_attr_net_bytes_redundant.attr,
	&dev_attr_net_redundancy_rate.attr,
	/* Cgroup/container tracking */
	&dev_attr_cgroup_top.attr,
	NULL,
};

static const struct attribute_group atomik_attr_group = {
	.attrs = atomik_attrs,
};

static const struct attribute_group *atomik_attr_groups[] = {
	&atomik_attr_group,
	NULL,
};

int atomik_sysfs_init(void)
{
	atomik_class = class_create("atomik");
	if (IS_ERR(atomik_class))
		return PTR_ERR(atomik_class);

	atomik_dev = device_create_with_groups(atomik_class, NULL,
					       MKDEV(0, 0), NULL,
					       atomik_attr_groups,
					       "atomik0");
	if (IS_ERR(atomik_dev)) {
		class_destroy(atomik_class);
		return PTR_ERR(atomik_dev);
	}

	return 0;
}

void atomik_sysfs_exit(void)
{
	device_destroy(atomik_class, MKDEV(0, 0));
	class_destroy(atomik_class);
}
