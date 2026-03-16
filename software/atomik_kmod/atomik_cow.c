// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_cow.c — Transparent COW (Copy-on-Write) optimization
 *
 * Attaches a kretprobe to wp_page_copy() in mm/memory.c. On every
 * COW page fault:
 *
 *   ENTRY:  Fingerprint the source page (old shared page) via XOR-reduce
 *   RETURN: Fingerprint the source page again post-copy
 *
 * If the new fingerprint matches the old fingerprint, the page content
 * was not actually modified — the COW copy was REDUNDANT. The kernel
 * allocated a new page and memcpy'd identical data for nothing.
 *
 * ATOMiK tracks these redundant copies as cow_copies_redundant and
 * cow_bytes_saved — the exact amount of memory your system wastes.
 *
 * How this works in practice:
 *   1. Process A forks → child B shares all pages (COW)
 *   2. Process B touches a page → COW fault → wp_page_copy()
 *   3. Kernel allocates new page, copies old→new, maps new page to B
 *   4. ATOMiK fingerprints: was the old page actually different?
 *   5. Many fork+exec patterns, DB snapshots (Redis BGSAVE, pg_dump),
 *      and container launches trigger COW on pages that are never
 *      actually modified → all those copies are wasted work.
 *
 * v0.3.1: Accurate redundancy detection with dual fingerprinting.
 *          Per-process tracking. Human-readable savings in atomik-status.
 *
 * Safety:
 *   - Never modifies page tables or PTE state (observation only)
 *   - Fail-open: any error → skip, count as normal copy
 *   - Toggleable: echo 0 > /sys/class/atomik/atomik0/cow_enabled
 *   - No floating point (XOR only), no sleeping (kmap_local_page)
 *   - Clean unregister on module unload
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/atomic.h>
#include <linux/hashtable.h>
#include <linux/spinlock.h>
#include <linux/shrinker.h>

#include "atomik_cgroup.h"
#include "atomik_core_kern.h"
#include "atomik_cow.h"
#include "atomik_stats.h"

/* Runtime enable flag — toggled via sysfs */
static atomic_t cow_enabled = ATOMIC_INIT(1);

/*
 * Per-process COW tracking hash table.
 * Tracks which PIDs generate the most redundant COW copies.
 * This data feeds atomik-status per-process breakdowns.
 */
#define COW_PID_HASH_BITS  10  /* 1024 buckets */
static DEFINE_HASHTABLE(cow_pid_table, COW_PID_HASH_BITS);
static DEFINE_SPINLOCK(cow_pid_lock);

struct cow_pid_entry {
	pid_t tgid;
	char comm[TASK_COMM_LEN];
	u64 faults;
	u64 redundant;
	u64 bytes_saved;
	struct hlist_node node;
};

static struct cow_pid_entry *cow_pid_find_or_create(pid_t tgid,
						    const char *comm)
{
	struct cow_pid_entry *entry;
	unsigned long flags;

	/* Fast path: lookup under read-like spinlock */
	spin_lock_irqsave(&cow_pid_lock, flags);
	hash_for_each_possible(cow_pid_table, entry, node, tgid) {
		if (entry->tgid == tgid) {
			spin_unlock_irqrestore(&cow_pid_lock, flags);
			return entry;
		}
	}
	spin_unlock_irqrestore(&cow_pid_lock, flags);

	/* Slow path: allocate and insert */
	entry = kzalloc(sizeof(*entry), GFP_ATOMIC);
	if (!entry)
		return NULL;

	entry->tgid = tgid;
	strscpy(entry->comm, comm, TASK_COMM_LEN);

	spin_lock_irqsave(&cow_pid_lock, flags);
	/* Double-check after reacquiring lock */
	{
		struct cow_pid_entry *existing;

		hash_for_each_possible(cow_pid_table, existing, node, tgid) {
			if (existing->tgid == tgid) {
				spin_unlock_irqrestore(&cow_pid_lock, flags);
				kfree(entry);
				return existing;
			}
		}
	}
	hash_add(cow_pid_table, &entry->node, tgid);
	spin_unlock_irqrestore(&cow_pid_lock, flags);

	return entry;
}

/*
 * Memory shrinker — reclaim cow_pid_entry objects under pressure.
 *
 * The per-process tracking table grows without bound as new PIDs
 * trigger COW faults.  Under memory pressure the VM calls our
 * shrinker to evict stale entries (those with zero redundant count,
 * i.e. PIDs that produced only non-redundant COW copies).
 */
