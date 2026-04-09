/*
 * libatomik.c - ATOMiK UIO Userspace Library Implementation
 *
 * Provides memory-mapped access to ATOMiK delta-state hardware on Zynq
 * via the Linux UIO (Userspace I/O) framework.
 *
 * ATOMiK Project — March 2026
 * SPDX-License-Identifier: MIT
 */

#include "libatomik.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* =========================================================================
 * Internal helpers
 * ========================================================================= */

#ifdef ATOMIK_MOCK
/* ── Mock backend: in-memory state table, no hardware ──────────────────── */

static uint64_t mock_state_table[256];
static uint64_t mock_accumulator;
static uint8_t  mock_active_addr;
static uint64_t mock_snapshot;
static uint8_t  mock_load_addr;
static uint32_t mock_load_data_lo;
static uint32_t mock_accum_lo;
static uint8_t  mock_enable = 1;

/* Adapter mock state */
static uint32_t mock_adapter_rs1;
static uint32_t mock_adapter_rs2;
static uint32_t mock_adapter_rd;
static uint32_t mock_adapter_init_lo;
static uint32_t mock_adapter_delta_lo;

static void mock_reset(void)
{
    memset(mock_state_table, 0, sizeof(mock_state_table));
    mock_accumulator = 0;
    mock_active_addr = 0;
    mock_snapshot = 0;
    mock_load_addr = 0;
    mock_load_data_lo = 0;
    mock_accum_lo = 0;
    mock_enable = 1;
    mock_adapter_rs1 = 0;
    mock_adapter_rs2 = 0;
    mock_adapter_rd = 0;
    mock_adapter_init_lo = 0;
    mock_adapter_delta_lo = 0;
}

static uint64_t mock_current_state(void)
{
    return mock_state_table[mock_active_addr] ^ mock_accumulator;
}

static void mock_csr_write(unsigned offset, uint32_t value)
{
    switch (offset) {
    case 0x00: mock_load_addr = value & 0xFF; break;
    case 0x04: mock_load_data_lo = value; break;
    case 0x08: { /* LOAD_DATA_HI triggers LOAD */
        uint64_t init = ((uint64_t)value << 32) | mock_load_data_lo;
        mock_active_addr = mock_load_addr;
        mock_state_table[mock_active_addr] = init;
        mock_accumulator = 0;
        break;
    }
    case 0x0C: mock_accum_lo = value; break;
    case 0x10: { /* ACCUM_HI triggers ACCUM */
        uint64_t delta = ((uint64_t)value << 32) | mock_accum_lo;
        mock_accumulator ^= delta;
        break;
    }
    case 0x14: /* SWAP (CSR layout) */
        mock_state_table[mock_active_addr] = mock_current_state();
        mock_accumulator = 0;
        mock_active_addr = value & 0xFF;
        break;
    case 0x18: /* CONFIG */
        mock_enable = value & 1;
        if (!mock_enable) mock_accumulator = 0;
        break;
    default: break;
    }
}

static uint32_t mock_csr_read(unsigned offset)
{
    switch (offset) {
    case 0x1C: mock_snapshot = mock_current_state(); return (uint32_t)(mock_snapshot & 0xFFFFFFFF);
    case 0x20: return (uint32_t)(mock_snapshot >> 32);
    case 0x24: return (0x02 << 16) | (0x01 << 8) | ((mock_accumulator == 0) ? 1 : 0);
    case 0x18: return mock_enable;
    default: return 0;
    }
}

static void mock_adapter_write(unsigned offset, uint32_t value)
{
    switch (offset) {
    case 0x00: { /* CMD */
        uint8_t funct3 = value & 7;
        switch (funct3) {
        case 0: /* LOAD */ mock_active_addr = mock_adapter_rs1 & 0xFF; mock_adapter_init_lo = mock_adapter_rs2; break;
        case 4: /* LOAD_HI */ mock_state_table[mock_active_addr] = ((uint64_t)mock_adapter_rs2 << 32) | mock_adapter_init_lo; mock_accumulator = 0; break;
        case 1: /* ACCUM */ mock_adapter_delta_lo = mock_adapter_rs1; break;
        case 5: /* ACCUM_HI */ mock_accumulator ^= ((uint64_t)mock_adapter_rs1 << 32) | mock_adapter_delta_lo; break;
        case 2: /* READ */ mock_adapter_rd = (uint32_t)(mock_current_state() & 0xFFFFFFFF); break;
        case 6: /* READ_HI */ mock_adapter_rd = (uint32_t)(mock_current_state() >> 32); break;
        case 3: /* SWAP */ mock_active_addr = mock_adapter_rs1 & 0xFF; mock_state_table[mock_active_addr] = mock_current_state(); mock_accumulator = 0; break;
        case 7: /* STATUS */ mock_adapter_rd = (0x02 << 16) | (0x01 << 8) | ((mock_accumulator == 0) ? 1 : 0); break;
        }
        break;
    }
    case 0x04: mock_adapter_rs1 = value; break;
    case 0x08: mock_adapter_rs2 = value; break;
    default: break;
    }
}

