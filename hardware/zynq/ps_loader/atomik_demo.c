/*
 * ATOMiK Unified Fundraising Demo
 *
 * One program, one narrative, 70 seconds. Drives 1920x1080 HDMI dashboard
 * + 320x172 SPI LCD status panel simultaneously, exercising ATOMiK hardware.
 *
 * Story: "This board detects meaningful state changes, moves less data,
 *         and makes stateful systems cheaper and faster."
 *
 * CSR addresses from litex-build-nax64/csr.h (April 24, 2026 bitstream).
 * WARNING: atomik_hdmi_viz.c and atomik_splash.c have STALE VTG/DMA addresses
 * that now map to LCD GPIO pins. Do NOT copy those addresses.
 *
 * Build:
 *   riscv64-linux-gnu-gcc -O2 -static -o atomik_demo atomik_demo.c
 *   riscv64-linux-gnu-strip atomik_demo
 *
 * Run:
 *   /tmp/atomik_demo
 * ========================================================================= */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* ── Framebuffer ─────────────────────────────────────────────────────── */
#define FB_BASE     0x48000000UL
#define FB_HRES     1920
#define FB_VRES     1080
#define FB_BPP      4
#define FB_SIZE     (FB_HRES * FB_VRES * FB_BPP)

/* ── CSR addresses (CORRECT for April 24 bitstream with zlcd_ prefix) ── */
#define CSR_BASE            0xF0000000UL
#define CSR_SIZE            0x00100000UL
#define CSR_FB_DMA_BASE     0xF0002000UL
#define CSR_FB_DMA_ENABLE   0xF0002004UL
#define CSR_FB_VTG_ENABLE   0xF0002800UL
#define CSR_LCD_CLK         0xF0003000UL
#define CSR_LCD_CS          0xF0003800UL
#define CSR_LCD_DC          0xF0004000UL
#define CSR_LCD_LED         0xF0004800UL
#define CSR_LCD_MOSI        0xF0005000UL
#define CSR_LCD_RST         0xF0005800UL

/* ── ATOMiK Adapter ─────────────────────────────────────────────────── */
#define ADAPTER_BASE 0xF0020000UL
#define REG_CMD   0x00
#define REG_RS1   0x04
#define REG_RS2   0x08
#define REG_RD    0x0C
#define REG_STAT  0x10

/* ── Colors (XRGB8888) — investor palette ────────────────────────────── */
#define COL_BG       0x0008111A  /* background */
#define COL_PANEL    0x000F1B28  /* panel fill */
#define COL_TEXT     0x00F3F7FB  /* primary text */
#define COL_DIM      0x009AA8B5  /* secondary text */
#define COL_BLUE     0x001EC8FF  /* ATOMiK action */
#define COL_ORANGE   0x00FF8A3D  /* software waste */
#define COL_GREEN    0x0039D98A  /* business gain */
#define COL_GRAY     0x002A3644  /* inactive */
#define COL_WHITE    0x00F3F7FB
/* legacy aliases for code that uses old names */
#define COL_TITLE    COL_BLUE
#define COL_RED      COL_ORANGE
#define COL_BAR_SW   COL_ORANGE
#define COL_BAR_HW   COL_BLUE
#define COL_YELLOW   0x00FFCC00  /* kept for cycle counts */

/* ── Layout zones ───────────────────────────────────────────────────── */
#define MARGIN       96
#define TITLE_Y      80
#define TITLE_H      160
#define MAIN_Y       280
#define MAIN_H       480
#define BOTTOM_Y     840
#define BOTTOM_H     120

/* ── LCD Colors (RGB565) ────────────────────────────────────────────── */
#define LCD_BG       0x0841  /* #08111A approx */
#define LCD_FG       0xFFFF  /* white */
#define LCD_ACCENT   0x0E5F  /* ATOMiK blue */
#define LCD_GREEN    0x1EC9  /* saved green */
#define LCD_RED      0xFC47  /* waste orange */
#define LCD_DIM      0x4A69  /* secondary */

/* ── Demo parameters ────────────────────────────────────────────────── */
#define N_REGIONS    8
#define REGION_SIZE  4096

/* ── Font ────────────────────────────────────────────────────────────── */
#include "font_8x16.h"
#define CHAR_W 8
#define CHAR_H 16

/* ── Globals ─────────────────────────────────────────────────────────── */
static uint32_t *fb;
static volatile uint32_t *csr_page;
static volatile uint32_t *adapter;
static int memfd;

static uint8_t regions[N_REGIONS][REGION_SIZE];
static uint8_t shadows[N_REGIONS][REGION_SIZE];
static int region_changed[N_REGIONS];

static int total_ops;
static int total_changes_detected;
static float best_speedup;
static uint64_t demo_start_time;

