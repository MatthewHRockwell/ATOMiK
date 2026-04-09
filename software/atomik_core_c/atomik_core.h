/*
 * atomik_core.h — ATOMiK delta-state algebra, single-header C library
 *
 * Pure software implementation of the ATOMiK instruction set.
 * Zero dependencies beyond <stdint.h> and <string.h>.
 * Works on any C99+ compiler: GCC, Clang, MSVC, ARM, RISC-V, etc.
 *
 * Usage:
 *   #define ATOMIK_IMPLEMENTATION   // in exactly ONE .c file
 *   #include "atomik_core.h"
 *
 * Quick start:
 *   atomik_ctx_t ctx;
 *   atomik_init(&ctx);
 *   atomik_load(&ctx, 0xDEADBEEFCAFEBABEULL);
 *   atomik_accum(&ctx, 0x00000000000000FFULL);
 *   uint64_t state = atomik_read(&ctx);
 *   // state == 0xDEADBEEFCAFEBA41
 *
 * Multi-context table:
 *   atomik_table_t table;
 *   atomik_table_init(&table, 256);
 *   atomik_table_load(&table, 0, 0xCAFEBABE);
 *   atomik_table_accum(&table, 0, 0x00000001);
 *   uint64_t s = atomik_table_read(&table, 0);  // 0xCAFEBABF
 *   atomik_table_free(&table);
 *
 * Fingerprinting:
 *   atomik_fingerprint_t fp;
 *   atomik_fp_init(&fp);
 *   atomik_fp_load(&fp, data, len);
 *   atomik_fp_update(&fp, new_data, len);
 *   if (atomik_fp_changed(&fp)) { ... }
 *
 * Backed by 108 Lean4 theorems. Hardware-validated on Gowin + Xilinx.
 *
 * SPDX-License-Identifier: Apache-2.0
 * ATOMiK Project — 2026
 */

#ifndef ATOMIK_CORE_H
#define ATOMIK_CORE_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Single Context
 * ========================================================================= */

typedef struct {
    uint64_t reference;
    uint64_t accumulator;
    uint32_t delta_count;
} atomik_ctx_t;

static inline void atomik_init(atomik_ctx_t *ctx) {
    ctx->reference = 0;
    ctx->accumulator = 0;
    ctx->delta_count = 0;
}

static inline void atomik_load(atomik_ctx_t *ctx, uint64_t value) {
    ctx->reference = value;
    ctx->accumulator = 0;
    ctx->delta_count = 0;
}

static inline void atomik_accum(atomik_ctx_t *ctx, uint64_t delta) {
    ctx->accumulator ^= delta;
    ctx->delta_count++;
}

static inline uint64_t atomik_read(const atomik_ctx_t *ctx) {
    return ctx->reference ^ ctx->accumulator;
}

static inline uint64_t atomik_swap(atomik_ctx_t *ctx) {
    uint64_t current = ctx->reference ^ ctx->accumulator;
    ctx->reference = current;
    ctx->accumulator = 0;
    ctx->delta_count = 0;
    return current;
}

static inline int atomik_is_clean(const atomik_ctx_t *ctx) {
    return ctx->accumulator == 0;
}

static inline void atomik_merge(atomik_ctx_t *dst, const atomik_ctx_t *src) {
    dst->accumulator ^= src->accumulator;
    dst->delta_count += src->delta_count;
}

/* =========================================================================
 * Multi-Context Table
 * ========================================================================= */

#ifndef ATOMIK_TABLE_STATIC_SIZE
#define ATOMIK_TABLE_STATIC_SIZE 0
#endif

typedef struct {
    atomik_ctx_t *contexts;
    uint32_t num_contexts;
#if ATOMIK_TABLE_STATIC_SIZE > 0
    atomik_ctx_t _static_buf[ATOMIK_TABLE_STATIC_SIZE];
#endif
} atomik_table_t;

