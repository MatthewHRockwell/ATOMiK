/* pcap_prog.c — Zynq7000 PL bitstream programmer via PCAP/DEVCFG.
 *
 * Bitstream input must be a .bit.bin (bootgen -process_bitstream bin), already
 * byteswapped for PCAP little-endian mode.
 *
 * Flow follows Linux drivers/fpga/zynq-fpga.c:
 *   1. SLCR unlock + assert PL fabric resets (FPGA_RST_CTRL = 0xF)
 *   2. DEVCFG unlock, disable PCAP loopback
 *   3. Set PCAP_PR + PCAP_MODE in CTRL
 *   4. Pulse PROG_B (assert/wait-INIT_B-low/deassert/wait-INIT_B-high)
 *   5. Re-set PCAP_PR + PCAP_MODE (defensive — PROG_B reset can clear them)
 *   6. Clear sticky INT_STS error bits
 *   7. Wait DMA queue empty
 *   8. DSB to flush ARM store buffer (bitstream bytes must be in DDR not SB)
 *   9. Setup DMA registers
 *  10. Wait for D_P_DONE (DMA + PCAP both done)
 *  11. Wait for PCFG_DONE
 *  12. Deassert PL fabric resets
 */

#include <stdint.h>

/* DEVCFG */
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

/* SLCR */
#define SLCR_UNLOCK         0xF8000008u
#define SLCR_LOCK           0xF8000004u
#define SLCR_FPGA_RST_CTRL  0xF8000240u

#define SLCR_UNLOCK_KEY     0xDF0Du
#define SLCR_LOCK_KEY       0x767Bu
#define DEVCFG_UNLOCK_KEY   0x757BDF0Du

/* PCFG_CTRL bits */
#define PCFG_CTRL_PCAP_PR      (1u << 27)
#define PCFG_CTRL_PCAP_MODE    (1u << 26)
#define PCFG_CTRL_PCFG_PROG_B  (1u << 30)

/* INT_STS bits (per Xilinx FSBL pcap.h) */
#define IXR_AXI_WTO        (1u << 23)
#define IXR_AXI_WERR       (1u << 22)
#define IXR_AXI_RTO        (1u << 21)
#define IXR_AXI_RERR       (1u << 20)
#define IXR_RX_FIFO_OV     (1u << 18)
#define IXR_DMA_CMD_ERR    (1u << 15)
#define IXR_DMA_Q_OV       (1u << 14)
#define IXR_DMA_DONE       (1u << 13)
#define IXR_D_P_DONE       (1u << 12)
#define IXR_P2D_LEN_ERR    (1u << 11)
#define IXR_PCFG_DONE      (1u <<  2)
#define IXR_ALL_ERRORS     (IXR_AXI_WTO | IXR_AXI_WERR | IXR_AXI_RTO | \
                             IXR_AXI_RERR | IXR_RX_FIFO_OV |           \
                             IXR_DMA_CMD_ERR | IXR_DMA_Q_OV | IXR_P2D_LEN_ERR)

/* STATUS bits */
#define STATUS_DMA_Q_F      (1u << 31)
#define STATUS_DMA_Q_E      (1u << 30)
#define STATUS_PCFG_INIT_B  (1u <<  4)
#define STATUS_PCFG_DONE    (1u <<  2)

/* MCTRL bits */
#define MCTRL_PCAP_LPBK     (1u <<  4)

static inline void w32(uint32_t a, uint32_t v) { *(volatile uint32_t *)(uintptr_t)a = v; }
static inline uint32_t r32(uint32_t a)         { return *(volatile uint32_t *)(uintptr_t)a; }
static inline void dsb(void) { __asm__ volatile("dsb sy" ::: "memory"); }

#define PCAP_TIMEOUT  10000000