static uint32_t mock_adapter_read(unsigned offset)
{
    switch (offset) {
    case 0x0C: return mock_adapter_rd;
    case 0x10: return 0x02; /* rsp_ok=1, busy=0 */
    case 0x04: return mock_adapter_rs1;
    case 0x08: return mock_adapter_rs2;
    default: return 0;
    }
}

/* Active mock layout — set by atomik_open_devmem */
static atomik_layout_t mock_active_layout = ATOMIK_LAYOUT_CSR;

static inline void reg_write(volatile uint32_t *regs, unsigned offset,
                              uint32_t value)
{
    (void)regs;
    if (mock_active_layout == ATOMIK_LAYOUT_ADAPTER)
        mock_adapter_write(offset, value);
    else
        mock_csr_write(offset, value);
}

static inline uint32_t reg_read(volatile uint32_t *regs, unsigned offset)
{
    (void)regs;
    if (mock_active_layout == ATOMIK_LAYOUT_ADAPTER)
        return mock_adapter_read(offset);
    else
        return mock_csr_read(offset);
}

static inline void mmio_barrier(volatile uint32_t *regs, unsigned status_off)
{
    (void)regs; (void)status_off; /* No-op in mock mode */
}

#else
/* ── Real hardware: MMIO register access ───────────────────────────────── */

static inline void reg_write(volatile uint32_t *regs, unsigned offset,
                              uint32_t value)
{
    regs[offset / 4] = value;
}

static inline uint32_t reg_read(volatile uint32_t *regs, unsigned offset)
{
    return regs[offset / 4];
}

static inline void mmio_barrier(volatile uint32_t *regs, unsigned status_off)
{
#if defined(__riscv)
    __asm__ volatile("fence iorw, iorw" ::: "memory");
#else
    __sync_synchronize();
#endif
    (void)reg_read(regs, status_off);
}

#endif /* ATOMIK_MOCK */

/* Forward declaration for adapter CMD-based STATUS query */
static uint32_t adapter_status_word(atomik_t *a);

static void resolve_layout(atomik_t *a, atomik_layout_t layout)
{
    a->layout = layout;
    if (layout == ATOMIK_LAYOUT_ADAPTER) {
        /* CMD-protocol adapter: offsets point to adapter registers */
        a->off_state_lo  = ATOMIK_ADAPTER_RD;     /* read result via RD */
        a->off_state_hi  = ATOMIK_ADAPTER_RD;     /* same register */
        a->off_status    = ATOMIK_ADAPTER_STATUS;
        a->off_swap_addr = ATOMIK_ADAPTER_RS1;    /* SWAP uses RS1 */
        a->off_config    = ATOMIK_ADAPTER_STATUS;  /* no config reg; read-only */
    } else if (layout == ATOMIK_LAYOUT_CSR) {
        a->off_state_lo  = ATOMIK_CSR_STATE_LO;
        a->off_state_hi  = ATOMIK_CSR_STATE_HI;
        a->off_status    = ATOMIK_CSR_STATUS;
        a->off_swap_addr = ATOMIK_CSR_SWAP_ADDR;
        a->off_config    = ATOMIK_CSR_CONFIG;
    } else {
        a->off_state_lo  = ATOMIK_AXI_STATE_LO;
        a->off_state_hi  = ATOMIK_AXI_STATE_HI;
        a->off_status    = ATOMIK_AXI_STATUS;
        a->off_swap_addr = ATOMIK_AXI_SWAP_ADDR;
        a->off_config    = ATOMIK_AXI_CONFIG;
    }
}

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

atomik_t *atomik_open(const char *uio_dev)
{
    atomik_t *a;
    uint32_t status;

    if (!uio_dev) {
        errno = EINVAL;
        return NULL;
    }

    a = calloc(1, sizeof(*a));
    if (!a)
        return NULL;

    a->fd = open(uio_dev, O_RDWR | O_SYNC);
    if (a->fd < 0) {
        free(a);
        return NULL;
    }

    a->regs = mmap(NULL, ATOMIK_MMAP_SIZE, PROT_READ | PROT_WRITE,
                   MAP_SHARED, a->fd, 0);
    if (a->regs == MAP_FAILED) {
        close(a->fd);
        free(a);
        return NULL;
    }

    /* UIO always uses AXI layout */
    resolve_layout(a, ATOMIK_LAYOUT_AXI);

    /* Read STATUS register to populate device info */
    status = reg_read(a->regs, a->off_status);
    a->n_banks = ATOMIK_STATUS_N_BANKS(status);
    a->version = ATOMIK_STATUS_VERSION(status);

    return a;
}

