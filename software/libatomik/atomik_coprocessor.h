/*
 * atomik_coprocessor.h - ATOMiK RV64I Co-Processor Dispatch API
 *
 * Provides ARM-side control of the RV64I co-processor in Zynq PL:
 *   - Firmware loading into DDR3 (via /dev/mem mmap)
 *   - Mailbox register access (via /dev/uio1 or direct mmap)
 *   - CPU lifecycle: reset, run, halt, status polling
 *   - Command/response protocol for ARM→RV64I workload dispatch
 *
 * The co-processor has its own direct-wire ATOMiK N=1 core (custom
 * instructions, zero overhead). ARM dispatches workloads via the mailbox,
 * and the RV64I CPU processes them with native ATOMiK instructions.
 *
 * Memory map (Zynq PS view):
 *   0x43C00000  ATOMiK N=16 accelerator (Phase A, see libatomik.h)
 *   0x43C10000  Co-processor mailbox (Phase B, this API)
 *   0x10000000  DDR3 firmware region (256 MB, co-processor code + data)
 *
 * Usage:
 *   coproc_t *cp = coproc_open("/dev/uio1", "/dev/mem");
 *   coproc_load_firmware(cp, "firmware.bin");
 *   coproc_start(cp);
 *   coproc_send_cmd(cp, CMD_ACCUM_WORKLOAD, buf_phys, buf_size, 0);
 *   coproc_wait_resp(cp, &resp0, &resp1, 1000);
 *   coproc_stop(cp);
 *   coproc_close(cp);
 *
 * ATOMiK Project — March 2026
 * SPDX-License-Identifier: MIT
 */

#ifndef ATOMIK_COPROCESSOR_H
#define ATOMIK_COPROCESSOR_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Mailbox register map (matches atomik_mailbox.v)
 * ========================================================================= */

#define MBOX_REG_CTRL       0x00  /* [0]=run, [1]=halt request              */
#define MBOX_REG_STATUS     0x04  /* [0]=running,[1]=halted,[2]=cmd,[3]=resp*/
#define MBOX_REG_CMD        0x08  /* Command register (ARM writes)          */
#define MBOX_REG_ARG0       0x0C  /* Argument 0 (ARM writes)               */
#define MBOX_REG_ARG1       0x10  /* Argument 1 (ARM writes)               */
#define MBOX_REG_ARG2       0x14  /* Argument 2 (ARM writes)               */
#define MBOX_REG_RESP0      0x18  /* Response 0 (RV64I writes, ARM reads)  */
#define MBOX_REG_RESP1      0x1C  /* Response 1 (RV64I writes, ARM reads)  */
#define MBOX_REG_DEBUG_PC   0x20  /* RV64I program counter (read-only)     */
#define MBOX_REG_FW_ADDR    0x24  /* Firmware base address in DDR3         */
#define MBOX_REG_FW_SIZE    0x28  /* Firmware size in bytes                */
#define MBOX_REG_DOORBELL   0x2C  /* [0]=ARM→RV64I, [1]=RV64I→ARM         */

#define MBOX_REG_SPACE_SIZE 0x30  /* Total register space                  */
#define MBOX_MMAP_SIZE      0x1000 /* UIO page-aligned mmap               */

/* STATUS register field extraction */
#define MBOX_STATUS_RUNNING(s)     ((s) & 0x1)
#define MBOX_STATUS_HALTED(s)      (((s) >> 1) & 0x1)
#define MBOX_STATUS_CMD_PENDING(s) (((s) >> 2) & 0x1)
#define MBOX_STATUS_RESP_READY(s)  (((s) >> 3) & 0x1)

/* CTRL register bits */
#define MBOX_CTRL_RUN       (1 << 0)
#define MBOX_CTRL_HALT      (1 << 1)

/* Default firmware load address (physical DDR3) */
#define COPROC_DEFAULT_FW_ADDR  0x10000000
#define COPROC_DEFAULT_FW_SIZE  (16 * 1024 * 1024)  /* 16 MB reserved */

/* =========================================================================
 * Handle
 * ========================================================================= */

typedef struct {
    volatile uint32_t *mbox_regs;   /* mmap'd mailbox register base        */
    volatile uint8_t  *fw_region;   /* mmap'd DDR3 firmware region         */
    int                mbox_fd;     /* /dev/uioN file descriptor           */
    int                mem_fd;      /* /dev/mem file descriptor            */
    uint32_t           fw_phys;     /* Physical address of firmware region */
    uint32_t           fw_max_size; /* Maximum firmware size               */
    uint32_t           fw_loaded;   /* Actual loaded firmware size         */
} coproc_t;

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

