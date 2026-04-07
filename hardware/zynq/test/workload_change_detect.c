/*
 * workload_change_detect.c — ATOMiK multi-buffer change detection workload
 *
 * End-to-end task benchmark: track N memory regions, detect which ones
 * changed, report how many regions/sec can be monitored.
 *
 * Two approaches compared:
 *   Software: memcmp each region against a shadow copy — O(N * region_size)
 *   ATOMiK:   read acc_zero flag per region — O(N) regardless of region size
 *
 * This models a real workload: a service monitoring state buffers (pages,
 * config blocks, agent memory, sensor frames) for changes.
 *
 * Environment notes (printed in output):
 *   - VexRiscv SMP: 4 KB D-cache, 4 KB I-cache, no L2 cache
 *   - memcmp performance degrades sharply above 4 KB (cache thrashing)
 *   - ATOMiK detection is a single MMIO register read (~262 cycles)
 *   - Timer: rdtime at 100 MHz (10 ns resolution)
 *
 * Build:
 *   riscv32-buildroot-linux-gnu-gcc -O2 -static -o workload_change_detect \
 *       workload_change_detect.c ../../software/libatomik/libatomik.c \
 *       -I../../software/libatomik
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "libatomik.h"

/* ── Timer ────────────────────────────────────────────────────────── */

