// =============================================================================
// ATOMiK Hardware Abstraction Layer
//
// Inline C API for ATOMiK delta accumulator on PicoRV32 SoC.
// Base address: 0xC000_0000 (S3 Wishbone port)
// Single-bank design; bank parameter reserved for future multi-bank extension.
//
// Target: Tang Nano 9K (GW1NR-9) + PicoRV32 @ 25.2 MHz
// =============================================================================

#ifndef ATOMIK_H
#define ATOMIK_H

#include <stdint.h>

// Register offsets per bank (32 bytes per bank)
#define ATOMIK_BASE       0xC0000000
#define ATOMIK_BANK_STRIDE 0x20

#define ATOMIK_OFF_LOAD   0x00
#define ATOMIK_OFF_ACCUM  0x04
#define ATOMIK_OFF_STATE  0x08
#define ATOMIK_OFF_STATUS 0x0C
#define ATOMIK_OFF_CONFIG 0x10
#define ATOMIK_OFF_INIT   0x14
#define ATOMIK_OFF_DELTA  0x18

// Register access macro (bank-aware)
#define ATOMIK_REG(bank, off) \
    (*(volatile uint32_t*)(ATOMIK_BASE + (bank) * ATOMIK_BANK_STRIDE + (off)))

// --- Core operations ---

static inline void atomik_init(uint32_t bank) {
    ATOMIK_REG(bank, ATOMIK_OFF_CONFIG) = 1;  // soft reset
}

static inline void atomik_load(uint32_t bank, uint32_t value) {
    ATOMIK_REG(bank, ATOMIK_OFF_LOAD) = value;
}

static inline void atomik_accumulate(uint32_t bank, uint32_t delta) {
    ATOMIK_REG(bank, ATOMIK_OFF_ACCUM) = delta;
}

static inline uint32_t atomik_state(uint32_t bank) {
    return ATOMIK_REG(bank, ATOMIK_OFF_STATE);
}

static inline uint32_t atomik_get_delta(uint32_t bank) {
    return ATOMIK_REG(bank, ATOMIK_OFF_DELTA);
}

static inline uint32_t atomik_get_initial(uint32_t bank) {
    return ATOMIK_REG(bank, ATOMIK_OFF_INIT);
}

static inline int atomik_unchanged(uint32_t bank) {
    return ATOMIK_REG(bank, ATOMIK_OFF_STATUS) & 1;
}

// XOR is self-inverse: undo == accumulate the same delta again
static inline void atomik_undo(uint32_t bank, uint32_t delta) {
    ATOMIK_REG(bank, ATOMIK_OFF_ACCUM) = delta;
}

// --- Higher-level operations ---

// Compute XOR fingerprint of a word-aligned memory region
static inline uint32_t atomik_fingerprint(uint32_t bank,
                                          const uint32_t *buf,
                                          uint32_t n_words) {
    ATOMIK_REG(bank, ATOMIK_OFF_LOAD) = 0;
    for (uint32_t i = 0; i < n_words; i++)
        ATOMIK_REG(bank, ATOMIK_OFF_ACCUM) = buf[i];
    return ATOMIK_REG(bank, ATOMIK_OFF_STATE);
}

// Verify a region matches a previously computed fingerprint
static inline int atomik_verify(uint32_t bank,
                                const uint32_t *buf,
                                uint32_t n_words,
                                uint32_t expected) {
    return atomik_fingerprint(bank, buf, n_words) == expected;
}

#endif // ATOMIK_H
