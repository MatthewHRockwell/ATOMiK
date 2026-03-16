/* SPDX-License-Identifier: GPL-2.0 */
/*
 * atomik_cgroup.h — Per-container/cgroup waste tracking
 *
 * Aggregates COW and network redundancy stats per cgroup, giving
 * Kubernetes/Docker users per-pod/per-container visibility into
 * memory and network waste.
 */
#ifndef ATOMIK_CGROUP_H
#define ATOMIK_CGROUP_H

#include <linux/types.h>

int atomik_cgroup_init(void);
void atomik_cgroup_exit(void);

/* Called from COW and network probes to attribute waste to a cgroup */
void atomik_cgroup_record_cow(u64 bytes_wasted);
void atomik_cgroup_record_cow_fault(void);
void atomik_cgroup_record_net_redundant(u64 bytes);
void atomik_cgroup_record_net_send(u64 bytes);

/* Sysfs export: top cgroups by waste */
int atomik_cgroup_top(char *buf, int buflen, int max_entries);

#endif /* ATOMIK_CGROUP_H */
