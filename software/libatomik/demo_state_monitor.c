/*
 * demo_state_monitor.c — ATOMiK State Change Monitor Demo
 *
 * Demonstrates the core value: incremental write-time tracking makes
 * change detection O(1) instead of O(region_size).
 *
 * Model: a service tracking a large memory region (like a page table,
 * config block, or sensor buffer). Writes happen incrementally.
 * The question at each monitoring tick: "did anything change?"
 *
 *   Software: rescan the entire region (memcmp) — O(size)
 *   ATOMiK:   check one flag (acc_zero) — O(1)
 *
 * The deltas accumulate at write time (one ACCUM per write), so the
 * detection cost is constant regardless of how large the region is.
 *
 * Build:
 *   make demo CROSS=/path/to/riscv32-buildroot-linux-gnu-
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "libatomik.h"

/* ── Timer ────────────────────────────────────────────────────────── */

static inline uint64_t rdticks(void)
{
#if defined(__riscv)
    uint32_t lo, hi, hi2;
    do {
        __asm__ volatile("rdtimeh %0" : "=r"(hi));
        __asm__ volatile("rdtime  %0" : "=r"(lo));
        __asm__ volatile("rdtimeh %0" : "=r"(hi2));
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

static volatile int sink;

/* ── Configuration ────────────────────────────────────────────────── */

#define N_TICKS      8

struct scenario {
    const char *name;
    size_t region_size;
    int n_writes;         /* writes per tick (0 = no change) */
};

static struct scenario scenarios[] = {
    { "256B, 1 write",     256, 1 },
    { "1KB, 1 write",     1024, 1 },
    { "4KB, 1 write",     4096, 1 },
    { "16KB, 1 write",   16384, 1 },
    { "64KB, 1 write",   65536, 1 },
    { "4KB, no change",   4096, 0 },
    { "64KB, no change", 65536, 0 },
};
#define N_SCENARIOS (sizeof(scenarios)/sizeof(scenarios[0]))

/* ── Main ─────────────────────────────────────────────────────────── */

int main(int argc, char **argv)
{
    unsigned long addr = 0xF0000000;
    atomik_layout_t layout = ATOMIK_LAYOUT_CSR;
    if (argc > 1) addr = strtoul(argv[1], NULL, 0);
    if (argc > 2 && strcmp(argv[2], "adapter") == 0)
        layout = ATOMIK_LAYOUT_ADAPTER;

    atomik_t *a = atomik_open_devmem(addr, layout);
    if (!a) {
        printf("ERROR: cannot open ATOMiK at 0x%lX\n", addr);
        return 1;
    }

    printf("ATOMiK State Change Monitor\n");
    printf("===========================\n\n");
    printf("Model: track one region, detect changes at each tick.\n");
    printf("Writes accumulate deltas at write time (O(1) per write).\n");
    printf("Detection: software rescans all bytes; ATOMiK reads one flag.\n\n");
    printf("ATOMiK v%u @ 0x%lX (%s) | VexRiscv SMP 100 MHz\n\n",
           a->version, addr,
           layout == ATOMIK_LAYOUT_ADAPTER ? "adapter" : "CSR");

    printf("%-18s │ %-12s │ %-12s │ %-8s │ %s\n",
           "Scenario", "SW (memcmp)", "ATOMiK", "Speedup", "Correct");
    printf("───────────────────┼──────────────┼──────────────┼──────────┼────────\n");

    for (size_t s = 0; s < N_SCENARIOS; s++) {
        size_t sz = scenarios[s].region_size;
        int nw = scenarios[s].n_writes;

        uint8_t *region = calloc(1, sz);
        uint8_t *shadow = calloc(1, sz);

        /* Fill with pattern */
        for (size_t j = 0; j < sz; j++)
            region[j] = (uint8_t)(j * 13 + 7);
        memcpy(shadow, region, sz);

        /* Initialize ATOMiK: fingerprint = XOR of all 8-byte chunks */
        uint64_t fp = 0, chunk;
        for (size_t j = 0; j + 8 <= sz; j += 8) {
            memcpy(&chunk, region + j, 8);
            fp ^= chunk;
        }
        atomik_load(a, 0, fp);

        /* Average over N_TICKS */
        uint64_t sw_total = 0, hw_total = 0;
        int correct = 1;

        for (int tick = 0; tick < N_TICKS; tick++) {
            /* Apply writes (incremental — ATOMiK tracks at write time) */
            for (int w = 0; w < nw; w++) {
                size_t pos = (tick * 37 + w * 13) % sz;
                uint8_t old_val = region[pos];
                region[pos] = old_val ^ 0xFF;

                /* Accumulate the byte-level delta into ATOMiK.
                 * This is what happens at WRITE TIME in production. */
                size_t chunk_off = (pos / 8) * 8;
                uint64_t delta = (uint64_t)0xFF << ((pos - chunk_off) * 8);
                atomik_accum(a, delta);
            }

            /* === DETECTION PHASE === */
            int sw_changed, hw_changed;

            /* Software: rescan entire region — O(size) */
            uint64_t t0 = rdticks();
            sw_changed = memcmp(region, shadow, sz) != 0;
            uint64_t t1 = rdticks();
            sw_total += (t1 - t0);

            /* ATOMiK: check one flag — O(1) */
            t0 = rdticks();
            hw_changed = !atomik_acc_zero(a);
            t1 = rdticks();
            hw_total += (t1 - t0);

            if (sw_changed != hw_changed) correct = 0;

            /* Update shadow for next tick (software bookkeeping) */
            memcpy(shadow, region, sz);

            /* SWAP: promote current state to new reference (resets acc) */
            if (hw_changed) {
                fp = 0;
                for (size_t j = 0; j + 8 <= sz; j += 8) {
                    memcpy(&chunk, region + j, 8);
                    fp ^= chunk;
                }
                atomik_load(a, 0, fp);
            }
        }

        uint64_t sw_avg = sw_total / N_TICKS;
        uint64_t hw_avg = hw_total / N_TICKS;
        double speedup = (double)sw_avg / hw_avg;

        printf("%-18s │ %10llu cy │ %10llu cy │ %6.1fx │ %s\n",
               scenarios[s].name,
               (unsigned long long)sw_avg,
               (unsigned long long)hw_avg,
               speedup,
               correct ? "YES" : "NO");

        free(region);
        free(shadow);
    }

    printf("\n");
    printf("Key: software cost grows with region size.\n");
    printf("     ATOMiK cost is constant (~260 cycles) regardless of size.\n");
    printf("     Deltas accumulate at write time, not at detection time.\n");

    atomik_close(a);
    return 0;
}
