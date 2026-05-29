/* perf_counter.c — performance instrumentation, v0.33-A.
 *
 * "Make performance observable everywhere" (ChatGPT 2026-05-08).
 *
 * RV64 cycle counter is the primary timing source: csrr cycle is one
 * instruction, no syscall, no kernel transition, ~10 ns granularity at
 * our 100 MHz fabric clock.  Linux exposes cycle (CSR 0xC00) to U-mode
 * by default on RISC-V — no special permission needed.  This is the
 * cleanest way to time tight loops without poisoning the measurement.
 *
 * The perf_sample_t lifecycle:
 *   perf_begin(s, personality)      // snapshot start cycle, zero counters
 *   perf_op(s, in, out, region)     // each user-visible op
 *   perf_hw_op(s)                   // each hardware MMIO actually issued
 *   perf_batch_summary(s, n, u)     // coalesce metadata at commit
 *   perf_end(s)                     // sort per-op cycles, compute p50/p95/p99
 *                                   // publish to s_last + s_last_per[p]
 *
 * After perf_end, Resource Fabric reads via perf_last_sample() or
 * perf_last_for(personality).  No mutation after perf_end — sample
 * is owned by caller; we copy what we need into the published slot.
 */
#include "atomik_os.h"
#include <string.h>
#if !defined(__riscv)
#include <time.h>      /* host preview: clock_gettime pseudo-cycle source */
#endif

/* Last-completed sample, global + per-personality.  Used by the live
 * Resource Fabric metrics surface (v0.33-E).  Sized small (one struct
 * per personality + one global), no allocation. */
static perf_sample_t s_last;
static int           s_last_valid = 0;
static perf_sample_t s_last_per[PERSONALITY_AGENT + 1];
static int           s_last_per_valid[PERSONALITY_AGENT + 1] = {0};

/* Per-sample working state.  We store the begin-cycle in cycles_total
 * during the lifetime of the sample so perf_op can compute per-op
 * deltas; perf_end converts cycles_total to actual elapsed by
 * subtracting the begin-cycle from the end-cycle. */
static uint64_t s_begin_cycle_for(perf_sample_t *s) {
    /* The first uint64 inside per_op_cycles is unused at perf_begin —
     * we stash the begin-cycle in cycles_total temporarily.  See
     * perf_begin / perf_end for the lifecycle. */
    return s->cycles_total;
}

uint64_t perf_rdcycle(void) {
#if defined(__riscv)
    uint64_t v;
    __asm__ volatile("csrr %0, cycle" : "=r"(v));
    return v;
#else
    /* Host preview build: no RV64 cycle CSR.  Monotonic ns as a pseudo-cycle
     * source — plausible relative timing for LAYOUT preview only, never
     * published as a hardware measurement. */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

void perf_begin(perf_sample_t *s, personality_t p) {
    memset(s, 0, sizeof(*s));
    s->active_personality = p;
    s->cycles_total       = perf_rdcycle();   /* stash begin-cycle here */
}

void perf_op(perf_sample_t *s, uint32_t bytes_in, uint32_t bytes_out,
             uint32_t region_id) {
    (void)region_id;    /* reserved for region-coverage tracking, v0.33-D */
    s->ops_logical++;
    s->bytes_processed += bytes_in;
    if (bytes_out < bytes_in) {
        s->bytes_avoided += (bytes_in - bytes_out);
    }
    /* Per-op cycle sample for percentiles.  We sample the first
     * PERF_MAX_OPS ops; subsequent ops contribute to totals but not
     * to the percentile distribution.  Could swap to reservoir
     * sampling later if mega-batches need fairer p99. */
    if (s->per_op_count < PERF_MAX_OPS) {
        s->per_op_cycles[s->per_op_count++] = perf_rdcycle();
    }
}

void perf_hw_op(perf_sample_t *s) {
    s->ops_issued++;
}

void perf_batch_summary(perf_sample_t *s, uint32_t logical, uint32_t unique) {
    s->batch_count++;
    /* Use this as the authoritative coalesce metric — caller knows the
     * shape of the batch better than we can infer from per_op timing. */
    if (logical) s->ops_logical    = logical;
    if (unique)  s->regions_unique = unique;
}

/* Quicksort the per-op cycle array in place so perf_end can pull
 * percentile indices directly. */
static void sort_u64(uint64_t *a, int n) {
    /* Tiny insertion sort — n is at most PERF_MAX_OPS = 256, so the
     * O(n^2) is fine and avoids stack-recursion concerns. */
    for (int i = 1; i < n; i++) {
        uint64_t v = a[i];
        int      j = i - 1;
        while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; }
        a[j + 1] = v;
    }
}

void perf_end(perf_sample_t *s) {
    uint64_t end_cycle = perf_rdcycle();
    uint64_t begin     = s_begin_cycle_for(s);
    /* Convert the stashed begin-cycle into an elapsed-cycle total.
     * Wraparound: the cycle counter is 64-bit; at 100 MHz it wraps in
     * ~5800 years.  Don't bother handling overflow. */
    s->cycles_total = end_cycle - begin;

    /* Compute per-op deltas FROM THE STASHED RAW SAMPLES.  The samples
     * in per_op_cycles are absolute cycle counts at the moment perf_op
     * was called; convert each to a delta from the previous one (or
     * from begin for the first).  Then sort for percentiles. */
    int n = s->per_op_count;
    if (n > 0) {
        uint64_t prev = begin;
        for (int i = 0; i < n; i++) {
            uint64_t at  = s->per_op_cycles[i];
            uint64_t dur = at - prev;
            prev = at;
            s->per_op_cycles[i] = dur;
        }
        sort_u64(s->per_op_cycles, n);
        s->p50 = s->per_op_cycles[(n * 50) / 100];
        s->p95 = s->per_op_cycles[(n * 95) / 100];
        s->p99 = s->per_op_cycles[(n * 99) / 100];
    }

    /* Publish.  Copy into the global slot AND the personality-specific
     * slot so Resource Fabric can read either independently without
     * cross-contamination. */
    s_last       = *s;
    s_last_valid = 1;
    int p = (int)s->active_personality;
    if (p >= 0 && p <= (int)PERSONALITY_AGENT) {
        s_last_per[p]       = *s;
        s_last_per_valid[p] = 1;
    }
}

double perf_speedup(const perf_sample_t *s) {
    if (!s || s->cycles_atomik == 0 || s->cycles_software_baseline == 0)
        return 0.0;
    return (double)s->cycles_software_baseline / (double)s->cycles_atomik;
}

double perf_bytes_avoided_pct(const perf_sample_t *s) {
    if (!s || s->bytes_processed == 0) return 0.0;
    return 100.0 * (double)s->bytes_avoided / (double)s->bytes_processed;
}

double perf_coalesce_ratio(const perf_sample_t *s) {
    if (!s || s->ops_issued == 0) return 0.0;
    return (double)s->ops_logical / (double)s->ops_issued;
}

const perf_sample_t *perf_last_sample(void) {
    return s_last_valid ? &s_last : NULL;
}

const perf_sample_t *perf_last_for(personality_t p) {
    int i = (int)p;
    if (i < 0 || i > (int)PERSONALITY_AGENT) return NULL;
    return s_last_per_valid[i] ? &s_last_per[i] : NULL;
}