/*
 * Open the co-processor interface.
 *
 * @param uio_dev   UIO device for mailbox, e.g. "/dev/uio1" (NULL for devmem)
 * @param mem_dev   Memory device for DDR3 access, e.g. "/dev/mem"
 * @param fw_phys   Physical address of firmware region (0 = default 0x10000000)
 * @param fw_size   Max firmware region size (0 = default 16 MB)
 * @return          Handle on success, NULL on failure (errno set)
 *
 * If uio_dev is NULL, mailbox is accessed via /dev/mem at 0x43C10000.
 */
coproc_t *coproc_open(const char *uio_dev, const char *mem_dev,
                      uint32_t fw_phys, uint32_t fw_size);

/*
 * Close the co-processor interface and release resources.
 * Stops the co-processor if running.
 *
 * @param cp  Handle from coproc_open() (NULL-safe)
 */
void coproc_close(coproc_t *cp);

/* =========================================================================
 * Firmware Loading
 * ========================================================================= */

/*
 * Load firmware binary into the DDR3 firmware region.
 * The co-processor must be stopped before loading firmware.
 *
 * @param cp        Handle
 * @param filename  Path to raw RV64I binary (output of objcopy -O binary)
 * @return          0 on success, -1 on failure (errno set)
 */
int coproc_load_firmware(coproc_t *cp, const char *filename);

/*
 * Load firmware from a memory buffer.
 *
 * @param cp    Handle
 * @param data  Pointer to firmware binary data
 * @param size  Size in bytes
 * @return      0 on success, -1 on failure
 */
int coproc_load_firmware_buf(coproc_t *cp, const void *data, size_t size);

/* =========================================================================
 * CPU Lifecycle
 * ========================================================================= */

/*
 * Start the co-processor (deassert reset).
 * Firmware must be loaded first. Sets CTRL[0] = 1.
 *
 * @param cp  Handle
 * @return    0 on success, -1 if no firmware loaded
 */
int coproc_start(coproc_t *cp);

/*
 * Stop the co-processor (assert reset).
 * Sets CTRL[0] = 0. CPU state is lost.
 *
 * @param cp  Handle
 */
void coproc_stop(coproc_t *cp);

/*
 * Check if the co-processor is running.
 *
 * @param cp  Handle
 * @return    1 if running (CTRL[0] = 1), 0 if stopped
 */
int coproc_is_running(coproc_t *cp);

/*
 * Read the co-processor's current program counter.
 * Useful for debugging firmware hangs.
 *
 * @param cp  Handle
 * @return    Current PC value (32-bit, lower bits of 64-bit PC)
 */
uint32_t coproc_read_pc(coproc_t *cp);

/* =========================================================================
 * Command / Response Protocol
 * ========================================================================= */

/*
 * Send a command to the co-processor.
 * Writes CMD, ARG0, ARG1, ARG2 registers. The CMD write sets the
 * cmd_pending flag, which the co-processor firmware polls.
 *
 * @param cp    Handle
 * @param cmd   Command code (application-defined)
 * @param arg0  Argument 0
 * @param arg1  Argument 1
 * @param arg2  Argument 2
 * @return      0 on success, -1 if previous command still pending
 */
int coproc_send_cmd(coproc_t *cp, uint32_t cmd,
                    uint32_t arg0, uint32_t arg1, uint32_t arg2);

/*
 * Wait for a response from the co-processor.
 * Polls the resp_ready status bit. Returns when the co-processor has
 * written RESP0 (which sets resp_ready). Reading RESP0 clears the flag.
 *
 * @param cp         Handle
 * @param resp0      Output: RESP0 value (NULL to ignore)
 * @param resp1      Output: RESP1 value (NULL to ignore)
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return           0 on success, -1 on timeout
 */
int coproc_wait_resp(coproc_t *cp, uint32_t *resp0, uint32_t *resp1,
                     uint32_t timeout_ms);

/*
 * Ring the doorbell (ARM→RV64I interrupt).
 * Sets DOORBELL[0], which triggers an interrupt to the co-processor.
 *
 * @param cp  Handle
 */
void coproc_doorbell(coproc_t *cp);

/* =========================================================================
 * Direct Register Access (for advanced use)
 * ========================================================================= */

/*
 * Read a mailbox register.
 *
 * @param cp      Handle
 * @param offset  Register offset (MBOX_REG_*)
 * @return        Register value
 */
static inline uint32_t coproc_reg_read(coproc_t *cp, uint32_t offset) {
    return cp->mbox_regs[offset >> 2];
}

/*
 * Write a mailbox register.
 *
 * @param cp      Handle
 * @param offset  Register offset (MBOX_REG_*)
 * @param value   Value to write
 */
static inline void coproc_reg_write(coproc_t *cp, uint32_t offset,
                                    uint32_t value) {
    cp->mbox_regs[offset >> 2] = value;
}

#ifdef __cplusplus
}
#endif

#endif /* ATOMIK_COPROCESSOR_H */