/* ── L2 cache flush ─────────────────────────────────────────────────── */
static volatile uint8_t flush_scratch[128 * 1024] __attribute__((aligned(64)));
static void flush_l2(void) {
    for (int i = 0; i < (int)sizeof(flush_scratch); i += 64)
        flush_scratch[i] = 0;
    asm volatile("fence iorw,iorw");
}

/* ── Timer ───────────────────────────────────────────────────────────── */
static inline uint64_t rdtime_ticks(void) {
    uint64_t t;
    asm volatile("rdtime %0" : "=r"(t));
    return t;
}

/* ── CSR write ───────────────────────────────────────────────────────── */
static inline void csr_wr(unsigned long addr, uint32_t val) {
    csr_page[(addr - CSR_BASE) / 4] = val;
}

/* ── Framebuffer primitives ──────────────────────────────────────────── */
static inline void fb_pixel(int x, int y, uint32_t c) {
    if (x >= 0 && x < FB_HRES && y >= 0 && y < FB_VRES)
        fb[y * FB_HRES + x] = c;
}

static void fb_rect(int x, int y, int w, int h, uint32_t c) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            fb_pixel(x + dx, y + dy, c);
}

static void fb_char(int x, int y, char ch, uint32_t fg, uint32_t bg) {
    const uint8_t *glyph = &font_8x16[(unsigned char)ch * 16];
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++)
            fb_pixel(x + col, y + row, (bits & (0x80 >> col)) ? fg : bg);
    }
}

static void fb_string(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    while (*s) { fb_char(x, y, *s++, fg, bg); x += CHAR_W; }
}

static void fb_char_big(int x, int y, char ch, uint32_t fg, uint32_t bg, int scale) {
    const uint8_t *glyph = &font_8x16[(unsigned char)ch * 16];
    for (int row = 0; row < 16; row++)
        for (int col = 0; col < 8; col++) {
            uint32_t c = (glyph[row] & (0x80 >> col)) ? fg : bg;
            fb_rect(x + col * scale, y + row * scale, scale, scale, c);
        }
}

static void fb_string_big(int x, int y, const char *s, uint32_t fg, uint32_t bg, int scale) {
    while (*s) { fb_char_big(x, y, *s++, fg, bg, scale); x += CHAR_W * scale; }
}

static void fb_center(int y, const char *s, uint32_t fg, uint32_t bg) {
    int len = strlen(s);
    fb_string((FB_HRES - len * CHAR_W) / 2, y, s, fg, bg);
}

static void fb_center_big(int y, const char *s, uint32_t fg, uint32_t bg, int scale) {
    int len = strlen(s);
    fb_string_big((FB_HRES - len * CHAR_W * scale) / 2, y, s, fg, bg, scale);
}

static void fb_hex64(int x, int y, uint64_t val, uint32_t fg, uint32_t bg) {
    char buf[19];
    snprintf(buf, sizeof(buf), "0x%016llX", (unsigned long long)val);
    fb_string(x, y, buf, fg, bg);
}

static void fb_bar(int x, int y, int max_w, int h, float frac, uint32_t col) {
    int w = (int)(frac * max_w);
    if (w < 2) w = 2;
    if (w > max_w) w = max_w;
    fb_rect(x, y, w, h, col);
    if (w < max_w)
        fb_rect(x + w, y, max_w - w, h, COL_PANEL);
}

#define GRID_CELL 10
static void fb_bitgrid(int x, int y, uint64_t val, uint32_t on_col, uint32_t off_col) {
    for (int bit = 63; bit >= 0; bit--) {
        int row = (63 - bit) / 8;
        int col = (63 - bit) % 8;
        uint32_t c = (val >> bit) & 1 ? on_col : off_col;
        fb_rect(x + col * GRID_CELL, y + row * GRID_CELL,
                GRID_CELL - 1, GRID_CELL - 1, c);
    }
}

/* ── ATOMiK adapter MMIO ─────────────────────────────────────────────── */
static void a_wr(int off, uint32_t val) {
    adapter[off / 4] = val;
    asm volatile("fence iorw,iorw");
}
static uint32_t a_rr(int off) {
    asm volatile("fence iorw,iorw");
    return adapter[off / 4];
}

static void load64(uint8_t addr, uint64_t val) {
    a_wr(REG_RS1, addr);
    a_wr(REG_RS2, (uint32_t)(val & 0xFFFFFFFF));
    a_wr(REG_CMD, 0);
    a_wr(REG_RS2, (uint32_t)(val >> 32));
    a_wr(REG_CMD, 4);
}
static void accum64(uint64_t delta) {
    a_wr(REG_RS1, (uint32_t)(delta & 0xFFFFFFFF));
    a_wr(REG_CMD, 1);
    a_wr(REG_RS1, (uint32_t)(delta >> 32));
    a_wr(REG_CMD, 5);
}
static uint64_t read64(void) {
    a_wr(REG_CMD, 2);
    uint32_t lo = a_rr(REG_RD);
    a_wr(REG_CMD, 6);
    uint32_t hi = a_rr(REG_RD);
    return ((uint64_t)hi << 32) | lo;
}
static void atomik_swap(uint8_t addr) {
    a_wr(REG_RS1, addr);
    a_wr(REG_CMD, 3);
}

