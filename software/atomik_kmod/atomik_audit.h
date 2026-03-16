/* SPDX-License-Identifier: GPL-2.0 */
/*
 * atomik_audit.h — /proc/atomik/audit ring buffer
 *
 * Records the last ATOMIK_AUDIT_RING_SIZE operations for
 * post-mortem debugging and compliance auditing.
 */

#ifndef _ATOMIK_AUDIT_H
#define _ATOMIK_AUDIT_H

#include <linux/types.h>

struct seq_file;

#define ATOMIK_AUDIT_RING_SIZE  1024

struct atomik_audit_entry {
	u64 timestamp_ns;   /* ktime_get_real_ns() */
	u32 pid;
	u32 table_id;
	u32 addr;
	u8  op_type;        /* LOAD=0, ACCUM=1, READ=2, SWAP=3 */
	u8  _pad[3];
	u64 value;
};

int  atomik_audit_init(void);
void atomik_audit_exit(void);

void atomik_audit_record(u8 op_type, u32 table_id, u32 addr, u64 value);
int  atomik_audit_read(struct seq_file *m, void *v);

#endif /* _ATOMIK_AUDIT_H */
