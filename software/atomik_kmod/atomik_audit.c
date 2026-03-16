// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_audit.c — /proc/atomik/audit ring buffer
 *
 * Lock-free (single-writer per-entry) ring buffer that records the
 * last 1024 ATOMiK operations.  Readable via /proc/atomik/audit
 * for post-mortem debugging and compliance auditing.
 *
 * The ring uses an atomic head index; readers snapshot head and
 * walk backwards, so there is no contention between writers and
 * readers in the common case.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/atomic.h>
#include <linux/ktime.h>
#include <linux/sched.h>

#include "atomik_audit.h"

static struct atomik_audit_entry *ring;
static atomic_t head = ATOMIC_INIT(0);
static bool initialized;

int atomik_audit_init(void)
{
	ring = kvcalloc(ATOMIK_AUDIT_RING_SIZE, sizeof(*ring), GFP_KERNEL);
	if (!ring)
		return -ENOMEM;

	atomic_set(&head, 0);
	initialized = true;
	return 0;
}

void atomik_audit_exit(void)
{
	initialized = false;
	kvfree(ring);
	ring = NULL;
}

void atomik_audit_record(u8 op_type, u32 table_id, u32 addr, u64 value)
{
	unsigned int idx;
	struct atomik_audit_entry *e;

	if (!initialized)
		return;

	idx = (unsigned int)atomic_fetch_add(1, &head) %
	      ATOMIK_AUDIT_RING_SIZE;

	e = &ring[idx];
	e->timestamp_ns = ktime_get_real_ns();
	e->pid          = current->pid;
	e->table_id     = table_id;
	e->addr         = addr;
	e->op_type      = op_type;
	e->value        = value;
}

/*
 * Dump the ring buffer as human-readable text lines.
 *
 * Format (one line per entry, newest first):
 *   <timestamp_ns> pid=<pid> op=<LOAD|ACCUM|READ|SWAP> table=<id> addr=<a> value=0x<v>
 *
 * Empty (zeroed) slots are skipped.
 */
static const char * const op_names[] = { "LOAD", "ACCUM", "READ", "SWAP" };

int atomik_audit_read(struct seq_file *m, void *v)
{
	unsigned int h, i, idx, count;

	if (!initialized) {
		seq_puts(m, "# audit not initialized\n");
		return 0;
	}

	h = (unsigned int)atomic_read(&head);
	count = min_t(unsigned int, h, ATOMIK_AUDIT_RING_SIZE);

	seq_printf(m, "# ATOMiK audit ring — %u entries (newest first)\n",
		   count);

	for (i = 0; i < count; i++) {
		const struct atomik_audit_entry *e;

		/*
		 * Walk backwards from head-1.  The modulo handles
		 * the unsigned wrap correctly for any head value.
		 */
		idx = (h - 1 - i) % ATOMIK_AUDIT_RING_SIZE;
		e = &ring[idx];

		/* Skip empty slots (possible during first fill) */
		if (e->timestamp_ns == 0)
			continue;

		seq_printf(m, "%llu pid=%u op=%s table=%u addr=%u "
			   "value=0x%llx\n",
			   e->timestamp_ns,
			   e->pid,
			   (e->op_type < ARRAY_SIZE(op_names))
				? op_names[e->op_type] : "???",
			   e->table_id,
			   e->addr,
			   e->value);
	}

	return 0;
}