/* Forward declarations — implemented below or in ATOMIK_IMPLEMENTATION block */
static inline void atomik_table_load(atomik_table_t *t, uint32_t addr, uint64_t value);
static inline void atomik_table_accum(atomik_table_t *t, uint32_t addr, uint64_t delta);
static inline uint64_t atomik_table_read(const atomik_table_t *t, uint32_t addr);
static inline uint64_t atomik_table_swap(atomik_table_t *t, uint32_t addr);

static inline void atomik_table_load(atomik_table_t *t, uint32_t addr, uint64_t value) {
    if (addr < t->num_contexts) atomik_load(&t->contexts[addr], value);
}

static inline void atomik_table_accum(atomik_table_t *t, uint32_t addr, uint64_t delta) {
    if (addr < t->num_contexts) atomik_accum(&t->contexts[addr], delta);
}

static inline uint64_t atomik_table_read(const atomik_table_t *t, uint32_t addr) {
    if (addr < t->num_contexts) return atomik_read(&t->contexts[addr]);
    return 0;
}

static inline uint64_t atomik_table_swap(atomik_table_t *t, uint32_t addr) {
    if (addr < t->num_contexts) return atomik_swap(&t->contexts[addr]);
    return 0;
}

/* =========================================================================
 * Fingerprinting (XOR-reduce change detection)
 * ========================================================================= */

typedef struct {
    uint64_t reference;
    uint64_t accumulator;
} atomik_fingerprint_t;

static inline void atomik_fp_init(atomik_fingerprint_t *fp) {
    fp->reference = 0;
    fp->accumulator = 0;
}

static inline uint64_t atomik_fp_reduce(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint64_t result = 0;
    size_t i = 0;
    /* Process 8 bytes at a time */
    for (; i + 8 <= len; i += 8) {
        uint64_t chunk;
        memcpy(&chunk, p + i, 8);
        result ^= chunk;
    }
    /* Handle remaining bytes */
    if (i < len) {
        uint64_t chunk = 0;
        memcpy(&chunk, p + i, len - i);
        result ^= chunk;
    }
    return result;
}

static inline void atomik_fp_load(atomik_fingerprint_t *fp, const void *data, size_t len) {
    fp->reference = atomik_fp_reduce(data, len);
    fp->accumulator = 0;
}

static inline void atomik_fp_update(atomik_fingerprint_t *fp, const void *data, size_t len) {
    uint64_t current = atomik_fp_reduce(data, len);
    fp->accumulator = fp->reference ^ current;
}

static inline int atomik_fp_changed(const atomik_fingerprint_t *fp) {
    return fp->accumulator != 0;
}

static inline void atomik_fp_accum_delta(atomik_fingerprint_t *fp,
                                          uint64_t old_val, uint64_t new_val) {
    fp->accumulator ^= (old_val ^ new_val);
}

#ifdef __cplusplus
}
#endif

/* =========================================================================
 * Implementation (include in exactly one .c file)
 * ========================================================================= */

#ifdef ATOMIK_IMPLEMENTATION

#include <stdlib.h>

int atomik_table_init(atomik_table_t *t, uint32_t num_contexts) {
#if ATOMIK_TABLE_STATIC_SIZE > 0
    if (num_contexts <= ATOMIK_TABLE_STATIC_SIZE) {
        t->contexts = t->_static_buf;
    } else {
        t->contexts = (atomik_ctx_t *)calloc(num_contexts, sizeof(atomik_ctx_t));
        if (!t->contexts) return -1;
    }
#else
    t->contexts = (atomik_ctx_t *)calloc(num_contexts, sizeof(atomik_ctx_t));
    if (!t->contexts) return -1;
#endif
    t->num_contexts = num_contexts;
    return 0;
}

void atomik_table_free(atomik_table_t *t) {
#if ATOMIK_TABLE_STATIC_SIZE > 0
    if (t->contexts != t->_static_buf)
#endif
    {
        free(t->contexts);
    }
    t->contexts = NULL;
    t->num_contexts = 0;
}

#endif /* ATOMIK_IMPLEMENTATION */

#endif /* ATOMIK_CORE_H */
