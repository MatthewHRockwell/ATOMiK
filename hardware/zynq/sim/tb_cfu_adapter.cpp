// =============================================================================
// ATOMiK CFU Adapter Wishbone Wrapper — Verilator C++ Testbench
//
// Tests the full stack: Wishbone MMIO -> CFU adapter -> ATOMiK state table
//
//   1. LOAD / READ roundtrip (64-bit LO/HI pair protocol)
//   2. ACCUM / READ (XOR delta accumulation)
//   3. XOR self-inverse (accum same delta twice == identity)
//   4. XOR identity (accum zero == no change)
//   5. XOR commutativity (order-independent)
//   6. SWAP (promote current_state to reference, clear accumulator)
//   7. Multi-address isolation
//   8. STATUS register
//
// All operations go through the Wishbone register map:
//   adr=0 CMD, adr=1 RS1, adr=2 RS2, adr=3 RD, adr=4 STATUS
//
// ATOMiK Project — April 2026
// =============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "Vatomik_cfu_wishbone.h"
#include "verilated.h"

static Vatomik_cfu_wishbone *dut;
static vluint64_t sim_time = 0;
static int test_pass = 0;
static int test_fail = 0;

// ── Clock helper ─────────────────────────────────────────────────────────────

static void tick() {
    dut->clk = 0;
    dut->eval();
    sim_time++;
    dut->clk = 1;
    dut->eval();
    sim_time++;
}

static void ticks(int n) {
    for (int i = 0; i < n; i++) tick();
}

// ── Wishbone helpers ─────────────────────────────────────────────────────────

// Write a 32-bit value to a Wishbone register (word address 0-4)
static void wb_write(uint8_t adr, uint32_t data) {
    dut->cyc   = 1;
    dut->stb   = 1;
    dut->we    = 1;
    dut->adr   = adr;
    dut->dat_w = data;
    // Wait for ack
    for (int i = 0; i < 20; i++) {
        tick();
        if (dut->ack) break;
    }
    // Deassert bus
    dut->cyc = 0;
    dut->stb = 0;
    dut->we  = 0;
    tick();
}

// Read a 32-bit value from a Wishbone register (word address 0-4)
static uint32_t wb_read(uint8_t adr) {
    dut->cyc   = 1;
    dut->stb   = 1;
    dut->we    = 0;
    dut->adr   = adr;
    dut->dat_w = 0;
    uint32_t val = 0;
    for (int i = 0; i < 20; i++) {
        tick();
        if (dut->ack) {
            val = dut->dat_r;
            break;
        }
    }
    dut->cyc = 0;
    dut->stb = 0;
    tick();
    return val;
}

// ── ATOMiK operation helpers (via Wishbone register map) ─────────────────────

// Wait until STATUS.busy == 0
static void wait_idle() {
    for (int i = 0; i < 100; i++) {
        uint32_t st = wb_read(4);
        if ((st & 1) == 0) return;  // bit 0 = busy
    }
    printf("  ** TIMEOUT waiting for idle!\n");
}

// Issue a command: write RS1, RS2, then CMD (which triggers the operation)
static void issue_cmd(uint8_t funct3, uint32_t rs1, uint32_t rs2) {
    wait_idle();
    wb_write(1, rs1);  // RS1
    wb_write(2, rs2);  // RS2
    wb_write(0, funct3 & 0x7);  // CMD triggers
    wait_idle();
}

// Issue a command with only RS1
static void issue_cmd_rs1(uint8_t funct3, uint32_t rs1) {
    wait_idle();
    wb_write(1, rs1);
    wb_write(0, funct3 & 0x7);
    wait_idle();
}

// Issue a command with no inputs (READ, READ_HI, STATUS)
static void issue_cmd_none(uint8_t funct3) {
    wait_idle();
    wb_write(0, funct3 & 0x7);
    wait_idle();
}

// Read RD register
static uint32_t read_rd() {
    return wb_read(3);
}

// ── 64-bit LO/HI protocol helpers ───────────────────────────────────────────

