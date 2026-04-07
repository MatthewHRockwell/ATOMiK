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
    uint32_t lo, hi;
    __asm__ volatile("rdcycleh %0" : "=r"(hi));
    __asm__ volatile("rdcycle  %0" : "=r"(lo));
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

/* ── ATOMiK fingerprint-based change detection ────────────────────── */

static int detect_change_atomik(atomik_t *a, const uint8_t *buf, size_t n)
{
    /*
     * Compute XOR fingerprint of buffer, accumulate into ATOMiK.
     * If accumulator is non-zero, buffer changed since last LOAD.
     *
     * This reduces to: for each 8-byte chunk, ACCUM the chunk.
     * After all chunks, check acc_zero.
     *
     * The key insight: this is ALSO O(n) for the accumulation step.
     * But the DETECTION is O(1) — just read acc_zero.
     * In a real system, the accumulation happens incrementally as
     * writes occur (via hardware interception), making detection O(1).
     *
     * For this benchmark, we measure the full scan+accumulate to show
     * that even with the scan, ATOMiK is competitive, and the acc_zero
     * check is trivially fast.
     */
    size_t i;
    uint64_t chunk;

    /* Reset: load the fingerprint of the "known" state */
    atomik_load(a, 0, 0);

    /* Accumulate every 8-byte chunk */
    for (i = 0; i + 8 <= n; i += 8) {
        memcpy(&chunk, buf + i, 8);
        atomik_accum(a, chunk);
    }

    /* Handle tail bytes */
    if (i < n) {
        chunk = 0;
        memcpy(&chunk, buf + i, n - i);
        atomik_accum(a, chunk);
    }

    /* O(1) check: is accumulator non-zero? */
    return !atomik_acc_zero(a);
}

/* ── Benchmark runner ─────────────────────────────────────────────── */

#define ITERATIONS 100

static void run_bench(atomik_t *a, size_t bufsize)
{
    uint8_t *prev, *curr;
    uint64_t t0, t1;
    uint64_t cycles_memcmp = 0, cycles_atomik = 0;
    int i, changed;

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
        curr[j] = prev[j]; /* identical initially */
    }

    /* --- Benchmark: no change (identical buffers) --- */

    /* memcmp */
    t0 = rdcycle();
    for (i = 0; i < ITERATIONS; i++) {
        changed = detect_change_memcmp(prev, curr, bufsize);
        (void)changed;
    }
    t1 = rdcycle();
    cycles_memcmp = (t1 - t0) / ITERATIONS;

    /* ATOMiK (accumulate curr, compare to prev fingerprint) */
    /* First, load the fingerprint of prev */
    {
        uint64_t chunk;
        uint64_t fp = 0;
        for (size_t j = 0; j + 8 <= bufsize; j += 8) {
            memcpy(&chunk, prev + j, 8);
            fp ^= chunk;
        }
        atomik_load(a, 0, fp);
    }

    t0 = rdcycle();
    for (i = 0; i < ITERATIONS; i++) {
        /* Accumulate curr buffer into ATOMiK */
        size_t j;
        uint64_t chunk;
        /* Reset accumulator for this iteration */
        atomik_load(a, 0, 0);
        for (j = 0; j + 8 <= bufsize; j += 8) {
            memcpy(&chunk, curr + j, 8);
            atomik_accum(a, chunk);
        }
        /* Check: acc_zero means fingerprints match (no change) */
        changed = !atomik_acc_zero(a);
        (void)changed;
    }
    t1 = rdcycle();
    cycles_atomik = (t1 - t0) / ITERATIONS;

    printf("  %6zu bytes | memcmp: %8llu cy | atomik: %8llu cy | ratio: %.2fx\n",
           bufsize, (unsigned long long)cycles_memcmp,
           (unsigned long long)cycles_atomik,
           cycles_memcmp > 0 ? (double)cycles_atomik / cycles_memcmp : 0.0);

    /* --- Now change 1 byte at the END and re-bench --- */
    curr[bufsize - 1] ^= 0xFF;

    t0 = rdcycle();
    for (i = 0; i < ITERATIONS; i++) {
        changed = detect_change_memcmp(prev, curr, bufsize);
        (void)changed;
    }
    t1 = rdcycle();
    cycles_memcmp = (t1 - t0) / ITERATIONS;

    t0 = rdcycle();
    for (i = 0; i < ITERATIONS; i++) {
        size_t j;
        uint64_t chunk;
        atomik_load(a, 0, 0);
        for (j = 0; j + 8 <= bufsize; j += 8) {
            memcpy(&chunk, curr + j, 8);
            atomik_accum(a, chunk);
        }
        changed = !atomik_acc_zero(a);
        (void)changed;
    }
    t1 = rdcycle();
    cycles_atomik = (t1 - t0) / ITERATIONS;

    printf("  %6zu +1chg | memcmp: %8llu cy | atomik: %8llu cy | ratio: %.2fx\n",
           bufsize, (unsigned long long)cycles_memcmp,
           (unsigned long long)cycles_atomik,
           cycles_memcmp > 0 ? (double)cycles_atomik / cycles_memcmp : 0.0);

    free(prev);
    free(curr);
}

/* ── Main ─────────────────────────────────────────────────────────── */

int main(void)
{
    atomik_t *a;

    printf("ATOMiK Change Detection Benchmark\n");
    printf("==================================\n\n");

    a = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);
    if (!a) {
        printf("ERROR: cannot open ATOMiK at 0xF0000000\n");
        printf("  Ensure: mknod /dev/mem c 1 1; chmod 666 /dev/mem\n");
        return 1;
    }

    printf("ATOMiK v%u, %u bank(s)\n\n", a->version, a->n_banks);

    printf("%-14s | %-22s | %-22s | %s\n",
           "Buffer", "Software (memcmp)", "ATOMiK (hw fingerprint)", "Ratio");
    printf("%.14s-+-%.22s-+-%.22s-+-%s\n",
           "--------------", "----------------------",
           "----------------------", "--------");

    size_t sizes[] = { 64, 256, 1024, 4096, 16384, 65536 };
    for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
        run_bench(a, sizes[s]);
    }

    printf("\nNote: ATOMiK scan+accumulate is O(n) like memcmp in this benchmark.\n");
    printf("In production, deltas accumulate incrementally at write time,\n");
    printf("making change DETECTION O(1) — just read the acc_zero flag.\n");

    atomik_close(a);
    return 0;
}