atomik_t *atomik_open_devmem(unsigned long phys_addr, atomik_layout_t layout)
{
    atomik_t *a;
    uint32_t status;

    a = calloc(1, sizeof(*a));
    if (!a)
        return NULL;

#ifdef ATOMIK_MOCK
    (void)phys_addr;
    mock_reset();
    mock_active_layout = layout;
    a->regs = mmap(NULL, ATOMIK_MMAP_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (a->regs == MAP_FAILED) {
        free(a);
        return NULL;
    }
    a->fd = -1;
#else
    unsigned long page_base;
    unsigned long page_offset;

    a->fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (a->fd < 0) {
        free(a);
        return NULL;
    }

    /* Align to page boundary */
    page_base = phys_addr & ~(unsigned long)(ATOMIK_MMAP_SIZE - 1);
    page_offset = phys_addr - page_base;

    void *map = mmap(NULL, ATOMIK_MMAP_SIZE, PROT_READ | PROT_WRITE,
                     MAP_SHARED, a->fd, page_base);
    if (map == MAP_FAILED) {
        close(a->fd);
        free(a);
        return NULL;
    }
    a->regs = (volatile uint32_t *)((char *)map + page_offset);
#endif

    resolve_layout(a, layout);

    /* Read STATUS to populate device info */
    if (layout == ATOMIK_LAYOUT_ADAPTER) {
        status = adapter_status_word(a);
    } else {
        status = reg_read(a->regs, a->off_status);
    }
    a->n_banks = ATOMIK_STATUS_N_BANKS(status);
    a->version = ATOMIK_STATUS_VERSION(status);

    return a;
}

void atomik_close(atomik_t *a)
{
    if (!a)
        return;

    if (a->regs && a->regs != MAP_FAILED)
        munmap((void *)a->regs, ATOMIK_MMAP_SIZE);

    if (a->fd >= 0)
        close(a->fd);

    free(a);
}

/* =========================================================================
 * ATOMiK Operations
 *
 * All 64-bit operations use the LO-then-HI pattern:
 *   1. Write the low 32 bits (latched in hardware)
 *   2. Write the high 32 bits (triggers the operation)
 *
 * This matches the atomik_axi4lite_wrapper.v register protocol.
 * ========================================================================= */

void atomik_load(atomik_t *a, uint8_t addr, uint64_t initial_state)
{
    if (a->layout == ATOMIK_LAYOUT_ADAPTER) {
        /* CMD protocol: RS1=addr, RS2=init_lo, CMD=0 → RS2=init_hi, CMD=4 */
        reg_write(a->regs, ATOMIK_ADAPTER_RS1, addr);
        reg_write(a->regs, ATOMIK_ADAPTER_RS2,
                  (uint32_t)(initial_state & 0xFFFFFFFF));
        reg_write(a->regs, ATOMIK_ADAPTER_CMD, 0);  /* F_LOAD */
        mmio_barrier(a->regs, a->off_status);
        reg_write(a->regs, ATOMIK_ADAPTER_RS2,
                  (uint32_t)(initial_state >> 32));
        reg_write(a->regs, ATOMIK_ADAPTER_CMD, 4);  /* F_LOAD_HI */
        mmio_barrier(a->regs, a->off_status);
    } else {
        reg_write(a->regs, ATOMIK_REG_LOAD_ADDR, addr);
        reg_write(a->regs, ATOMIK_REG_LOAD_DATA_LO,
                  (uint32_t)(initial_state & 0xFFFFFFFF));
        reg_write(a->regs, ATOMIK_REG_LOAD_DATA_HI,
                  (uint32_t)(initial_state >> 32));
        mmio_barrier(a->regs, a->off_status);
    }
}

void atomik_accum(atomik_t *a, uint64_t delta)
{
    if (a->layout == ATOMIK_LAYOUT_ADAPTER) {
        /* CMD protocol: RS1=delta_lo, CMD=1 → RS1=delta_hi, CMD=5 */
        reg_write(a->regs, ATOMIK_ADAPTER_RS1,
                  (uint32_t)(delta & 0xFFFFFFFF));
        reg_write(a->regs, ATOMIK_ADAPTER_CMD, 1);  /* F_ACCUM */
        mmio_barrier(a->regs, a->off_status);
        reg_write(a->regs, ATOMIK_ADAPTER_RS1,
                  (uint32_t)(delta >> 32));
        reg_write(a->regs, ATOMIK_ADAPTER_CMD, 5);  /* F_ACCUM_HI */
        mmio_barrier(a->regs, a->off_status);
    } else {
        reg_write(a->regs, ATOMIK_REG_ACCUM_LO,
                  (uint32_t)(delta & 0xFFFFFFFF));
        reg_write(a->regs, ATOMIK_REG_ACCUM_HI,
                  (uint32_t)(delta >> 32));
        mmio_barrier(a->regs, a->off_status);
    }
}

uint64_t atomik_read(atomik_t *a)
{
    uint32_t lo, hi;

    if (a->layout == ATOMIK_LAYOUT_ADAPTER) {
        /* CMD protocol: CMD=2 → read RD (lo), CMD=6 → read RD (hi) */
        reg_write(a->regs, ATOMIK_ADAPTER_CMD, 2);  /* F_READ */
        mmio_barrier(a->regs, a->off_status);
        lo = reg_read(a->regs, ATOMIK_ADAPTER_RD);
        reg_write(a->regs, ATOMIK_ADAPTER_CMD, 6);  /* F_READ_HI */
        mmio_barrier(a->regs, a->off_status);
        hi = reg_read(a->regs, ATOMIK_ADAPTER_RD);
    } else {
        lo = reg_read(a->regs, a->off_state_lo);
        hi = reg_read(a->regs, a->off_state_hi);
    }

    return ((uint64_t)hi << 32) | lo;
}

void atomik_swap(atomik_t *a, uint8_t addr)
{
    if (a->layout == ATOMIK_LAYOUT_ADAPTER) {
        /* CMD protocol: RS1=addr, CMD=3 */
        reg_write(a->regs, ATOMIK_ADAPTER_RS1, addr);
        reg_write(a->regs, ATOMIK_ADAPTER_CMD, 3);  /* F_SWAP */
    } else {
        reg_write(a->regs, a->off_swap_addr, addr);
    }
    mmio_barrier(a->regs, a->off_status);
}

/* =========================================================================
 * Status
 * ========================================================================= */

static uint32_t adapter_status_word(atomik_t *a)
{
    reg_write(a->regs, ATOMIK_ADAPTER_CMD, 7);  /* F_STATUS */
    mmio_barrier(a->regs, a->off_status);
    return reg_read(a->regs, ATOMIK_ADAPTER_RD);
}

int atomik_acc_zero(atomik_t *a)
{
    uint32_t status;
    if (a->layout == ATOMIK_LAYOUT_ADAPTER)
        status = adapter_status_word(a);
    else
        status = reg_read(a->regs, a->off_status);
    return ATOMIK_STATUS_ACC_ZERO(status);
}

uint8_t atomik_bank_count(atomik_t *a)
{
    return a->n_banks;
}

uint8_t atomik_version(atomik_t *a)
{
    return a->version;
}

int atomik_detect_changed(atomik_t *a, uint8_t addr, uint64_t expected_fp,
                           const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t i;

    /* Compute current fingerprint: XOR of all 8-byte chunks */
    uint64_t current_fp = 0;
    for (i = 0; i + 8 <= len; i += 8) {
        uint64_t chunk;
        memcpy(&chunk, p + i, 8);
        current_fp ^= chunk;
    }

    /* Use ATOMiK to compare: LOAD(0), ACCUM(expected), ACCUM(current).
     * If expected == current, acc = expected XOR current = 0 → acc_zero = 1.
     * If different, acc != 0 → acc_zero = 0. */
    atomik_load(a, addr, 0);
    atomik_accum(a, expected_fp);
    atomik_accum(a, current_fp);
    int changed = !atomik_acc_zero(a);

    /* Store current_fp as new reference for next call */
    atomik_load(a, addr, current_fp);

    return changed;
}

const char *atomik_version_string(void)
{
    return LIBATOMIK_VERSION_STRING;
}

void atomik_set_enable(atomik_t *a, int enable)
{
    if (a->layout == ATOMIK_LAYOUT_ADAPTER)
        return;  /* Adapter has no config register — always enabled */
    reg_write(a->regs, a->off_config, enable ? 1 : 0);
}

int atomik_is_enabled(atomik_t *a)
{
    if (a->layout == ATOMIK_LAYOUT_ADAPTER)
        return 1;  /* Adapter is always enabled */
    uint32_t config = reg_read(a->regs, a->off_config);
    return config & 1;
}
