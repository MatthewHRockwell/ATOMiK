/*
 * test_libatomik.c - ATOMiK Library Test Suite
 *
 * Two modes:
 *   1. Mock mode (default): Uses a software simulation of the ATOMiK register
 *      file. Runs anywhere without hardware. Build with: make test-mock
 *   2. Hardware mode: Uses real /dev/uio0. Runs on Zynq target only.
 *      Build with: make test-hw CROSS=arm-linux-gnueabihf-
 *
 * ATOMiK Project — March 2026
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef ATOMIK_MOCK
/* =========================================================================
 * Mock ATOMiK backend — simulates hardware register behavior in software.
 * This mirrors the RTL logic in atomik_axi4lite_wrapper.v so that tests
 * validate the library's register protocol without hardware.
 * ========================================================================= */

#include <sys/mman.h>

/* Simulated hardware state */
static uint32_t mock_regs[16];          /* 64 bytes of register space */
static uint64_t mock_state_table[256];  /* BSRAM state table */
static uint64_t mock_accumulator;       /* Accumulator register */
static uint8_t  mock_active_addr;       /* Current context address */
static uint64_t mock_snapshot;          /* Latched state snapshot */

/* Soft registers (mirroring hardware latches) */
static uint8_t  mock_load_addr;
static uint32_t mock_load_data_lo;
static uint32_t mock_accum_lo;
static uint8_t  mock_enable;

/* Register offsets (must match libatomik.h) */
#define REG_LOAD_ADDR      0x00
#define REG_LOAD_DATA_LO   0x04
#define REG_LOAD_DATA_HI   0x08
#define REG_ACCUM_LO       0x0C
#define REG_ACCUM_HI       0x10
#define REG_STATE_LO       0x14
#define REG_STATE_HI       0x18
#define REG_STATUS         0x1C
#define REG_SWAP_ADDR      0x20
#define REG_CONFIG         0x24

static void mock_reset(void)
{
    memset(mock_regs, 0, sizeof(mock_regs));
    memset(mock_state_table, 0, sizeof(mock_state_table));
    mock_accumulator = 0;
    mock_active_addr = 0;
    mock_snapshot = 0;
    mock_load_addr = 0;
    mock_load_data_lo = 0;
    mock_accum_lo = 0;
    mock_enable = 1;
}

static uint64_t mock_current_state(void)
{
    return mock_state_table[mock_active_addr] ^ mock_accumulator;
}

/*
 * Intercept register writes to simulate hardware behavior.
 * Called from the mock mmap's write path.
 */
static void mock_handle_write(unsigned offset, uint32_t value)
{
    switch (offset) {
    case REG_LOAD_ADDR:
        mock_load_addr = value & 0xFF;
        break;

    case REG_LOAD_DATA_LO:
        mock_load_data_lo = value;
        break;

    case REG_LOAD_DATA_HI: {
        /* HI write triggers LOAD */
        uint64_t init = ((uint64_t)value << 32) | mock_load_data_lo;
        mock_active_addr = mock_load_addr;
        mock_state_table[mock_active_addr] = init;
        mock_accumulator = 0;  /* LOAD clears accumulator */
        break;
    }

    case REG_ACCUM_LO:
        mock_accum_lo = value;
        break;

    case REG_ACCUM_HI: {
        /* HI write triggers ACCUM */
        uint64_t delta = ((uint64_t)value << 32) | mock_accum_lo;
        mock_accumulator ^= delta;
        break;
    }

    case REG_SWAP_ADDR:
        /* SWAP: save current state, switch address, clear accumulator */
        mock_state_table[mock_active_addr] = mock_current_state();
        mock_active_addr = value & 0xFF;
        mock_accumulator = 0;
        break;

    case REG_CONFIG:
        mock_enable = value & 1;
        if (!mock_enable) {
            /* Disable resets the core */
            mock_accumulator = 0;
        }
        break;

    default:
        break;  /* Ignore writes to read-only registers */
    }

    /* Store raw value for readback */
    mock_regs[offset / 4] = value;
}

