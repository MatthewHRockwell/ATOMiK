/* ============================================================================
 * Minimal SDHC 2.0 driver (polled, PIO) for Zynq-7000 SDIO1
 *
 * Enough to initialize an SD v2 / SDHC / SDXC card and read 512-byte blocks
 * via CMD17. No 4-bit mode switch, no high-speed mode (25 MHz init is plenty
 * for the Linux image load — 8 MB kernel / 30 MB cpio at 25 MHz × 512 B/block
 * is a few seconds).
 * ========================================================================= */

#ifndef SDHC_H
#define SDHC_H

#include <stdint.h>
#include "zynq_regs.h"

/* ---- SDHC register bitfields we actually use ---- */
#define SW_RESET_ALL          0x01u
#define SW_RESET_CMD          0x02u
#define SW_RESET_DAT          0x04u

#define CLK_INTCLK_EN         (1u << 0)
#define CLK_INTCLK_STABLE     (1u << 1)
#define CLK_SDCLK_EN          (1u << 2)
/* 10-bit divisor mapping for SDHC 2.0:
 *   bits [15:8]  = divisor[7:0]
 *   bits [7:6]   = divisor[9:8] (extended, for SDHC 3.0 — we don't need it)
 * Divisor value N means base clock / (2 * N). N=0 means base clock unchanged.
 *   Base = 50 MHz → N=63 → ≈ 396 kHz (SD init requires <400 kHz).
 *   N=1 → 25 MHz.
 */
#define CLK_DIV_SHIFT         8

#define PWR_ON                0x01u
#define PWR_33V               0x0Eu            /* bits [3:1] = 111 (3.3V) */

#define HC_HSPEED             0x04u
#define HC_WIDTH4             0x02u

#define PRES_CMD_INHIBIT_CMD  (1u << 0)
#define PRES_CMD_INHIBIT_DAT  (1u << 1)
#define PRES_BUF_READ_RDY     (1u << 11)
#define PRES_BUF_WRITE_RDY    (1u << 10)
#define PRES_DAT0             (1u << 20)

#define INT_CMD_COMPLETE      (1u << 0)
#define INT_XFER_COMPLETE     (1u << 1)
#define INT_BUF_READ_RDY      (1u << 5)
#define INT_CARD_INSERTION    (1u << 6)
#define INT_CARD_REMOVAL      (1u << 7)
#define INT_ERR_MASK          0xFFFF0000u
#define INT_ERR_BIT           (1u << 15)

#define CMD_IDX(n)            ((uint16_t)((n) & 0x3F) << 8)
#define CMD_TYPE_NORMAL       (0u << 6)
#define CMD_CRC_CHECK         (1u << 3)
#define CMD_INDEX_CHECK       (1u << 4)
#define CMD_DATA_PRESENT      (1u << 5)
#define CMD_RESP_NONE         (0u)
#define CMD_RESP_136          (1u)             /* R2                 */
#define CMD_RESP_48           (2u)             /* R1 / R3 / R6 / R7  */
#define CMD_RESP_48_BUSY      (3u)             /* R1b                */

/* Transfer mode bits (low half of TRANSFER_MODE/COMMAND 32-bit word) */
#define TM_DMA_EN             (1u << 0)
#define TM_BLK_CNT_EN         (1u << 1)
#define TM_AUTO_CMD12         (1u << 2)
#define TM_READ               (1u << 4)
#define TM_MULTI_BLOCK        (1u << 5)

/* ---- status ---- */
typedef enum {
    SDHC_OK          = 0,
    SDHC_ETIMEOUT    = -1,
    SDHC_ECMDERR     = -2,
    SDHC_EBADCARD    = -3,
    SDHC_ENOSUPPORT  = -4,
    SDHC_EDATAERR    = -5,
    SDHC_ENOCARD     = -6,
} sdhc_err_t;

typedef struct {
    uint32_t base;         /* controller base (SD0_BASE or SD1_BASE)      */
    uint32_t rca;          /* 16-bit Relative Card Address, upper half of ARG */
    uint32_t ocr;
    uint8_t  sdhc;         /* 1 = high-capacity (block-addressed)         */
    uint8_t  version2;     /* 1 = SD v2 or later (passed CMD8)            */
    uint8_t  cid[16];
} sd_card_t;

/* ---- public API ---- */
sdhc_err_t sdhc_controller_init(sd_card_t *c, uint32_t base);
sdhc_err_t sdhc_card_init      (sd_card_t *c);
sdhc_err_t sdhc_read_block     (sd_card_t *c, uint32_t lba, void *dst_512);

#endif
