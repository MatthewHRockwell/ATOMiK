// ATOMiK v3 Custom Instruction Wrappers
//
// Custom-0 opcode (0x0B) R-type format:
//   funct7[6:0] | rs2[4:0] | rs1[4:0] | funct3[2:0] | rd[4:0] | 0001011
//
// Instructions:
//   atomik.load  rd, rs1, rs2  — funct3=000: Load initial state (rs1=addr, rs2=init_data)
//   atomik.accum rd, rs1       — funct3=001: Accumulate delta (rs1=delta)
//   atomik.read  rd, rs1       — funct3=010: Read current state (rs1=addr) → rd
//   atomik.swap  rd, rs1       — funct3=011: Swap reference state (rs1=addr) → rd=old state

#ifndef ATOMIK_V3_H
#define ATOMIK_V3_H

#include <stdint.h>

// ATOMIK.LOAD: Load initial state for slot addressed by rs1, with initial value rs2
// rd receives acc_zero status
static inline uint64_t atomik_load(uint64_t addr, uint64_t init_state) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 0, 0, %0, %1, %2"
        : "=r"(result)
        : "r"(addr), "r"(init_state)
    );
    return result;
}

// ATOMIK.ACCUM: Accumulate delta into the accumulator
// rd receives acc_zero status
static inline uint64_t atomik_accum(uint64_t delta) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 1, 0, %0, %1, x0"
        : "=r"(result)
        : "r"(delta)
    );
    return result;
}

// ATOMIK.READ: Read current state = initial_state XOR accumulator
// rd receives reconstructed state
static inline uint64_t atomik_read(uint64_t addr) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 2, 0, %0, %1, x0"
        : "=r"(result)
        : "r"(addr)
    );
    return result;
}

// ATOMIK.SWAP: Swap reference state (new_ref = current_state, reset accumulator)
// rd receives previous current_state
static inline uint64_t atomik_swap(uint64_t addr) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 3, 0, %0, %1, x0"
        : "=r"(result)
        : "r"(addr)
    );
    return result;
}

#endif // ATOMIK_V3_H