static inline uint64_t rdtime_ticks(void)
{
    uint32_t lo, hi, hi2;
    do {
        __asm__ volatile("rdtimeh %0" : "=r"(hi));
        __asm__ volatile("rdtime  %0" : "=r"(lo));
        __asm__ volatile("rdtimeh %0" : "=r"(hi2));
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

#define TIMER_HZ 100000000ULL  /* 100 MHz */

static volatile int sink;

/* ── Workload: software scan ──────────────────────────────────────── */

static int sw_detect_changes(uint8_t **regions, uint8_t **shadows,
                              int n_regions, size_t region_size,
                              int *changed_out)
{
    int count = 0;
    for (int i = 0; i < n_regions; i++) {
        int c = memcmp(regions[i], shadows[i], region_size) != 0;
        if (changed_out) changed_out[i] = c;
        count += c;
    }
    return count;
}

/* ── Workload: ATOMiK detect ──────────────────────────────────────── */

static int atomik_detect_changes(atomik_t *a, int n_regions, int *changed_out)
{
    /*
     * Each region uses one ATOMiK context address (0..N-1).
     * Fingerprints were pre-loaded. Deltas were accumulated when
     * regions were "written to." Detection is just reading acc_zero
     * for each context.
     *
     * In this benchmark, we pre-accumulate deltas for changed regions,
     * then measure only the detection sweep.
     */
    int count = 0;
    for (int i = 0; i < n_regions; i++) {
        /* SWAP to this region's context, then check acc_zero */
        atomik_swap(a, (uint8_t)i);
        int c = !atomik_acc_zero(a);
        if (changed_out) changed_out[i] = c;
        count += c;
    }
    return count;
}

/* ── Setup: initialize regions and ATOMiK contexts ────────────────── */

static void setup_regions(uint8_t **regions, uint8_t **shadows,
                           atomik_t *a, int n_regions, size_t region_size)
{
    for (int i = 0; i < n_regions; i++) {
        regions[i] = calloc(1, region_size);
        shadows[i] = calloc(1, region_size);

        /* Fill with deterministic pattern */
        for (size_t j = 0; j < region_size; j++)
            regions[i][j] = (uint8_t)(i * 37 + j * 13 + 7);

        /* Shadow = copy of region */
        memcpy(shadows[i], regions[i], region_size);

        /* Load ATOMiK context with XOR fingerprint of region */
        uint64_t fp = 0, chunk;
        for (size_t j = 0; j + 8 <= region_size; j += 8) {
            memcpy(&chunk, regions[i] + j, 8);
            fp ^= chunk;
        }
        atomik_load(a, (uint8_t)i, fp);
    }
}

static void apply_changes(uint8_t **regions, atomik_t *a,
                           int n_regions, size_t region_size,
                           int change_pct)
{
    /* Modify change_pct% of regions (1 byte each) */
    int n_change = (n_regions * change_pct + 99) / 100;
    for (int i = 0; i < n_change && i < n_regions; i++) {
        /* Flip last byte */
        regions[i][region_size - 1] ^= 0xFF;

        /* Accumulate the delta into ATOMiK (simulates write-time tracking) */
        uint64_t old_chunk = 0, new_chunk = 0;
        size_t tail_off = (region_size / 8) * 8;
        if (tail_off + 8 > region_size) tail_off = region_size - 8;
        memcpy(&old_chunk, regions[i] + tail_off, 8);
        /* old_chunk is the NEW value (we already flipped); original was ^0xFF */
        /* Delta = old XOR new for the changed chunk */
        new_chunk = old_chunk;
        /* We need to accumulate the delta: old_val ^ new_val for that chunk */
        /* Since we flipped 1 byte, the delta has 0xFF in that byte position */
        uint64_t delta = 0;
        size_t byte_in_chunk = (region_size - 1) - tail_off;
        delta = (uint64_t)0xFF << (byte_in_chunk * 8);
        atomik_swap(a, (uint8_t)i);
        atomik_accum(a, delta);
    }
}

/* ── Main ─────────────────────────────────────────────────────────── */

int main(void)
{
    atomik_t *a;

    printf("ATOMiK Multi-Buffer Change Detection Workload\n");
    printf("==============================================\n\n");
    printf("Environment: VexRiscv SMP @ 100 MHz, 4 KB D-cache, no L2\n");
    printf("Timer: rdtime @ 100 MHz (10 ns resolution)\n");
    printf("Note: memcmp degrades above 4 KB due to cache thrashing\n\n");

    a = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);
    if (!a) {
        printf("ERROR: cannot open ATOMiK\n");
        return 1;
    }
    printf("ATOMiK v%u, %u bank(s)\n\n", a->version, a->n_banks);

    /* Test configurations: (n_regions, region_size, change_pct) */
    struct {
        int n_regions;
        size_t region_size;
        int change_pct;
    } configs[] = {
        {  8,   256, 25 },  /* 8 x 256B, 25% changed — small, cache-friendly */
        {  8,  1024, 25 },  /* 8 x 1KB */
        {  8,  4096, 25 },  /* 8 x 4KB — at cache boundary */
        { 32,   256, 10 },  /* 32 x 256B, 10% changed — many regions */
        { 32,  1024, 10 },  /* 32 x 1KB */
        { 64,   256, 5  },  /* 64 x 256B, 5% changed — sparse changes */
        { 64,  1024, 5  },  /* 64 x 1KB */
    };
    int n_configs = sizeof(configs) / sizeof(configs[0]);

    printf("%-20s | %-12s | %-12s | %-10s | %-12s\n",
           "Workload", "SW (memcmp)", "ATOMiK", "Speedup", "Regions/sec");
    printf("%-20s-+-%-12s-+-%-12s-+-%-10s-+-%-12s\n",
           "--------------------", "------------", "------------",
           "----------", "------------");

    for (int c = 0; c < n_configs; c++) {
        int nr = configs[c].n_regions;
        size_t rs = configs[c].region_size;
        int cp = configs[c].change_pct;

        uint8_t **regions = calloc(nr, sizeof(uint8_t *));
        uint8_t **shadows = calloc(nr, sizeof(uint8_t *));
        int *changed = calloc(nr, sizeof(int));

        /* Setup */
        setup_regions(regions, shadows, a, nr, rs);
        apply_changes(regions, a, nr, rs, cp);

        /* Benchmark: software path */
        int iters = 50;
        uint64_t t0 = rdtime_ticks();
        for (int it = 0; it < iters; it++)
            sink = sw_detect_changes(regions, shadows, nr, rs, changed);
        uint64_t t1 = rdtime_ticks();
        uint64_t sw_ticks = (t1 - t0) / iters;

        /* Benchmark: ATOMiK path (detection only) */
        t0 = rdtime_ticks();
        for (int it = 0; it < iters; it++)
            sink = atomik_detect_changes(a, nr, changed);
        t1 = rdtime_ticks();
        uint64_t hw_ticks = (t1 - t0) / iters;

        double speedup = (double)sw_ticks / hw_ticks;
        double regions_per_sec = (double)nr * TIMER_HZ / hw_ticks;

        char label[32];
        snprintf(label, sizeof(label), "%dx%zuB %d%%chg", nr, rs, cp);

        printf("%-20s | %10llu cy | %10llu cy | %8.1fx | %10.0f\n",
               label,
               (unsigned long long)sw_ticks,
               (unsigned long long)hw_ticks,
               speedup, regions_per_sec);

        /* Cleanup */
        for (int i = 0; i < nr; i++) {
            free(regions[i]);
            free(shadows[i]);
        }
        free(regions);
        free(shadows);
        free(changed);
    }

    printf("\n");
    printf("SW path: memcmp each region against shadow copy — O(N * size)\n");
    printf("ATOMiK:  swap context + read acc_zero flag — O(N), size-independent\n");

    atomik_close(a);
    return 0;
}