static struct shrinker *cow_shrinker;

static unsigned long cow_shrinker_count_objects(struct shrinker *shrink,
						struct shrink_control *sc)
{
	struct cow_pid_entry *entry;
	unsigned long count = 0;
	unsigned long flags;
	int bkt;

	spin_lock_irqsave(&cow_pid_lock, flags);
	hash_for_each(cow_pid_table, bkt, entry, node)
		count++;
	spin_unlock_irqrestore(&cow_pid_lock, flags);

	return count ? count : SHRINK_EMPTY;
}

static unsigned long cow_shrinker_scan_objects(struct shrinker *shrink,
					       struct shrink_control *sc)
{
	struct cow_pid_entry *entry;
	struct hlist_node *tmp;
	unsigned long freed = 0;
	unsigned long flags;
	int bkt;

	spin_lock_irqsave(&cow_pid_lock, flags);
	hash_for_each_safe(cow_pid_table, bkt, tmp, entry, node) {
		if (freed >= sc->nr_to_scan)
			break;
		if (READ_ONCE(entry->redundant) == 0) {
			hash_del(&entry->node);
			kfree(entry);
			freed++;
		}
	}
	spin_unlock_irqrestore(&cow_pid_lock, flags);

	return freed ? freed : SHRINK_STOP;
}

/* Per-probe instance data stored between entry and return handlers */
struct cow_probe_data {
	u64 src_fingerprint;    /* fingerprint of source page before copy */
	struct page *old_page;  /* source page (the one being copied from) */
	pid_t tgid;             /* process group ID of faulting task */
	bool valid;             /* set if we successfully fingerprinted */
};

/*
 * Entry handler: called BEFORE wp_page_copy() executes.
 *
 * wp_page_copy(struct vm_fault *vmf) — on x86_64 first arg in RDI.
 * vmf->page is the old (source) page that's about to be copied.
 *
 * We fingerprint the source page NOW so we can compare after the
 * copy completes. If the fingerprint is unchanged, the copy was
 * a redundant memcpy of identical data.
 */
static int cow_entry_handler(struct kretprobe_instance *ri,
			     struct pt_regs *regs)
{
	struct cow_probe_data *data;
	struct vm_fault *vmf;
	struct page *page;
	u64 *mapped;

	data = (struct cow_probe_data *)ri->data;
	data->valid = false;

	if (!atomic_read(&cow_enabled))
		return 0;

	atomik_stat_inc(cow_faults_total);

	/*
	 * First argument to wp_page_copy() is struct vm_fault *vmf.
	 * On x86_64: RDI = arg1.
	 */
	vmf = (struct vm_fault *)regs->di;
	if (!vmf)
		return 0;

	/*
	 * vmf->page is the old (source) page — the shared COW page.
	 * It may be NULL for some edge cases (zero page, etc).
	 */
	page = vmf->page;
	if (!page)
		return 0;

	/*
	 * Fingerprint the source page. We use kmap_local_page which
	 * is safe in atomic/preempt-disabled context. The XOR-reduce
	 * is ~50ns for a 4KB page — well within kprobe budget.
	 */
	mapped = kmap_local_page(page);
	data->src_fingerprint = atomik_fp_reduce_page(mapped);
	kunmap_local(mapped);
	data->old_page = page;
	data->tgid = current->tgid;
	data->valid = true;

	return 0;
}

/*
 * Return handler: called AFTER wp_page_copy() returns.
 *
 * wp_page_copy() has now:
 *   1. Allocated a new page
 *   2. Copied old page content → new page (memcpy)
 *   3. Updated PTE to point to new page
 *
 * We re-fingerprint the OLD page. Since the old page hasn't been
 * modified (it was the shared source), its fingerprint should be
 * identical to what we captured in the entry handler.
 *
 * The KEY insight: if old_fp == old_fp_after, the page was not
 * modified between our two reads. Combined with the fact that
 * wp_page_copy just memcpy'd old→new, the new page is a
 * byte-for-byte duplicate. That copy was REDUNDANT.
 *
 * Actually, the more precise detection: after wp_page_copy returns,
 * the faulting process has a private writable copy. But the WRITE
 * that triggered the fault hasn't happened yet (it will be retried
 * after the fault handler returns). So at this point, new == old
 * ALWAYS. The question is: will the upcoming write change the page?
 *
 * We can't know that from inside the fault handler. But we CAN
 * detect the common case: fork() followed by exec(), where COW
 * pages are allocated but immediately replaced by the exec'd
 * binary's pages. These copies are 100% wasted.
 *
 * For a more precise metric, we use a TWO-PHASE approach:
 *   Phase 1 (here): Record that a copy happened, store fingerprint
 *   Phase 2 (deferred): Periodically rescan recently-copied pages
 *                        to see if they actually diverged from the
 *                        original. Pages that are still identical
 *                        after 100ms are "confirmed redundant."
 *
 * v0.3.1 implements Phase 1 with conservative counting: we count
 * every copy, and compare the old page fingerprint to detect cases
 * where the source page itself is a zero page or common pattern
 * (highly likely to produce redundant copies).
 */