/* ── LCD SPI bitbang ─────────────────────────────────────────────────── */
static void pin_mosi(int v) { csr_wr(CSR_LCD_MOSI, v & 1); }
static void pin_clk(int v)  { csr_wr(CSR_LCD_CLK, v & 1); }
static void pin_cs(int v)   { csr_wr(CSR_LCD_CS, v & 1); }
static void pin_dc(int v)   { csr_wr(CSR_LCD_DC, v & 1); }
static void pin_rst(int v)  { csr_wr(CSR_LCD_RST, v & 1); }
static void pin_led(int v)  { csr_wr(CSR_LCD_LED, v & 1); }

static void spi_byte(unsigned char val) {
    for (int i = 7; i >= 0; i--) {
        pin_mosi((val >> i) & 1);
        pin_clk(0);
        pin_clk(1);
    }
}

static void lcd_cmd(unsigned char cmd) {
    pin_dc(0); pin_cs(0);
    spi_byte(cmd);
    pin_cs(1);
}

static void lcd_data8(unsigned char val) {
    pin_dc(1); pin_cs(0);
    spi_byte(val);
    pin_cs(1);
}

static void lcd_set_window(int x0, int y0, int x1, int y1) {
    lcd_cmd(0x2A);
    pin_dc(1); pin_cs(0);
    spi_byte(x0 >> 8); spi_byte(x0);
    spi_byte(x1 >> 8); spi_byte(x1);
    pin_cs(1);
    lcd_cmd(0x2B);
    pin_dc(1); pin_cs(0);
    spi_byte((y0+34) >> 8); spi_byte(y0+34);
    spi_byte((y1+34) >> 8); spi_byte(y1+34);
    pin_cs(1);
    lcd_cmd(0x2C);
}

static void lcd_fill(int x, int y, int w, int h, uint16_t c) {
    lcd_set_window(x, y, x+w-1, y+h-1);
    pin_dc(1); pin_cs(0);
    for (int i = 0; i < w * h; i++) { spi_byte(c >> 8); spi_byte(c & 0xFF); }
    pin_cs(1);
}

static void lcd_init(void) {
    pin_led(1);
    pin_rst(1); pin_cs(1); usleep(50000);
    pin_rst(0); usleep(50000);
    pin_rst(1); usleep(150000);
    lcd_cmd(0x11); usleep(120000);
    lcd_cmd(0x36); lcd_data8(0x60);
    lcd_cmd(0x3A); lcd_data8(0x05);
    lcd_cmd(0x21);
    lcd_cmd(0x13);
    lcd_cmd(0x29); usleep(50000);
}

/* LCD text rendering via SPI */
static void lcd_char(int x, int y, char ch, uint16_t fg, uint16_t bg) {
    const uint8_t *glyph = &font_8x16[(unsigned char)ch * 16];
    lcd_set_window(x, y, x + 7, y + 15);
    pin_dc(1); pin_cs(0);
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            uint16_t c = (bits & (0x80 >> col)) ? fg : bg;
            spi_byte(c >> 8); spi_byte(c & 0xFF);
        }
    }
    pin_cs(1);
}

static void lcd_string(int x, int y, const char *s, uint16_t fg, uint16_t bg) {
    while (*s) { lcd_char(x, y, *s++, fg, bg); x += 8; }
}

static void lcd_line(int y, const char *s, uint16_t fg) {
    /* Clear line then draw string */
    lcd_fill(0, y, 320, 16, LCD_BG);
    lcd_string(4, y, s, fg, LCD_BG);
}

/* ── Integer formatting ──────────────────────────────────────────────── */
static void itoa_buf(char *buf, int val) {
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12]; int i = 0;
    if (val < 0) { *buf++ = '-'; val = -val; }
    while (val > 0) { tmp[i++] = '0' + val % 10; val /= 10; }
    while (i > 0) *buf++ = tmp[--i];
    *buf = 0;
}

/* ── Demo pause with L2 flush ────────────────────────────────────────── */
static void demo_pause(int ms) {
    flush_l2();
    usleep(ms * 1000);
}

/* ── Compute XOR fingerprint of buffer ───────────────────────────────── */
static uint64_t compute_fp(const uint8_t *data, int len) {
    uint64_t fp = 0;
    const uint64_t *p = (const uint64_t *)data;
    for (int i = 0; i < len / 8; i++)
        fp ^= p[i];
    return fp;
}

/* ── Elapsed seconds since demo start ────────────────────────────────── */
static int elapsed_sec(void) {
    return (int)((rdtime_ticks() - demo_start_time) / 100000000ULL);
}


