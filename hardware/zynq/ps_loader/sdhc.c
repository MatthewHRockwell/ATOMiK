/* ============================================================================
 * sdhc.c — Minimal SDHC 2.0 driver for Zynq-7000 SDIO1 (polled, PIO)
 * ========================================================================= */

#include "sdhc.h"

/* Rough loop delay: A9 at 666 MHz with a simple subs-loop is ~2 cycles/iter,
 * so 1 us ≈ 333 iterations. Be generous. */
static void busy_us(uint32_t us)
{
    volatile uint32_t n = us * 400u;
    while (n--) { }
}

/* Wait for a register bit with a timeout (expressed in iterations, not us) */
static sdhc_err_t wait_bit32_clear(uint32_t addr, uint32_t mask, uint32_t tries)
{
    while (tries--) {
        if ((r32(addr) & mask) == 0) return SDHC_OK;
    }
    return SDHC_ETIMEOUT;
}

/* Debug probe — written to fixed scratch so xsdb can see which step failed.
 * Ring buffer of 16 entries × 4 bytes at 0x200050..0x20008C. */
#define DBG_BASE  0x00200050u
#define DBG_N     16
static uint32_t dbg_phase = 0;
static void dbg(uint32_t tag, uint32_t v)
{
    w32(DBG_BASE + ((dbg_phase % DBG_N) * 4), (tag << 24) | (v & 0x00FFFFFFu));
    dbg_phase++;
}

/* Reset the SDHC CMD line (required after any command error per SDHC 2.0 spec). */
static void cmd_line_reset(uint32_t base)
{
    uint8_t sr = r8(base + SDHC_SW_RESET);
    w8(base + SDHC_SW_RESET, sr | SW_RESET_CMD);
    uint32_t tries = 100000;
    while (tries-- && (r8(base + SDHC_SW_RESET) & SW_RESET_CMD)) { }
    w32(base + SDHC_INT_STATUS, 0xFFFFFFFFu);
}

/* Issue command, wait for CMD_COMPLETE or error.
 *
 * cmd_word = [31:16] command register (CMD_IDX | type | resp | flags)
 *            [15:0]  transfer mode register
 */
static sdhc_err_t issue_cmd(sd_card_t *c,
                            uint8_t  cmd_idx,
                            uint32_t arg,
                            uint16_t cmd_flags,
                            uint16_t xfer_mode)
{
    dbg(cmd_idx, r32(c->base + SDHC_PRESENT_STATE));
    /* Wait until controller isn't busy with a previous command */
    sdhc_err_t e = wait_bit32_clear(c->base + SDHC_PRESENT_STATE,
                                    PRES_CMD_INHIBIT_CMD, 1000000);
    if (e) { dbg(0xE1, r32(c->base + SDHC_PRESENT_STATE)); return e; }
    if (cmd_flags & CMD_DATA_PRESENT) {
        e = wait_bit32_clear(c->base + SDHC_PRESENT_STATE,
                             PRES_CMD_INHIBIT_DAT, 1000000);
        if (e) { dbg(0xE2, r32(c->base + SDHC_PRESENT_STATE)); return e; }
    }

    /* Clear all interrupt status */
    w32(c->base + SDHC_INT_STATUS, 0xFFFFFFFFu);
    dsb();

    /* Argument */
    w32(c->base + SDHC_ARGUMENT, arg);

    /* TransferMode + Command as a single 32-bit write: lower half is
     * transfer mode at offset 0x0C, upper half is command at 0x0E. */
    uint32_t cmd = ((uint32_t)(CMD_IDX(cmd_idx) | cmd_flags) << 16)
                 | (uint32_t)xfer_mode;
    w32(c->base + SDHC_TRANSFER_MODE, cmd);
    dsb();

    /* Wait for cmd complete or error */
    uint32_t tries = 1000000;
    uint32_t is = 0;
    while (tries--) {
        is = r32(c->base + SDHC_INT_STATUS);
        if (is & INT_CMD_COMPLETE) break;
        if (is & INT_ERR_BIT)      { dbg(0xE3, is); return SDHC_ECMDERR; }
    }
    if (tries == 0) { dbg(0xE4, is); return SDHC_ETIMEOUT; }
    dbg(0xC0 | (cmd_idx & 0x3F), is);

    /* Clear CMD_COMPLETE */
    w32(c->base + SDHC_INT_STATUS, INT_CMD_COMPLETE);
    return SDHC_OK;
}

