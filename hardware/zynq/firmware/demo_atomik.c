/*
 * demo_atomik.c — ATOMiK Co-Processor Demo Firmware
 *
 * Runs on the RV64I co-processor in Zynq PL. Exercises ATOMiK custom
 * instructions and communicates results to ARM via mailbox.
 *
 * Command protocol (ARM sends CMD, co-processor responds):
 *   CMD=0x01: Self-test — run ATOMiK verification suite, RESP0=pass count
 *   CMD=0x02: ACCUM workload — process ARG0 deltas at address ARG1
 *   CMD=0x03: Read state — RESP0=state_lo, RESP1=state_hi
 *   CMD=0x04: Benchmark — ARG0=iterations, RESP0=cycles (from mcycle CSR)
 *   CMD=0xFF: Shutdown — halt co-processor
 *
 * Build:
 *   make -C hardware/zynq/firmware
 *
 * ATOMiK Project — March 2026
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include "atomik_v3.h"
#include "mailbox.h"

/* Command codes */
#define CMD_SELFTEST    0x01
#define CMD_ACCUM       0x02
#define CMD_READ_STATE  0x03
#define CMD_BENCHMARK   0x04
#define CMD_SHUTDOWN    0xFF

/* Read cycle counter */
static inline uint64_t read_mcycle(void) {
    uint64_t val;
    __asm__ volatile ("csrr %0, mcycle" : "=r"(val));
    return val;
}

/*
 * ATOMiK self-test suite.
 * Returns number of tests passed (out of 8).
 */
static uint32_t selftest(void)
{
    uint32_t pass = 0;
    uint64_t state;

    /* Test 1: LOAD + READ */
    atomik_load(0, 0xDEADBEEFCAFEBABEULL);
    state = atomik_read(0);
    if (state == 0xDEADBEEFCAFEBABEULL) pass++;

    /* Test 2: ACCUM + READ */
    atomik_load(1, 0xAA00AA00AA00AA00ULL);
    atomik_accum(0x00FF00FF00FF00FFULL);
    state = atomik_read(1);
    if (state == (0xAA00AA00AA00AA00ULL ^ 0x00FF00FF00FF00FFULL)) pass++;

    /* Test 3: XOR self-inverse */
    atomik_load(2, 0x1234567890ABCDEFULL);
    atomik_accum(0xFFFFFFFFFFFFFFFFULL);
    atomik_accum(0xFFFFFFFFFFFFFFFFULL);
    state = atomik_read(2);
    if (state == 0x1234567890ABCDEFULL) pass++;

    /* Test 4: Identity (zero delta) */
    atomik_load(3, 0x42);
    atomik_accum(0);
    state = atomik_read(3);
    if (state == 0x42) pass++;

    /* Test 5: Multi-address isolation */
    atomik_load(10, 0xAAAAAAAAAAAAAAAAULL);
    atomik_load(20, 0x5555555555555555ULL);
    atomik_accum(0xFF);  /* Affects active addr (20) only */
    state = atomik_read(10);
    /* Reading addr 10 changes active_addr, so we need to check */
    if (state == 0xAAAAAAAAAAAAAAAAULL) pass++;

    /* Test 6: SWAP */
    atomik_load(5, 0xAA);
    atomik_accum(0xFF);
    /* state = 0xAA ^ 0xFF = 0x55 */
    state = atomik_swap(5);
    if (state == 0x55) pass++;

    /* Test 7: READ after SWAP (ref updated, acc cleared) */
    state = atomik_read(5);
    if (state == 0x55) pass++;  /* new ref = 0x55, acc = 0 → state = 0x55 */

    /* Test 8: ACCUM after SWAP */
    atomik_accum(0x11);
    state = atomik_read(5);
    if (state == (0x55 ^ 0x11)) pass++;  /* 0x44 */

    return pass;
}

/*
 * ACCUM workload: accumulate N deltas into a slot.
 * delta pattern: 0x01, 0x02, 0x04, ... (bit walk)
 */
static void accum_workload(uint32_t n_deltas, uint32_t addr)
{
    uint64_t delta = 1;

    atomik_load(addr, 0);

    for (uint32_t i = 0; i < n_deltas; i++) {
        atomik_accum(delta);
        delta = (delta << 1) | (delta >> 63);  /* Rotate left */
    }
}

int main(void)
{
    uint32_t cmd, arg0, arg1;
    uint64_t state, t0, t1;
    uint32_t result;

    /* Signal boot complete — write magic to RESP0 */
    mbox_send_resp(0xB007ED, 0);

    /* Main command loop */
    for (;;) {
        cmd = mbox_wait_cmd();
        arg0 = mbox_read(MBOX_ARG0);
        arg1 = mbox_read(MBOX_ARG1);

        switch (cmd) {
        case CMD_SELFTEST:
            result = selftest();
            mbox_send_resp(result, 8);  /* RESP0=pass, RESP1=total */
            break;

        case CMD_ACCUM:
            accum_workload(arg0, (uint32_t)(arg1 & 0xFF));
            state = atomik_read(arg1 & 0xFF);
            mbox_send_resp((uint32_t)state, (uint32_t)(state >> 32));
            break;

        case CMD_READ_STATE:
            state = atomik_read(arg0 & 0xFF);
            mbox_send_resp((uint32_t)state, (uint32_t)(state >> 32));
            break;

        case CMD_BENCHMARK:
            t0 = read_mcycle();
            accum_workload(arg0, 0);
            t1 = read_mcycle();
            mbox_send_resp((uint32_t)(t1 - t0), arg0);
            break;

        case CMD_SHUTDOWN:
            mbox_send_resp(0xDEAD, 0);
            return 0;  /* Returns to crt, writes retval to RESP0, halts */

        default:
            mbox_send_resp(0xBADC0DE, cmd);
            break;
        }
    }
}
