// ATOMiK v3 Tracked Memory Operations
// Software baselines + ATOMiK-tracked variants for integrity and change detection.
// Adapted for RV64I with 64-bit operations.

#ifndef ATOMIK_V3_MEM_H
#define ATOMIK_V3_MEM_H

#include <stdint.h>

// --- Software baselines ---
void *sw_memcpy(void *dst, const void *src, uint32_t n);
void *sw_memset(void *dst, int val, uint32_t n);
int   sw_memcmp(const void *a, const void *b, uint32_t n);

// --- ATOMiK-tracked variants (64-bit) ---

// Copy n bytes from src to dst, compute XOR fingerprint of source into *fp_out.
// n must be a multiple of 8 (64-bit word-aligned).
void *atomik_memcpy_tracked(void *dst, const void *src, uint32_t n, uint64_t *fp_out);

// Fill n bytes of dst with val, compute fingerprint of filled region into *fp_out.
// n must be a multiple of 8.
void *atomik_memset_verified(void *dst, int val, uint32_t n, uint64_t *fp_out);

// Check if a word-aligned region has changed since a fingerprint was taken.
// Returns 1 if changed, 0 if unchanged.
int atomik_region_changed_mem(const uint64_t *buf, uint32_t n_words, uint64_t saved_fp);

// Compute cumulative XOR delta between two equal-length buffers.
uint64_t atomik_region_delta(const uint64_t *a, const uint64_t *b, uint32_t n_words);

// --- Benchmark helper ---
// Returns cycle count for rdcycle CSR (64-bit)
static inline uint64_t cycles64(void) {
    uint64_t c;
    __asm__ volatile ("rdcycle %0" : "=r"(c));
    return c;
}

// 32-bit cycle count (for compatibility)
static inline uint32_t cycles(void) {
    return (uint32_t)cycles64();
}

#endif
