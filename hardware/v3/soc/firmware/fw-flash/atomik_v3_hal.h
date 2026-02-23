// =============================================================================
// ATOMiK v3 Hardware Abstraction Layer
//
// High-level API wrapping ATOMiK v3 custom instructions (RV64I).
// Provides compatibility layer similar to v2 MMIO API but using custom-0 opcodes.
//
// Target: Tang Nano 9K + RV64I @ 13.5 MHz
// =============================================================================

#ifndef ATOMIK_V3_HAL_H
#define ATOMIK_V3_HAL_H

#include <stdint.h>
#include "atomik_v3.h"

// --- Core operations ---

// Initialize ATOMiK bank (soft reset equivalent)
// In v3, we reset by loading zero with accumulator implicitly zero
static inline void atomik_init(uint32_t bank) {
    atomik_load(bank, 0ULL);
}

// Load initial reference state
static inline void atomik_load_state(uint32_t bank, uint64_t value) {
    atomik_load(bank, value);
}

// Accumulate a delta value
static inline void atomik_accumulate(uint32_t bank, uint64_t delta) {
    atomik_accum(delta);
}

// Get current state (initial XOR accumulator)
static inline uint64_t atomik_state(uint32_t bank) {
    return atomik_read(bank);
}

// Check if accumulator is zero (state unchanged from initial)
// In v3, we can test by comparing state to what we loaded
// This is a simplified version - true unchanged detection would need swap
static inline int atomik_unchanged(uint32_t bank) {
    // In v3 we don't have direct acc_zero status like v2 MMIO
    // This is a best-effort approximation
    return 0; // Conservative: assume changed
}

// Undo a delta (XOR is self-inverse)
static inline void atomik_undo(uint32_t bank, uint64_t delta) {
    atomik_accum(delta);
}

// Swap reference state and return old current state
static inline uint64_t atomik_checkpoint(uint32_t bank) {
    return atomik_swap(bank);
}

// --- Higher-level operations ---

// Compute XOR fingerprint of a 64-bit word-aligned memory region
static inline uint64_t atomik_fingerprint(uint32_t bank,
                                          const uint64_t *buf,
                                          uint32_t n_words) {
    atomik_load(bank, 0ULL);
    for (uint32_t i = 0; i < n_words; i++)
        atomik_accumulate(bank, buf[i]);
    return atomik_state(bank);
}

// Verify a region matches a previously computed fingerprint
static inline int atomik_verify(uint32_t bank,
                                const uint64_t *buf,
                                uint32_t n_words,
                                uint64_t expected) {
    return atomik_fingerprint(bank, buf, n_words) == expected;
}

// Check if a region has changed from saved fingerprint
static inline int atomik_region_changed(const uint64_t *buf,
                                        uint32_t n_words,
                                        uint64_t saved_fp) {
    uint64_t current = atomik_fingerprint(0, buf, n_words);
    return current != saved_fp;
}

#endif // ATOMIK_V3_HAL_H
