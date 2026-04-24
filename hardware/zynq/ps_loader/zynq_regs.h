/* ============================================================================
 * ATOMiK PS Loader — Zynq-7000 register map (the subset we need)
 *
 * Sources: UG585 "Zynq-7000 SoC Technical Reference Manual" Appendix B.
 * ========================================================================= */

#ifndef ZYNQ_REGS_H
#define ZYNQ_REGS_H

#include <stdint.h>

/* -- SLCR (System Level Control Registers) -- */
#define SLCR_BASE           0xF8000000u
#define SLCR_UNLOCK         (SLCR_BASE + 0x008)   /* write 0xDF0D to unlock */
#define SLCR_LOCK           (SLCR_BASE + 0x004)   /* write 0x767B to lock   */
#define SLCR_APER_CLK_CTRL  (SLCR_BASE + 0x12C)   /* AMBA peripheral clocks */
#define SLCR_SDIO_CLK_CTRL  (SLCR_BASE + 0x150)   /* SDIO ref clock divisor */
#define SLCR_SDIO_RST_CTRL  (SLCR_BASE + 0x218)   /* SDIO reset             */
#define SLCR_MIO_PIN(n)     (SLCR_BASE + 0x700 + ((n) * 4))

/* MIO pin config value for SDIO1 function (per board memory):
 *   L3_SEL = SDIO, TRI off, pull-up enabled, LVCMOS18 disabled, fast CMOS.
 *   Value 0x1280 confirmed in ATOMiK memory notes.
 */
#define MIO_CFG_SDIO1       0x1280u

/* APER_CLK_CTRL bits */
#define APER_CLK_SDI0       (1u << 10)
#define APER_CLK_SDI1       (1u << 11)

/* SDIO_RST_CTRL bits */
#define SDIO_RST_CPU1       (1u << 1)
#define SDIO_RST_CPU0       (1u << 0)
#define SDIO_RST_REF1       (1u << 5)
#define SDIO_RST_REF0       (1u << 4)

/* SDIO_CLK_CTRL: selects IO PLL / ARM PLL / DDR PLL source + divisor.
 * For RK-ZYNQ7020-F IO PLL is 1000 MHz; we want 50 MHz SDIO reference,
 * so divisor = 20.  Source = 0 (IO PLL).  Enable CLKACT0/CLKACT1.
 *   [5:4]   SRCSEL   = 00 (IO PLL)
 *   [13:8]  DIVISOR0 = 20 (0x14) -> 1000/20 = 50 MHz ref for SDIO0
 *   [21:16] DIVISOR1 = 20 (0x14)                  for SDIO1
 *   [0]     CLKACT0  = 1
 *   [1]     CLKACT1  = 1
 * Computed: 0x00141403
 */
#define SDIO_CLK_CTRL_50MHz 0x00141403u

/* -- SDIO1 controller (Arasan SDHC 2.0) -- */
#define SD1_BASE            0xE0101000u
#define SD0_BASE            0xE0100000u

/* SDHC standard register offsets */
#define SDHC_DMA_ADDR       0x00
#define SDHC_BLOCK_SIZE     0x04  /* [15:0] */
#define SDHC_BLOCK_COUNT    0x06  /* [15:0] (same 32-bit word) */
#define SDHC_ARGUMENT       0x08
#define SDHC_TRANSFER_MODE  0x0C  /* [15:0] */
#define SDHC_COMMAND        0x0E  /* [15:0] (same 32-bit word as TRANS_MODE) */
#define SDHC_RESPONSE       0x10  /* 4 × 32-bit: RESP0..RESP3 */
#define SDHC_BUFFER_DATA    0x20
#define SDHC_PRESENT_STATE  0x24
#define SDHC_HOST_CTRL      0x28  /* byte at +0x28 */
#define SDHC_POWER_CTRL     0x29
#define SDHC_BLOCK_GAP      0x2A
#define SDHC_WAKEUP_CTRL    0x2B
#define SDHC_CLOCK_CTRL     0x2C  /* [15:0] */
#define SDHC_TIMEOUT_CTRL   0x2E
#define SDHC_SW_RESET       0x2F
#define SDHC_INT_STATUS     0x30
#define SDHC_INT_ENABLE     0x34
#define SDHC_INT_SIGNAL_EN  0x38
#define SDHC_CAPABILITIES   0x40
#define SDHC_HOST_VERSION   0xFE  /* [15:0] */

/* Accessors */
static inline void     w32(uint32_t a, uint32_t v) { *(volatile uint32_t *)(uintptr_t)a = v; }
static inline uint32_t r32(uint32_t a)             { return *(volatile uint32_t *)(uintptr_t)a; }
static inline void     w16(uint32_t a, uint16_t v) { *(volatile uint16_t *)(uintptr_t)a = v; }
static inline uint16_t r16(uint32_t a)             { return *(volatile uint16_t *)(uintptr_t)a; }
static inline void     w8 (uint32_t a, uint8_t  v) { *(volatile uint8_t  *)(uintptr_t)a = v; }
static inline uint8_t  r8 (uint32_t a)             { return *(volatile uint8_t  *)(uintptr_t)a; }

static inline void dsb(void) { __asm__ volatile ("dsb sy" ::: "memory"); }

#endif
