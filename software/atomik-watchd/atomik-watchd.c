/*
 * atomik-watchd — ATOMiK State Change Detection Service
 *
 * Monitors N memory regions, detects which changed each tick using
 * ATOMiK hardware (O(1) per region) vs software memcmp baseline.
 * Emits one JSON line per tick for consumption by dashboards, logs,
 * or downstream services.
 *
 * Usage:
 *   atomik-watchd [options]
 *
 * Options:
 *   -n N        Number of regions (default: 8)
 *   -s SIZE     Region size in bytes (default: 4096)
 *   -t TICKS    Number of monitoring ticks (default: 10, 0 = infinite)
 *   -c PCT      Percent of regions mutated per tick (default: 25)
 *   -a ADDR     ATOMiK physical address (default: 0xF0000000)
 *   -l LAYOUT   Backend layout: csr, adapter, axi (default: csr)
 *   -m          Mock mode (no hardware, in-memory simulation)
 *   -h          Print help
 *
 * Output (one JSON line per tick):
 *   {"tick":1,"n_regions":8,"changed":[0,2],"n_changed":2,
 *    "detect_cy":1234,"baseline_cy":56789,"speedup":46.0}
 *
 * Build:
 *   gcc -O2 -I../libatomik -o atomik-watchd atomik-watchd.c ../libatomik/libatomik.c
 *   # Mock: add -DATOMIK_MOCK
 *   # Zynq: use riscv32-buildroot-linux-gnu-gcc -static
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

/* ── Region state ─────────────────────────────────────────────────── */

typedef struct {
    uint8_t  *data;       /* current data */
    uint8_t  *shadow;     /* shadow copy for memcmp baseline */
    uint64_t  fp;         /* XOR fingerprint (ATOMiK reference) */
} region_t;

static region_t *regions;
static int n_regions;
static size_t region_size;
static int change_pct;
static volatile int sink;

/* ── Fingerprint helper ───────────────────────────────────────────── */

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

/* ── Setup ────────────────────────────────────────────────────────── */

static void setup(void)
{
    regions = calloc(n_regions, sizeof(region_t));
    for (int i = 0; i < n_regions; i++) {
        regions[i].data = calloc(1, region_size);
        regions[i].shadow = calloc(1, region_size);

        /* Fill with deterministic pattern */
        for (size_t j = 0; j < region_size; j++)
            regions[i].data[j] = (uint8_t)(i * 37 + j * 13 + 7);
        memcpy(regions[i].shadow, regions[i].data, region_size);
        regions[i].fp = compute_fp(regions[i].data, region_size);
    }
}

static void teardown(void)
{
    for (int i = 0; i < n_regions; i++) {
        free(regions[i].data);
        free(regions[i].shadow);
    }
    free(regions);
}

/* ── Mutate ───────────────────────────────────────────────────────── */

static int mutate(int tick)
{
    int n_change = (n_regions * change_pct + 99) / 100;
    int start = (tick * n_change) % n_regions;
    for (int j = 0; j < n_change && j < n_regions; j++) {
        int i = (start + j) % n_regions;
        size_t pos = (tick * 37 + j * 13) % region_size;
        regions[i].data[pos] ^= 0xFF;
    }
    return n_change;
}

/* ── Detect: software baseline ────────────────────────────────────── */

static int sw_detect(int *changed_out)
{
    int count = 0;
    for (int i = 0; i < n_regions; i++) {
        int c = memcmp(regions[i].data, regions[i].shadow, region_size) != 0;
        changed_out[i] = c;
        if (c) {
            memcpy(regions[i].shadow, regions[i].data, region_size);
            count++;
        }
    }
    return count;
}

/* ── Detect: ATOMiK ───────────────────────────────────────────────── */

static int hw_detect(atomik_t *a, int *changed_out)
{
    int count = 0;
    for (int i = 0; i < n_regions; i++) {
        uint64_t new_fp;
        int c = atomik_detect_changed(a, (uint8_t)i, regions[i].fp,
                                       regions[i].data, region_size, &new_fp);
        changed_out[i] = c;
        if (c) {
            regions[i].fp = new_fp;
            count++;
        }
    }
    return count;
}

/* ── JSON output ──────────────────────────────────────────────────── */

static void emit_json(int tick, int *changed, int n_changed,
                       uint64_t detect_cy, uint64_t baseline_cy)
{
    printf("{\"tick\":%d,\"n_regions\":%d,\"changed\":[", tick, n_regions);
    int first = 1;
    for (int i = 0; i < n_regions; i++) {
        if (changed[i]) {
            if (!first) printf(",");
            printf("%d", i);
            first = 0;
        }
    }
    printf("],\"n_changed\":%d,\"detect_cy\":%llu,\"baseline_cy\":%llu,\"speedup\":%.1f}\n",
           n_changed,
           (unsigned long long)detect_cy,
           (unsigned long long)baseline_cy,
           baseline_cy > 0 ? (double)baseline_cy / detect_cy : 0.0);
    fflush(stdout);
}