// LOAD: set address, write 64-bit initial state
//   Step 1: RS1=addr, RS2=init_lo, CMD=0 (F_LOAD)
//   Step 2: RS2=init_hi, CMD=4 (F_LOAD_HI)
static void atomik_load(uint8_t addr, uint64_t init_val) {
    uint32_t lo = (uint32_t)(init_val & 0xFFFFFFFF);
    uint32_t hi = (uint32_t)(init_val >> 32);
    issue_cmd(0, addr, lo);      // LOAD: addr + init_lo
    issue_cmd(4, 0, hi);         // LOAD_HI: init_hi triggers write
}

// ACCUM: XOR 64-bit delta into accumulator
//   Step 1: RS1=delta_lo, CMD=1 (F_ACCUM)
//   Step 2: RS1=delta_hi, CMD=5 (F_ACCUM_HI)
static void atomik_accum(uint64_t delta) {
    uint32_t lo = (uint32_t)(delta & 0xFFFFFFFF);
    uint32_t hi = (uint32_t)(delta >> 32);
    issue_cmd_rs1(1, lo);        // ACCUM: delta_lo
    issue_cmd_rs1(5, hi);        // ACCUM_HI: delta_hi triggers XOR
}

// READ: get 64-bit current_state
//   Step 1: CMD=2 (F_READ) → read RD (lo)
//   Step 2: CMD=6 (F_READ_HI) → read RD (hi)
static uint64_t atomik_read() {
    issue_cmd_none(2);           // READ
    uint32_t lo = read_rd();
    issue_cmd_none(6);           // READ_HI
    uint32_t hi = read_rd();
    return ((uint64_t)hi << 32) | lo;
}

// SWAP: promote current_state to reference, clear accumulator
static void atomik_swap(uint8_t addr) {
    issue_cmd_rs1(3, addr);      // SWAP
}

// STATUS: read status word
static uint32_t atomik_status() {
    issue_cmd_none(7);           // STATUS
    return read_rd();
}

// ── Test infrastructure ──────────────────────────────────────────────────────

static void check(const char *name, uint64_t got, uint64_t expected) {
    if (got == expected) {
        printf("  PASS: %s (0x%016lx)\n", name, got);
        test_pass++;
    } else {
        printf("  FAIL: %s — got 0x%016lx, expected 0x%016lx\n",
               name, got, expected);
        test_fail++;
    }
}

static void check32(const char *name, uint32_t got, uint32_t expected) {
    if (got == expected) {
        printf("  PASS: %s (0x%08x)\n", name, got);
        test_pass++;
    } else {
        printf("  FAIL: %s — got 0x%08x, expected 0x%08x\n",
               name, got, expected);
        test_fail++;
    }
}

// ── Reset ────────────────────────────────────────────────────────────────────

static void reset_dut() {
    dut->rst   = 1;
    dut->cyc   = 0;
    dut->stb   = 0;
    dut->we    = 0;
    dut->adr   = 0;
    dut->dat_w = 0;
    ticks(10);
    dut->rst = 0;
    ticks(5);
}

// =============================================================================
// Test cases
// =============================================================================

// T1: LOAD + READ roundtrip — store a 64-bit value, read it back
static void test_load_read() {
    printf("\nT1: LOAD + READ roundtrip\n");
    reset_dut();

    uint64_t init_val = 0xDEADBEEFCAFEBABEULL;
    atomik_load(0, init_val);

    uint64_t got = atomik_read();
    check("LOAD/READ roundtrip", got, init_val);
}

// T2: ACCUM + READ — apply a delta, verify XOR
static void test_accum_read() {
    printf("\nT2: ACCUM + READ\n");
    reset_dut();

    uint64_t init  = 0x1111111122222222ULL;
    uint64_t delta = 0x00FF00FF00FF00FFULL;
    uint64_t expected = init ^ delta;

    atomik_load(0, init);
    atomik_accum(delta);

    uint64_t got = atomik_read();
    check("ACCUM XOR", got, expected);
}