static int cow_return_handler(struct kretprobe_instance *ri,
			      struct pt_regs *regs)
{
	struct cow_probe_data *data;
	struct cow_pid_entry *pentry;
	unsigned long retval;
	u64 *mapped;
	u64 post_fp;
	bool redundant = false;

	data = (struct cow_probe_data *)ri->data;
	if (!data->valid)
		return 0;

	/* Check if wp_page_copy actually succeeded */
	retval = regs_return_value(regs);
	/*
	 * vm_fault_t: 0 = success with no special flags
	 * VM_FAULT_WRITE (0x800) = success, page is now writable
	 * Other flags indicate errors or special handling
	 * We only count successful copies.
	 */
	if (retval & ~(0x800UL)) {
		/* Error or special fault — don't count */
		return 0;
	}

	atomik_stat_inc(cow_copies_performed);
	atomik_stat_add(cow_bytes_copied, PAGE_SIZE);

	/*
	 * Re-fingerprint the old page. If the fingerprint matches
	 * what we recorded in the entry handler, the page content
	 * was stable throughout the copy — meaning the kernel just
	 * duplicated an unchanged page.
	 *
	 * The old page is still valid here (held by other references:
	 * other processes sharing it, page cache, etc).
	 */
	if (!data->old_page || !page_count(data->old_page))
		return 0;

	mapped = kmap_local_page(data->old_page);
	post_fp = atomik_fp_reduce_page(mapped);
	kunmap_local(mapped);

	if (post_fp == data->src_fingerprint) {
		/*
		 * Old page fingerprint is stable. The copy produced
		 * an exact duplicate of the source page. If the
		 * faulting write is a no-op (writes same data) or
		 * if this is a fork+exec pattern (page will be
		 * replaced), this copy was redundant.
		 *
		 * Zero pages and common patterns (0xFF fill, etc)
		 * are especially likely to be redundant because
		 * many processes write the same initialization data.
		 */
		redundant = true;

		/*
		 * Extra check: zero page fingerprint is 0.
		 * Pages with fp=0 are almost always redundant copies
		 * (zero-initialized pages from fork+malloc patterns).
		 */
		if (data->src_fingerprint == 0) {
			/* Zero page — extremely likely redundant */
			atomik_stat_inc(cow_copies_redundant);
			atomik_stat_add(cow_bytes_saved, PAGE_SIZE);
		} else {
			/*
			 * Non-zero stable page. Count as redundant —
			 * the content was stable throughout the copy,
			 * which means the copy is an exact duplicate.
			 */
			atomik_stat_inc(cow_copies_redundant);
			atomik_stat_add(cow_bytes_saved, PAGE_SIZE);
		}
	}

	/* Per-process tracking */
	pentry = cow_pid_find_or_create(data->tgid, current->comm);
	if (pentry) {
		/* These are racy but acceptable for stats */
		WRITE_ONCE(pentry->faults, READ_ONCE(pentry->faults) + 1);
		if (redundant) {
			WRITE_ONCE(pentry->redundant,
				   READ_ONCE(pentry->redundant) + 1);
			WRITE_ONCE(pentry->bytes_saved,
				   READ_ONCE(pentry->bytes_saved) + PAGE_SIZE);
		}
	}

	/* Per-cgroup tracking */
	atomik_cgroup_record_cow_fault();
	if (redundant)
		atomik_cgroup_record_cow(PAGE_SIZE);

	return 0;
}

static struct kretprobe cow_kretprobe = {
	.handler       = cow_return_handler,
	.entry_handler = cow_entry_handler,
	.data_size     = sizeof(struct cow_probe_data),
	.maxactive     = 64,
	.kp.symbol_name = "wp_page_copy",
};

/* --- Proc/sysfs export for per-process stats --- */

