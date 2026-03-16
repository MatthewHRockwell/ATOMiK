/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ATOMIK_STATS_H
#define ATOMIK_STATS_H

#include <linux/percpu.h>

struct atomik_stats {
	u64 ops_load;
	u64 ops_accum;
	u64 ops_read;
	u64 ops_swap;
	u64 ops_merge;
	u64 fp_checks_total;
	u64 fp_checks_unchanged;
	u64 fp_bytes_fingerprinted;
	u64 fp_bytes_skipped;
	u64 fp_regions_active;
	u64 tables_active;
	u64 contexts_active;
	/* COW optimization stats (v0.3) */
	u64 cow_faults_total;		/* total COW faults intercepted */
	u64 cow_copies_performed;	/* copies that proceeded */
	u64 cow_copies_redundant;	/* copies where content was identical */
	u64 cow_bytes_copied;		/* bytes actually copied */
	u64 cow_bytes_saved;		/* bytes of redundant copies detected */
	/* Network monitoring stats (v0.4) */
	u64 net_sends_total;		/* total TCP sends intercepted */
	u64 net_sends_redundant;	/* consecutive identical sends */
	u64 net_bytes_total;		/* total bytes sent */
	u64 net_bytes_redundant;	/* bytes in redundant sends */
};

DECLARE_PER_CPU(struct atomik_stats, atomik_pcpu_stats);

#define atomik_stat_inc(field)  this_cpu_inc(atomik_pcpu_stats.field)
#define atomik_stat_add(field, val) this_cpu_add(atomik_pcpu_stats.field, val)

static inline void atomik_stats_aggregate(struct atomik_stats *out)
{
	int cpu;

	memset(out, 0, sizeof(*out));
	for_each_possible_cpu(cpu) {
		const struct atomik_stats *s = per_cpu_ptr(&atomik_pcpu_stats, cpu);
		out->ops_load += READ_ONCE(s->ops_load);
		out->ops_accum += READ_ONCE(s->ops_accum);
		out->ops_read += READ_ONCE(s->ops_read);
		out->ops_swap += READ_ONCE(s->ops_swap);
		out->ops_merge += READ_ONCE(s->ops_merge);
		out->fp_checks_total += READ_ONCE(s->fp_checks_total);
		out->fp_checks_unchanged += READ_ONCE(s->fp_checks_unchanged);
		out->fp_bytes_fingerprinted += READ_ONCE(s->fp_bytes_fingerprinted);
		out->fp_bytes_skipped += READ_ONCE(s->fp_bytes_skipped);
		out->cow_faults_total += READ_ONCE(s->cow_faults_total);
		out->cow_copies_performed += READ_ONCE(s->cow_copies_performed);
		out->cow_copies_redundant += READ_ONCE(s->cow_copies_redundant);
		out->cow_bytes_copied += READ_ONCE(s->cow_bytes_copied);
		out->cow_bytes_saved += READ_ONCE(s->cow_bytes_saved);
		out->net_sends_total += READ_ONCE(s->net_sends_total);
		out->net_sends_redundant += READ_ONCE(s->net_sends_redundant);
		out->net_bytes_total += READ_ONCE(s->net_bytes_total);
		out->net_bytes_redundant += READ_ONCE(s->net_bytes_redundant);
	}
}

#endif /* ATOMIK_STATS_H */
