// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_net.c — Network delta compression monitoring
 *
 * Attaches a kprobe to tcp_sendmsg() to fingerprint outgoing TCP data.
 * For each socket, maintains a rolling XOR-reduce fingerprint of the
 * last send. When consecutive sends have identical fingerprints,
 * ATOMiK flags them as redundant network traffic.
 *
 * Common sources of redundant network traffic:
 *   - Prometheus/metrics exporters: identical metrics every scrape
 *   - Database replication: no-op transaction WAL entries
 *   - WebSocket heartbeats: identical keepalive payloads
 *   - Polling APIs: "no update" responses
 *   - Monitoring agents: identical system stats every interval
 *
 * v0.4: Observation + telemetry only. We fingerprint sends and
 * track redundancy, but don't suppress or compress traffic.
 * This data shows customers exactly how much bandwidth they waste.
 *
 * Safety:
 *   - Never modifies socket state or packet data
 *   - Fail-open: errors skip the fingerprint, traffic flows normally
 *   - Toggleable: echo 0 > /sys/class/atomik/atomik0/net_enabled
 *   - No sleeping in probe handlers
 *   - Per-socket tracking via hashtable (cleaned on socket close)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/hashtable.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/uio.h>
#include <linux/crc32.h>

#include "atomik_cgroup.h"
#include "atomik_net.h"
#include "atomik_stats.h"

static atomic_t net_enabled = ATOMIC_INIT(1);

/*
 * Per-socket fingerprint tracking.
 * Keyed by sock pointer — unique per connection.
 */
#define NET_SOCK_HASH_BITS  12  /* 4096 buckets */
static DEFINE_HASHTABLE(net_sock_table, NET_SOCK_HASH_BITS);
static DEFINE_SPINLOCK(net_sock_lock);

struct net_sock_entry {
	unsigned long sock_ptr;     /* key: (unsigned long)sk */
	u64 last_fingerprint;       /* fingerprint of last send */
	u32 consecutive_dup;        /* consecutive identical sends */
	u64 total_sends;
	u64 redundant_sends;
	u64 redundant_bytes;
	struct hlist_node node;
};

static struct net_sock_entry *net_sock_find_or_create(struct sock *sk)
{
	struct net_sock_entry *entry;
	unsigned long key = (unsigned long)sk;
	unsigned long flags;

	spin_lock_irqsave(&net_sock_lock, flags);
	hash_for_each_possible(net_sock_table, entry, node, key) {
		if (entry->sock_ptr == key) {
			spin_unlock_irqrestore(&net_sock_lock, flags);
			return entry;
		}
	}
	spin_unlock_irqrestore(&net_sock_lock, flags);

	entry = kzalloc(sizeof(*entry), GFP_ATOMIC);
	if (!entry)
		return NULL;

	entry->sock_ptr = key;

	spin_lock_irqsave(&net_sock_lock, flags);
	/* Double-check */
	{
		struct net_sock_entry *existing;

		hash_for_each_possible(net_sock_table, existing, node, key) {
			if (existing->sock_ptr == key) {
				spin_unlock_irqrestore(&net_sock_lock, flags);
				kfree(entry);
				return existing;
			}
		}
	}
	hash_add(net_sock_table, &entry->node, key);
	spin_unlock_irqrestore(&net_sock_lock, flags);

	return entry;
}

/*
 * Fingerprint a msghdr's data using hardware-accelerated CRC32C.
 *
 * This runs in kprobe context (process context for tcp_sendmsg).
 * copy_from_iter_full does NOT work on stack-copied iov_iters in
 * kprobe context, so we extract the user buffer pointer directly
 * and use copy_from_user().
 *
 * CRC32C is used because:
 *   - x86 has hardware CRC32 instructions (SSE4.2), near-zero overhead
 *   - No algebraic weaknesses (rotate-XOR has period-8 collapse on
 *     byte-repeating inputs; plain XOR cancels on even-count uniform words)
 *   - Kernel provides crc32c() with automatic HW acceleration
 *
 * We fingerprint the first page (4KB) for speed.
 */