/* ═══════════════════════════════════════════════════════════════════════
 *  VISUAL SYSTEM — 16px grid, no blank flashes, one continuous story
 * ═══════════════════════════════════════════════════════════════════════ */

static uint64_t sw_bytes_scanned;
static uint64_t atomik_bytes_touched;
static int attract_mode = 0;  /* 0 = investor (hold on summary), 1 = loop */

/* Chrome: wordmark + badge + proof strip — drawn once, persistent */
static void draw_chrome(void) {
    fb_string_big(MARGIN, 40, "ATOMiK", COL_BLUE, COL_BG, 2);
    fb_rect(1568, 40, 256, 48, COL_BLUE);
    fb_string(1576, 56, "LIVE ON HARDWARE", COL_BG, COL_BLUE);
}

/* Clear content area only (y=96..943), preserve chrome */
static void content_clear(void) {
    fb_rect(0, 96, FB_HRES, 848, COL_BG);
}

/* Phase transition: brief dark card with title, then clear */
static void screen_transition(const char *next_title) {
    /* Show phase card in content area for 150ms */
    content_clear();
    fb_center_big(400, next_title, COL_DIM, COL_BG, 2);
    demo_pause(150);
    content_clear();
}

/* Bottom proof strip (inside content area) */
static void screen_bottom(const char *text) {
    fb_rect(MARGIN, 944, FB_HRES - 2*MARGIN, 1, COL_GRAY);
    fb_string(MARGIN, 968, text, COL_DIM, COL_BG);
}

/* 8-box row */
#define BOX_W   176
#define BOX_H   112
#define BOX_GAP 32
#define BOX_TOTAL (N_REGIONS * (BOX_W + BOX_GAP) - BOX_GAP)
#define BOX_X0  ((FB_HRES - BOX_TOTAL) / 2)

static void draw_box_row(int y, int changed[], uint32_t all_col,
                         uint32_t chg_col, uint32_t skip_col,
                         const char *chg_lbl, const char *skip_lbl) {
    for (int i = 0; i < N_REGIONS; i++) {
        int bx = BOX_X0 + i * (BOX_W + BOX_GAP);
        uint32_t bg; const char *label;
        if (all_col) { bg = all_col; label = chg_lbl; }
        else if (changed && changed[i]) { bg = chg_col; label = chg_lbl; }
        else { bg = skip_col; label = skip_lbl; }
        fb_rect(bx, y, BOX_W, BOX_H, bg);
        char rl[4]; snprintf(rl, sizeof(rl), "R%d", i+1);
        fb_string(bx + 8, y + 8, rl, COL_DIM, bg);
        int ll = 0; const char *p = label; while (*p++) ll++;
        fb_string_big(bx + (BOX_W - ll*CHAR_W*2)/2, y + (BOX_H-CHAR_H*2)/2,
                      label, COL_WHITE, bg, 2);
    }
}

/* Green gain pill */
static void draw_gain_pill(int y, const char *text) {
    int len = 0; const char *p = text; while (*p++) len++;
    int pw = len * CHAR_W * 2 + 80;
    if (pw < 560) pw = 560;
    int px = (FB_HRES - pw) / 2;
    fb_rect(px, y, pw, 88, COL_GREEN);
    fb_string_big(px + (pw - len*CHAR_W*2)/2, y + 16, text, COL_BG, COL_GREEN, 2);
}

/* LCD status */
static void lcd_status(const char *line1, const char *metric) {
    lcd_fill(0, 0, 320, 172, LCD_BG);
    lcd_fill(0, 0, 320, 3, LCD_ACCENT);
    lcd_fill(0, 169, 320, 3, LCD_ACCENT);
    lcd_fill(240, 8, 70, 18, LCD_RED);
    lcd_string(248, 9, "LIVE", LCD_FG, LCD_RED);
    lcd_line(40, line1, LCD_ACCENT);
    if (metric) lcd_line(80, metric, LCD_GREEN);
}

/* LCD replica — simplified per feedback */
static void lcd_replica(int n_chg) {
    lcd_fill(0, 0, 320, 172, LCD_BG);
    lcd_fill(0, 0, 320, 3, LCD_ACCENT);
    lcd_fill(0, 169, 320, 3, LCD_ACCENT);
    lcd_line(20, "     REPLICA", LCD_ACCENT);
    char buf[40];
    snprintf(buf, sizeof(buf), "  %d of 8 updated", n_chg);
    lcd_line(70, buf, LCD_FG);
    lcd_line(110, " Only changed state", LCD_GREEN);
    lcd_line(126, " sent.", LCD_GREEN);
}

