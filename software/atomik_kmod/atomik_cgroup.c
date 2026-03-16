// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_cgroup.c — Per-container/cgroup waste tracking
 *
 * Maintains a hashtable of cgroup paths → aggregated waste stats.
 * COW and network probes call into this module to attribute waste
 * to the cgroup of the current task.
 *
 * For Kubernetes: each pod gets its own cgroup, so per-cgroup stats
 * map directly to per-pod waste visibility.
 *
 * For Docker: each container is a cgroup under /system.slice/ or
 * /docker/ depending on the runtime.
 *
 * The cgroup path is truncated to the last 2 components to keep
 * entries readable (e.g. "kubepods/pod-abc123" instead of the
 * full /sys/fs/cgroup/... path).
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hashtable.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/cgroup.h>
#include <linux/atomic.h>
#include <linux/string.h>

#include "atomik_cgroup.h"

#define CGROUP_HASH_BITS  8  /* 256 buckets */
#define CGROUP_NAME_MAX   128

static DEFINE_HASHTABLE(cgroup_table, CGROUP_HASH_BITS);
static DEFINE_SPINLOCK(atomik_cg_lock);

struct cgroup_entry {
	char name[CGROUP_NAME_MAX];
	unsigned long hash_key;
	u64 cow_faults;
	u64 cow_bytes_wasted;
	u64 net_sends;
	u64 net_bytes_redundant;
	struct hlist_node node;
};

/*
 * Extract a short, readable cgroup name from the current task.
 * Uses cgroup_path_ns to get the full path, then truncates to
 * the last meaningful component(s).
 *
 * Examples:
 *   /user.slice/user-1000.slice/...konsole.scope → "konsole.scope"
 *   /system.slice/docker-abc.scope → "docker-abc.scope"
 *   /kubepods/burstable/pod-xyz → "pod-xyz"
 *   / → "root"
 */
static int get_cgroup_name(char *out, size_t outlen)
{
	struct cgroup *cgrp;
	char pathbuf[256];
	char *last_slash, *name;
	int ret;

	rcu_read_lock();
	cgrp = task_dfl_cgroup(current);
	if (!cgrp) {
		rcu_read_unlock();
		strscpy(out, "unknown", outlen);
		return 0;
	}

	ret = cgroup_path_ns(cgrp, pathbuf, sizeof(pathbuf),
			     current->nsproxy->cgroup_ns);
	rcu_read_unlock();

	if (ret < 0 || pathbuf[0] == '\0') {
		strscpy(out, "unknown", outlen);
		return 0;
	}

	/* If path is just "/", use "root" */
	if (strcmp(pathbuf, "/") == 0) {
		strscpy(out, "root", outlen);
		return 0;
	}

	/* Find last path component */
	last_slash = strrchr(pathbuf, '/');
	name = last_slash ? last_slash + 1 : pathbuf;

	if (name[0] == '\0') {
		/* Trailing slash — go back one more */
		if (last_slash > pathbuf) {
			*last_slash = '\0';
			last_slash = strrchr(pathbuf, '/');
			name = last_slash ? last_slash + 1 : pathbuf;
		}
	}

	/* Truncate long Docker/K8s hashes for readability */
	strscpy(out, name, outlen);

	/* Trim .scope/.service suffix if name is still long */
	if (strlen(out) > 40) {
		char *dot = strrchr(out, '.');

		if (dot && (strcmp(dot, ".scope") == 0 ||
			    strcmp(dot, ".service") == 0))
			*dot = '\0';
	}

	return 0;
}

static unsigned long hash_name(const char *name)
{
	unsigned long h = 0;

	while (*name)
		h = h * 31 + (unsigned char)*name++;
	return h;
}

static struct cgroup_entry *find_or_create(const char *name)
{
	struct cgroup_entry *entry;
	unsigned long key = hash_name(name);
	unsigned long flags;

	spin_lock_irqsave(&atomik_cg_lock, flags);
	hash_for_each_possible(cgroup_table, entry, node, key) {
		if (strcmp(entry->name, name) == 0) {
			spin_unlock_irqrestore(&atomik_cg_lock, flags);
			return entry;
		}
	}
	spin_unlock_irqrestore(&atomik_cg_lock, flags);

	entry = kzalloc(sizeof(*entry), GFP_ATOMIC);
	if (!entry)
		return NULL;

	strscpy(entry->name, name, sizeof(entry->name));
	entry->hash_key = key;

	spin_lock_irqsave(&atomik_cg_lock, flags);
	/* Double-check under lock */
	{
		struct cgroup_entry *existing;

		hash_for_each_possible(cgroup_table, existing, node, key) {
			if (strcmp(existing->name, name) == 0) {
				spin_unlock_irqrestore(&atomik_cg_lock, flags);
				kfree(entry);
				return existing;
			}
		}
	}
	hash_add(cgroup_table, &entry->node, key);
	spin_unlock_irqrestore(&atomik_cg_lock, flags);