/* SDHC 2.0 divisor field (CLOCK_CTRL[15:8]) is "SDCLK Frequency Select":
 *   0 -> base clock (no division), 1 -> /2, 2 -> /4, ..., 0x80 -> /256.
 * Only ONE bit may be set — it's an N-of-8 encoding, not binary. */
static uint32_t compute_div(uint32_t base_hz, uint32_t want_hz)
{
    if (want_hz >= base_hz) return 0;
    uint32_t div;
    for (div = 1; div < 0x100; div <<= 1) {
        if ((base_hz / (2 * div)) <= want_hz) break;
    }
    if (div > 0x80) div = 0x80;
    return div;
}

/* Set SD clock (does NOT enable SDCLK to card yet; caller must enable). */
static sdhc_err_t set_intclk(uint32_t base, uint32_t base_hz, uint32_t want_hz)
{
    /* Turn off everything first */
    w16(base + SDHC_CLOCK_CTRL, 0);
    dsb();

    uint32_t div = compute_div(base_hz, want_hz);
    uint16_t cc = (uint16_t)((div << CLK_DIV_SHIFT) | CLK_INTCLK_EN);
    w16(base + SDHC_CLOCK_CTRL, cc);
    dsb();

    /* Wait for internal clock stable */
    uint32_t tries = 100000;
    while (tries--) {
        if (r16(base + SDHC_CLOCK_CTRL) & CLK_INTCLK_STABLE) break;
    }
    if (tries == 0) return SDHC_ETIMEOUT;
    return SDHC_OK;
}

static void enable_sdclk(uint32_t base)
{
    uint16_t cc = r16(base + SDHC_CLOCK_CTRL);
    w16(base + SDHC_CLOCK_CTRL, cc | (uint16_t)CLK_SDCLK_EN);
    dsb();
}

/* Set SD clock to freq Hz — disable SDCLK, reprogram divisor, re-enable. */
static sdhc_err_t set_sdclk(uint32_t base, uint32_t base_hz, uint32_t want_hz)
{
    /* Disable SD clock while we change divisor */
    uint16_t cc = r16(base + SDHC_CLOCK_CTRL);
    cc &= ~(uint16_t)CLK_SDCLK_EN;
    w16(base + SDHC_CLOCK_CTRL, cc);
    dsb();

    sdhc_err_t e = set_intclk(base, base_hz, want_hz);
    if (e) return e;
    enable_sdclk(base);
    return SDHC_OK;
}

/* Public: controller setup + software reset + initial 400 kHz clock + power on */
sdhc_err_t sdhc_controller_init(sd_card_t *c, uint32_t base)
{
    c->base = base;
    c->rca = 0;
    c->ocr = 0;
    c->sdhc = 0;
    c->version2 = 0;

    /* Full software reset */
    w8(base + SDHC_SW_RESET, SW_RESET_ALL);
    dsb();
    uint32_t tries = 100000;
    while (tries-- && (r8(base + SDHC_SW_RESET) & SW_RESET_ALL)) { }
    if (tries == 0) return SDHC_ETIMEOUT;

    /* Enable interrupt status bits (poll them, no IRQ delivery) */
    w32(base + SDHC_INT_ENABLE,    0x00FF00FFu | INT_ERR_MASK);
    w32(base + SDHC_INT_SIGNAL_EN, 0);

    /* Force Card Detect = inserted (bit 6 = use test value, bit 7 = inserted).
     * If PS-side CD pin is misrouted/unwired on this board, this lets us
     * still try commands. Doesn't affect electrical card response. */
    uint8_t hc = r8(base + SDHC_HOST_CTRL);
    w8(base + SDHC_HOST_CTRL, hc | 0xC0);
    dsb();

    /* Set timeout to max before issuing commands */
    w8(base + SDHC_TIMEOUT_CTRL, 0x0E);

    /* SDHC spec power-up sequence:
     *   1. Program internal clock divisor + enable INTCLK (no SDCLK yet).
     *   2. Wait INTCLK stable.
     *   3. Program voltage select.
     *   4. Set power on. Wait for power to stabilize.
     *   5. Enable SDCLK.
     *   6. Wait >= 1 ms (well over 74 cycles @ 400 kHz) before first command.
     */
    sdhc_err_t e = set_intclk(base, 50000000u, 400000u);
    if (e) return e;

    w8(base + SDHC_POWER_CTRL, PWR_33V);         /* voltage select, power off */
    dsb();
    busy_us(100);
    w8(base + SDHC_POWER_CTRL, PWR_33V | PWR_ON); /* power on */
    dsb();
    busy_us(5000);                                /* supply settle */

    enable_sdclk(base);
    busy_us(2000);                                /* >>74 clocks @ 400 kHz */
    return SDHC_OK;
}