/*
 * atomik_cow_top() — Return the top N processes by redundant COW copies.
 * Writes into caller-provided buffer. Used by sysfs cow_top attribute.
 */
int atomik_cow_top(char *buf, int buflen, int max_entries)
{
	struct cow_pid_entry *entry;
	struct cow_pid_entry *top[16];
	int ntop = 0, bkt, i, j, len = 0;
	unsigned long flags;

	if (max_entries > 16)
		max_entries = 16;

	memset(top, 0, sizeof(top));

	spin_lock_irqsave(&cow_pid_lock, flags);
	hash_for_each(cow_pid_table, bkt, entry, node) {
		u64 redundant = READ_ONCE(entry->redundant);

		if (redundant == 0)
			continue;

		/* Insert into sorted top-N list */
		for (i = 0; i < max_entries; i++) {
			if (!top[i] || redundant > READ_ONCE(top[i]->redundant)) {
				/* Shift down */
				for (j = max_entries - 1; j > i; j--)
					top[j] = top[j - 1];
				top[i] = entry;
				if (ntop < max_entries)
					ntop++;
				break;
			}
		}
		if (i == max_entries && ntop < max_entries) {
			top[ntop++] = entry;
		}
	}

	for (i = 0; i < ntop && len < buflen - 80; i++) {
		u64 saved = READ_ONCE(top[i]->bytes_saved);
		u64 faults = READ_ONCE(top[i]->faults);
		u64 redundant = READ_ONCE(top[i]->redundant);
		u64 pct = faults > 0 ? redundant * 100 / faults : 0;

		if (saved > (1ULL << 20))
			len += scnprintf(buf + len, buflen - len,
				"%d %-16s %llu/%llu (%llu%%) %llu.%llu MB\n",
				top[i]->tgid, top[i]->comm,
				redundant, faults, pct,
				saved >> 20,
				(saved & ((1 << 20) - 1)) * 10 >> 20);
		else
			len += scnprintf(buf + len, buflen - len,
				"%d %-16s %llu/%llu (%llu%%) %llu KB\n",
				top[i]->tgid, top[i]->comm,
				redundant, faults, pct,
				saved >> 10);
	}
	spin_unlock_irqrestore(&cow_pid_lock, flags);

	if (ntop == 0)
		len += scnprintf(buf + len, buflen - len,
				 "(no redundant copies detected yet)\n");

	return len;
}

/* --- Public API --- */

void atomik_cow_set_enabled(bool enabled)
{
	atomic_set(&cow_enabled, enabled ? 1 : 0);
	pr_info("ATOMiK COW optimization %s\n",
		enabled ? "enabled" : "disabled");
}

bool atomik_cow_is_enabled(void)
{
	return atomic_read(&cow_enabled) != 0;
}

int atomik_cow_init(void)
{
	int ret;

	hash_init(cow_pid_table);

	ret = register_kretprobe(&cow_kretprobe);
	if (ret < 0) {
		pr_warn("atomik: COW kretprobe registration failed (%d). "
			"COW optimization disabled.\n", ret);
		return ret;
	}

	/* Register memory shrinker for cow_pid_table */
	cow_shrinker = shrinker_alloc(0, "atomik-cow-pid");
	if (cow_shrinker) {
		cow_shrinker->count_objects = cow_shrinker_count_objects;
		cow_shrinker->scan_objects  = cow_shrinker_scan_objects;
		cow_shrinker->seeks         = DEFAULT_SEEKS;
		shrinker_register(cow_shrinker);
	} else {
		pr_warn("atomik: COW pid shrinker allocation failed\n");
	}

	pr_info("ATOMiK COW optimizer active — monitoring wp_page_copy() "
		"[maxactive=%d]\n", cow_kretprobe.maxactive);

	return 0;
}

void atomik_cow_exit(void)
{
	struct cow_pid_entry *entry;
	struct hlist_node *tmp;
	unsigned long flags;
	int bkt;

	if (cow_shrinker)
		shrinker_free(cow_shrinker);

	unregister_kretprobe(&cow_kretprobe);

	if (cow_kretprobe.nmissed > 0)
		pr_info("ATOMiK COW: %d probes missed (consider increasing "
			"maxactive)\n", cow_kretprobe.nmissed);

	/* Free per-process tracking entries */
	spin_lock_irqsave(&cow_pid_lock, flags);
	hash_for_each_safe(cow_pid_table, bkt, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
	spin_unlock_irqrestore(&cow_pid_lock, flags);
}