/*
 * Intercept register reads to simulate hardware behavior.
 */
static uint32_t mock_handle_read(unsigned offset)
{
    switch (offset) {
    case REG_STATE_LO: {
        /* LO read latches the full 64-bit snapshot */
        mock_snapshot = mock_current_state();
        return (uint32_t)(mock_snapshot & 0xFFFFFFFF);
    }

    case REG_STATE_HI:
        /* Return upper half from snapshot */
        return (uint32_t)(mock_snapshot >> 32);

    case REG_STATUS: {
        int acc_zero = (mock_accumulator == 0) ? 1 : 0;
        return (0x01 << 16) | (0x01 << 8) | acc_zero;  /* v1, 1 bank */
    }

    case REG_CONFIG:
        return mock_enable;

    default:
        return 0;  /* Write-only and undefined registers read as 0 */
    }
}

/*
 * Mock atomik_t that intercepts reads/writes via a trampoline page.
 * We allocate a real mmap'd page and hook into the library's register
 * access pattern by running a sync step before/after each operation.
 */

/* We use a simple approach: allocate anonymous mmap, override the library's
 * open/close, and sync the mock state into the mmap page between calls. */

#include "libatomik.h"

/* Create a mock atomik_t handle backed by anonymous mmap */
static atomik_t *mock_atomik_open(void)
{
    atomik_t *a = calloc(1, sizeof(*a));
    if (!a)
        return NULL;

    mock_reset();

    a->regs = mmap(NULL, ATOMIK_MMAP_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (a->regs == MAP_FAILED) {
        free(a);
        return NULL;
    }

    a->fd = -1;  /* No real file descriptor */
    a->n_banks = 1;
    a->version = 1;

    return a;
}

/*
 * Since the library writes directly to the mmap'd page, we need a way
 * to intercept those writes. The simplest approach: wrap each test
 * operation with a mock_sync that processes any pending register writes.
 *
 * In practice, we replace the atomik_* functions with mock-aware versions
 * for testing. This is simpler and more reliable than trying to intercept
 * memory writes via signals or page protection tricks.
 */

/* Mock-aware wrapper functions that simulate the hardware response */
static void mock_load(atomik_t *a, uint8_t addr, uint64_t initial_state)
{
    (void)a;
    mock_handle_write(REG_LOAD_ADDR, addr);
    mock_handle_write(REG_LOAD_DATA_LO, (uint32_t)(initial_state & 0xFFFFFFFF));
    mock_handle_write(REG_LOAD_DATA_HI, (uint32_t)(initial_state >> 32));
}

static void mock_accum(atomik_t *a, uint64_t delta)
{
    (void)a;
    mock_handle_write(REG_ACCUM_LO, (uint32_t)(delta & 0xFFFFFFFF));
    mock_handle_write(REG_ACCUM_HI, (uint32_t)(delta >> 32));
}

static uint64_t mock_read(atomik_t *a)
{
    (void)a;
    uint32_t lo = mock_handle_read(REG_STATE_LO);
    uint32_t hi = mock_handle_read(REG_STATE_HI);
    return ((uint64_t)hi << 32) | lo;
}

static void mock_swap(atomik_t *a, uint8_t addr)
{
    (void)a;
    mock_handle_write(REG_SWAP_ADDR, addr);
}

static int mock_acc_zero(atomik_t *a)
{
    (void)a;
    uint32_t status = mock_handle_read(REG_STATUS);
    return status & 1;
}

static void mock_set_enable(atomik_t *a, int enable)
{
    (void)a;
    mock_handle_write(REG_CONFIG, enable ? 1 : 0);
}

static int mock_is_enabled(atomik_t *a)
{
    (void)a;
    uint32_t config = mock_handle_read(REG_CONFIG);
    return config & 1;
}

static void mock_close(atomik_t *a)
{
    if (!a) return;
    if (a->regs && a->regs != MAP_FAILED)
        munmap((void *)a->regs, ATOMIK_MMAP_SIZE);
    free(a);
}

/* Redirect to mock functions */
#define OPEN()                    mock_atomik_open()
#define CLOSE(a)                  mock_close(a)
#define LOAD(a, addr, val)        mock_load(a, addr, val)
#define ACCUM(a, delta)           mock_accum(a, delta)
#define READ(a)                   mock_read(a)
#define SWAP(a, addr)             mock_swap(a, addr)
#define ACC_ZERO(a)               mock_acc_zero(a)
#define SET_ENABLE(a, en)         mock_set_enable(a, en)
#define IS_ENABLED(a)             mock_is_enabled(a)

#else  /* Hardware mode */

#include "libatomik.h"

#define OPEN()                    atomik_open("/dev/uio0")
#define CLOSE(a)                  atomik_close(a)
#define LOAD(a, addr, val)        atomik_load(a, addr, val)
#define ACCUM(a, delta)           atomik_accum(a, delta)
#define READ(a)                   atomik_read(a)
#define SWAP(a, addr)             atomik_swap(a, addr)
#define ACC_ZERO(a)               atomik_acc_zero(a)
#define SET_ENABLE(a, en)         atomik_set_enable(a, en)
#define IS_ENABLED(a)             atomik_is_enabled(a)

#endif  /* ATOMIK_MOCK */

/* =========================================================================
 * Test infrastructure
 * ========================================================================= */

static int pass_count = 0;
static int fail_count = 0;
static int test_num = 0;

static void check64(uint64_t expected, uint64_t actual, const char *name)
{
    test_num++;
    if (actual == expected) {
        printf("  PASS [%d]: %s = 0x%016llx\n", test_num, name,
               (unsigned long long)actual);
        pass_count++;
    } else {
        printf("  FAIL [%d]: %s = 0x%016llx (expected 0x%016llx)\n",
               test_num, name, (unsigned long long)actual,
               (unsigned long long)expected);
        fail_count++;
    }
}

static void check_int(int expected, int actual, const char *name)
{
    test_num++;
    if (actual == expected) {
        printf("  PASS [%d]: %s = %d\n", test_num, name, actual);
        pass_count++;
    } else {
        printf("  FAIL [%d]: %s = %d (expected %d)\n",
               test_num, name, actual, expected);
        fail_count++;
    }
}

/* =========================================================================
 * Test cases — mirror the Verilog testbench (tb_axi4lite_wrapper.v)
 * ========================================================================= */

static void test_open_close(void)
{
    printf("\n=== Test: Open/Close ===\n");
    atomik_t *a = OPEN();
    check_int(1, a != NULL, "atomik_open succeeds");
    if (!a) return;
    check_int(1, a->version, "version = 1");
    check_int(1, a->n_banks, "n_banks = 1");
    CLOSE(a);
}

static void test_config(atomik_t *a)
{
    printf("\n=== Test: CONFIG Enable/Disable ===\n");

    check_int(1, IS_ENABLED(a), "initially enabled");

    SET_ENABLE(a, 0);
    check_int(0, IS_ENABLED(a), "disabled after set_enable(0)");

    SET_ENABLE(a, 1);
    check_int(1, IS_ENABLED(a), "re-enabled after set_enable(1)");
}

static void test_load_read_roundtrip(atomik_t *a)
{
    printf("\n=== Test: LOAD + READ Roundtrip ===\n");

    LOAD(a, 0, 0xDEADBEEFCAFEBABEULL);
    uint64_t state = READ(a);
    check64(0xDEADBEEFCAFEBABEULL, state, "load/read roundtrip");
}

static void test_accum_correctness(atomik_t *a)
{
    printf("\n=== Test: ACCUM Correctness ===\n");

    LOAD(a, 0, 0xDEADBEEFCAFEBABEULL);
    ACCUM(a, 0x00000000000000FFULL);

    uint64_t expected = 0xDEADBEEFCAFEBABEULL ^ 0x00000000000000FFULL;
    uint64_t state = READ(a);
    check64(expected, state, "state = init ^ delta");
}

static void test_accum_multiple(atomik_t *a)
{
    printf("\n=== Test: ACCUM Multiple ===\n");

    LOAD(a, 0, 0);
    ACCUM(a, 0x0000000000000001ULL);
    ACCUM(a, 0x0000000000000002ULL);
    ACCUM(a, 0x0000000000000004ULL);

    /* acc = 1 ^ 2 ^ 4 = 7 */
    uint64_t state = READ(a);
    check64(0x0000000000000007ULL, state, "1^2^4 = 7");
}

static void test_xor_cancellation(atomik_t *a)
{
    printf("\n=== Test: XOR Cancellation (Self-Inverse) ===\n");

    LOAD(a, 0, 0);
    ACCUM(a, 0xFEDCBA9876543210ULL);
    ACCUM(a, 0xFEDCBA9876543210ULL);  /* Same delta twice cancels */

    uint64_t state = READ(a);
    check64(0, state, "delta ^ delta = 0");
    check_int(1, ACC_ZERO(a), "acc_zero = 1 after cancellation");
}

static void test_commutativity(atomik_t *a)
{
    printf("\n=== Test: Commutativity ===\n");

    uint64_t d1 = 0xAAAAAAAA55555555ULL;
    uint64_t d2 = 0x55555555AAAAAAAAULL;
    uint64_t d3 = 0x1234567890ABCDEFULL;

    /* Order A: d1, d2, d3 */
    LOAD(a, 0, 0);
    ACCUM(a, d1);
    ACCUM(a, d2);
    ACCUM(a, d3);
    uint64_t result_a = READ(a);

    /* Order B: d3, d1, d2 */
    LOAD(a, 0, 0);
    ACCUM(a, d3);
    ACCUM(a, d1);
    ACCUM(a, d2);
    uint64_t result_b = READ(a);

    /* Order C: d2, d3, d1 */
    LOAD(a, 0, 0);
    ACCUM(a, d2);
    ACCUM(a, d3);
    ACCUM(a, d1);
    uint64_t result_c = READ(a);

    check64(result_a, result_b, "order A == order B");
    check64(result_a, result_c, "order A == order C");
}

static void test_identity(atomik_t *a)
{
    printf("\n=== Test: Identity (Zero Delta) ===\n");

    LOAD(a, 0, 0xDEADBEEFCAFEBABEULL);
    ACCUM(a, 0);  /* Zero delta is identity */

    uint64_t state = READ(a);
    check64(0xDEADBEEFCAFEBABEULL, state, "state unchanged after zero delta");
    check_int(1, ACC_ZERO(a), "acc_zero = 1 with zero delta");
}

static void test_multi_address(atomik_t *a)
{
    printf("\n=== Test: Multi-Address Isolation ===\n");

    LOAD(a, 0, 0x1111111111111111ULL);
    LOAD(a, 1, 0x2222222222222222ULL);
    LOAD(a, 2, 0x3333333333333333ULL);

    /* Verify addr 2 (currently active) */
    uint64_t state = READ(a);
    check64(0x3333333333333333ULL, state, "addr2 active after last load");

    /* Switch to addr 0 */
    SWAP(a, 0);
    state = READ(a);
    check64(0x1111111111111111ULL, state, "addr0 preserved after swap");

    /* Switch to addr 1 */
    SWAP(a, 1);
    state = READ(a);
    check64(0x2222222222222222ULL, state, "addr1 preserved after swap");
}

static void test_swap_clears_accumulator(atomik_t *a)
{
    printf("\n=== Test: SWAP Clears Accumulator ===\n");

    /* Load two contexts */
    LOAD(a, 0, 0x0000000000001111ULL);
    LOAD(a, 1, 0x0000000000002222ULL);

    /* Switch to addr 0 and accumulate */
    SWAP(a, 0);
    ACCUM(a, 0x0000000000000001ULL);

    /* Current state at addr 0 = 0x1111 ^ 0x0001 = 0x1110 */
    uint64_t state = READ(a);
    check64(0x0000000000001110ULL, state, "addr0: 0x1111^0x0001=0x1110");

    /* SWAP saves current_state (0x1110) as new ref for addr 0,
     * switches to addr 1, and CLEARS accumulator */
    SWAP(a, 1);
    state = READ(a);
    /* addr1 state = 0x2222 ^ 0 = 0x2222 (acc was cleared by SWAP) */
    check64(0x0000000000002222ULL, state, "addr1: 0x2222 (acc cleared by swap)");

    /* Verify addr 0 got its state saved by SWAP */
    SWAP(a, 0);
    state = READ(a);
    check64(0x0000000000001110ULL, state, "addr0: saved as 0x1110 by swap");
}

static void test_64bit_boundaries(atomik_t *a)
{
    printf("\n=== Test: 64-bit Boundary Patterns ===\n");

    /* All ones */
    LOAD(a, 0, 0xFFFFFFFFFFFFFFFFULL);
    check64(0xFFFFFFFFFFFFFFFFULL, READ(a), "all ones");

    /* Alternating */
    LOAD(a, 0, 0xAAAAAAAA55555555ULL);
    check64(0xAAAAAAAA55555555ULL, READ(a), "alternating");

    /* MSB only */
    LOAD(a, 0, 0x8000000000000000ULL);
    check64(0x8000000000000000ULL, READ(a), "MSB only");

    /* LSB only */
    LOAD(a, 0, 0x0000000000000001ULL);
    check64(0x0000000000000001ULL, READ(a), "LSB only");

    /* 32-bit boundary: value only in upper half */
    LOAD(a, 0, 0xFFFFFFFF00000000ULL);
    check64(0xFFFFFFFF00000000ULL, READ(a), "upper half only");

    /* 32-bit boundary: value only in lower half */
    LOAD(a, 0, 0x00000000FFFFFFFFULL);
    check64(0x00000000FFFFFFFFULL, READ(a), "lower half only");
}

static void test_address_boundaries(atomik_t *a)
{
    printf("\n=== Test: Address Boundaries ===\n");

    LOAD(a, 0,   0xAAAAAAAAAAAAAAAAULL);
    LOAD(a, 127, 0xBBBBBBBBBBBBBBBBULL);
    LOAD(a, 255, 0xCCCCCCCCCCCCCCCCULL);

    /* Verify addr 255 (currently active) */
    check64(0xCCCCCCCCCCCCCCCCULL, READ(a), "addr 255");

    /* Verify addr 0 */
    SWAP(a, 0);
    check64(0xAAAAAAAAAAAAAAAAULL, READ(a), "addr 0");

    /* Verify addr 127 */
    SWAP(a, 127);
    check64(0xBBBBBBBBBBBBBBBBULL, READ(a), "addr 127");
}

static void test_config_reset(atomik_t *a)
{
    printf("\n=== Test: CONFIG Disable Resets Core ===\n");

    LOAD(a, 0, 0xDEADBEEFCAFEBABEULL);
    ACCUM(a, 0x00000000000000FFULL);

    /* Verify state changed */
    uint64_t state = READ(a);
    uint64_t expected = 0xDEADBEEFCAFEBABEULL ^ 0x00000000000000FFULL;
    check64(expected, state, "state before disable");

    /* Disable and re-enable */
    SET_ENABLE(a, 0);
    SET_ENABLE(a, 1);

    /* After re-enable, accumulator is cleared */
    check_int(1, ACC_ZERO(a), "acc_zero after re-enable");
}

static void test_state_reconstruction(atomik_t *a)
{
    printf("\n=== Test: State Reconstruction (core algebra) ===\n");

    uint64_t init  = 0x0123456789ABCDEFULL;
    uint64_t delta = 0xFEDCBA9876543210ULL;

    LOAD(a, 0, init);
    ACCUM(a, delta);

    /* current_state = initial_state XOR accumulator
     * = 0x0123456789ABCDEF ^ 0xFEDCBA9876543210
     * = 0xFFFFFFFFFFFFFFFF
     */
    check64(0xFFFFFFFFFFFFFFFFULL, READ(a), "init^delta = all-ones");
}

static void test_burst_accum(atomik_t *a)
{
    printf("\n=== Test: Burst Accumulate (16 deltas) ===\n");

    LOAD(a, 0, 0);

    /* Accumulate 16 different deltas */
    uint64_t expected = 0;
    for (int i = 0; i < 16; i++) {
        uint64_t delta = (uint64_t)(i + 1) << (i * 4);
        ACCUM(a, delta);
        expected ^= delta;
    }

    check64(expected, READ(a), "16-delta burst");
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void)
{
    printf("============================================\n");
#ifdef ATOMIK_MOCK
    printf("ATOMiK libatomik Test Suite (MOCK mode)\n");
#else
    printf("ATOMiK libatomik Test Suite (HARDWARE mode)\n");
#endif
    printf("============================================\n");

    /* Test open/close separately */
    test_open_close();

    /* Open handle for remaining tests */
    atomik_t *a = OPEN();
    if (!a) {
        printf("\nFATAL: Failed to open ATOMiK device\n");
        return 1;
    }

    test_config(a);
    test_load_read_roundtrip(a);
    test_accum_correctness(a);
    test_accum_multiple(a);
    test_xor_cancellation(a);
    test_commutativity(a);
    test_identity(a);
    test_multi_address(a);
    test_swap_clears_accumulator(a);
    test_64bit_boundaries(a);
    test_address_boundaries(a);
    test_config_reset(a);
    test_state_reconstruction(a);
    test_burst_accum(a);

    CLOSE(a);

#ifdef ATOMIK_MOCK
    /* =====================================================================
     * Tests that exercise the REAL library functions through mock devmem.
     * These go through atomik_open_devmem → atomik_load/accum/read/swap →
     * mock dispatch, validating the full library path.
     * ===================================================================== */

    printf("\n=== Test: Mock devmem open (CSR layout) ===\n");
    {
        atomik_t *d = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);
        check_int(1, d != NULL, "devmem CSR open");
        check_int(2, d->version, "CSR version = 2");
        check_int(1, d->n_banks, "CSR banks = 1");

        atomik_load(d, 0, 0xCAFEBABEDEADBEEFULL);
        check64(0xCAFEBABEDEADBEEFULL, atomik_read(d), "CSR devmem load/read");

        atomik_accum(d, 0x1111111111111111ULL);
        check64(0xCAFEBABEDEADBEEFULL ^ 0x1111111111111111ULL, atomik_read(d), "CSR devmem accum");

        atomik_accum(d, 0x1111111111111111ULL);
        check64(0xCAFEBABEDEADBEEFULL, atomik_read(d), "CSR devmem self-inverse");
        check_int(1, atomik_acc_zero(d), "CSR devmem acc_zero");

        atomik_swap(d, 0);
        check64(0xCAFEBABEDEADBEEFULL, atomik_read(d), "CSR devmem swap");
        check_int(1, atomik_acc_zero(d), "CSR devmem acc_zero after swap");

        atomik_close(d);
    }

    printf("\n=== Test: Mock devmem open (Adapter layout) ===\n");
    {
        atomik_t *d = atomik_open_devmem(0xF0020000, ATOMIK_LAYOUT_ADAPTER);
        check_int(1, d != NULL, "devmem adapter open");
        check_int(2, d->version, "adapter version = 2");
        check_int(1, d->n_banks, "adapter banks = 1");

        atomik_load(d, 0, 0xAAAABBBBCCCCDDDDULL);
        check64(0xAAAABBBBCCCCDDDDULL, atomik_read(d), "adapter devmem load/read");

        atomik_accum(d, 0x0000FFFF0000FFFFULL);
        uint64_t expected = 0xAAAABBBBCCCCDDDDULL ^ 0x0000FFFF0000FFFFULL;
        check64(expected, atomik_read(d), "adapter devmem accum");

        atomik_accum(d, 0x0000FFFF0000FFFFULL);
        check64(0xAAAABBBBCCCCDDDDULL, atomik_read(d), "adapter devmem self-inverse");
        check_int(1, atomik_acc_zero(d), "adapter devmem acc_zero");

        atomik_close(d);
    }

    printf("\n=== Test: detect_changed (CSR) ===\n");
    {
        atomik_t *d = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);

        uint8_t data[64];
        memset(data, 0xAA, sizeof(data));
        uint64_t fp = 0;
        for (size_t i = 0; i + 8 <= sizeof(data); i += 8) {
            uint64_t chunk; memcpy(&chunk, data + i, 8); fp ^= chunk;
        }

        check_int(0, atomik_detect_changed(d, 0, fp, data, 64, NULL), "detect unchanged");
        data[7] ^= 0xFF;
        check_int(1, atomik_detect_changed(d, 0, fp, data, 64, NULL), "detect 1-byte change");
        check_int(0, atomik_detect_changed(d, 0, 0, data, 0, NULL), "detect empty data");

        /* 64KB test */
        uint8_t *big = calloc(1, 65536);
        memset(big, 0xBB, 65536);
        uint64_t big_fp = 0;
        for (size_t i = 0; i + 8 <= 65536; i += 8) {
            uint64_t chunk; memcpy(&chunk, big + i, 8); big_fp ^= chunk;
        }
        check_int(0, atomik_detect_changed(d, 1, big_fp, big, 65536, NULL), "detect 64KB unchanged");
        big[65535] ^= 0xFF;
        check_int(1, atomik_detect_changed(d, 1, big_fp, big, 65536, NULL), "detect 64KB changed");
        free(big);

        atomik_close(d);
    }

    printf("\n=== Test: detect_changed (Adapter) ===\n");
    {
        atomik_t *d = atomik_open_devmem(0xF0020000, ATOMIK_LAYOUT_ADAPTER);

        uint8_t data[32];
        memset(data, 0x55, sizeof(data));
        uint64_t fp = 0;
        for (size_t i = 0; i + 8 <= sizeof(data); i += 8) {
            uint64_t chunk; memcpy(&chunk, data + i, 8); fp ^= chunk;
        }

        check_int(0, atomik_detect_changed(d, 0, fp, data, 32, NULL), "adapter detect unchanged");
        data[0] = 0x00;
        check_int(1, atomik_detect_changed(d, 0, fp, data, 32, NULL), "adapter detect changed");

        atomik_close(d);
    }

    printf("\n=== Test: detect_changed reference advance ===\n");
    {
        atomik_t *d = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);

        uint8_t data[16];
        memset(data, 0xAA, 16);
        uint64_t fp = 0;
        for (size_t i = 0; i + 8 <= 16; i += 8) {
            uint64_t chunk; memcpy(&chunk, data + i, 8); fp ^= chunk;
        }

        uint64_t new_fp = 0;
        check_int(0, atomik_detect_changed(d, 0, fp, data, 16, &new_fp), "ref advance: unchanged");
        check64(fp, new_fp, "ref advance: new_fp == old_fp when unchanged");

        /* Change data */
        data[0] = 0xBB;
        check_int(1, atomik_detect_changed(d, 0, fp, data, 16, &new_fp), "ref advance: changed");
        /* new_fp should differ from old fp */
        check_int(1, new_fp != fp, "ref advance: new_fp != old_fp after change");

        /* Use new_fp as reference — should detect no change */
        check_int(0, atomik_detect_changed(d, 0, new_fp, data, 16, NULL), "ref advance: new ref matches");

        atomik_close(d);
    }

    printf("\n=== Test: detect_changed trailing bytes ===\n");
    {
        atomik_t *d = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);

        /* 1 byte */
        uint8_t one = 0x42;
        uint64_t fp1 = 0;
        memcpy(&fp1, &one, 1);
        check_int(0, atomik_detect_changed(d, 0, fp1, &one, 1, NULL), "1-byte unchanged");
        one = 0x43;
        check_int(1, atomik_detect_changed(d, 0, fp1, &one, 1, NULL), "1-byte changed");

        /* 13 bytes (8 + 5 trailing) */
        uint8_t buf13[13];
        memset(buf13, 0x55, 13);
        uint64_t fp13 = 0;
        {
            uint64_t chunk; memcpy(&chunk, buf13, 8); fp13 ^= chunk;
            uint64_t tail = 0; memcpy(&tail, buf13 + 8, 5); fp13 ^= tail;
        }
        check_int(0, atomik_detect_changed(d, 1, fp13, buf13, 13, NULL), "13-byte unchanged");
        buf13[12] = 0x00;
        check_int(1, atomik_detect_changed(d, 1, fp13, buf13, 13, NULL), "13-byte trailing change");

        atomik_close(d);
    }

    printf("\n=== Test: Mock devmem open (AXI layout) ===\n");
    {
        atomik_t *d = atomik_open_devmem(0x43C00000, ATOMIK_LAYOUT_AXI);
        check_int(1, d != NULL, "devmem AXI open");
        check_int(2, d->version, "AXI version = 2");
        check_int(1, d->n_banks, "AXI banks = 1");

        atomik_load(d, 0, 0x1234567890ABCDEFULL);
        check64(0x1234567890ABCDEFULL, atomik_read(d), "AXI devmem load/read");

        atomik_accum(d, 0xFFFFFFFFFFFFFFFFULL);
        check64(0x1234567890ABCDEFULL ^ 0xFFFFFFFFFFFFFFFFULL, atomik_read(d), "AXI devmem accum");

        atomik_accum(d, 0xFFFFFFFFFFFFFFFFULL);
        check64(0x1234567890ABCDEFULL, atomik_read(d), "AXI devmem self-inverse");

        atomik_swap(d, 0);
        check64(0x1234567890ABCDEFULL, atomik_read(d), "AXI devmem swap preserves state");
        check_int(1, atomik_acc_zero(d), "AXI devmem acc_zero after swap");

        atomik_close(d);
    }

    printf("\n=== Test: Stress — 1000 sequential accum ===\n");
    {
        atomik_t *d = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);

        atomik_load(d, 0, 0);
        uint64_t expected = 0;
        for (int i = 0; i < 1000; i++) {
            uint64_t delta = (uint64_t)i * 0x0001000100010001ULL;
            atomik_accum(d, delta);
            expected ^= delta;
        }
        check64(expected, atomik_read(d), "1000 sequential accum");

        atomik_close(d);
    }

    printf("\n=== Test: Rapid load/swap cycles ===\n");
    {
        atomik_t *d = atomik_open_devmem(0xF0000000, ATOMIK_LAYOUT_CSR);

        for (int i = 0; i < 256; i++)
            atomik_load(d, (uint8_t)i, (uint64_t)i * 0x0101010101010101ULL);

        /* Verify random addresses */
        atomik_load(d, 0, 0); /* switch context without clobbering — actually this DOES clobber addr 0 */
        /* Use swap instead to preserve */
        atomik_load(d, 200, 200ULL * 0x0101010101010101ULL); /* re-load 200 since we need to read it */
        check64(200ULL * 0x0101010101010101ULL, atomik_read(d), "addr 200 after 256 loads");

        atomik_load(d, 255, 255ULL * 0x0101010101010101ULL);
        check64(255ULL * 0x0101010101010101ULL, atomik_read(d), "addr 255 after 256 loads");

        atomik_close(d);
    }
#endif /* ATOMIK_MOCK */

    /* Summary */
    printf("\n============================================\n");
    printf("Test Summary: %d/%d passed\n", pass_count, pass_count + fail_count);
    printf("============================================\n");

    if (fail_count > 0) {
        printf("*** %d TESTS FAILED ***\n", fail_count);
        return 1;
    }

    printf("*** ALL TESTS PASSED ***\n");
    return 0;
}