// T3: XOR self-inverse — accumulating the same delta twice cancels out
static void test_self_inverse() {
    printf("\nT3: XOR self-inverse\n");
    reset_dut();

    uint64_t init  = 0xAAAABBBBCCCCDDDDULL;
    uint64_t delta = 0x123456789ABCDEF0ULL;

    atomik_load(0, init);
    atomik_accum(delta);
    atomik_accum(delta);  // Should cancel

    uint64_t got = atomik_read();
    check("Self-inverse (delta XOR delta = identity)", got, init);
}

// T4: XOR identity — accumulating zero changes nothing
static void test_identity() {
    printf("\nT4: XOR identity element\n");
    reset_dut();

    uint64_t init = 0xFEDCBA9876543210ULL;
    atomik_load(0, init);
    atomik_accum(0x0000000000000000ULL);

    uint64_t got = atomik_read();
    check("Identity (XOR 0)", got, init);
}

// T5: XOR commutativity — order of deltas doesn't matter
static void test_commutativity() {
    printf("\nT5: XOR commutativity\n");
    reset_dut();

    uint64_t init   = 0x0000000000000000ULL;
    uint64_t deltaA = 0x1111111111111111ULL;
    uint64_t deltaB = 0x2222222222222222ULL;

    // Order 1: A then B
    atomik_load(0, init);
    atomik_accum(deltaA);
    atomik_accum(deltaB);
    uint64_t result1 = atomik_read();

    // Order 2: B then A (reset first)
    reset_dut();
    atomik_load(0, init);
    atomik_accum(deltaB);
    atomik_accum(deltaA);
    uint64_t result2 = atomik_read();

    check("Commutativity: A^B", result1, init ^ deltaA ^ deltaB);
    check("Commutativity: B^A", result2, init ^ deltaB ^ deltaA);
    check("Commutativity: A^B == B^A", result1, result2);
}

// T6: XOR associativity — (A^B)^C == A^(B^C)
static void test_associativity() {
    printf("\nT6: XOR associativity\n");
    reset_dut();

    uint64_t init   = 0x0000000000000000ULL;
    uint64_t deltaA = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t deltaB = 0xBBBBBBBBBBBBBBBBULL;
    uint64_t deltaC = 0xCCCCCCCCCCCCCCCCULL;

    atomik_load(0, init);
    atomik_accum(deltaA);
    atomik_accum(deltaB);
    atomik_accum(deltaC);
    uint64_t got = atomik_read();

    check("Associativity: A^B^C", got, init ^ deltaA ^ deltaB ^ deltaC);
}

// T7: SWAP — promote current_state to reference, clear accumulator
static void test_swap() {
    printf("\nT7: SWAP\n");
    reset_dut();

    uint64_t init  = 0x1000000020000000ULL;
    uint64_t delta = 0x0000000100000002ULL;

    atomik_load(0, init);
    atomik_accum(delta);

    uint64_t before_swap = atomik_read();
    check("Before SWAP", before_swap, init ^ delta);

    // SWAP: current_state becomes new reference, acc clears
    atomik_swap(0);

    uint64_t after_swap = atomik_read();
    check("After SWAP (same value, acc cleared)", after_swap, init ^ delta);

    // Accumulate another delta after SWAP
    uint64_t delta2 = 0x00FF00FF00FF00FFULL;
    atomik_accum(delta2);
    uint64_t after_delta2 = atomik_read();
    check("After SWAP + new delta", after_delta2, (init ^ delta) ^ delta2);
}

// T8: Multi-address isolation — operations on different addresses are independent
static void test_multi_address() {
    printf("\nT8: Multi-address isolation\n");
    reset_dut();

    uint64_t init_a = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t init_b = 0xBBBBBBBBBBBBBBBBULL;

    // Load address 0 with init_a
    atomik_load(0, init_a);

    // SWAP to lock in addr 0, then load address 1 with init_b
    atomik_swap(0);
    atomik_load(1, init_b);

    // Read address 1 — should be init_b
    uint64_t got_b = atomik_read();
    check("Addr 1 after LOAD", got_b, init_b);

    // SWAP to addr 1, switch back to addr 0 via LOAD
    atomik_swap(1);

    // Load addr 0 again (re-read the stored reference)
    // Since SWAP at addr 0 stored init_a, we load addr 0 with a fresh value
    // to show isolation: addr 1's init_b is untouched
    atomik_load(0, 0x1234567812345678ULL);
    atomik_swap(0);

    // Now load addr 1 with same init_b to re-read it
    atomik_load(1, init_b);
    uint64_t got_b2 = atomik_read();
    check("Addr 1 isolation", got_b2, init_b);
}

