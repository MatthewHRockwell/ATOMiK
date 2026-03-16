/* SPDX-License-Identifier: GPL-2.0 */
/*
 * atomik_trace.h -- ATOMiK tracepoint definitions
 *
 * Defines tracepoints for core ops, COW faults, and network sends.
 * These are definition-only: the actual trace_*() calls are NOT
 * inserted into hot paths yet.  Creating the definitions now lets
 * perf/ftrace/bpftrace enumerate the events, and the call-site
 * wiring can be reviewed separately.
 *
 * Usage (once call-sites are wired):
 *   perf record -e atomik:atomik_op -a -- sleep 5
 *   bpftrace -e 'tracepoint:atomik:atomik_cow_fault { @[args->pid] = count(); }'
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM atomik

#if !defined(_ATOMIK_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _ATOMIK_TRACE_H

#include <linux/tracepoint.h>

/*
 * atomik_op -- fires on every LOAD / ACCUM / READ / SWAP
 *
 * @op_type:  ATOMIK_OP_LOAD(0), _ACCUM(1), _READ(2), _SWAP(3)
 * @table_id: context table ID from idr
 * @addr:     context address within the table
 * @value:    initial_state (LOAD), delta (ACCUM), result (READ/SWAP)
 */
TRACE_EVENT(atomik_op,

	TP_PROTO(u8 op_type, u32 table_id, u32 addr, u64 value),

	TP_ARGS(op_type, table_id, addr, value),

	TP_STRUCT__entry(
		__field(u8,  op_type)
		__field(u32, table_id)
		__field(u32, addr)
		__field(u64, value)
	),

	TP_fast_assign(
		__entry->op_type  = op_type;
		__entry->table_id = table_id;
		__entry->addr     = addr;
		__entry->value    = value;
	),

	TP_printk("op=%s table=%u addr=%u value=0x%llx",
		__print_symbolic(__entry->op_type,
			{ 0, "LOAD"  },
			{ 1, "ACCUM" },
			{ 2, "READ"  },
			{ 3, "SWAP"  }),
		__entry->table_id,
		__entry->addr,
		__entry->value)
);

/*
 * atomik_cow_fault -- fires on every COW page-fault interception
 *
 * @pid:       current->pid at fault time
 * @tgid:      current->tgid (thread group leader)
 * @redundant: 1 if the copy was detected as redundant, 0 otherwise
 * @bytes:     bytes involved (always PAGE_SIZE today)
 */
TRACE_EVENT(atomik_cow_fault,

	TP_PROTO(pid_t pid, pid_t tgid, int redundant, u32 bytes),

	TP_ARGS(pid, tgid, redundant, bytes),

	TP_STRUCT__entry(
		__field(pid_t, pid)
		__field(pid_t, tgid)
		__field(int,   redundant)
		__field(u32,   bytes)
	),

	TP_fast_assign(
		__entry->pid       = pid;
		__entry->tgid      = tgid;
		__entry->redundant = redundant;
		__entry->bytes     = bytes;
	),

	TP_printk("pid=%d tgid=%d redundant=%d bytes=%u",
		__entry->pid,
		__entry->tgid,
		__entry->redundant,
		__entry->bytes)
);

/*
 * atomik_net_send -- fires on every TCP send interception
 *
 * @sock_ptr:    struct sock pointer (opaque, for grouping)
 * @bytes:       payload size of this send
 * @redundant:   1 if fingerprint matches previous send, 0 otherwise
 * @fingerprint: CRC32C-based fingerprint of the payload
 */
TRACE_EVENT(atomik_net_send,

	TP_PROTO(unsigned long sock_ptr, u32 bytes, int redundant,
		 u64 fingerprint),

	TP_ARGS(sock_ptr, bytes, redundant, fingerprint),

	TP_STRUCT__entry(
		__field(unsigned long, sock_ptr)
		__field(u32,           bytes)
		__field(int,           redundant)
		__field(u64,           fingerprint)
	),

	TP_fast_assign(
		__entry->sock_ptr    = sock_ptr;
		__entry->bytes       = bytes;
		__entry->redundant   = redundant;
		__entry->fingerprint = fingerprint;
	),

	TP_printk("sock=%p bytes=%u redundant=%d fp=0x%llx",
		(void *)__entry->sock_ptr,
		__entry->bytes,
		__entry->redundant,
		__entry->fingerprint)
);

#endif /* _ATOMIK_TRACE_H */

/* This part must be outside the include guard */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE atomik_trace
#include <trace/define_trace.h>
