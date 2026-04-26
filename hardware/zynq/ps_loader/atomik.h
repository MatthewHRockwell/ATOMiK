/*
 * atomik.h — ATOMiK Hardware API
 *
 * Standard C header for ATOMiK delta-state operations.
 * No custom compiler required. Works with standard GCC / Clang.
 *
 * Two backends, same API:
 *
 *   ATOMIK_NATIVE  — Custom RISC-V instructions (opcode 0x0B)
 *                    For CPUs with ATOMiK ISA extensions (v3 SoC)
 *
 *   ATOMIK_MMIO    — Memory-mapped adapter (default)
 *                    For any CPU with ATOMiK adapter on the bus
 *                    Call atomik_init_mmio(base_addr) before use
 *
 * Usage:
 *   #include "atomik.h"
 *
 *   atomik_init_mmio(0xF0020000);      // point to adapter (MMIO mode)
 *   atomik_load(0, initial_state);     // initialize slot 0
 *   atomik_accum(delta);               // accumulate change
 *   uint64_t current = atomik_read(0); // read: initial XOR acc
 *   atomik_swap(0);                    // checkpoint
 */

#ifndef ATOMIK_H
#define ATOMIK_H

#include <stdint.h>

#ifdef ATOMIK_NATIVE
/* ── Native custom instruction path (v3 ATOMiK CPU) ───────────── */

static inline uint64_t atomik_load(uint64_t addr, uint64_t init) {
    uint64_t rd;
    __asm__ volatile(".insn r 0x0B, 0, 0, %0, %1, %2"
                     : "=r"(rd) : "r"(addr), "r"(init));
    return rd;
}
static inline uint64_t atomik_accum(uint64_t delta) {
    uint64_t rd;
    __asm__ volatile(".insn r 0x0B, 1, 0, %0, %1, x0"
                     : "=r"(rd) : "r"(delta));
    return rd;
}
static inline uint64_t atomik_read(uint64_t addr) {
    uint64_t rd;
    __asm__ volatile(".insn r 0x0B, 2, 0, %0, %1, x0"
                     : "=r"(rd) : "r"(addr));
    return rd;
}
static inline uint64_t atomik_swap(uint64_t addr) {
    uint64_t rd;
    __asm__ volatile(".insn r 0x0B, 3, 0, %0, %1, x0"
                     : "=r"(rd) : "r"(addr));
    return rd;
}
static inline void atomik_init_mmio(uint64_t base) { (void)base; }

#else
/* ── MMIO adapter path (any CPU with ATOMiK on the bus) ────────── */

static volatile uint32_t *_atomik_base;

static inline void _aw(int off, uint32_t v) {
    _atomik_base[off/4] = v;
    __asm__ volatile("fence iorw,iorw");
}
static inline uint32_t _ar(int off) {
    __asm__ volatile("fence iorw,iorw");
    return _atomik_base[off/4];
}

static inline void atomik_init_mmio(uint64_t base) {
    _atomik_base = (volatile uint32_t *)(uintptr_t)base;
}

static inline uint64_t atomik_load(uint64_t addr, uint64_t init) {
    _aw(0x04, (uint32_t)addr);
    _aw(0x08, (uint32_t)(init & 0xFFFFFFFF));
    _aw(0x00, 0);  /* F_LOAD */
    _aw(0x08, (uint32_t)(init >> 32));
    _aw(0x00, 4);  /* F_LOAD_HI */
    return 0;
}
static inline uint64_t atomik_accum(uint64_t delta) {
    _aw(0x04, (uint32_t)(delta & 0xFFFFFFFF));
    _aw(0x00, 1);  /* F_ACCUM */
    _aw(0x04, (uint32_t)(delta >> 32));
    _aw(0x00, 5);  /* F_ACCUM_HI */
    return 0;
}
static inline uint64_t atomik_read(uint64_t addr) {
    (void)addr;
    _aw(0x00, 2);  /* F_READ */
    uint32_t lo = _ar(0x0C);
    _aw(0x00, 6);  /* F_READ_HI */
    uint32_t hi = _ar(0x0C);
    return ((uint64_t)hi << 32) | lo;
}
static inline uint64_t atomik_swap(uint64_t addr) {
    _aw(0x04, (uint32_t)addr);
    _aw(0x00, 3);  /* F_SWAP */
    return 0;
}

#endif /* ATOMIK_NATIVE */

/* Convenience: XOR fingerprint of a buffer */
static inline uint64_t atomik_fingerprint(uint64_t slot,
                                          const uint64_t *buf,
                                          uint64_t n_words) {
    atomik_load(slot, 0);
    for (uint64_t i = 0; i < n_words; i++)
        atomik_accum(buf[i]);
    return atomik_read(slot);
}

/* Convenience: detect if buffer changed since last fingerprint */
static inline int atomik_changed(uint64_t slot,
                                 const uint64_t *buf,
                                 uint64_t n_words,
                                 uint64_t saved_fp) {
    return atomik_fingerprint(slot, buf, n_words) != saved_fp;
}

#endif /* ATOMIK_H */
