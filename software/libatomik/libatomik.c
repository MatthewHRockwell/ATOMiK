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

static inline void reg_write(volatile uint32_t *regs, unsigned offset,
                              uint32_t value)
{
    regs[offset / 4] = value;
}

static inline uint32_t reg_read(volatile uint32_t *regs, unsigned offset)
{
    return regs[offset / 4];
}

/*
 * MMIO ordering barrier: ensures prior CSR writes complete on the Wishbone
 * bus before subsequent reads. Required on VexRiscv SMP — without this,
 * STATE_LO reads after LOAD/ACCUM return stale data.
 *
 * Uses: fence iorw,iorw + dummy STATUS read (bus round-trip).
 */
static inline void mmio_barrier(volatile uint32_t *regs, unsigned status_off)
{
#if defined(__riscv)
    __asm__ volatile("fence iorw, iorw" ::: "memory");
#else
    __sync_synchronize();
#endif
    (void)reg_read(regs, status_off);
}

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
    unsigned long page_base;
    unsigned long page_offset;

    a = calloc(1, sizeof(*a));
    if (!a)
        return NULL;

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
