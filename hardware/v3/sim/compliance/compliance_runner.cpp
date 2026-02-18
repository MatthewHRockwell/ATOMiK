// =============================================================================
// ATOMiK v3 RISC-V Compliance Test Runner (Verilator)
//
// Loads riscv-tests ELF binaries, simulates the v3 CPU, monitors tohost
// for pass/fail. Usage:
//   compliance_runner <elf_file> [--trace] [--timeout N]
//
// Exit code: 0 = PASS, 1 = FAIL, 2 = TIMEOUT, 3 = ERROR
// =============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "Vatomik_v3_cpu.h"
#include "verilated.h"

#ifdef TRACE
#include "verilated_fst_c.h"
#endif

#include "elf_loader.h"
#include "mem_model.h"

static vluint64_t sim_time = 0;
static uint64_t max_cycles = 500000;  // Default timeout

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);

    // Parse arguments
    const char *elf_file = nullptr;
    bool do_trace = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--trace") == 0) {
            do_trace = true;
        } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            max_cycles = strtoull(argv[++i], nullptr, 0);
        } else if (argv[i][0] != '-' && argv[i][0] != '+') {
            elf_file = argv[i];
        }
    }

    if (!elf_file) {
        fprintf(stderr, "Usage: compliance_runner <elf_file> [--trace] [--timeout N]\n");
        return 3;
    }

    // Load ELF
    MemModel mem;
    ElfLoader elf;

    if (!elf.load(elf_file, [&](uint32_t addr, const uint8_t *data, uint32_t len) {
        mem.write_block(addr, data, len);
    })) {
        fprintf(stderr, "ERROR: Failed to load ELF: %s\n", elf_file);
        return 3;
    }

    if (elf.has_tohost) {
        mem.set_tohost_addr((uint32_t)elf.tohost_addr);
    }

    printf("ELF: %s\n", elf_file);
    printf("  entry:  0x%08lx\n", (unsigned long)elf.entry_point);
    printf("  tohost: 0x%08lx%s\n", (unsigned long)elf.tohost_addr,
           elf.has_tohost ? "" : " (NOT FOUND)");

    // Create CPU with entry point as reset PC
    // riscv-tests typically start at 0x80000000
    Vatomik_v3_cpu *cpu = new Vatomik_v3_cpu;

    // We need to set RESET_PC parameter — done at Verilator compile time
    // The Makefile will pass -GRESET_PC=... or we'll use default 0x80000000

#ifdef TRACE
    VerilatedFstC *tfp = nullptr;
    if (do_trace) {
        Verilated::traceEverOn(true);
        tfp = new VerilatedFstC;
        cpu->trace(tfp, 99);
        // Extract test name for trace file
        const char *base = strrchr(elf_file, '/');
        char trace_name[256];
        snprintf(trace_name, sizeof(trace_name), "%s.fst", base ? base + 1 : elf_file);
        tfp->open(trace_name);
    }
#else
    (void)do_trace;
#endif

    // Reset
    cpu->clk = 0;
    cpu->rst_n = 0;
    cpu->mem_ready = 0;
    cpu->mem_rdata = 0;

    for (int i = 0; i < 10; i++) {
        cpu->clk = 0;
        cpu->eval();
#ifdef TRACE
        if (tfp) tfp->dump(sim_time++);
#endif
        cpu->clk = 1;
        cpu->eval();
#ifdef TRACE
        if (tfp) tfp->dump(sim_time++);
#endif
    }
    cpu->rst_n = 1;

    // Run simulation
    uint64_t cycle = 0;
    int result = 2;  // Default: timeout

    while (cycle < max_cycles) {
        // Falling edge
        cpu->clk = 0;
        cpu->eval();

        // Memory model responds to bus
        uint32_t rdata = 0;
        uint8_t ready = 0;
        mem.tick(cpu->mem_valid, cpu->mem_addr, cpu->mem_wdata,
                 cpu->mem_wstrb, rdata, ready);
        cpu->mem_ready = ready;
        cpu->mem_rdata = rdata;

#ifdef TRACE
        if (tfp) tfp->dump(sim_time++);
#endif

        // Rising edge
        cpu->clk = 1;
        cpu->eval();
#ifdef TRACE
        if (tfp) tfp->dump(sim_time++);
#endif

        cycle++;

        // Check tohost
        if (mem.tohost_written()) {
            uint64_t tohost = mem.get_tohost_value();
            if (tohost == 1) {
                printf("PASS (cycle %lu)\n", (unsigned long)cycle);
                result = 0;
                break;
            } else if (tohost != 0) {
                uint64_t test_num = tohost >> 1;
                printf("FAIL: test case %lu (tohost=0x%lx, cycle %lu)\n",
                       (unsigned long)test_num, (unsigned long)tohost,
                       (unsigned long)cycle);
                result = 1;
                break;
            }
            mem.clear_tohost();
        }
    }

    if (result == 2) {
        printf("TIMEOUT after %lu cycles\n", (unsigned long)max_cycles);
    }

#ifdef TRACE
    if (tfp) {
        tfp->close();
        delete tfp;
    }
#endif
    delete cpu;

    return result;
}
