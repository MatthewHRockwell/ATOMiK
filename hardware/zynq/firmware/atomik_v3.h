/*
 * atomik_v3.h — ATOMiK Custom Instruction Wrappers (Zynq Co-Processor)
 *
 * Identical to hardware/v3/soc/firmware/fw-flash/atomik_v3.h — the custom
 * instruction encoding is the same (custom-0 opcode 0x0B, R-type format).
 * The co-processor's ATOMiK core is direct-wired (zero overhead).
 *
 * This copy exists so Zynq firmware can be built independently without
 * reaching into the v3 SoC tree.
 *
 * Custom-0 opcode (0x0B) R-type format:
 *   funct7[6:0] | rs2[4:0] | rs1[4:0] | funct3[2:0] | rd[4:0] | 0001011
 *
 * Instructions:
 *   atomik.load  rd, rs1, rs2  — funct3=000: Load initial state
 *   atomik.accum rd, rs1       — funct3=001: Accumulate delta
 *   atomik.read  rd, rs1       — funct3=010: Read current state
 *   atomik.swap  rd, rs1       — funct3=011: Swap reference state
 *
 * ATOMiK Project — March 2026
 * SPDX-License-Identifier: MIT
 */

#ifndef ATOMIK_V3_H
#define ATOMIK_V3_H

#include <stdint.h>

/* ATOMIK.LOAD: Load initial state for slot addressed by rs1 */
static inline uint64_t atomik_load(uint64_t addr, uint64_t init_state) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 0, 0, %0, %1, %2"
        : "=r"(result)
        : "r"(addr), "r"(init_state)
    );
    return result;
}

/* ATOMIK.ACCUM: XOR delta into accumulator */
static inline uint64_t atomik_accum(uint64_t delta) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 1, 0, %0, %1, x0"
        : "=r"(result)
        : "r"(delta)
    );
    return result;
}

/* ATOMIK.READ: Read current_state = initial_state XOR accumulator */
static inline uint64_t atomik_read(uint64_t addr) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 2, 0, %0, %1, x0"
        : "=r"(result)
        : "r"(addr)
    );
    return result;
}

/* ATOMIK.SWAP: Update reference = current_state, clear accumulator */
static inline uint64_t atomik_swap(uint64_t addr) {
    uint64_t result;
    __asm__ volatile (
        ".insn r 0x0B, 3, 0, %0, %1, x0"
        : "=r"(result)
        : "r"(addr)
    );
    return result;
}

#endif /* ATOMIK_V3_H */