/* Public: full SD card init (CMD0, CMD8, ACMD41, CMD2, CMD3, CMD7). */
sdhc_err_t sdhc_card_init(sd_card_t *c)
{
    /* CMD0: GO_IDLE_STATE, no response */
    sdhc_err_t e = issue_cmd(c, 0, 0, CMD_RESP_NONE, 0);
    if (e) return e;
    busy_us(2000);

    /* CMD8: SEND_IF_COND — check pattern 0xAA, 2.7-3.6V (0x1).
     * Response is R7 (48-bit). If card echoes pattern, it is SD v2+.
     * If it times out with illegal-command, treat as SD v1. */
    e = issue_cmd(c, 8, 0x000001AAu,
                  CMD_RESP_48 | CMD_CRC_CHECK | CMD_INDEX_CHECK, 0);
    if (e == SDHC_OK) {
        uint32_t r = r32(c->base + SDHC_RESPONSE);
        if ((r & 0xFF) == 0xAA) c->version2 = 1;
        else                    return SDHC_ENOSUPPORT;
    } else if (e == SDHC_ECMDERR) {
        /* Card likely v1.x — acceptable if OCR matches below.
         * After command error SDHC spec mandates CMD-line reset or the next
         * command will see CMD_INHIBIT_CMD stuck at 1. */
        c->version2 = 0;
        cmd_line_reset(c->base);
    } else {
        return e;
    }

    /* ACMD41: SD_SEND_OP_COND. Preceded by CMD55 (APP_CMD). Loop until OCR
     * ready bit [31] is set. Request HCS=1 (v2 only), 3.3V window. */
    uint32_t ocr_arg = c->version2 ? (0x40FF8000u) : 0x00FF8000u;
    for (int iter = 0; iter < 1000; iter++) {
        e = issue_cmd(c, 55, 0, CMD_RESP_48 | CMD_CRC_CHECK | CMD_INDEX_CHECK, 0);
        if (e) return e;
        e = issue_cmd(c, 41, ocr_arg, CMD_RESP_48, 0);   /* R3: no CRC, no idx */
        if (e) return e;
        c->ocr = r32(c->base + SDHC_RESPONSE);
        if (c->ocr & 0x80000000u) break;                 /* card ready */
        busy_us(10000);
    }
    if ((c->ocr & 0x80000000u) == 0) return SDHC_ETIMEOUT;
    c->sdhc = (c->ocr & 0x40000000u) ? 1 : 0;

    /* CMD2: ALL_SEND_CID — R2 (136-bit) */
    e = issue_cmd(c, 2, 0, CMD_RESP_136 | CMD_CRC_CHECK, 0);
    if (e) return e;
    for (int i = 0; i < 4; i++) {
        uint32_t w = r32(c->base + SDHC_RESPONSE + i * 4);
        c->cid[15 - i*4 - 0] = (uint8_t)(w >> 24);
        c->cid[15 - i*4 - 1] = (uint8_t)(w >> 16);
        c->cid[15 - i*4 - 2] = (uint8_t)(w >> 8);
        c->cid[15 - i*4 - 3] = (uint8_t)(w >> 0);
    }

    /* CMD3: SEND_RELATIVE_ADDR — R6, returns RCA in [31:16] */
    e = issue_cmd(c, 3, 0, CMD_RESP_48 | CMD_CRC_CHECK | CMD_INDEX_CHECK, 0);
    if (e) return e;
    c->rca = r32(c->base + SDHC_RESPONSE) & 0xFFFF0000u;

    /* Bump clock to 25 MHz now that card is in stand-by */
    e = set_sdclk(c->base, 50000000u, 25000000u);
    if (e) return e;

    /* CMD7: SELECT_CARD — R1b */
    e = issue_cmd(c, 7, c->rca, CMD_RESP_48_BUSY | CMD_CRC_CHECK | CMD_INDEX_CHECK, 0);
    if (e) return e;

    return SDHC_OK;
}

