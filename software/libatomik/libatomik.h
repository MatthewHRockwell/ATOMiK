/*
 * libatomik.h - ATOMiK UIO Userspace Library
 *
 * Provides memory-mapped access to ATOMiK delta-state hardware on Zynq
 * via the Linux UIO (Userspace I/O) framework.
 *
 * The library handles the 32-bit AXI <-> 64-bit ATOMiK datapath splitting
 * internally. Software writes the low 32 bits first, then the high 32 bits,
 * which triggers the hardware operation.
 *
 * Usage:
 *   atomik_t *a = atomik_open("/dev/uio0");
 *   atomik_load(a, 0, 0xDEADBEEFCAFEBABE);
 *   atomik_accum(a, 0x00000000000000FF);
 *   uint64_t state = atomik_read(a);
 *   atomik_close(a);
 *
 * ATOMiK Project — March 2026
 * SPDX-License-Identifier: MIT
 */

#ifndef LIBATOMIK_H
#define LIBATOMIK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Register map (matches atomik_axi4lite_wrapper.v)
 * ========================================================================= */

#define ATOMIK_REG_LOAD_ADDR      0x00  /* (W)   Address for next LOAD [7:0]        */
#define ATOMIK_REG_LOAD_DATA_LO   0x04  /* (W)   Initial state bits [31:0]          */
#define ATOMIK_REG_LOAD_DATA_HI   0x08  /* (W)   Initial state bits [63:32], trigger*/
#define ATOMIK_REG_ACCUM_LO       0x0C  /* (W)   Delta bits [31:0]                  */
#define ATOMIK_REG_ACCUM_HI       0x10  /* (W)   Delta bits [63:32], trigger        */
#define ATOMIK_REG_STATE_LO       0x14  /* (R)   Current state [31:0], latches snap */
#define ATOMIK_REG_STATE_HI       0x18  /* (R)   Current state [63:32] from latch   */
#define ATOMIK_REG_STATUS         0x1C  /* (R)   {ver[15:8], banks[7:0], acc_zero}  */
#define ATOMIK_REG_SWAP_ADDR      0x20  /* (W)   Address + trigger SWAP [7:0]       */
#define ATOMIK_REG_CONFIG         0x24  /* (R/W) Bit 0 = enable (resets to 1)       */

#define ATOMIK_REG_SPACE_SIZE     0x28  /* Total register space (10 registers)      */
#define ATOMIK_MMAP_SIZE          0x1000 /* UIO maps at page granularity            */

/* STATUS register field extraction */
#define ATOMIK_STATUS_ACC_ZERO(s)   ((s) & 0x1)
#define ATOMIK_STATUS_N_BANKS(s)    (((s) >> 8) & 0xFF)
#define ATOMIK_STATUS_VERSION(s)    (((s) >> 16) & 0xFF)

/* =========================================================================
 * Handle
 * ========================================================================= */

typedef struct {
    volatile uint32_t *regs;    /* mmap'd register base                     */
    int                fd;      /* /dev/uioN file descriptor                */
    uint8_t            n_banks; /* Number of parallel banks (from STATUS)   */
    uint8_t            version; /* Hardware version (from STATUS)           */
} atomik_t;

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

/*
 * Open an ATOMiK device via UIO.
 *
 * @param uio_dev  UIO device path, e.g. "/dev/uio0"
 * @return         Handle on success, NULL on failure (errno set)
 */
atomik_t *atomik_open(const char *uio_dev);

/*
 * Close an ATOMiK device and release resources.
 *
 * @param a  Handle from atomik_open() (NULL-safe)
 */
void atomik_close(atomik_t *a);

/* =========================================================================
 * ATOMiK Operations (match the delta-state algebra)
 * ========================================================================= */

/*
 * LOAD: Initialize a context address with an initial state.
 * Sets the active address, writes initial_state to the BSRAM state table,
 * and clears the accumulator.
 *
 * @param a              Handle
 * @param addr           Context address (0-255)
 * @param initial_state  64-bit initial state value
 */
void atomik_load(atomik_t *a, uint8_t addr, uint64_t initial_state);

/*
 * ACCUM: Accumulate a delta into the current context.
 * Performs acc = acc XOR delta.
 *
 * @param a      Handle
 * @param delta  64-bit delta value to accumulate
 */
void atomik_accum(atomik_t *a, uint64_t delta);

/*
 * READ: Read the current reconstructed state.
 * Returns initial_state XOR accumulator for the active context.
 * The read latches a 64-bit atomic snapshot (LO read triggers latch).
 *
 * @param a  Handle
 * @return   64-bit current state
 */
uint64_t atomik_read(atomik_t *a);

/*
 * SWAP: Switch to a different context address.
 * The accumulator is preserved across the swap (unlike LOAD which clears it).
 *
 * @param a     Handle
 * @param addr  Context address to switch to (0-255)
 */
void atomik_swap(atomik_t *a, uint8_t addr);

/* =========================================================================
 * Status
 * ========================================================================= */

/*
 * Check if the accumulator is zero (no deltas accumulated, or deltas cancel).
 *
 * @param a  Handle
 * @return   1 if accumulator is zero, 0 otherwise
 */
int atomik_acc_zero(atomik_t *a);

/*
 * Get the number of parallel accumulator banks.
 *
 * @param a  Handle
 * @return   Number of banks (1 for Phase 1)
 */
uint8_t atomik_bank_count(atomik_t *a);

/*
 * Get the hardware version.
 *
 * @param a  Handle
 * @return   Version byte
 */
uint8_t atomik_version(atomik_t *a);

/*
 * Enable or disable the ATOMiK core.
 * Disabling asserts reset on the core (accumulator cleared on re-enable).
 *
 * @param a       Handle
 * @param enable  1 to enable, 0 to disable (reset)
 */
void atomik_set_enable(atomik_t *a, int enable);

/*
 * Check if the ATOMiK core is enabled.
 *
 * @param a  Handle
 * @return   1 if enabled, 0 if disabled
 */
int atomik_is_enabled(atomik_t *a);

#ifdef __cplusplus
}
#endif

#endif /* LIBATOMIK_H */