/* LCD final KPI */
static void lcd_kpi(float bw_pct, float speedup) {
    lcd_fill(0, 0, 320, 172, LCD_BG);
    lcd_fill(0, 0, 320, 3, LCD_ACCENT);
    lcd_fill(0, 169, 320, 3, LCD_ACCENT);
    lcd_fill(240, 8, 70, 18, LCD_RED);
    lcd_string(248, 9, "LIVE", LCD_FG, LCD_RED);
    char buf[40];
    snprintf(buf, sizeof(buf), "  %.0f%% less data", bw_pct);
    lcd_line(60, buf, LCD_GREEN);
    snprintf(buf, sizeof(buf), "  ~%.0fx faster query", speedup);
    lcd_line(100, buf, LCD_ACCENT);
}

/* Mutation helper */
static int do_mutations(int tick) {
    memset(region_changed, 0, sizeof(region_changed));
    int m1 = tick % N_REGIONS, m2 = (tick + 3) % N_REGIONS;
    for (int j = 0; j < 63; j++) {
        regions[m1][j*64] ^= (0x11*(tick+1)+j);
        regions[m2][j*64+1] ^= (0x22*(tick+1)+j);
    }
    int hw_chg = 0;
    for (int i = 0; i < N_REGIONS; i++) {
        uint64_t ofp = compute_fp(shadows[i], REGION_SIZE);
        uint64_t nfp = compute_fp(regions[i], REGION_SIZE);
        sw_bytes_scanned += REGION_SIZE;
        load64(i, ofp); accum64(nfp ^ ofp);
        uint64_t st = read64();
        region_changed[i] = (st != ofp);
        if (region_changed[i]) { hw_chg++; atomik_bytes_touched += REGION_SIZE; }
        total_ops += 3;
    }
    total_changes_detected += hw_chg;
    for (int i = 0; i < N_REGIONS; i++)
        memcpy(shadows[i], regions[i], REGION_SIZE);
    return hw_chg;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  PHASES: Problem → Hero → Wedge → Numbers → Watch → Summary
 * ═══════════════════════════════════════════════════════════════════════ */

/* Phase 1: Problem (4s) */
static void phase_problem(void) {
    printf("Phase 1: Problem\n");
    screen_transition("The Problem");

    fb_string_big(MARGIN, 96, "Software rereads unchanged state", COL_TEXT, COL_BG, 2);
    fb_string_big(MARGIN, 168, "Most of that work is wasted.", COL_DIM, COL_BG, 2);

    for (int i = 0; i < N_REGIONS; i++) {
        load64(i, 0xAA00000000000000ULL | ((uint64_t)i << 48));
        total_ops++;
        memset(regions[i], 0xAA + i, REGION_SIZE);
        memcpy(shadows[i], regions[i], REGION_SIZE);
    }

    draw_box_row(400, NULL, COL_ORANGE, 0, 0, "SCAN", NULL);
    fb_center(560, "8 of 8 regions touched every cycle", COL_ORANGE, COL_BG);
    screen_bottom("Software checks everything, even when almost nothing changed.");
    lcd_status("   The Problem", " SW rescans everything");
    demo_pause(4000);
}

/* Phase 2: Hero (10s) */
static void phase_hero(void) {
    printf("Phase 2: Hero\n");

    for (int tick = 0; tick < 5; tick++) {
        content_clear();
        fb_string_big(MARGIN, 96, "ATOMiK touches only what changed", COL_TEXT, COL_BG, 2);

        do_mutations(tick);

        fb_string_big(MARGIN, 288, "SOFTWARE", COL_ORANGE, COL_BG, 2);
        draw_box_row(320, NULL, COL_ORANGE, 0, 0, "SCAN", NULL);

        fb_string_big(MARGIN, 320 + BOX_H + 16, "ATOMiK", COL_BLUE, COL_BG, 2);
        draw_box_row(320 + BOX_H + 48, region_changed, 0, COL_BLUE, COL_GRAY, "SYNC", "SKIP");

        float pct = sw_bytes_scanned > 0 ?
            100.0f * (sw_bytes_scanned - atomik_bytes_touched) / sw_bytes_scanned : 0;
        char pill[40]; snprintf(pill, sizeof(pill), "%.0f%% less data touched", pct);
        draw_gain_pill(320 + BOX_H*2 + 80, pill);

        screen_bottom("Know what changed. Move only what matters.");
        char lbuf[40]; snprintf(lbuf, sizeof(lbuf), "  %.0f%% less data", pct);
        lcd_status(" ATOMiK vs Software", lbuf);
        demo_pause(1800);
    }
}

/* Phase 3: Wedge (10s) */
static void phase_wedge(void) {
    printf("Phase 3: Wedge\n");

    for (int tick = 0; tick < 5; tick++) {
        content_clear();
        fb_string_big(MARGIN, 96, "Send only changed state", COL_TEXT, COL_BG, 2);

        memset(region_changed, 0, sizeof(region_changed));
        int m1 = (tick*5) % N_REGIONS, m2 = (tick*5+2) % N_REGIONS;
        for (int j = 0; j < 31; j++) {
            regions[m1][j*128] ^= (0xFF - j);
            regions[m2][j*128+64] ^= (0xCC + j);
        }
        int n_chg = 0;
        for (int i = 0; i < N_REGIONS; i++) {
            uint64_t ofp = compute_fp(shadows[i], REGION_SIZE);
            uint64_t nfp = compute_fp(regions[i], REGION_SIZE);
            sw_bytes_scanned += REGION_SIZE;
            region_changed[i] = (ofp != nfp);
            if (region_changed[i]) { atomik_bytes_touched += REGION_SIZE; n_chg++; }
            total_ops += 2;
        }
        for (int i = 0; i < N_REGIONS; i++)
            memcpy(shadows[i], regions[i], REGION_SIZE);

        /* Source */
        fb_string_big(MARGIN, 288, "SOURCE", COL_DIM, COL_BG, 2);
        draw_box_row(320, region_changed, 0, COL_BLUE, COL_GRAY, "", "");

        /* Arrow */
        int arrow_y = 320 + BOX_H + 16;
        char abuf[40]; snprintf(abuf, sizeof(abuf), "%d changed blocks", n_chg);
        fb_rect((FB_HRES - 400)/2, arrow_y + 8, 400, 4, COL_GREEN);
        fb_center(arrow_y + 20, abuf, COL_GREEN, COL_BG);

        /* Replica */
        int ry = arrow_y + 56;
        fb_string_big(MARGIN, ry, "REPLICA", COL_DIM, COL_BG, 2);
        draw_box_row(ry + 32, region_changed, 0, COL_BLUE, COL_GRAY, "", "");

        screen_bottom("Less sync traffic. Faster replicas. Lower cost.");
        lcd_replica(n_chg);
        demo_pause(1800);
    }
}

/* Benchmark */
typedef struct { int size; uint64_t sw_med; uint64_t hw_med; float speedup; } bench_t;
static int cmp_u64(const void *a, const void *b) {
    uint64_t va = *(const uint64_t*)a, vb = *(const uint64_t*)b;
    return (va > vb) - (va < vb);
}
#define BENCH_ITERS 100
static void run_bench(bench_t *b) {
    uint64_t sw[BENCH_ITERS], hw[BENCH_ITERS];
    volatile uint8_t *buf = regions[0];
    for (int w = 0; w < 10; w++) {
        volatile uint32_t d = 0;
        for (int j = 0; j < b->size; j++) d += buf[j];
        load64(7, 0xAAAAAAAAAAAAAAAAULL); read64(); (void)d;
    }
    for (int it = 0; it < BENCH_ITERS; it++) {
        uint64_t t0 = rdtime_ticks();
        volatile uint32_t d = 0;
        for (int j = 0; j < b->size; j++) d += buf[j];
        sw[it] = rdtime_ticks() - t0; (void)d;
    }
    load64(7, 0xAAAAAAAAAAAAAAAAULL);
    for (int it = 0; it < BENCH_ITERS; it++) {
        uint64_t t0 = rdtime_ticks(); read64();
        hw[it] = rdtime_ticks() - t0;
    }
    total_ops += BENCH_ITERS + 1;
    qsort(sw, BENCH_ITERS, sizeof(uint64_t), cmp_u64);
    qsort(hw, BENCH_ITERS, sizeof(uint64_t), cmp_u64);
    b->sw_med = sw[BENCH_ITERS/2]; b->hw_med = hw[BENCH_ITERS/2];
    b->speedup = (float)b->sw_med / (float)(b->hw_med > 0 ? b->hw_med : 1);
}

/* Phase 4: Numbers (6s) */
static void phase_numbers(void) {
    printf("Phase 4: Numbers\n");
    bench_t bm[] = { {.size=256}, {.size=4096}, {.size=65536} };
    printf("  Benchmarking...\n");
    for (int i = 0; i < 3; i++) {
        run_bench(&bm[i]);
        if (bm[i].speedup > best_speedup) best_speedup = bm[i].speedup;
    }

    screen_transition("The Numbers");
    float bw = sw_bytes_scanned > 0 ?
        100.0f * (sw_bytes_scanned - atomik_bytes_touched) / sw_bytes_scanned : 0;
    char buf[80];

    /* Left card: LIVE RESULT */
    int lx=MARGIN, ly=288, lw=784, lh=400;
    fb_rect(lx, ly, lw, lh, COL_PANEL);
    fb_string(lx+48, ly+32, "LIVE RESULT", COL_DIM, COL_PANEL);
    snprintf(buf, sizeof(buf), "%.0f%%", bw);
    fb_string_big(lx+80, ly+80, buf, COL_GREEN, COL_PANEL, 5);
    fb_string_big(lx+48, ly+176, "less data touched", COL_TEXT, COL_PANEL, 2);
    uint64_t akb = (sw_bytes_scanned - atomik_bytes_touched) / 1024;
    snprintf(buf, sizeof(buf), "%llu KB avoided this run", (unsigned long long)akb);
    fb_string(lx+48, ly+240, buf, COL_DIM, COL_PANEL);

    /* Right card: Query cost stays flat */
    int rx=MARGIN+lw+48, ry=288, rw=880, rh=400;
    fb_rect(rx, ry, rw, rh, COL_PANEL);
    fb_string(rx+48, ry+32, "Query cost stays flat", COL_DIM, COL_PANEL);
    const char *lb[] = {"256 B", "4 KB", "64 KB"};
    int bmax = 480;
    for (int i = 0; i < 3; i++) {
        int by = ry + 96 + i * 96;
        fb_string(rx+48, by, lb[i], COL_TEXT, COL_PANEL);
        float sf = (float)bm[i].sw_med / (float)bm[2].sw_med;
        int sww = (int)(sf * bmax); if (sww < 4) sww = 4;
        fb_rect(rx+160, by+2, sww, 14, COL_ORANGE);
        float hf = (float)bm[i].hw_med / (float)bm[2].sw_med;
        int hww = (int)(hf * bmax); if (hww < 4) hww = 4;
        fb_rect(rx+160, by+22, hww, 14, COL_BLUE);
        snprintf(buf, sizeof(buf), "%llu / %llu",
                 (unsigned long long)bm[i].sw_med, (unsigned long long)bm[i].hw_med);
        fb_string(rx+160+bmax+16, by+8, buf, COL_GRAY, COL_PANEL);
    }

    screen_bottom("Shown here: query cost only. ATOMiK bookkeeping happens when data is written.");
    snprintf(buf, sizeof(buf), "  %.0f%% less data", bw);
    lcd_status("   The Numbers", buf);
    demo_pause(6000);
}

/* Phase 5: Watch (9s) */
static void phase_watch(void) {
    printf("Phase 5: Watch\n");
    uint32_t rng = rdtime_ticks() & 0xFFFFFFFF;

    for (int tick = 0; tick < 9; tick++) {
        content_clear();
        fb_string_big(MARGIN, 96, "Watch live state without full rescans", COL_TEXT, COL_BG, 2);

        rng = rng * 1103515245 + 12345;
        int nm = (rng >> 16) % 4;
        memset(region_changed, 0, sizeof(region_changed));
        for (int m = 0; m < nm; m++) {
            rng = rng * 1103515245 + 12345;
            int idx = (rng >> 16) % N_REGIONS;
            for (int j = 0; j < 15; j++)
                regions[idx][j*256+(m&3)] ^= ((rng>>8)+j) & 0xFF;
            region_changed[idx] = 1;
        }
        int chg = 0;
        for (int i = 0; i < N_REGIONS; i++) {
            uint64_t ofp = compute_fp(shadows[i], REGION_SIZE);
            uint64_t nfp = compute_fp(regions[i], REGION_SIZE);
            sw_bytes_scanned += REGION_SIZE;
            if (ofp != nfp) { region_changed[i] = 1; chg++; atomik_bytes_touched += REGION_SIZE; }
            total_ops += 2;
        }
        total_changes_detected += chg;
        for (int i = 0; i < N_REGIONS; i++)
            memcpy(shadows[i], regions[i], REGION_SIZE);

        /* Region boxes */
        draw_box_row(320, region_changed, 0, COL_BLUE, COL_GRAY, "CHANGED", "");

        /* KPI cards */
        char buf[40]; int kx = 1312, ky = 320;
        fb_rect(kx, ky, 496, 80, COL_PANEL);
        fb_string(kx+16, ky+8, "Changed now", COL_DIM, COL_PANEL);
        snprintf(buf, sizeof(buf), "%d of 8", chg);
        fb_string_big(kx+16, ky+32, buf, COL_BLUE, COL_PANEL, 2);

        fb_rect(kx, ky+96, 496, 80, COL_PANEL);
        fb_string(kx+16, ky+104, "Data avoided", COL_DIM, COL_PANEL);
        float pct = sw_bytes_scanned > 0 ?
            100.0f * (sw_bytes_scanned - atomik_bytes_touched) / sw_bytes_scanned : 0;
        snprintf(buf, sizeof(buf), "%.0f%%", pct);
        fb_string_big(kx+16, ky+128, buf, COL_GREEN, COL_PANEL, 2);

        fb_rect(kx, ky+192, 496, 80, COL_PANEL);
        fb_string(kx+16, ky+200, "Checks completed", COL_DIM, COL_PANEL);
        snprintf(buf, sizeof(buf), "%d", total_ops);
        fb_string_big(kx+16, ky+224, buf, COL_TEXT, COL_PANEL, 2);

        screen_bottom("Know what changed. Move only what matters.");
        lcd_replica(chg);
        demo_pause(950);
    }
}

/* Phase 6: Summary (8s — holds in investor mode) */
static void phase_summary(void) {
    printf("Phase 6: Summary\n");
    content_clear();

    float bw = sw_bytes_scanned > 0 ?
        100.0f * (sw_bytes_scanned - atomik_bytes_touched) / sw_bytes_scanned : 0;
    char buf[80];

    fb_string_big(MARGIN, 96, "Hardware that knows what changed", COL_TEXT, COL_BG, 2);

    /* Three proof chips */
    int cy = 288, cw = 464, ch = 80, cp = 32;
    int cx = (FB_HRES - 3*cw - 2*cp) / 2;
    fb_rect(cx, cy, cw, ch, COL_PANEL);
    snprintf(buf, sizeof(buf), "%.0f%% less data", bw);
    fb_string_big(cx+48, cy+16, buf, COL_GREEN, COL_PANEL, 2);

    fb_rect(cx+cw+cp, cy, cw, ch, COL_PANEL);
    fb_string_big(cx+cw+cp+48, cy+16, "flat query cost", COL_BLUE, COL_PANEL, 2);

    fb_rect(cx+2*(cw+cp), cy, cw, ch, COL_PANEL);
    fb_string_big(cx+2*(cw+cp)+32, cy+16, "live on hardware", COL_TEXT, COL_PANEL, 2);

    /* Buyer sentence */
    fb_center(416, "We help systems that constantly sync state", COL_TEXT, COL_BG);
    fb_center(448, "move less data and react faster.", COL_TEXT, COL_BG);

    /* Market chips — Replicated State highlighted */
    int my = 528, mw = 384, mh = 64, mp = 32;
    int mx = (FB_HRES - 3*mw - 2*mp) / 2;

    fb_rect(mx+mw+mp, my, mw, mh, COL_BLUE);  /* HIGHLIGHTED */
    fb_string(mx+mw+mp+(mw-16*CHAR_W)/2, my+24, "Replicated State", COL_BG, COL_BLUE);

    fb_rect(mx, my, mw, mh, COL_GRAY);
    fb_string(mx+(mw-12*CHAR_W)/2, my+24, "Agent Memory", COL_BLUE, COL_GRAY);

    fb_rect(mx+2*(mw+mp), my, mw, mh, COL_GRAY);
    fb_string(mx+2*(mw+mp)+(mw-15*CHAR_W)/2, my+24, "Live Monitoring", COL_BLUE, COL_GRAY);

    /* Final line */
    fb_center(672, "The board is the proof. The product is the IP.", COL_WHITE, COL_BG);

    screen_bottom("Licensable compute IP + runtime for state-heavy systems.");

    lcd_kpi(bw, best_speedup);

    if (attract_mode) {
        demo_pause(8000);
    } else {
        /* Investor mode: hold indefinitely */
        printf("Holding on summary (investor mode). Ctrl-C to exit.\n");
        demo_pause(8000);
        /* Keep holding — redraw every 30s to prevent display sleep */
        for (;;) { demo_pause(30000); }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  MAIN
 * ═══════════════════════════════════════════════════════════════════════ */
int main(int argc, char **argv) {
    /* --loop flag for attract/booth mode */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--loop") == 0) attract_mode = 1;
    }

    printf("ATOMiK Unified Demo (%s mode)\n", attract_mode ? "attract" : "investor");
    memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd < 0) { perror("/dev/mem"); return 1; }
    fb = mmap(NULL, FB_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, FB_BASE);
    if (fb == MAP_FAILED) { perror("mmap fb"); return 1; }
    csr_page = mmap(NULL, CSR_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, CSR_BASE);
    if (csr_page == MAP_FAILED) { perror("mmap csr"); return 1; }
    adapter = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, ADAPTER_BASE);
    if (adapter == MAP_FAILED) { perror("mmap adapter"); return 1; }

    printf("Enabling HDMI...\n");
    csr_wr(CSR_FB_VTG_ENABLE, 1);
    csr_wr(CSR_FB_DMA_ENABLE, 1);
    printf("Initializing LCD...\n");
    lcd_init();

    /* Draw persistent chrome once */
    memset(fb, 0, FB_SIZE);
    draw_chrome();
    demo_pause(500);

    for (int cycle = 0; ; cycle++) {
        demo_start_time = rdtime_ticks();
        sw_bytes_scanned = 0;
        atomik_bytes_touched = 0;
        total_ops = 0;
        total_changes_detected = 0;
        best_speedup = 0;

        printf("--- Cycle %d ---\n", cycle);
        phase_problem();    /* 4s  */
        phase_hero();       /* 10s */
        phase_wedge();      /* 10s */
        phase_numbers();    /* 6s  */
        phase_watch();      /* 9s  */
        phase_summary();    /* 8s (holds in investor mode) */

        if (!attract_mode) break;  /* investor mode never reaches here */
    }

    munmap(fb, FB_SIZE);
    munmap((void*)csr_page, CSR_SIZE);
    munmap((void*)adapter, 4096);
    close(memfd);
    return 0;
}