/* Public: read a single 512-byte block by LBA (or byte address on non-SDHC) */
sdhc_err_t sdhc_read_block(sd_card_t *c, uint32_t lba, void *dst)
{
    /* Block size + count */
    w16(c->base + SDHC_BLOCK_SIZE, 512);
    w16(c->base + SDHC_BLOCK_COUNT, 1);

    uint32_t arg = c->sdhc ? lba : (lba * 512u);

    /* CMD17: READ_SINGLE_BLOCK — R1, data read, no DMA, no multi-block */
    sdhc_err_t e = issue_cmd(c, 17, arg,
        CMD_RESP_48 | CMD_CRC_CHECK | CMD_INDEX_CHECK | CMD_DATA_PRESENT,
        TM_READ);
    if (e) return e;

    /* Wait for buffer-read-ready, then drain 128 × 32-bit words */
    uint32_t tries = 1000000;
    while (tries--) {
        uint32_t is = r32(c->base + SDHC_INT_STATUS);
        if (is & INT_BUF_READ_RDY) break;
        if (is & INT_ERR_BIT)      return SDHC_EDATAERR;
    }
    if (tries == 0) return SDHC_ETIMEOUT;
    w32(c->base + SDHC_INT_STATUS, INT_BUF_READ_RDY);

    uint32_t *d = (uint32_t *)dst;
    for (int i = 0; i < 128; i++) {
        d[i] = r32(c->base + SDHC_BUFFER_DATA);
    }

    /* Wait for transfer complete */
    tries = 1000000;
    while (tries--) {
        uint32_t is = r32(c->base + SDHC_INT_STATUS);
        if (is & INT_XFER_COMPLETE) break;
        if (is & INT_ERR_BIT)       return SDHC_EDATAERR;
    }
    if (tries == 0) return SDHC_ETIMEOUT;
    w32(c->base + SDHC_INT_STATUS, INT_XFER_COMPLETE);
    return SDHC_OK;
}

/* Public: write a single 512-byte block by LBA (or byte address on non-SDHC) */
sdhc_err_t sdhc_write_block(sd_card_t *c, uint32_t lba, const void *src)
{
    w16(c->base + SDHC_BLOCK_SIZE, 512);
    w16(c->base + SDHC_BLOCK_COUNT, 1);
    uint32_t arg = c->sdhc ? lba : (lba * 512u);

    /* CMD24: WRITE_SINGLE_BLOCK — R1, data present, no DMA, xfer mode = 0 (write) */
    sdhc_err_t e = issue_cmd(c, 24, arg,
        CMD_RESP_48 | CMD_CRC_CHECK | CMD_INDEX_CHECK | CMD_DATA_PRESENT, 0);
    if (e) return e;

    /* Wait for buffer-write-ready (bit 4), then push 128 × 32-bit words */
    uint32_t tries = 1000000;
    while (tries--) {
        uint32_t is = r32(c->base + SDHC_INT_STATUS);
        if (is & (1u << 4)) break;
        if (is & INT_ERR_BIT) return SDHC_EDATAERR;
    }
    if (tries == 0) return SDHC_ETIMEOUT;
    w32(c->base + SDHC_INT_STATUS, (1u << 4));

    const uint32_t *p = (const uint32_t *)src;
    for (int i = 0; i < 128; i++) {
        w32(c->base + SDHC_BUFFER_DATA, p[i]);
    }

    /* Wait for xfer complete — write commits via card busy-wait then deasserts */
    tries = 1000000;
    while (tries--) {
        uint32_t is = r32(c->base + SDHC_INT_STATUS);
        if (is & INT_XFER_COMPLETE) break;
        if (is & INT_ERR_BIT) return SDHC_EDATAERR;
    }
    if (tries == 0) return SDHC_ETIMEOUT;
    w32(c->base + SDHC_INT_STATUS, INT_XFER_COMPLETE);
    return SDHC_OK;
}
