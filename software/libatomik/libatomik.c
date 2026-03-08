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

    /* Read STATUS register to populate device info */
    status = reg_read(a->regs, ATOMIK_REG_STATUS);
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
    /* Set context address */
    reg_write(a->regs, ATOMIK_REG_LOAD_ADDR, addr);

    /* Write initial state: LO first, HI triggers LOAD */
    reg_write(a->regs, ATOMIK_REG_LOAD_DATA_LO,
              (uint32_t)(initial_state & 0xFFFFFFFF));
    reg_write(a->regs, ATOMIK_REG_LOAD_DATA_HI,
              (uint32_t)(initial_state >> 32));
}

void atomik_accum(atomik_t *a, uint64_t delta)
{
    /* Write delta: LO first, HI triggers ACCUM */
    reg_write(a->regs, ATOMIK_REG_ACCUM_LO,
              (uint32_t)(delta & 0xFFFFFFFF));
    reg_write(a->regs, ATOMIK_REG_ACCUM_HI,
              (uint32_t)(delta >> 32));
}

uint64_t atomik_read(atomik_t *a)
{
    uint32_t lo, hi;

    /*
     * Read LO first — this latches the full 64-bit snapshot in hardware.
     * Then read HI — returns the upper half from the latched value.
     * This guarantees atomic 64-bit reads over a 32-bit bus.
     */
    lo = reg_read(a->regs, ATOMIK_REG_STATE_LO);
    hi = reg_read(a->regs, ATOMIK_REG_STATE_HI);

    return ((uint64_t)hi << 32) | lo;
}

void atomik_swap(atomik_t *a, uint8_t addr)
{
    /* Write triggers SWAP immediately */
    reg_write(a->regs, ATOMIK_REG_SWAP_ADDR, addr);
}

/* =========================================================================
 * Status
 * ========================================================================= */

int atomik_acc_zero(atomik_t *a)
{
    uint32_t status = reg_read(a->regs, ATOMIK_REG_STATUS);
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
    reg_write(a->regs, ATOMIK_REG_CONFIG, enable ? 1 : 0);
}

int atomik_is_enabled(atomik_t *a)
{
    uint32_t config = reg_read(a->regs, ATOMIK_REG_CONFIG);
    return config & 1;
}