/* ── Usage ────────────────────────────────────────────────────────── */

static void usage(const char *prog)
{
    fprintf(stderr,
        "atomik-watchd — ATOMiK State Change Detection Service\n"
        "\n"
        "Usage: %s [options]\n"
        "  -n N       Number of regions (default: 8)\n"
        "  -s SIZE    Region size in bytes (default: 4096)\n"
        "  -t TICKS   Monitoring ticks (default: 10, 0 = infinite)\n"
        "  -c PCT     Percent mutated per tick (default: 25)\n"
        "  -a ADDR    ATOMiK address (default: 0xF0000000)\n"
        "  -l LAYOUT  csr, adapter, axi (default: csr)\n"
        "  -m         Mock mode (no hardware)\n"
        "  -h         Help\n", prog);
}

/* ── Main ─────────────────────────────────────────────────────────── */

int main(int argc, char **argv)
{
    unsigned long addr = 0xF0000000;
    atomik_layout_t layout = ATOMIK_LAYOUT_CSR;
    int n_ticks = 10;
    int mock_mode = 0;
    n_regions = 8;
    region_size = 4096;
    change_pct = 25;

#ifdef ATOMIK_MOCK
    mock_mode = 1;
#endif

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) n_regions = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) region_size = (size_t)atol(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) n_ticks = atoi(argv[++i]);
        else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) change_pct = atoi(argv[++i]);
        else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) addr = strtoul(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "adapter") == 0) layout = ATOMIK_LAYOUT_ADAPTER;
            else if (strcmp(argv[i], "axi") == 0) layout = ATOMIK_LAYOUT_AXI;
            else layout = ATOMIK_LAYOUT_CSR;
        }
        else if (strcmp(argv[i], "-m") == 0) mock_mode = 1;
        else if (strcmp(argv[i], "-h") == 0) { usage(argv[0]); return 0; }
        else { fprintf(stderr, "Unknown option: %s\n", argv[i]); usage(argv[0]); return 1; }
    }

    if (n_regions > 255) { fprintf(stderr, "Max 255 regions\n"); return 1; }
    if (n_regions < 1) { fprintf(stderr, "Need at least 1 region\n"); return 1; }

    /* Open ATOMiK */
    atomik_t *a;
    if (mock_mode) {
        a = atomik_open_devmem(addr, layout);
    } else {
        a = atomik_open_devmem(addr, layout);
    }
    if (!a) {
        fprintf(stderr, "ERROR: cannot open ATOMiK at 0x%lX\n", addr);
        return 1;
    }

    /* Header (stderr, not stdout — stdout is JSON only) */
    fprintf(stderr, "atomik-watchd: %d regions x %zu bytes, %d%% mutation, %d ticks\n",
            n_regions, region_size, change_pct, n_ticks);
    fprintf(stderr, "atomik-watchd: ATOMiK v%u @ 0x%lX (%s)\n",
            a->version, addr,
            layout == ATOMIK_LAYOUT_ADAPTER ? "adapter" :
            layout == ATOMIK_LAYOUT_AXI ? "axi" : "csr");

    setup();

    int *changed_hw = calloc(n_regions, sizeof(int));
    int *changed_sw = calloc(n_regions, sizeof(int));

    for (int tick = 1; n_ticks == 0 || tick <= n_ticks; tick++) {
        mutate(tick);

        /* Software baseline */
        uint64_t t0 = rdticks();
        int sw_n = sw_detect(changed_sw);
        uint64_t t1 = rdticks();
        uint64_t sw_cy = t1 - t0;

        /* ATOMiK detect */
        t0 = rdticks();
        int hw_n = hw_detect(a, changed_hw);
        t1 = rdticks();
        uint64_t hw_cy = t1 - t0;

        /* Verify correctness */
        int match = (sw_n == hw_n);
        for (int i = 0; i < n_regions && match; i++)
            if (changed_sw[i] != changed_hw[i]) match = 0;
        if (!match) {
            fprintf(stderr, "ERROR: tick %d MISMATCH (sw=%d hw=%d)\n", tick, sw_n, hw_n);
        }

        emit_json(tick, changed_hw, hw_n, hw_cy, sw_cy);
    }

    free(changed_hw);
    free(changed_sw);
    teardown();
    atomik_close(a);
    return 0;
}