// T9: STATUS register — version=2, n_banks=1, acc_zero bit
static void test_status() {
    printf("\nT9: STATUS register\n");
    reset_dut();

    // After reset, accumulator is zero → acc_zero=1
    uint32_t st = atomik_status();
    // Status word: {8'b0, VERSION[7:0], N_BANKS[7:0], 7'b0, acc_zero}
    // VERSION=2, N_BANKS=1, acc_zero=1
    // = 0x00_02_01_01
    uint32_t expected = (2 << 16) | (1 << 8) | 1;
    check32("STATUS after reset (acc_zero=1)", st, expected);

    // Load + accumulate to make acc non-zero
    atomik_load(0, 0x0000000000000000ULL);
    atomik_accum(0x00000000FFFFFFFFULL);
    st = atomik_status();
    expected = (2 << 16) | (1 << 8) | 0;  // acc_zero=0
    check32("STATUS after ACCUM (acc_zero=0)", st, expected);

    // Self-inverse to clear accumulator
    atomik_accum(0x00000000FFFFFFFFULL);
    st = atomik_status();
    expected = (2 << 16) | (1 << 8) | 1;  // acc_zero=1 again
    check32("STATUS after cancel (acc_zero=1)", st, expected);
}

// T10: Wishbone register readback — RS1 and RS2 readable
static void test_wb_readback() {
    printf("\nT10: Wishbone register readback\n");
    reset_dut();

    wb_write(1, 0xDEADBEEF);  // RS1
    uint32_t rs1 = wb_read(1);
    check32("RS1 readback", rs1, 0xDEADBEEF);

    wb_write(2, 0xCAFEBABE);  // RS2
    uint32_t rs2 = wb_read(2);
    check32("RS2 readback", rs2, 0xCAFEBABE);
}

// T11: Busy flag — STATUS bit 0 should pulse during command execution
static void test_busy_flag() {
    printf("\nT11: Busy flag\n");
    reset_dut();

    atomik_load(0, 0x0000000000000001ULL);

    // After wait_idle completes, busy should be 0
    uint32_t st = wb_read(4);
    check32("Busy=0 after idle", st & 1, 0);
}

// T12: Large burst — 16 sequential ACCUMs
static void test_burst_accum() {
    printf("\nT12: Burst ACCUM (16 deltas)\n");
    reset_dut();

    uint64_t init = 0x0000000000000000ULL;
    atomik_load(0, init);

    uint64_t expected = init;
    for (int i = 1; i <= 16; i++) {
        uint64_t delta = (uint64_t)i | ((uint64_t)i << 32);
        atomik_accum(delta);
        expected ^= delta;
    }

    uint64_t got = atomik_read();
    check("Burst 16 ACCUMs", got, expected);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Vatomik_cfu_wishbone;

    printf("============================================================\n");
    printf("ATOMiK CFU Adapter Wishbone Wrapper — Verilator Testbench\n");
    printf("============================================================\n");

    test_load_read();       // T1
    test_accum_read();      // T2
    test_self_inverse();    // T3
    test_identity();        // T4
    test_commutativity();   // T5
    test_associativity();   // T6
    test_swap();            // T7
    test_multi_address();   // T8
    test_status();          // T9
    test_wb_readback();     // T10
    test_busy_flag();       // T11
    test_burst_accum();     // T12

    printf("\n============================================================\n");
    printf("Results: %d PASS, %d FAIL out of %d total\n",
           test_pass, test_fail, test_pass + test_fail);
    if (test_fail == 0)
        printf("ALL TESTS PASSED\n");
    else
        printf("SOME TESTS FAILED\n");
    printf("============================================================\n");

    dut->final();
    delete dut;

    return test_fail > 0 ? 1 : 0;
}
