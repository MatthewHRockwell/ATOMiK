// ATOMiK Tracked Memory Operations — Implementation
#include "atomik_mem.h"
#include "atomik.h"

// GCC may auto-generate calls to memset/memcpy, so provide them
void *memset(void *dst, int val, uint32_t n);
void *memcpy(void *dst, const void *src, uint32_t n);

// --- Software baselines (byte-level) ---

void *sw_memcpy(void *dst, const void *src, uint32_t n)
{
    return memcpy(dst, src, n);
}

void *memset(void *dst, int val, uint32_t n)
{
    uint8_t *d = (uint8_t *)dst;
    for (uint32_t i = 0; i < n; i++)
        d[i] = (uint8_t)val;
    return dst;
}

void *memcpy(void *dst, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (uint32_t i = 0; i < n; i++)
        d[i] = s[i];
    return dst;
}

void *sw_memset(void *dst, int val, uint32_t n)
{
    return memset(dst, val, n);
}

int sw_memcmp(const void *a, const void *b, uint32_t n)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    for (uint32_t i = 0; i < n; i++) {
        if (pa[i] != pb[i])
            return (int)pa[i] - (int)pb[i];
    }
    return 0;
}

// --- ATOMiK-tracked variants ---

void *atomik_memcpy_tracked(void *dst, const void *src, uint32_t n, uint32_t *fp_out)
{
    const uint32_t *s = (const uint32_t *)src;
    uint32_t *d = (uint32_t *)dst;
    uint32_t n_words = n >> 2;

    // Load zero into accumulator — fingerprint starts at 0
    atomik_load(0, 0);

    for (uint32_t i = 0; i < n_words; i++) {
        uint32_t w = s[i];
        d[i] = w;
        atomik_accumulate(0, w);  // XOR word into fingerprint
    }

    if (fp_out)
        *fp_out = atomik_state(0);
    return dst;
}

void *atomik_memset_verified(void *dst, int val, uint32_t n, uint32_t *fp_out)
{
    uint32_t *d = (uint32_t *)dst;
    uint32_t n_words = n >> 2;

    // Build a word from the byte fill value
    uint32_t fill = (uint8_t)val;
    fill = fill | (fill << 8) | (fill << 16) | (fill << 24);

    atomik_load(0, 0);

    for (uint32_t i = 0; i < n_words; i++) {
        d[i] = fill;
        atomik_accumulate(0, fill);
    }

    if (fp_out)
        *fp_out = atomik_state(0);
    return dst;
}

int atomik_region_changed(const uint32_t *buf, uint32_t n_words, uint32_t saved_fp)
{
    uint32_t current_fp = atomik_fingerprint(0, buf, n_words);
    return current_fp != saved_fp;
}

uint32_t atomik_region_delta(const uint32_t *a, const uint32_t *b, uint32_t n_words)
{
    atomik_load(0, 0);
    for (uint32_t i = 0; i < n_words; i++)
        atomik_accumulate(0, a[i] ^ b[i]);
    return atomik_state(0);
}
