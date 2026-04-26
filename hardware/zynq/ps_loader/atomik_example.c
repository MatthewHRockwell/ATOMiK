/*
 * ATOMiK Example — Standard C, Standard GCC, Hardware Acceleration
 *
 * This program detects which buffers changed using ATOMiK hardware.
 * Same C. Same compiler. Different silicon economics.
 *
 * Build:
 *   riscv64-linux-gnu-gcc -O2 -o atomik_example atomik_example.c
 *
 * The ATOMiK API compiles with standard GCC. On this board, it
 * uses the MMIO adapter. On an ATOMiK-native CPU, the same header
 * emits custom RISC-V instructions (opcode 0x0B) — zero code change.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "atomik.h"

#define N_BUFFERS 4
#define BUF_SIZE  512  /* bytes */
#define ADAPTER_ADDR 0xF0020000UL

static uint64_t buffers[N_BUFFERS][BUF_SIZE / 8];
static uint64_t saved_fp[N_BUFFERS];

int main(void) {
    /* Map ATOMiK adapter */
    int fd = open("/dev/mem", 2);
    if (fd < 0) { perror("/dev/mem"); return 1; }
    void *m = mmap(NULL, 4096, 3, 1, fd, ADAPTER_ADDR);
    if (m == MAP_FAILED) { perror("mmap"); return 1; }
    atomik_init_mmio((uint64_t)(uintptr_t)m);

    printf("ATOMiK Change Detection Example\n");
    printf("================================\n\n");

    /* Initialize buffers and save fingerprints */
    for (int i = 0; i < N_BUFFERS; i++) {
        memset(buffers[i], 0xAA + i, BUF_SIZE);
        saved_fp[i] = atomik_fingerprint(i, buffers[i], BUF_SIZE / 8);
        printf("  Buffer %d: fp = 0x%016lx\n", i, saved_fp[i]);
    }

    /* Modify only buffers 1 and 3 */
    printf("\nModifying buffers 1 and 3...\n");
    buffers[1][0] ^= 0xDEADBEEF;
    buffers[3][7] ^= 0xCAFEBABE;

    /* Detect changes — ATOMiK hardware, not memcmp */
    printf("\nChange detection (ATOMiK hardware):\n");
    for (int i = 0; i < N_BUFFERS; i++) {
        int changed = atomik_changed(i, buffers[i], BUF_SIZE / 8, saved_fp[i]);
        printf("  Buffer %d: %s\n", i, changed ? "CHANGED" : "clean");
    }

    printf("\nDone. Only changed buffers need sync.\n");
    return 0;
}