	return entry;
}

/* --- Public recording API (called from COW/network probes) --- */

void atomik_cgroup_record_cow_fault(void)
{
	char name[CGROUP_NAME_MAX];
	struct cgroup_entry *entry;

	get_cgroup_name(name, sizeof(name));
	entry = find_or_create(name);
	if (entry)
		WRITE_ONCE(entry->cow_faults,
			   READ_ONCE(entry->cow_faults) + 1);
}

void atomik_cgroup_record_cow(u64 bytes_wasted)
{
	char name[CGROUP_NAME_MAX];
	struct cgroup_entry *entry;

	get_cgroup_name(name, sizeof(name));
	entry = find_or_create(name);
	if (entry)
		WRITE_ONCE(entry->cow_bytes_wasted,
			   READ_ONCE(entry->cow_bytes_wasted) + bytes_wasted);
}

void atomik_cgroup_record_net_send(u64 bytes)
{
	char name[CGROUP_NAME_MAX];
	struct cgroup_entry *entry;

	get_cgroup_name(name, sizeof(name));
	entry = find_or_create(name);
	if (entry)
		WRITE_ONCE(entry->net_sends,
			   READ_ONCE(entry->net_sends) + 1);
}

void atomik_cgroup_record_net_redundant(u64 bytes)
{
	char name[CGROUP_NAME_MAX];
	struct cgroup_entry *entry;

	get_cgroup_name(name, sizeof(name));
	entry = find_or_create(name);
	if (entry)
		WRITE_ONCE(entry->net_bytes_redundant,
			   READ_ONCE(entry->net_bytes_redundant) + bytes);
}

/* --- Sysfs export --- */

int atomik_cgroup_top(char *buf, int buflen, int max_entries)
{
	struct cgroup_entry *entry;
	struct cgroup_entry *top[16];
	int ntop = 0, bkt, i, j, len = 0;
	unsigned long flags;

	if (max_entries > 16)
		max_entries = 16;

	memset(top, 0, sizeof(top));

	spin_lock_irqsave(&atomik_cg_lock, flags);
	hash_for_each(cgroup_table, bkt, entry, node) {
		u64 total_waste = READ_ONCE(entry->cow_bytes_wasted) +
				  READ_ONCE(entry->net_bytes_redundant);

		if (total_waste == 0)
			continue;

		/* Insert into sorted top-N */
		for (i = 0; i < max_entries; i++) {
			if (!top[i]) {
				top[i] = entry;
				if (ntop < max_entries)
					ntop++;
				break;
			}
			{
				u64 top_waste = READ_ONCE(top[i]->cow_bytes_wasted) +
						READ_ONCE(top[i]->net_bytes_redundant);

				if (total_waste > top_waste) {
					for (j = max_entries - 1; j > i; j--)
						top[j] = top[j - 1];
					top[i] = entry;
					if (ntop < max_entries)
						ntop++;
					break;
				}
			}
		}
	}

	for (i = 0; i < ntop && len < buflen - 120; i++) {
		u64 cow_waste = READ_ONCE(top[i]->cow_bytes_wasted);
		u64 net_waste = READ_ONCE(top[i]->net_bytes_redundant);
		u64 cow_faults = READ_ONCE(top[i]->cow_faults);
		u64 net_sends = READ_ONCE(top[i]->net_sends);
		u64 total = cow_waste + net_waste;

		if (total > (1ULL << 20))
			len += scnprintf(buf + len, buflen - len,
				"%-40s COW: %llu faults (%llu.%llu MB)  NET: %llu sends (%llu.%llu MB)\n",
				top[i]->name, cow_faults,
				cow_waste >> 20, (cow_waste & ((1 << 20) - 1)) * 10 >> 20,
				net_sends,
				net_waste >> 20, (net_waste & ((1 << 20) - 1)) * 10 >> 20);
		else
			len += scnprintf(buf + len, buflen - len,
				"%-40s COW: %llu faults (%llu KB)  NET: %llu sends (%llu KB)\n",
				top[i]->name, cow_faults, cow_waste >> 10,
				net_sends, net_waste >> 10);
	}
	spin_unlock_irqrestore(&atomik_cg_lock, flags);

	if (ntop == 0)
		len += scnprintf(buf + len, buflen - len,
			"(no container/cgroup waste detected yet)\n");

	return len;
}

/* --- Init/exit --- */

int atomik_cgroup_init(void)
{
	hash_init(cgroup_table);
	pr_info("ATOMiK cgroup tracking active\n");
	return 0;
}

void atomik_cgroup_exit(void)
{
	struct cgroup_entry *entry;
	struct hlist_node *tmp;
	unsigned long flags;
	int bkt;

	spin_lock_irqsave(&atomik_cg_lock, flags);
	hash_for_each_safe(cgroup_table, bkt, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
	spin_unlock_irqrestore(&atomik_cg_lock, flags);
}
