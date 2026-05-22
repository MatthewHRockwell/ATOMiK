/* pcap_prog.c — minimal Zynq7000 PL bitstream programmer via PCAP/DEVCFG.
 * No Xilinx BSP required — raw register writes only.
 * Bitstream input must be a .bit.bin file (bootgen -process_bitstream bin),
 * which is already byteswapped for Zynq PCAP little-endian mode.
 *
 * Reference: UG585 Zynq-7000 TRM, Chapter 6 "Device Configuration".
 */

#include <stdint.h>

#define DEVCFG_BASE    0xF8007000u
#define PCFG_CTRL      (DEVCFG_BASE + 0x000u)
#define INT_STS        (DEVCFG_BASE + 0x00Cu)
#define INT_MASK       (DEVCFG_BASE + 0x010u)
#define STATUS         (DEVCFG_BASE + 0x014u)
#define DMA_SRC_ADDR   (DEVCFG_BASE + 0x018u)
#define DMA_DST_ADDR   (DEVCFG_BASE + 0x01Cu)
#define DMA_SRC_LEN    (DEVCFG_BASE + 0x020u)
#define DMA_DEST_LEN   (DEVCFG_BASE + 0x024u)
#define UNLOCK         (DEVCFG_BASE + 0x034u)
#define MCTRL          (DEVCFG_BASE + 0x080u)

/* PCFG_CTRL bits */
#define PCFG_CTRL_PCAP_PR      (1u << 27)  /* PCAP_MODE — select PCAP for cfg */
#define PCFG_CTRL_PCFG_PROG_B  (1u << 30)  /* PROG_B — pull low to reset FPGA */
#define PCFG_CTRL_QUARTER_PCAP (1u << 25)  /* Rate: 1=quarter, 0=full */

/* INT_STS / INT_MASK bits */
#define IXR_DMA_DONE   (1u << 13)
#define IXR_D_P_DONE   (1u << 12)
#define IXR_AXI_WERR   (1u <<  3)
#define IXR_AXI_RERR   (1u <<  2)
#define IXR_RX_FIFO_OV (1u << 18)
#define IXR_ERR_ALL    (IXR_AXI_WERR | IXR_AXI_RERR | IXR_RX_FIFO_OV)

/* STATUS bits */
#define STATUS_PCFG_INIT_B  (1u <<  4)   /* High when FPGA ready to accept cfg */
#define STATUS_PCFG_DONE    (1u <<  2)   /* High after successful config        */

/* MCTRL bits */
#define MCTRL_PCAP_LPBK     (1u <<  4)   /* Internal loopback — must be 0 for normal use */

static inline void w32(uint32_t a, uint32_t v) { *(volatile uint32_t *)(uintptr_t)a = v; }
static inline uint32_t r32(uint32_t a)         { return *(volatile uint32_t *)(uintptr_t)a; }
static void spin(volatile int n) { while (n-- > 0) {} }

/* Wait with a simple counter; ~10M iterations ≈ ~100ms at 100MHz. */
#define PCAP_TIMEOUT  10000000

/* Program the PL with the bitstream at `data` (must be 32-bit aligned).
 * `words` is the bitstream size in 32-bit words (= file_bytes / 4).
 * Returns 0 on success, non-zero on error.
 */
int pcap_program(const void *data, uint32_t words)
{
    volatile int i;

    /* Unlock DEVCFG registers */
    w32(UNLOCK, 0x757BDF0Du);

    /* Disable internal loopback */
    w32(MCTRL, r32(MCTRL) & ~MCTRL_PCAP_LPBK);

    /* Enable PCAP as configuration interface */
    w32(PCFG_CTRL, r32(PCFG_CTRL) | PCFG_CTRL_PCAP_PR);

    /* Assert PROG_B (active low) to reset FPGA configuration */
    w32(PCFG_CTRL, r32(PCFG_CTRL) & ~PCFG_CTRL_PCFG_PROG_B);
    spin(100000);  /* hold for a bit */

    /* Deassert PROG_B */
    w32(PCFG_CTRL, r32(PCFG_CTRL) | PCFG_CTRL_PCFG_PROG_B);

    /* Wait for INIT_B to go high (FPGA ready to accept configuration) */
    for (i = PCAP_TIMEOUT; i > 0; i--)
        if (r32(STATUS) & STATUS_PCFG_INIT_B) break;
    if (i == 0) return -1;  /* INIT_B never went high */

    /* Clear all interrupt status bits */
    w32(INT_STS, 0xFFFFFFFFu);

    /* Mask all error interrupts (we poll) */
    w32(INT_MASK, 0xFFFFFFFFu);

    /* Setup DMA transfer:
     *   src  = DDR address of bitstream | 0x1 (last-transfer bit)
     *   dst  = 0xFFFFFFFF (PCAP FIFO sink) | 0x1
     *   len  = words (32-bit word count)
     *   dlen = 0 (not used for PCAP write)
     */
    w32(DMA_SRC_ADDR, ((uint32_t)(uintptr_t)data) | 0x1u);
    w32(DMA_DST_ADDR, 0xFFFFFFFFu);  /* PCAP sink */
    w32(DMA_DEST_LEN, 0u);
    w32(DMA_SRC_LEN,  words);        /* triggers DMA */

    /* Wait for DMA done */
    for (i = PCAP_TIMEOUT; i > 0; i--)
        if (r32(INT_STS) & IXR_DMA_DONE) break;
    if (i == 0) return -2;

    /* Check for errors */
    if (r32(INT_STS) & IXR_ERR_ALL) return -3;

    /* Wait for PCFG_DONE — PL configuration complete */
    for (i = PCAP_TIMEOUT; i > 0; i--)
        if (r32(STATUS) & STATUS_PCFG_DONE) break;
    if (i == 0) return -4;

    return 0;
}
