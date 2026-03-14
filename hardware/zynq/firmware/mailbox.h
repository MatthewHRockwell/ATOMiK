/*
 * mailbox.h — Mailbox Register Access for RV64I Co-Processor Firmware
 *
 * Provides RV64I-side access to the mailbox registers for communication
 * with the ARM Cortex-A9. The mailbox is memory-mapped at 0x80000000
 * in the co-processor's address space.
 *
 * Protocol:
 *   1. ARM writes CMD + ARG0-2, sets cmd_pending flag
 *   2. RV64I polls cmd_pending (reads STATUS or CMD register)
 *   3. RV64I processes command, writes result to RESP0/RESP1
 *   4. Writing RESP0 sets resp_ready flag and triggers IRQ to ARM
 *   5. ARM reads RESP0 (clears resp_ready flag)
 *
 * ATOMiK Project — March 2026
 * SPDX-License-Identifier: MIT
 */

#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>

/* Mailbox base address (RV64I memory map) */
#define MBOX_BASE       ((volatile uint32_t *)0x80000000)

/* Register offsets (byte offsets, word-aligned) */
#define MBOX_CTRL       0   /* 0x00: Control — [0]=run, [1]=halt       */
#define MBOX_STATUS     1   /* 0x04: Status (read-only from ARM)       */
#define MBOX_CMD        2   /* 0x08: Command (ARM writes, RV64I reads) */
#define MBOX_ARG0       3   /* 0x0C: Argument 0                        */
#define MBOX_ARG1       4   /* 0x10: Argument 1                        */
#define MBOX_ARG2       5   /* 0x14: Argument 2                        */
#define MBOX_RESP0      6   /* 0x18: Response 0 (RV64I writes)         */
#define MBOX_RESP1      7   /* 0x1C: Response 1 (RV64I writes)         */
#define MBOX_DEBUG_PC   8   /* 0x20: Debug PC (read-only)              */
#define MBOX_FW_ADDR    9   /* 0x24: Firmware address                  */
#define MBOX_FW_SIZE   10   /* 0x28: Firmware size                     */
#define MBOX_DOORBELL  11   /* 0x2C: Doorbell IRQ                      */

/* STATUS register bits */
#define STATUS_RUNNING      (1 << 0)
#define STATUS_HALTED       (1 << 1)
#define STATUS_CMD_PENDING  (1 << 2)
#define STATUS_RESP_READY   (1 << 3)

/* Read a mailbox register */
static inline uint32_t mbox_read(int reg) {
    return MBOX_BASE[reg];
}

/* Write a mailbox register */
static inline void mbox_write(int reg, uint32_t val) {
    MBOX_BASE[reg] = val;
}

/*
 * Wait for a command from ARM.
 * Polls STATUS.cmd_pending until ARM writes CMD register.
 * Returns the command code and clears cmd_pending.
 */
static inline uint32_t mbox_wait_cmd(void) {
    /* Spin until cmd_pending is set */
    while (!(mbox_read(MBOX_STATUS) & STATUS_CMD_PENDING))
        ;
    /* Reading CMD clears cmd_pending */
    return mbox_read(MBOX_CMD);
}

/*
 * Send a response to ARM.
 * Writing RESP0 sets resp_ready and triggers irq_to_arm.
 */
static inline void mbox_send_resp(uint32_t resp0, uint32_t resp1) {
    mbox_write(MBOX_RESP1, resp1);
    mbox_write(MBOX_RESP0, resp0);  /* RESP0 write triggers IRQ */
}

#endif /* MAILBOX_H */
