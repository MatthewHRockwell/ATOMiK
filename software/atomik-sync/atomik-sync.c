/*
 * atomik-sync — ATOMiK Incremental Sync Demo
 *
 * Demonstrates selective synchronization: only transfer regions that
 * changed, instead of copying everything. Uses atomik_detect_changed()
 * to identify dirty regions, then "syncs" only those.
 *
 * Two sync strategies compared:
 *   - Full sync:  copy all N regions every cycle — O(N * region_size)
 *   - ATOMiK sync: detect changed regions, copy only those — O(N * region_size)
 *                  for fingerprint scan, but transfers only dirty regions
 *
 * The key metric is bytes transferred, not detection time: ATOMiK sync
 * skips unchanged regions, reducing transfer volume proportionally to
 * the change rate.
 *
 * Build:
 *   gcc -O2 -I../libatomik -DATOMIK_MOCK -o atomik-sync atomik-sync.c ../libatomik/libatomik.c
 *
 * ATOMiK Project — April 2026
 * SPDX-License-Identifier: MIT
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

#if defined(__riscv)
#define TIMER_UNIT "ticks"
#else
#define TIMER_UNIT "ns"
#endif

/* ── Config ───────────────────────────────────────────────────────── */

#define N_REGIONS    16
#define REGION_SIZE  4096
#define N_CYCLES     8
#define CHANGE_PCT   20

/* ── State ────────────────────────────────────────────────────────── */

typedef struct {
    uint8_t  *data;
    uint8_t  *dest;      /* "remote" copy (sync target) */
    uint64_t  fp;        /* XOR fingerprint reference */
} region_t;

static region_t regions[N_REGIONS];

static uint64_t compute_fp(const uint8_t *data, size_t len)
{
    uint64_t fp = 0;
    size_t i;
    for (i = 0; i + 8 <= len; i += 8) {
        uint64_t chunk;
        memcpy(&chunk, data + i, 8);
        fp ^= chunk;
    }
    if (i < len) {
        uint64_t tail = 0;
        memcpy(&tail, data + i, len - i);
        fp ^= tail;
    }
    return fp;
}

static void setup(void)
{
    for (int i = 0; i < N_REGIONS; i++) {
        regions[i].data = calloc(1, REGION_SIZE);
        regions[i].dest = calloc(1, REGION_SIZE);
        for (size_t j = 0; j < REGION_SIZE; j++)
            regions[i].data[j] = (uint8_t)(i * 37 + j * 13 + 7);
        memcpy(regions[i].dest, regions[i].data, REGION_SIZE);
        regions[i].fp = compute_fp(regions[i].data, REGION_SIZE);
    }
}

static void teardown(void)
{
    for (int i = 0; i < N_REGIONS; i++) {
        free(regions[i].data);
        free(regions[i].dest);
    }
}

static void mutate(int cycle)
{
    int n = (N_REGIONS * CHANGE_PCT + 99) / 100;
    int start = (cycle * n) % N_REGIONS;
    for (int j = 0; j < n && j < N_REGIONS; j++) {
        int i = (start + j) % N_REGIONS;
        size_t pos = (cycle * 37 + j * 13) % REGION_SIZE;
        regions[i].data[pos] ^= 0xFF;
    }
}

/* ── Sync strategies ──────────────────────────────────────────────── */

static size_t full_sync(void)
{
    size_t bytes = 0;
    for (int i = 0; i < N_REGIONS; i++) {
        memcpy(regions[i].dest, regions[i].data, REGION_SIZE);
        bytes += REGION_SIZE;
    }
    return bytes;
}

static size_t atomik_sync(atomik_t *a, int *sync_list, int *n_synced)
{
    size_t bytes = 0;
    *n_synced = 0;
    for (int i = 0; i < N_REGIONS; i++) {
        uint64_t new_fp;
        int changed = atomik_detect_changed(a, (uint8_t)i, regions[i].fp,
                                             regions[i].data, REGION_SIZE, &new_fp);
        if (changed) {
            memcpy(regions[i].dest, regions[i].data, REGION_SIZE);
            regions[i].fp = new_fp;
            sync_list[*n_synced] = i;
            (*n_synced)++;
            bytes += REGION_SIZE;
        }
    }
    return bytes;
}

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
        fprintf(stderr, "ERROR: cannot open ATOMiK at 0x%lX\n", addr);
        return 1;
    }

    printf("ATOMiK Incremental Sync Demo\n");
    printf("============================\n\n");
    printf("  %d regions x %d bytes = %d KB tracked state\n",
           N_REGIONS, REGION_SIZE, N_REGIONS * REGION_SIZE / 1024);
    printf("  %d%% mutation per cycle, %d cycles\n", CHANGE_PCT, N_CYCLES);
    printf("  ATOMiK v%u @ 0x%lX (%s)\n\n",
           a->version, addr,
           layout == ATOMIK_LAYOUT_ADAPTER ? "adapter" : "csr");

    printf("  Cycle │ Full Sync      │ ATOMiK Sync    │ Saved    │ Synced\n");
    printf("  ──────┼────────────────┼────────────────┼──────────┼───────\n");

    setup();

    size_t total_full = 0, total_atomik = 0;

    for (int cycle = 1; cycle <= N_CYCLES; cycle++) {
        mutate(cycle);

        /* Full sync: copy everything */
        uint64_t t0 = rdticks();
        size_t full_bytes = full_sync();
        uint64_t t1 = rdticks();
        uint64_t full_t = t1 - t0;

        /* ATOMiK sync: copy only changed */
        int sync_list[N_REGIONS];
        int n_synced = 0;
        t0 = rdticks();
        size_t atomik_bytes = atomik_sync(a, sync_list, &n_synced);
        t1 = rdticks();
        uint64_t atomik_t_val = t1 - t0;

        double saved_pct = 100.0 * (1.0 - (double)atomik_bytes / full_bytes);

        printf("  %5d │ %6zu B %5llu %s │ %6zu B %5llu %s │ %5.1f%%  │ [",
               cycle,
               full_bytes, (unsigned long long)full_t, TIMER_UNIT,
               atomik_bytes, (unsigned long long)atomik_t_val, TIMER_UNIT,
               saved_pct);
        for (int i = 0; i < n_synced; i++) {
            if (i > 0) printf(",");
            printf("%d", sync_list[i]);
        }
        printf("]\n");

        total_full += full_bytes;
        total_atomik += atomik_bytes;
    }

    double overall_saved = 100.0 * (1.0 - (double)total_atomik / total_full);
    printf("\n  Total: full=%zu B, atomik=%zu B, saved=%.1f%%\n",
           total_full, total_atomik, overall_saved);
    printf("  Full sync copies all %d regions every cycle.\n", N_REGIONS);
    printf("  ATOMiK sync copies only changed regions, skipping the rest.\n");

    teardown();
    atomik_close(a);
    return 0;
}