int pcap_program(const void *data, uint32_t words)
{
    volatile int i;

    /* 1. SLCR unlock and assert PL fabric resets */
    w32(SLCR_UNLOCK, SLCR_UNLOCK_KEY);
    w32(SLCR_FPGA_RST_CTRL, 0xFu);     /* assert all 4 FCLK domain resets */

    /* 2. DEVCFG unlock + disable PCAP internal loopback */
    w32(UNLOCK, DEVCFG_UNLOCK_KEY);
    w32(MCTRL, r32(MCTRL) & ~MCTRL_PCAP_LPBK);

    /* 3. Set PCAP_PR + PCAP_MODE */
    w32(PCFG_CTRL, r32(PCFG_CTRL) | PCFG_CTRL_PCAP_PR | PCFG_CTRL_PCAP_MODE);

    /* 4a. Assert PROG_B (active low) */
    w32(PCFG_CTRL, r32(PCFG_CTRL) & ~PCFG_CTRL_PCFG_PROG_B);

    /* 4b. Wait for INIT_B to go LOW (PL acknowledged PROG_B) */
    for (i = PCAP_TIMEOUT; i > 0; i--)
        if (!(r32(STATUS) & STATUS_PCFG_INIT_B)) break;
    if (i == 0) return -10;

    /* 4c. Deassert PROG_B */
    w32(PCFG_CTRL, r32(PCFG_CTRL) | PCFG_CTRL_PCFG_PROG_B);

    /* 4d. Wait for INIT_B to go HIGH (FPGA ready to receive config) */
    for (i = PCAP_TIMEOUT; i > 0; i--)
        if (r32(STATUS) & STATUS_PCFG_INIT_B) break;
    if (i == 0) return -1;

    /* 5. Re-set PCAP_PR + PCAP_MODE (PROG_B reset can clear PCAP_PR) */
    w32(PCFG_CTRL, r32(PCFG_CTRL) | PCFG_CTRL_PCAP_PR | PCFG_CTRL_PCAP_MODE);

    /* 6. Clear sticky errors + PCFG_DONE in INT_STS */
    w32(INT_STS, IXR_ALL_ERRORS | IXR_DMA_DONE | IXR_D_P_DONE | IXR_PCFG_DONE);

    /* 7. Wait DMA command queue empty */
    for (i = PCAP_TIMEOUT; i > 0; i--)
        if (r32(STATUS) & STATUS_DMA_Q_E) break;
    if (i == 0) return -20;

    /* 8. Drain ARM store buffer — bitstream bytes must be in DDR, not SB */
    dsb();

    /* 9. Setup DMA (write order matches Xilinx XSDK XDcfg_Transfer:
     *    SRC_ADDR, DST_ADDR, SRC_LEN, DEST_LEN. SRC_LEN triggers DMA.) */
    w32(DMA_SRC_ADDR, ((uint32_t)(uintptr_t)data) | 0x1u);
    w32(DMA_DST_ADDR, 0xFFFFFFFFu);
    w32(DMA_SRC_LEN,  words);
    w32(DMA_DEST_LEN, 0u);

    /* 10. Wait for D_P_DONE (both DMA fetch and PCAP transfer complete) */
    for (i = PCAP_TIMEOUT; i > 0; i--) {
        uint32_t s = r32(INT_STS);
        if (s & IXR_D_P_DONE) break;
        if (s & IXR_DMA_CMD_ERR) return -2;
        if (s & IXR_ALL_ERRORS)  return -3;
    }
    if (i == 0) return -4;

    /* 11. Wait PCFG_DONE in INT_STS (interrupt bit, not STATUS bit) */
    for (i = PCAP_TIMEOUT; i > 0; i--)
        if (r32(INT_STS) & IXR_PCFG_DONE) break;
    if (i == 0) return -5;

    /* 12. Deassert PL fabric resets so PL is alive */
    w32(SLCR_FPGA_RST_CTRL, 0x0u);
    w32(SLCR_LOCK, SLCR_LOCK_KEY);

    return 0;
}
