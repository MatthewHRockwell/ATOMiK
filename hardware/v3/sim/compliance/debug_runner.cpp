// Quick debug runner to trace CPU execution
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "Vatomik_v3_cpu.h"
#include "verilated.h"
#include "elf_loader.h"
#include "mem_model.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);

    const char *elf_file = (argc > 1) ? argv[1] : nullptr;

    MemModel mem;

    if (elf_file) {
        ElfLoader elf;
        if (!elf.load(elf_file, [&](uint32_t addr, const uint8_t *data, uint32_t len) {
            mem.write_block(addr, data, len);
        })) return 1;
        if (elf.has_tohost)
            mem.set_tohost_addr((uint32_t)elf.tohost_addr);
        printf("ELF Entry: 0x%08lx  tohost: 0x%08lx\n",
               (unsigned long)elf.entry_point, (unsigned long)elf.tohost_addr);
    } else {
        // Minimal test: store 0xDEADBEEF to 0x80001000, then load it back
        // ADDI x1, x0, -1       → x1 = 0xFFFFFFFFFFFFFFFF
        uint32_t prog[] = {
            0xfff00093,  // addi x1, x0, -1    (x1 = -1)
            0x00100113,  // addi x2, x0, 1     (x2 = 1)
            0x00001197,  // auipc x3, 1        (x3 = 0x80000008 + 0x1000)
            0xff818193,  // addi x3, x3, -8    (x3 = 0x80001000)
            0x0011a023,  // sw x1, 0(x3)       (mem[0x80001000] = x1)
            0x0001a203,  // lw x4, 0(x3)       (x4 = mem[0x80001000])
            0x0000006f,  // j 0  (infinite loop)
        };
        for (int i = 0; i < 7; i++) {
            uint32_t addr = 0x80000000 + i * 4;
            mem.write_byte(addr, prog[i] & 0xFF);
            mem.write_byte(addr+1, (prog[i] >> 8) & 0xFF);
            mem.write_byte(addr+2, (prog[i] >> 16) & 0xFF);
            mem.write_byte(addr+3, (prog[i] >> 24) & 0xFF);
        }
        mem.set_tohost_addr(0x80001000);
        printf("Running minimal SW/LW test\n");
    }

    printf("Memory at base:\n");
    for (int i = 0; i < 8; i++) {
        uint32_t addr = 0x80000000 + i * 4;
        printf("  0x%08x: 0x%08x\n", addr, mem.bus_read(addr));
    }

    Vatomik_v3_cpu *cpu = new Vatomik_v3_cpu;

    // Reset
    cpu->clk = 0;
    cpu->rst_n = 0;
    cpu->mem_ready = 0;
    cpu->mem_rdata = 0;
    for (int i = 0; i < 10; i++) {
        cpu->clk = !cpu->clk; cpu->eval();
    }
    // Ensure clk ends low
    cpu->clk = 0; cpu->eval();
    cpu->rst_n = 1;

    printf("\n--- Simulation (1000 cycles) ---\n");
    for (int cyc = 0; cyc < 1000; cyc++) {
        // Falling edge
        cpu->clk = 0;
        cpu->eval();

        // Memory model
        uint32_t rdata = 0;
        uint8_t ready = 0;
        mem.tick(cpu->mem_valid, cpu->mem_addr, cpu->mem_wdata,
                 cpu->mem_wstrb, rdata, ready);
        cpu->mem_ready = ready;
        cpu->mem_rdata = rdata;

        // Rising edge
        cpu->clk = 1;
        cpu->eval();

        // Track completed fetches — ready follows valid by 1 cycle
        static uint32_t last_addr = 0;
        static bool last_valid = false;
        static bool last_was_store = false;
        if (cpu->mem_ready && last_valid) {
            if (last_was_store) {
                printf("[%4d] STORE 0x%08x\n", cyc, last_addr);
            } else {
                printf("[%4d] FETCH/LOAD 0x%08x → 0x%08x\n", cyc, last_addr, cpu->mem_rdata);
            }
        }
        last_valid = cpu->mem_valid;
        last_addr = cpu->mem_addr;
        last_was_store = (cpu->mem_wstrb != 0);

        if (mem.tohost_written()) {
            printf("[%3d] TOHOST = 0x%lx\n", cyc, (unsigned long)mem.get_tohost_value());
            break;
        }
    }

    delete cpu;
    return 0;
}
