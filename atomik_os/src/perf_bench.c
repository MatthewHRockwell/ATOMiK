/* perf_bench.c — software baseline + four-way comparison harness, v0.33-D.
 *
 * Per ChatGPT 2026-05-08: "for every optimized ATOMiK path, build the
 * software equivalent, then compare four ways: software / ATOMiK direct
 * / ATOMiK batched / ATOMiK profile-selected.  That four-way comparison
 * shows architectural compounding."
 *
 * Same input sequence across all four columns; deterministic seed
 * derived from (regions, ops, profile) so the matrix is reproducible.
 * Each column's perf_sample_t captures cycles, hw ops, bytes processed,
 * bytes avoided, p50/p95/p99.  Caller can render the table however
 * they want — UART console, Resource Fabric, the gcc-demo executable.
 *
 * The point of the four-way (not two-way "ATOMiK vs software") split
 * is to attribute the win to the right architectural layer:
 *   col 1 → 2: hardware primitive (ATOMiK adapter exists)
 *   col 2 → 3: batching (one fence per batch, not per op)
 *   col 3 → 4: personality (skip-unchanged, relevance, etc.)
 *
 * Each layer is a separate architectural contribution.  Demo telling.
 */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

/* ===== deterministic workload generator ===== */

/* Linear-congruential — seeded so (regions, ops, profile) returns
 * the same sequence every call.  Standard Numerical Recipes coeffs;
 * fine for our purpose (just needs to spread region IDs and deltas). */
static uint32_t lcg_next(uint32_t *s) {
    *s = (*s) * 1664525u + 1013904223u;
    return *s;
}

/* Generate one op (region, delta) pair from the current LCG state. */
static void gen_op(uint32_t *s, int regions, uint32_t *region, uint32_t *delta) {
    *region = lcg_next(s) % (uint32_t)regions;
    *delta  = lcg_next(s);
}

/* ===== software baseline ===== */

/* The "software baseline" is what a software-only implementation must
 * do per logical op:
 *   1. Read the current state slot (memory load).
 *   2. Apply the delta (XOR or whatever the algebra is).  The point
 *      is software DOES NOT KNOW the algebra is XOR-commutative, so
 *      it can't dedupe — every op is a real load + compute + store.
 *   3. Issue a memory barrier so cross-domain consumers see ordering.
 *
 * We model this with a small static state buffer (one uint32_t per
 * region), one fence per op.  No coalesce, no skip.  This is the
 * lower bound on software work for the same workload. */

#define SW_BASELINE_REGIONS 64
static uint32_t s_sw_state[SW_BASELINE_REGIONS];

static void sw_baseline_run(int regions, int ops, perf_sample_t *out) {
    /* Reset state so the run is reproducible. */
    memset(s_sw_state, 0, sizeof s_sw_state);
    if (regions > SW_BASELINE_REGIONS) regions = SW_BASELINE_REGIONS;

    perf_begin(out, PERSONALITY_NONE);
    uint32_t seed = (uint32_t)regions * 31u + (uint32_t)ops * 7919u;
    for (int i = 0; i < ops; i++) {
        uint32_t r, d;
        gen_op(&seed, regions, &r, &d);
        /* Software op: load, modify, store, fence. */
        uint32_t cur = s_sw_state[r];
        cur ^= d;
        s_sw_state[r] = cur;
#if defined(__riscv)
        __asm__ volatile("fence" ::: "memory");   /* per-op fence */
#else
        __asm__ volatile("" ::: "memory");        /* host preview: compiler barrier */
#endif
        perf_op(out, sizeof(uint32_t), sizeof(uint32_t), r);
        perf_hw_op(out);
    }
    perf_batch_summary(out, (uint32_t)ops, (uint32_t)ops);
    out->cycles_software_baseline = 0;   /* set by harness */
    perf_end(out);
}

/* ===== ATOMiK column drivers ===== */

/* Replay the same deterministic sequence through atomik_batch.
 * mode chooses the commit path:
 *   0 = baseline (per-op MMIO, per-op fence) — col 2
 *   1 = batched, profile=NONE                — col 3
 *   2 = batched, given profile               — col 4
 *
 * We capture perf_last_sample after each commit because atomik_batch
 * publishes the perf data inside its own perf_end. */
static void atomik_run(int regions, int ops, int mode,
                       atomik_profile_t profile, perf_sample_t *out) {
    atomik_profile_t p = (mode == 2) ? profile : ATOMIK_PROFILE_NONE;
    atomik_batch_begin(p);
    uint32_t seed = (uint32_t)regions * 31u + (uint32_t)ops * 7919u;
    for (int i = 0; i < ops; i++) {
        uint32_t r, d;
        gen_op(&seed, regions, &r, &d);
        atomik_batch_add(r, d);
    }
    if (mode == 0) atomik_batch_commit_baseline();
    else           atomik_batch_commit();
    /* Copy the just-published sample.  perf_last_sample returns the
     * MOST RECENT completed sample, which is exactly the one we just
     * committed.  Snapshot it before another producer overwrites. */
    const perf_sample_t *last = perf_last_sample();
    if (last) *out = *last;
    else      memset(out, 0, sizeof *out);
}

