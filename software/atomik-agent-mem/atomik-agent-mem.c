/*
 * atomik-agent-mem — ATOMiK Agent Memory State Tracking Demo
 *
 * Simulates an agent with multiple state buffers (observation, action,
 * reward, model weights). Each tick, the agent "acts" — mutating some
 * buffers. The monitor reports which buffers changed since last
 * checkpoint, enabling selective backup/logging.
 *
 * Build:
 *   gcc -O2 -I../libatomik -DATOMIK_MOCK -o atomik-agent-mem atomik-agent-mem.c ../libatomik/libatomik.c
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

/* ── Agent buffers ────────────────────────────────────────────────── */

typedef struct {
    const char *name;
    uint8_t    *data;
    size_t      size;
    uint64_t    fp;
    int         atomik_addr;
} buffer_t;

#define N_BUFFERS 5

static buffer_t buffers[N_BUFFERS] = {
    { "observation",  NULL, 8192, 0, 0 },
    { "action",       NULL, 1024, 0, 1 },
    { "reward",       NULL,  256, 0, 2 },
    { "model_weights",NULL,16384, 0, 3 },
    { "hidden_state", NULL, 4096, 0, 4 },
};

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
    for (int i = 0; i < N_BUFFERS; i++) {
        buffers[i].data = calloc(1, buffers[i].size);
        for (size_t j = 0; j < buffers[i].size; j++)
            buffers[i].data[j] = (uint8_t)(i * 41 + j * 17 + 3);
        buffers[i].fp = compute_fp(buffers[i].data, buffers[i].size);
    }
}

static void teardown(void)
{
    for (int i = 0; i < N_BUFFERS; i++)
        free(buffers[i].data);
}

/* ── Agent action: mutates some buffers per tick ──────────────────── */

static void agent_act(int tick)
{
    /*
     * Simulate agent behavior patterns:
     *   - observation changes every tick (sensor input)
     *   - action changes every tick (output)
     *   - reward changes every 2 ticks
     *   - model_weights change every 4 ticks (learning step)
     *   - hidden_state changes every tick
     */
    /* observation: always */
    buffers[0].data[(tick * 7) % buffers[0].size] ^= 0xFF;

    /* action: always */
    buffers[1].data[(tick * 11) % buffers[1].size] ^= 0xFF;

    /* reward: every 2 ticks */
    if (tick % 2 == 0)
        buffers[2].data[(tick * 3) % buffers[2].size] ^= 0xFF;

    /* model_weights: every 4 ticks */
    if (tick % 4 == 0)
        buffers[3].data[(tick * 13) % buffers[3].size] ^= 0xFF;

    /* hidden_state: always */
    buffers[4].data[(tick * 19) % buffers[4].size] ^= 0xFF;
}

/* ── Main ─────────────────────────────────────────────────────────── */

#define N_TICKS 8

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

    printf("ATOMiK Agent Memory State Tracking\n");
    printf("==================================\n\n");
    printf("  Buffers:\n");
    size_t total_bytes = 0;
    for (int i = 0; i < N_BUFFERS; i++) {
        printf("    [%d] %-14s %5zu B\n", i, buffers[i].name, buffers[i].size);
        total_bytes += buffers[i].size;
    }
    printf("  Total tracked state: %zu B (%zu KB)\n", total_bytes, total_bytes / 1024);
    printf("  ATOMiK v%u @ 0x%lX (%s)\n\n",
           a->version, addr,
           layout == ATOMIK_LAYOUT_ADAPTER ? "adapter" : "csr");

    setup();

    printf("  Tick │ Changed Buffers                          │ Checkpoint Cost\n");
    printf("  ─────┼──────────────────────────────────────────┼────────────────\n");

    size_t total_checkpoint_full = 0, total_checkpoint_selective = 0;

    for (int tick = 1; tick <= N_TICKS; tick++) {
        agent_act(tick);

        /* Detect which buffers changed */
        int changed[N_BUFFERS];
        size_t selective_bytes = 0;

        for (int i = 0; i < N_BUFFERS; i++) {
            uint64_t new_fp;
            changed[i] = atomik_detect_changed(a, (uint8_t)buffers[i].atomik_addr,
                                                buffers[i].fp, buffers[i].data,
                                                buffers[i].size, &new_fp);
            if (changed[i]) {
                buffers[i].fp = new_fp;
                selective_bytes += buffers[i].size;
            }
        }

        /* Print changed buffer names */
        printf("  %4d │ ", tick);
        int first = 1;
        for (int i = 0; i < N_BUFFERS; i++) {
            if (changed[i]) {
                if (!first) printf(", ");
                printf("%s", buffers[i].name);
                first = 0;
            }
        }
        /* Pad to column */
        int pos = 0;
        for (int i = 0; i < N_BUFFERS; i++)
            if (changed[i]) pos += strlen(buffers[i].name) + 2;
        for (int p = pos; p < 40; p++) printf(" ");

        printf(" │ %5zu / %5zu B\n", selective_bytes, total_bytes);

        total_checkpoint_full += total_bytes;
        total_checkpoint_selective += selective_bytes;
    }

    double saved = 100.0 * (1.0 - (double)total_checkpoint_selective / total_checkpoint_full);
    printf("\n  Over %d ticks:\n", N_TICKS);
    printf("    Full checkpoint:      %6zu B (%zu KB)\n",
           total_checkpoint_full, total_checkpoint_full / 1024);
    printf("    Selective checkpoint: %6zu B (%zu KB)\n",
           total_checkpoint_selective, total_checkpoint_selective / 1024);
    printf("    Bandwidth saved:      %.1f%%\n", saved);
    printf("\n  ATOMiK identifies which agent buffers changed, enabling\n");
    printf("  selective checkpoint/logging instead of full state dump.\n");

    teardown();
    atomik_close(a);
    return 0;
}