static u64 fingerprint_msghdr(struct msghdr *msg, size_t len)
{
	struct iov_iter *iter = &msg->msg_iter;
	const void __user *ubase = NULL;
	size_t ulen = 0;
	u32 crc = 0;
	size_t remaining, off;
	u8 buf[512];
	size_t chunk;

	/* Extract user buffer pointer based on iter type */
	if (iter->iter_type == ITER_UBUF) {
		ubase = iter->ubuf;
		ulen = iter->count;
	} else if (iter->iter_type == ITER_IOVEC && iter->nr_segs > 0) {
		const struct iovec *iov = iter->__iov;

		ubase = iov->iov_base;
		ulen = iov->iov_len;
	} else {
		/* Kernel buffers (KVEC/BVEC) — skip fingerprinting */
		goto length_only;
	}

	if (!ubase || ulen == 0)
		goto length_only;

	/* Account for partial iterator consumption via iov_offset */
	ubase += iter->iov_offset;
	ulen -= iter->iov_offset;

	remaining = ulen > PAGE_SIZE ? PAGE_SIZE : ulen;
	if (remaining > len)
		remaining = len;
	off = 0;

	while (remaining > 0) {
		chunk = remaining > sizeof(buf) ? sizeof(buf) : remaining;

		if (copy_from_user(buf, ubase + off, chunk))
			break;

		crc = crc32c(crc, buf, chunk);

		off += chunk;
		remaining -= chunk;
	}

length_only:
	/* Combine CRC with length into a 64-bit fingerprint */
	return ((u64)crc << 32) | (u32)(len * 0x9E3779B9U);
}

/*
 * Kprobe pre-handler on tcp_sendmsg().
 *
 * tcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size)
 *
 * On x86_64: RDI=sk, RSI=msg, RDX=size
 */
static int net_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct sock *sk;
	struct msghdr *msg;
	size_t size;
	struct net_sock_entry *entry;
	u64 fp;

	if (!atomic_read(&net_enabled))
		return 0;

	sk = (struct sock *)regs->di;
	msg = (struct msghdr *)regs->si;
	size = (size_t)regs->dx;

	if (!sk || !msg || size == 0)
		return 0;

	atomik_stat_inc(net_sends_total);
	atomik_stat_add(net_bytes_total, size);
	atomik_cgroup_record_net_send(size);

	/* Fingerprint the outgoing data */
	fp = fingerprint_msghdr(msg, size);

	/* Look up or create per-socket entry */
	entry = net_sock_find_or_create(sk);
	if (!entry)
		return 0;

	WRITE_ONCE(entry->total_sends,
		   READ_ONCE(entry->total_sends) + 1);

	if (READ_ONCE(entry->total_sends) > 1 &&
	    fp == READ_ONCE(entry->last_fingerprint)) {
		/* Consecutive identical send detected */
		WRITE_ONCE(entry->consecutive_dup,
			   READ_ONCE(entry->consecutive_dup) + 1);
		WRITE_ONCE(entry->redundant_sends,
			   READ_ONCE(entry->redundant_sends) + 1);
		WRITE_ONCE(entry->redundant_bytes,
			   READ_ONCE(entry->redundant_bytes) + size);

		atomik_stat_inc(net_sends_redundant);
		atomik_stat_add(net_bytes_redundant, size);
		atomik_cgroup_record_net_redundant(size);
	} else {
		WRITE_ONCE(entry->consecutive_dup, 0);
	}

	WRITE_ONCE(entry->last_fingerprint, fp);

	return 0;
}

static struct kprobe net_kprobe = {
	.pre_handler = net_pre_handler,
	.symbol_name = "tcp_sendmsg",
};

/* --- Public API --- */

void atomik_net_set_enabled(bool enabled)
{
	atomic_set(&net_enabled, enabled ? 1 : 0);
	pr_info("ATOMiK network monitoring %s\n",
		enabled ? "enabled" : "disabled");
}

bool atomik_net_is_enabled(void)
{
	return atomic_read(&net_enabled) != 0;
}

int atomik_net_init(void)
{
	int ret;

	hash_init(net_sock_table);

	ret = register_kprobe(&net_kprobe);
	if (ret < 0) {
		pr_warn("atomik: network kprobe registration failed (%d). "
			"Network monitoring disabled.\n", ret);
		return ret;
	}

	pr_info("ATOMiK network monitor active — monitoring tcp_sendmsg()\n");
	return 0;
}

void atomik_net_exit(void)
{
	struct net_sock_entry *entry;
	struct hlist_node *tmp;
	unsigned long flags;
	int bkt;

	unregister_kprobe(&net_kprobe);

	spin_lock_irqsave(&net_sock_lock, flags);
	hash_for_each_safe(net_sock_table, bkt, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
	spin_unlock_irqrestore(&net_sock_lock, flags);
}
