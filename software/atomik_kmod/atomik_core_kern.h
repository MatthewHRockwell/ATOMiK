/* SPDX-License-Identifier: GPL-2.0 */
/*
 * atomik_core_kern.h — Kernel-space ATOMiK delta-state algebra
 *
 * Ported from atomik_core.h (userspace, Apache-2.0).
 * All operations are O(1), deterministic, timing-safe.
 * No floating point, no dynamic allocation in hot paths.
 */

#ifndef ATOMIK_CORE_KERN_H
#define ATOMIK_CORE_KERN_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

/* Single context */
struct atomik_ctx {
	u64 reference;
	u64 accumulator;
	u32 delta_count;
};

/* Multi-context table */
struct atomik_table {
	struct atomik_ctx *contexts;   /* software-backed contexts (always valid for sw) */
	u32 num_contexts;
	spinlock_t lock;               /* protects SWAP (multi-write) */
	int hw_base;                   /* >=0: hardware context base slot, -1: sw only */
};

/* --- Single Context Operations --- */

static inline void atomik_ctx_init(struct atomik_ctx *ctx)
{
	ctx->reference = 0;
	ctx->accumulator = 0;
	ctx->delta_count = 0;
}

static inline void atomik_ctx_load(struct atomik_ctx *ctx, u64 value)
{
	WRITE_ONCE(ctx->reference, value);
	WRITE_ONCE(ctx->accumulator, 0);
	WRITE_ONCE(ctx->delta_count, 0);
}

static inline void atomik_ctx_accum(struct atomik_ctx *ctx, u64 delta)
{
	WRITE_ONCE(ctx->accumulator, READ_ONCE(ctx->accumulator) ^ delta);
	WRITE_ONCE(ctx->delta_count, READ_ONCE(ctx->delta_count) + 1);
}

static inline u64 atomik_ctx_read(const struct atomik_ctx *ctx)
{
	return READ_ONCE(ctx->reference) ^ READ_ONCE(ctx->accumulator);
}

static inline u64 atomik_ctx_swap(struct atomik_ctx *ctx)
{
	u64 state = READ_ONCE(ctx->reference) ^ READ_ONCE(ctx->accumulator);
	WRITE_ONCE(ctx->reference, state);
	WRITE_ONCE(ctx->accumulator, 0);
	WRITE_ONCE(ctx->delta_count, 0);
	return state;
}

static inline int atomik_ctx_is_clean(const struct atomik_ctx *ctx)
{
	return READ_ONCE(ctx->accumulator) == 0;
}

static inline void atomik_ctx_merge(struct atomik_ctx *dst,
				     const struct atomik_ctx *src)
{
	WRITE_ONCE(dst->accumulator,
		   READ_ONCE(dst->accumulator) ^ READ_ONCE(src->accumulator));
	WRITE_ONCE(dst->delta_count,
		   READ_ONCE(dst->delta_count) + READ_ONCE(src->delta_count));
}

/* --- Table Operations --- */

static inline int atomik_table_init(struct atomik_table *t, u32 num_contexts)
{
	t->contexts = kvmalloc_array(num_contexts, sizeof(struct atomik_ctx),
				     GFP_KERNEL | __GFP_ZERO);
	if (!t->contexts)
		return -ENOMEM;
	t->num_contexts = num_contexts;
	t->hw_base = -1;  /* default: software only */
	spin_lock_init(&t->lock);
	return 0;
}

static inline void atomik_table_free(struct atomik_table *t)
{
	kvfree(t->contexts);
	t->contexts = NULL;
	t->num_contexts = 0;
}

static inline void atomik_table_load(struct atomik_table *t, u32 addr, u64 val)
{
	if (likely(addr < t->num_contexts))
		atomik_ctx_load(&t->contexts[addr], val);
}

static inline void atomik_table_accum(struct atomik_table *t, u32 addr, u64 d)
{
	if (likely(addr < t->num_contexts))
		atomik_ctx_accum(&t->contexts[addr], d);
}

static inline u64 atomik_table_read(const struct atomik_table *t, u32 addr)
{
	if (likely(addr < t->num_contexts))
		return atomik_ctx_read(&t->contexts[addr]);
	return 0;
}

static inline u64 atomik_table_swap(struct atomik_table *t, u32 addr)
{
	u64 result = 0;
	unsigned long flags;

	if (likely(addr < t->num_contexts)) {
		spin_lock_irqsave(&t->lock, flags);
		result = atomik_ctx_swap(&t->contexts[addr]);
		spin_unlock_irqrestore(&t->lock, flags);
	}
	return result;
}

/* --- Page Fingerprinting --- */

static inline u64 atomik_fp_reduce_page(const u64 *data)
{
	u64 acc = 0;
	int i;

	for (i = 0; i < PAGE_SIZE / sizeof(u64); i += 8) {
		acc ^= data[i] ^ data[i + 1] ^ data[i + 2] ^ data[i + 3]
		     ^ data[i + 4] ^ data[i + 5] ^ data[i + 6] ^ data[i + 7];
	}
	return acc;
}

#endif /* ATOMIK_CORE_KERN_H */