void perf_bench_run(int regions, int ops, atomik_profile_t profile,
                    perf_bench_result_t *out) {
    memset(out, 0, sizeof *out);
    out->regions = regions;
    out->ops     = ops;
    out->profile = profile;

    sw_baseline_run(regions, ops, &out->software);
    atomik_run     (regions, ops, 0, profile, &out->atomik_direct);
    atomik_run     (regions, ops, 1, profile, &out->atomik_batched);
    atomik_run     (regions, ops, 2, profile, &out->atomik_profile_col);

    /* Cross-fill: each ATOMiK column gets the software baseline as
     * its denominator so perf_speedup() yields a meaningful ratio. */
    out->atomik_direct.cycles_software_baseline      = out->software.cycles_total;
    out->atomik_direct.cycles_atomik                 = out->atomik_direct.cycles_total;
    out->atomik_batched.cycles_software_baseline     = out->software.cycles_total;
    out->atomik_batched.cycles_atomik                = out->atomik_batched.cycles_total;
    out->atomik_profile_col.cycles_software_baseline = out->software.cycles_total;
    out->atomik_profile_col.cycles_atomik            = out->atomik_profile_col.cycles_total;
}

/* ===== printable matrix ===== */

static const char *profile_short(atomik_profile_t p) {
    switch (p) {
    case ATOMIK_PROFILE_STATE: return "STATE";
    case ATOMIK_PROFILE_SYNC:  return "SYNC ";
    case ATOMIK_PROFILE_AGENT: return "AGENT";
    default:                   return "----";
    }
}

void perf_bench_print_header(void) {
    /* Eight-segment header: regions, ops, profile, then per-column
     * summary (cycles + hw ops or bytes_avoided).  Aligned for a
     * 80-col UART console. */
    printf("\n");
    printf("regs  ops  prof  | software cyc  | atomik direct  | atomik batched | atomik profile\n");
    printf("                  | (hw ops/byts) | (hw ops/byts)  | (hw ops/byts)  | (hw ops/byts av)\n");
    printf("----  ---  ----- | ------------  | -------------  | -------------  | ----------------\n");
}

static void print_col(const perf_sample_t *s) {
    /* Per-column: total cycles + (hw_ops, bytes_avoided).  Tight on
     * width because we want all four columns on one row. */
    printf("%9llu (%2u/%4u) ",
           (unsigned long long)s->cycles_total,
           (unsigned)s->ops_issued,
           (unsigned)s->bytes_avoided);
}

void perf_bench_print_row(const perf_bench_result_t *r) {
    printf("%4d  %3d  %s | ", r->regions, r->ops, profile_short(r->profile));
    print_col(&r->software);
    printf("| ");
    print_col(&r->atomik_direct);
    printf("| ");
    print_col(&r->atomik_batched);
    printf("| ");
    print_col(&r->atomik_profile_col);
    printf("\n");
}

void perf_bench_matrix(void) {
    /* Standard matrix per project_v033_substance_plan.md.
     * Each row varies workload shape; each cell is one perf_bench_run.
     * Three workload sizes × three region counts × three profiles =
     * 27 runs.  ~5-10 seconds on the AX7020.
     *
     * Profile column varies by row so we exercise STATE/SYNC/AGENT
     * commit paths under different workload shapes. */
    perf_bench_print_header();

    struct { int regs, ops; atomik_profile_t prof; } shapes[] = {
        /* small */
        {  8,  64, ATOMIK_PROFILE_STATE },
        {  8,  64, ATOMIK_PROFILE_SYNC  },
        {  8,  64, ATOMIK_PROFILE_AGENT },
        /* medium */
        { 32, 256, ATOMIK_PROFILE_STATE },
        { 32, 256, ATOMIK_PROFILE_SYNC  },
        { 32, 256, ATOMIK_PROFILE_AGENT },
        /* large — exercise the AGENT relevance sort under load */
        { 64, 1024, ATOMIK_PROFILE_STATE },
        { 64, 1024, ATOMIK_PROFILE_SYNC  },
        { 64, 1024, ATOMIK_PROFILE_AGENT },
    };
    int n = (int)(sizeof shapes / sizeof shapes[0]);

    perf_bench_result_t r;
    for (int i = 0; i < n; i++) {
        perf_bench_run(shapes[i].regs, shapes[i].ops, shapes[i].prof, &r);
        perf_bench_print_row(&r);
    }
    printf("\n");
    printf("legend: cycles_total (hw_ops_emitted / bytes_avoided)\n");
    printf("speedup = software_cycles / atomik_cycles (use perf_speedup)\n");
    printf("\n");
}
