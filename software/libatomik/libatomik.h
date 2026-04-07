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
 * Register maps
 *
 * Two layouts exist depending on the bus path:
 *   AXI4-Lite wrapper (ARM GP0 at 0x43C00000) — atomik_axi4lite_wrapper.v
 *   LiteX Wishbone CSR (VexRiscv at 0xF0000000) — LiteX Migen CSR module
 *
 * Both share offsets 0x00–0x10. They differ at 0x14+.
 * The layout is selected at open time.
 * ========================================================================= */

/* --- Common registers (identical in both layouts) --- */
#define ATOMIK_REG_LOAD_ADDR      0x00  /* (W)   Address for next LOAD [7:0]        */
#define ATOMIK_REG_LOAD_DATA_LO   0x04  /* (W)   Initial state bits [31:0]          */
#define ATOMIK_REG_LOAD_DATA_HI   0x08  /* (W)   Initial state bits [63:32], trigger*/
#define ATOMIK_REG_ACCUM_LO       0x0C  /* (W)   Delta bits [31:0]                  */
#define ATOMIK_REG_ACCUM_HI       0x10  /* (W)   Delta bits [63:32], trigger        */

/* --- AXI4-Lite layout (atomik_axi4lite_wrapper.v) --- */
#define ATOMIK_AXI_STATE_LO       0x14
#define ATOMIK_AXI_STATE_HI       0x18
#define ATOMIK_AXI_STATUS         0x1C
#define ATOMIK_AXI_SWAP_ADDR      0x20
#define ATOMIK_AXI_CONFIG         0x24

/* --- LiteX Wishbone CSR layout (validated on Zynq, 16/16 PASS) --- */
#define ATOMIK_CSR_SWAP_ADDR      0x14
#define ATOMIK_CSR_CONFIG         0x18
#define ATOMIK_CSR_STATE_LO       0x1C
#define ATOMIK_CSR_STATE_HI       0x20
#define ATOMIK_CSR_STATUS         0x24

/* Default to AXI layout for backward compat; CSR layout used by atomik_open_devmem() */
#define ATOMIK_REG_STATE_LO       ATOMIK_AXI_STATE_LO
#define ATOMIK_REG_STATE_HI       ATOMIK_AXI_STATE_HI
#define ATOMIK_REG_STATUS         ATOMIK_AXI_STATUS
#define ATOMIK_REG_SWAP_ADDR      ATOMIK_AXI_SWAP_ADDR
#define ATOMIK_REG_CONFIG         ATOMIK_AXI_CONFIG

#define ATOMIK_REG_SPACE_SIZE     0x28  /* Total register space (10 registers)      */
#define ATOMIK_MMAP_SIZE          0x1000 /* Maps at page granularity                */

/* STATUS register field extraction */
#define ATOMIK_STATUS_ACC_ZERO(s)   ((s) & 0x1)
#define ATOMIK_STATUS_N_BANKS(s)    (((s) >> 8) & 0xFF)
#define ATOMIK_STATUS_VERSION(s)    (((s) >> 16) & 0xFF)

/* =========================================================================
 * Handle
 * ========================================================================= */

typedef enum {
    ATOMIK_LAYOUT_AXI,  /* AXI4-Lite wrapper (ARM GP0 path) */
    ATOMIK_LAYOUT_CSR,  /* LiteX Wishbone CSR (VexRiscv path) */
} atomik_layout_t;

typedef struct {
    volatile uint32_t *regs;    /* mmap'd register base                     */
    int                fd;      /* file descriptor (/dev/uioN or /dev/mem)  */
    uint8_t            n_banks; /* Number of parallel banks (from STATUS)   */
    uint8_t            version; /* Hardware version (from STATUS)           */
    atomik_layout_t    layout;  /* Register layout variant                  */
    /* Per-layout register offsets (resolved at open time) */
    uint8_t            off_state_lo;
    uint8_t            off_state_hi;
    uint8_t            off_status;
    uint8_t            off_swap_addr;
    uint8_t            off_config;
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
 * Open ATOMiK via /dev/mem at a known physical address.
 * Use this when no UIO driver is available (e.g., early bringup).
 * Includes MMIO ordering fences for Wishbone bus correctness.
 *
 * @param phys_addr  Physical base address of ATOMiK CSRs (e.g., 0xF0000000)
 * @param layout     Register layout (ATOMIK_LAYOUT_AXI or ATOMIK_LAYOUT_CSR)
 * @return           Handle on success, NULL on failure (errno set)
 */
atomik_t *atomik_open_devmem(unsigned long phys_addr, atomik_layout_t layout);

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
