// =============================================================================
// ATOMiK v3 Smoke Testbench (Verilator C++)
//
// Description: Verilator test harness for atomik_v3_stub. Validates basic
//              delta algebra properties (reset, accumulate, self-inverse,
//              commutativity). Supports FST waveform tracing via --trace.
//
// Build: Handled by Makefile (verilate + compile)
// Author: ATOMiK Project
// Date:   February 2026
// =============================================================================

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include "Vatomik_v3_stub.h"
#include "verilated.h"

#ifdef TRACE
#include "verilated_fst_c.h"
#endif

static int test_count = 0;
static int pass_count = 0;
static int fail_count = 0;
static vluint64_t sim_time = 0;

static Vatomik_v3_stub *dut;
#ifdef TRACE
static VerilatedFstC *tfp;
#endif

static void tick() {
    dut->clk = 0;
    dut->eval();
#ifdef TRACE
    tfp->dump(sim_time++);
#endif
    dut->clk = 1;
    dut->eval();
#ifdef TRACE
    tfp->dump(sim_time++);
#endif
}

static void reset() {
    dut->rst_n = 0;
    dut->delta_in = 0;
    dut->delta_valid = 0;
    for (int i = 0; i < 3; i++) tick();
    dut->rst_n = 1;
    tick();
}

static void accumulate(uint64_t delta) {
    dut->delta_in = delta;
    dut->delta_valid = 1;
    tick();
    dut->delta_valid = 0;
    dut->delta_in = 0;
    tick();
}

static void check(uint64_t exp_accum, int exp_zero, const char *name) {
    test_count++;
    tick();  // Let outputs settle
    uint64_t got_accum = dut->accumulator;
    int got_zero = dut->accumulator_zero;

    if (got_accum != exp_accum) {
        printf("FAIL [%s]: accumulator = %016lx, expected %016lx\n",
               name, got_accum, exp_accum);
        fail_count++;
    } else if (got_zero != exp_zero) {
        printf("FAIL [%s]: accumulator_zero = %d, expected %d\n",
               name, got_zero, exp_zero);
        fail_count++;
    } else {
        printf("PASS [%s]\n", name);
        pass_count++;
    }
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    dut = new Vatomik_v3_stub;

#ifdef TRACE
    Verilated::traceEverOn(true);
    tfp = new VerilatedFstC;
    dut->trace(tfp, 99);
    tfp->open("tb_v3_smoke.fst");
#endif

    printf("==============================================\n");
    printf("ATOMiK v3 Smoke Test (Verilator)\n");
    printf("==============================================\n\n");

    // Test 1: Reset clears accumulator
    printf("--- Test 1: Reset ---\n");
    reset();
    check(0x0ULL, 1, "Reset clears accumulator");

    // Test 2: Single accumulate
    printf("--- Test 2: Single accumulate ---\n");
    accumulate(0xDEADBEEFCAFEBABEULL);
    check(0xDEADBEEFCAFEBABEULL, 0, "Single accumulate");

    // Test 3: Self-inverse
    printf("--- Test 3: Self-inverse ---\n");
    reset();
    accumulate(0xAAAABBBBCCCCDDDDULL);
    accumulate(0xAAAABBBBCCCCDDDDULL);
    check(0x0ULL, 1, "Self-inverse cancels");

    // Test 4: Commutativity (order A)
    printf("--- Test 4: Commutativity ---\n");
    reset();
    accumulate(0xFF00FF00FF00FF00ULL);
    accumulate(0x00FF00FF00FF00FFULL);
    check(0xFFFFFFFFFFFFFFFFULL, 0, "Commutativity (order A)");

    // Test 4b: Commutativity (order B)
    reset();
    accumulate(0x00FF00FF00FF00FFULL);
    accumulate(0xFF00FF00FF00FF00ULL);
    check(0xFFFFFFFFFFFFFFFFULL, 0, "Commutativity (order B)");

    // Summary
    printf("\n==============================================\n");
    printf("Test Summary: %d/%d passed\n", pass_count, test_count);
    printf("==============================================\n");

    if (fail_count == 0) {
        printf("*** ALL TESTS PASSED ***\n");
    } else {
        printf("*** %d TESTS FAILED ***\n", fail_count);
    }

#ifdef TRACE
    tfp->close();
    delete tfp;
#endif
    delete dut;

    return (fail_count == 0) ? 0 : 1;
}
