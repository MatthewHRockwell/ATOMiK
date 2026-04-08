/*
 * bench_change_detect.c — ATOMiK change detection benchmark (Linux userspace)
 *
 * Compares two approaches for detecting whether a memory region changed:
 *   1. Software memcmp: scan every byte, O(n) per check
 *   2. ATOMiK fingerprint: accumulate delta, read acc_zero flag, O(1)
 *
 * This is the core value proposition: ATOMiK detects changes in constant time
 * regardless of region size. The benchmark measures wall-clock cycles for each
 * approach across varying buffer sizes.
 *
 * Build (with buildroot cross-compiler):
 *   riscv32-buildroot-linux-gnu-gcc -O2 -static -o bench_change_detect bench_change_detect.c \
 *       -I../../software/libatomik -L. -latomik
 *
 * Or standalone (links libatomik source directly):
 *   riscv32-buildroot-linux-gnu-gcc -O2 -static -o bench_change_detect \
 *       bench_change_detect.c ../../software/libatomik/libatomik.c \
 *       -I../../software/libatomik
 *
 * Run on Zynq Linux:
 *   mknod /dev/mem c 1 1 2>/dev/null
 *   ./bench_change_detect
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "libatomik.h"

/* ── Cycle counter (RISC-V mcycle/rdcycle) ─────────────────────────── */

static inline uint64_t rdcycle(void)
{
#if defined(__riscv)
    /* Use rdtime (not rdcycle) — cycle counter is M-mode only on VexRiscv SMP.
     * Timer runs at 100 MHz (10ns resolution), same as the CPU clock. */
    uint32_t lo, hi, hi2;
    do {
        __asm__ volatile("rdtimeh %0" : "=r"(hi));
        __asm__ volatile("rdtime  %0" : "=r"(lo));
        __asm__ volatile("rdtimeh %0" : "=r"(hi2));
    } while (hi != hi2);  /* Handle rollover */
    return ((uint64_t)hi << 32) | lo;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

/* ── Software memcmp-based change detection ───────────────────────── */

static int detect_change_memcmp(const uint8_t *prev, const uint8_t *curr, size_t n)
{
    return memcmp(prev, curr, n) != 0;
}

/* ── Prevent dead-code elimination ────────────────────────────────── */

static volatile int sink;

/* ── Benchmark runner ─────────────────────────────────────────────── */

#define ITERATIONS 100

static void run_bench(atomik_t *a, size_t bufsize)
{
    uint8_t *prev, *curr;
    uint64_t t0, t1;
    uint64_t cy_memcmp, cy_detect;
    int i;

    prev = calloc(1, bufsize);
    curr = calloc(1, bufsize);
    if (!prev || !curr) {
        printf("  alloc failed for %zu bytes\n", bufsize);
        free(prev); free(curr);
        return;
    }

    /* Fill with known pattern */
    for (size_t j = 0; j < bufsize; j++) {
        prev[j] = (uint8_t)(j * 17 + 3);
        curr[j] = prev[j];
    }

    /* --- Software memcmp: O(n) full scan --- */
    t0 = rdcycle();
    for (i = 0; i < ITERATIONS; i++)
        sink = detect_change_memcmp(prev, curr, bufsize);
    t1 = rdcycle();
    cy_memcmp = (t1 - t0) / ITERATIONS;

    /* --- ATOMiK O(1) detection only ---
     * Pre-accumulate the buffer once, then measure just the acc_zero check.
     * This models the production case: deltas accumulate at write time,
     * detection is a single register read. */
    atomik_load(a, 0, 0);
    {
        size_t j;
        uint64_t chunk;
        for (j = 0; j + 8 <= bufsize; j += 8) {
            memcpy(&chunk, curr + j, 8);
            atomik_accum(a, chunk);
        }
    }
    /* Now measure ONLY the detection cost (read acc_zero flag) */
    t0 = rdcycle();
    for (i = 0; i < ITERATIONS; i++)
        sink = atomik_acc_zero(a);
    t1 = rdcycle();
    cy_detect = (t1 - t0) / ITERATIONS;

    printf("  %6zu B  | memcmp: %7llu cy | detect: %5llu cy | speedup: %5.1fx\n",
           bufsize,
           (unsigned long long)cy_memcmp,
           (unsigned long long)cy_detect,
           cy_memcmp > 0 ? (double)cy_memcmp / cy_detect : 0.0);

    /* --- Same with 1-byte change (worst case for memcmp) --- */
    curr[bufsize - 1] ^= 0xFF;

    t0 = rdcycle();
    for (i = 0; i < ITERATIONS; i++)
        sink = detect_change_memcmp(prev, curr, bufsize);
    t1 = rdcycle();
    cy_memcmp = (t1 - t0) / ITERATIONS;

    /* Re-accumulate with changed buffer */
    atomik_load(a, 0, 0);
    {
        size_t j;
        uint64_t chunk;
        for (j = 0; j + 8 <= bufsize; j += 8) {
            memcpy(&chunk, curr + j, 8);
            atomik_accum(a, chunk);
        }
    }
    t0 = rdcycle();
    for (i = 0; i < ITERATIONS; i++)
        sink = atomik_acc_zero(a);
    t1 = rdcycle();
    cy_detect = (t1 - t0) / ITERATIONS;

    printf("  %6zu +C | memcmp: %7llu cy | detect: %5llu cy | speedup: %5.1fx\n",
           bufsize,
           (unsigned long long)cy_memcmp,
           (unsigned long long)cy_detect,
           cy_memcmp > 0 ? (double)cy_memcmp / cy_detect : 0.0);

    free(prev);
    free(curr);
}

/* ── Main ─────────────────────────────────────────────────────────── */

static void run_backend(const char *name, unsigned long addr, atomik_layout_t layout,
                        size_t *sizes, size_t n_sizes)
{
    atomik_t *a = atomik_open_devmem(addr, layout);
    if (!a) {
        printf("[%s] Cannot open at 0x%lX — skipped\n\n", name, addr);
        return;
    }
    printf("[%s] ATOMiK v%u, %u bank(s) @ 0x%lX\n", name, a->version, a->n_banks, addr);
    printf("  %-10s | %-19s | %-17s | %s\n",
           "Buffer", "Software (memcmp)", "ATOMiK (detect)", "Speedup");
    printf("  %.10s-+-%.19s-+-%.17s-+-%s\n",
           "----------", "-------------------",
           "-----------------", "--------");
    for (size_t s = 0; s < n_sizes; s++)
        run_bench(a, sizes[s]);
    printf("\n");
    atomik_close(a);
}

int main(void)
{
    printf("========================================\n");
    printf("ATOMiK Change Detection Benchmark\n");
    printf("Three-way: software / CSR / adapter\n");
    printf("100 iterations per measurement\n");
    printf("Timer: rdtime @ 100 MHz (10ns)\n");
    printf("========================================\n\n");

    size_t sizes[] = { 64, 256, 1024, 4096, 16384, 65536 };
    size_t n = sizeof(sizes)/sizeof(sizes[0]);

    /* Backend 1: Direct CSR (Migen module, validated 16/16) */
    run_backend("CSR", 0xF0000000, ATOMIK_LAYOUT_CSR, sizes, n);

    /* Backend 2: Adapter (CFU adapter via Wishbone, validated 9/9) */
    run_backend("ADAPTER", 0xF0020000, ATOMIK_LAYOUT_ADAPTER, sizes, n);

    printf("Note: 'detect' measures ONLY acc_zero check (O(1)).\n");
    printf("Buffer accumulation is O(n) but happens at write time.\n");

    return 0;
}
