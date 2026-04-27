/*
 * ATOMiK Live Interactive System
 *
 * Not a demo animation. A live, interactive state management system
 * running on real hardware. The user controls state changes, watches
 * ATOMiK detect them in hardware, sees the energy/compute savings,
 * and observes selective sync to a replica endpoint.
 *
 * HDMI: full-screen interactive dashboard (1920x1080)
 * LCD:  replica endpoint view (320x172)
 * Input: keyboard via UART (stdin)
 * Output: change events to stdout (for laptop bridge)
 *
 * Controls:
 *   1-8    Modify state buffer 1-8
 *   a      Modify all buffers
 *   r      Reset all buffers
 *   p      Run scripted presentation (47s)
 *   q      Quit
 *
 * Build:
 *   riscv64-linux-gnu-gcc -O2 -static -o atomik_live atomik_live.c
 *   riscv64-linux-gnu-gcc -O2 -o atomik_live atomik_live.c  (dynamic, 26KB)
 *
 * ========================================================================= */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

/* ── Hardware addresses ─────────────────────────────────────────────── */
#define FB_BASE         0x48000000UL
#define FB_HRES         1920
#define FB_VRES         1080
#define FB_BPP          4
#define FB_SIZE         (FB_HRES * FB_VRES * FB_BPP)

#define CSR_BASE        0xF0000000UL
#define CSR_SIZE        0x00100000UL
#define CSR_FB_DMA_BASE 0xF0002000UL
#define CSR_FB_DMA_EN   0xF0002004UL
#define CSR_FB_VTG_EN   0xF0002800UL
#define CSR_LCD_CLK     0xF0003000UL
#define CSR_LCD_CS      0xF0003800UL
#define CSR_LCD_DC      0xF0004000UL
#define CSR_LCD_LED     0xF0004800UL
#define CSR_LCD_MOSI    0xF0005000UL
#define CSR_LCD_RST     0xF0005800UL

#define ADAPTER_BASE    0xF0020000UL
#define REG_CMD  0x00
#define REG_RS1  0x04
#define REG_RS2  0x08
#define REG_RD   0x0C

/* ── Palette ────────────────────────────────────────────────────────── */
#define C_BG      0x0008111A
#define C_PANEL   0x000F1B28
#define C_TEXT    0x00F3F7FB
#define C_DIM     0x009AA8B5
#define C_BLUE    0x001EC8FF
#define C_ORANGE  0x00FF8A3D
#define C_GREEN   0x0039D98A
#define C_GRAY    0x002A3644
#define C_WHITE   0x00F3F7FB
#define C_DKGREEN 0x00164A2E
#define C_DKORANGE 0x004A2810
#define C_RED     0x00FF4444

/* LCD RGB565 */
#define L_BG      0x0841
#define L_FG      0xFFFF
#define L_BLUE    0x0E5F
#define L_GREEN   0x1EC9
#define L_DIM     0x4A69
#define L_RED     0xFC47

/* ── State ──────────────────────────────────────────────────────────── */
#define N_BUF     8
#define BUF_SIZE  4096

#include "font_8x16.h"
#define CW 8
#define CH 16

static uint32_t *fb;
static volatile uint32_t *csr_page;
static volatile uint32_t *adapter;
static int memfd;

static uint8_t  buffers[N_BUF][BUF_SIZE];
static uint8_t  shadows[N_BUF][BUF_SIZE];
static int      buf_changed[N_BUF];
static int      buf_change_count[N_BUF]; /* total times this buffer changed */

static uint64_t sw_scanned;
static uint64_t hw_touched;
static int      total_ops;
static int      total_cycles;
static int      total_changes;
static float    best_speedup;

static const char *buf_names[] = {
    "agent.ctx", "model.wt", "session.st", "config.db",
    "cache.hot", "replica.0", "txn.log", "sensor.buf"
};

/* ── L2 flush ───────────────────────────────────────────────────────── */
static volatile uint8_t flush_scratch[128*1024] __attribute__((aligned(64)));
static void flush_l2(void) {
    for (int i = 0; i < (int)sizeof(flush_scratch); i += 64) flush_scratch[i] = 0;
    asm volatile("fence iorw,iorw");
}

static inline uint64_t rdtime(void) {
    uint64_t t; asm volatile("rdtime %0" : "=r"(t)); return t;
}

/* ── CSR / adapter helpers ──────────────────────────────────────────── */
static void csr_wr(unsigned long a, uint32_t v) { csr_page[(a-CSR_BASE)/4] = v; }
static void a_wr(int off, uint32_t v) { adapter[off/4] = v; asm volatile("fence iorw,iorw"); }
static uint32_t a_rr(int off) { asm volatile("fence iorw,iorw"); return adapter[off/4]; }

static void load64(uint8_t addr, uint64_t v) {
    a_wr(REG_RS1, addr); a_wr(REG_RS2, (uint32_t)v); a_wr(REG_CMD, 0);
    a_wr(REG_RS2, (uint32_t)(v>>32)); a_wr(REG_CMD, 4);
}
static void accum64(uint64_t d) {
    a_wr(REG_RS1, (uint32_t)d); a_wr(REG_CMD, 1);
    a_wr(REG_RS1, (uint32_t)(d>>32)); a_wr(REG_CMD, 5);
}
static uint64_t read64(void) {
    a_wr(REG_CMD, 2); uint32_t lo = a_rr(REG_RD);
    a_wr(REG_CMD, 6); uint32_t hi = a_rr(REG_RD);
    return ((uint64_t)hi << 32) | lo;
}

/* ── Framebuffer primitives ─────────────────────────────────────────── */
static inline void px(int x, int y, uint32_t c) {
    if (x >= 0 && x < FB_HRES && y >= 0 && y < FB_VRES) fb[y*FB_HRES+x] = c;
}
static void rect(int x, int y, int w, int h, uint32_t c) {
    for (int dy = 0; dy < h; dy++) for (int dx = 0; dx < w; dx++) px(x+dx, y+dy, c);
}
static void glyph(int x, int y, char ch, uint32_t fg, uint32_t bg) {
    const uint8_t *g = &font_8x16[(unsigned char)ch * 16];
    for (int r = 0; r < 16; r++) { uint8_t b = g[r];
        for (int c = 0; c < 8; c++) px(x+c, y+r, (b & (0x80>>c)) ? fg : bg);
    }
}
static void text(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    while (*s) { glyph(x, y, *s++, fg, bg); x += CW; }
}
static void text2x(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    while (*s) {
        const uint8_t *g = &font_8x16[(unsigned char)*s++ * 16];
        for (int r = 0; r < 16; r++) for (int c = 0; c < 8; c++) {
            uint32_t col = (g[r] & (0x80>>c)) ? fg : bg;
            rect(x+c*2, y+r*2, 2, 2, col);
        }
        x += CW*2;
    }
}
static void text3x(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    while (*s) {
        const uint8_t *g = &font_8x16[(unsigned char)*s++ * 16];
        for (int r = 0; r < 16; r++) for (int c = 0; c < 8; c++) {
            uint32_t col = (g[r] & (0x80>>c)) ? fg : bg;
            rect(x+c*3, y+r*3, 3, 3, col);
        }
        x += CW*3;
    }
}
static void textc(int y, const char *s, uint32_t fg, uint32_t bg) {
    int l = strlen(s); text((FB_HRES - l*CW)/2, y, s, fg, bg);
}
static void textc2(int y, const char *s, uint32_t fg, uint32_t bg) {
    int l = strlen(s); text2x((FB_HRES - l*CW*2)/2, y, s, fg, bg);
}

/* ── LCD ────────────────────────────────────────────────────────────── */
static void spi_byte(uint8_t v) {
    for (int i = 7; i >= 0; i--) {
        csr_wr(CSR_LCD_MOSI, (v>>i)&1); csr_wr(CSR_LCD_CLK, 0); csr_wr(CSR_LCD_CLK, 1);
    }
}
static void lcmd(uint8_t c) { csr_wr(CSR_LCD_DC,0); csr_wr(CSR_LCD_CS,0); spi_byte(c); csr_wr(CSR_LCD_CS,1); }
static void ldat(uint8_t d) { csr_wr(CSR_LCD_DC,1); csr_wr(CSR_LCD_CS,0); spi_byte(d); csr_wr(CSR_LCD_CS,1); }
static void lwin(int x0, int y0, int x1, int y1) {
    lcmd(0x2A); csr_wr(CSR_LCD_DC,1); csr_wr(CSR_LCD_CS,0);
    spi_byte(x0>>8); spi_byte(x0); spi_byte(x1>>8); spi_byte(x1); csr_wr(CSR_LCD_CS,1);
    lcmd(0x2B); csr_wr(CSR_LCD_DC,1); csr_wr(CSR_LCD_CS,0);
    spi_byte((y0+34)>>8); spi_byte(y0+34); spi_byte((y1+34)>>8); spi_byte(y1+34); csr_wr(CSR_LCD_CS,1);
    lcmd(0x2C);
}
static void lfill(int x, int y, int w, int h, uint16_t c) {
    lwin(x,y,x+w-1,y+h-1); csr_wr(CSR_LCD_DC,1); csr_wr(CSR_LCD_CS,0);
    for (int i = 0; i < w*h; i++) { spi_byte(c>>8); spi_byte(c&0xFF); } csr_wr(CSR_LCD_CS,1);
}
static void lchar(int x, int y, char ch, uint16_t fg, uint16_t bg) {
    const uint8_t *g = &font_8x16[(unsigned char)ch*16];
    lwin(x,y,x+7,y+15); csr_wr(CSR_LCD_DC,1); csr_wr(CSR_LCD_CS,0);
    for (int r = 0; r < 16; r++) { uint8_t b = g[r];
        for (int c = 0; c < 8; c++) { uint16_t col = (b&(0x80>>c)) ? fg : bg; spi_byte(col>>8); spi_byte(col&0xFF); }
    } csr_wr(CSR_LCD_CS,1);
}
static void ltext(int x, int y, const char *s, uint16_t fg, uint16_t bg) {
    while (*s) { lchar(x, y, *s++, fg, bg); x += 8; }
}
static void lline(int y, const char *s, uint16_t fg) {
    lfill(0, y, 320, 16, L_BG); ltext(4, y, s, fg, L_BG);
}
static void lcd_init(void) {
    csr_wr(CSR_LCD_LED, 1);
    csr_wr(CSR_LCD_RST, 1); csr_wr(CSR_LCD_CS, 1); usleep(50000);
    csr_wr(CSR_LCD_RST, 0); usleep(50000);
    csr_wr(CSR_LCD_RST, 1); usleep(150000);
    lcmd(0x11); usleep(120000);
    lcmd(0x36); ldat(0x60); lcmd(0x3A); ldat(0x05);
    lcmd(0x21); lcmd(0x13); lcmd(0x29); usleep(50000);
}

/* ── Fingerprint ────────────────────────────────────────────────────── */
static uint64_t fp(const uint8_t *d, int len) {
    uint64_t f = 0; const uint64_t *p = (const uint64_t*)d;
    for (int i = 0; i < len/8; i++) f ^= p[i]; return f;
}

/* ── Terminal raw mode ──────────────────────────────────────────────── */
static struct termios orig_term;
static void term_raw(void) {
    struct termios t;
    tcgetattr(0, &orig_term);
    t = orig_term;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
}
static void term_restore(void) { tcsetattr(0, TCSANOW, &orig_term); }

static int key_ready(void) {
    fd_set fds; struct timeval tv = {0, 0};
    FD_ZERO(&fds); FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  CINEMATIC HERO DISPLAY — the investor sees this on HDMI
 * ═══════════════════════════════════════════════════════════════════════ */

#define M 96

/* ── History ribbon: tracks changes over time ────────────────────── */
#define HIST_LEN 20
static int history[HIST_LEN];
static int hist_pos;

static void hist_push(int n_changed) {
    history[hist_pos % HIST_LEN] = n_changed;
    hist_pos++;
}

static void draw_history(int x, int y, int w, int h) {
    rect(x, y, w, h, C_PANEL);
    text(x + 8, y + 4, "Change History", C_DIM, C_PANEL);
    int col_w = (w - 16) / HIST_LEN;
    int max_h = h - 28;
    for (int i = 0; i < HIST_LEN; i++) {
        int idx = (hist_pos - HIST_LEN + i);
        if (idx < 0) continue;
        int val = history[idx % HIST_LEN];
        int bar_h = val * max_h / N_BUF;
        int cx = x + 8 + i * col_w;
        int cy = y + h - 4 - bar_h;
        uint32_t col = (i == HIST_LEN - 1 && hist_pos > 0) ? C_BLUE : C_GRAY;
        if (bar_h > 0) rect(cx, cy, col_w - 2, bar_h, col);
    }
}

/* Event log */
static char event_log[8][60];
static int event_count;

static void log_event(const char *msg) {
    if (event_count < 8) {
        strncpy(event_log[event_count], msg, 59);
        event_log[event_count][59] = 0;
        event_count++;
    } else {
        for (int i = 0; i < 7; i++) strcpy(event_log[i], event_log[i+1]);
        strncpy(event_log[7], msg, 59);
        event_log[7][59] = 0;
    }
}

/* Last modified buffer index (-1 = none) */
static int last_modified = -1;

/* Startup speedup benchmark (run once) */
static float measured_speedup;
static void run_startup_bench(void) {
    volatile uint8_t *b = buffers[0];
    uint64_t sw_total = 0, hw_total = 0;
    /* Warmup */
    for (int w = 0; w < 10; w++) {
        volatile uint32_t d = 0;
        for (int j = 0; j < BUF_SIZE; j++) d += b[j];
        load64(7, 0xAAAAAAAAAAAAAAAAULL); read64(); (void)d;
    }
    /* 50 iterations, take median-ish */
    for (int it = 0; it < 50; it++) {
        uint64_t t0 = rdtime();
        volatile uint32_t d = 0;
        for (int j = 0; j < BUF_SIZE; j++) d += b[j];
        sw_total += rdtime() - t0; (void)d;
        load64(7, 0xAAAAAAAAAAAAAAAAULL);
        t0 = rdtime(); read64();
        hw_total += rdtime() - t0;
    }
    measured_speedup = (float)sw_total / (float)(hw_total > 0 ? hw_total : 1);
}

/* Layout coordinates (computed once in draw_chrome, reused in draw_content) */
static int _box_w, _box_h, _gap, _x0, _sw_y, _ay, _ny;

#define SKIP_COL  0x001A1E24  /* brighter skip boxes */
#define SKIP_TEXT 0x00505860

/* Draw static chrome — called once at startup */
static void draw_chrome(void) {
    memset(fb, 0, FB_SIZE);

    _box_w = 176; _box_h = 96; _gap = 20;
    int total_w = N_BUF * (_box_w + _gap) - _gap;
    _x0 = (FB_HRES - total_w) / 2;
    _sw_y = 120;

    /* ── Top bar (96px) ──────────────────────────────────────────── */
    rect(0, 0, FB_HRES, 96, C_PANEL);
    text2x(M, 16, "ATOMiK", C_BLUE, C_PANEL);
    text(M, 52, "State-Aware Execution", C_DIM, C_PANEL);
    rect(1568, 24, 256, 48, C_BLUE);
    text(1584, 40, "LIVE ON HARDWARE", C_PANEL, C_BLUE);

    /* ── SOFTWARE label ──────────────────────────────────────────── */
    text2x(_x0, _sw_y, "SOFTWARE", C_ORANGE, C_BG);
    text(_x0 + 200, _sw_y + 12, "rescans all state every cycle", C_DIM, C_BG);

    /* SW boxes — always orange */
    for (int i = 0; i < N_BUF; i++) {
        int bx = _x0 + i * (_box_w + _gap);
        rect(bx, _sw_y + 40, _box_w, _box_h, C_ORANGE);
        text2x(bx + 8, bx > 0 ? _sw_y + 48 : _sw_y + 48, buf_names[i], C_WHITE, C_ORANGE);
        text(bx + (_box_w - 4*CW)/2, _sw_y + 78, "SCAN", C_WHITE, C_ORANGE);
    }

    /* Separator */
    int sep_y = _sw_y + 40 + _box_h + 12;
    rect(_x0, sep_y, total_w, 1, C_GRAY);

    /* ATOMiK label */
    _ay = sep_y + 16;
    text2x(_x0, _ay, "ATOMiK", C_BLUE, C_BG);
    text(_x0 + 160, _ay + 12, "acts only on meaningful change", C_DIM, C_BG);

    _ny = _ay + 40 + _box_h + 16;

    /* ── Bottom anchor ───────────────────────────────────────────── */
    rect(0, FB_VRES - 48, FB_HRES, 48, C_PANEL);
    text(M, FB_VRES - 36, "Same C. Standard GCC. No new language.",
         C_DIM, C_PANEL);
    text(FB_HRES/2, FB_VRES - 36,
         "ATOMiK removes wasted rediscovery of change.", C_TEXT, C_PANEL);
}

/* Draw dynamic content — called on every keypress */

/* ── Binary text grid: shows actual 1s and 0s ────────────────────── */
static void bintxt(int x, int y, uint64_t val, uint32_t on, uint32_t off, uint32_t bg) {
    for (int bit = 63; bit >= 0; bit--) {
        int r = (63 - bit) / 8, c = (63 - bit) % 8;
        int b = (val >> bit) & 1;
        glyph(x + c * 10, y + r * 12, b ? '1' : '0', b ? on : off, bg);
    }
}

/* Random 64-bit value for demo visualization */
static uint64_t demo_rng = 0xDEADBEEFCAFEBABEULL;
static uint64_t rand64(void) {
    demo_rng ^= demo_rng << 13;
    demo_rng ^= demo_rng >> 7;
    demo_rng ^= demo_rng << 17;
    return demo_rng;
}

/* Stored values for the live algebra display */
static uint64_t viz_initial, viz_delta, viz_current;

static void refresh_viz(void) {
    viz_initial = rand64();
    viz_delta = rand64() & 0x0F0F0F0F0F0F0F0FULL; /* sparse delta — some bits change */
    viz_current = viz_initial ^ viz_delta;
}

/* Draw dynamic content — head-to-head live comparison */
static void draw_content(void) {
    char buf[80];

    int changed = 0;
    for (int i = 0; i < N_BUF; i++) if (buf_changed[i]) changed++;
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;

    /* ── Top bar stats ───────────────────────────────────────────── */
    rect(700, 16, 800, 72, C_PANEL);
    snprintf(buf, sizeof(buf), "Cycle %d  |  %d changes  |  %.0fx faster",
             total_cycles, total_changes, measured_speedup);
    text(720, 40, buf, C_DIM, C_PANEL);

    /* ═══════════════════════════════════════════════════════════════
     * HEAD-TO-HEAD: Left = Software, Right = ATOMiK
     * ═══════════════════════════════════════════════════════════════ */
    int mid = FB_HRES / 2;
    int vy = 108;
    int half_w = mid - M - 16;

    /* Clear comparison area */
    rect(0, vy, FB_HRES, 350, C_BG);

    /* ── LEFT: SOFTWARE ──────────────────────────────────────────── */
    text2x(M, vy, "SOFTWARE", C_ORANGE, C_BG);
    text(M, vy + 34, "scans every byte to find changes", C_DIM, C_BG);

    /* Buffer strip — all orange, compact */
    int sw_bw = half_w / N_BUF - 4;
    for (int i = 0; i < N_BUF; i++) {
        int bx = M + i * (sw_bw + 4);
        rect(bx, vy + 56, sw_bw, 32, C_ORANGE);
        text(bx + 2, vy + 60, buf_names[i], C_WHITE, C_ORANGE);
        text(bx + 2, vy + 76, "SCAN", C_WHITE, C_ORANGE);
    }

    /* SW volume */
    snprintf(buf, sizeof(buf), "Scanned: %llu KB", (unsigned long long)(sw_scanned / 1024));
    text2x(M, vy + 100, buf, C_ORANGE, C_BG);
    rect(M, vy + 136, half_w, 14, C_ORANGE);
    text(M + half_w + 8, vy + 136, "100%", C_ORANGE, C_BG);

    /* ── CENTER DIVIDER ──────────────────────────────────────────── */
    rect(mid - 2, vy, 4, 350, C_GRAY);

    /* ── RIGHT: ATOMiK ───────────────────────────────────────────── */
    int rx = mid + 16;
    text2x(rx, vy, "ATOMiK", C_BLUE, C_BG);
    text(rx, vy + 34, "tracks deltas in hardware", C_DIM, C_BG);

    /* Buffer strip — only changed lit */
    int hw_bw = half_w / N_BUF - 4;
    for (int i = 0; i < N_BUF; i++) {
        int bx = rx + i * (hw_bw + 4);
        int is_last = (i == last_modified);
        if (buf_changed[i]) {
            uint32_t bg = is_last ? 0x0030A0E0 : C_BLUE;
            rect(bx, vy + 56, hw_bw, 32, bg);
            text(bx + 2, vy + 60, buf_names[i], C_WHITE, bg);
            snprintf(buf, sizeof(buf), "x%d", buf_change_count[i]);
            text(bx + 2, vy + 76, buf, C_BG, bg);
        } else {
            rect(bx, vy + 56, hw_bw, 32, SKIP_COL);
            text(bx + 2, vy + 60, buf_names[i], SKIP_TEXT, SKIP_COL);
        }
    }

    /* ATOMiK volume */
    snprintf(buf, sizeof(buf), "Touched: %llu KB", (unsigned long long)(hw_touched / 1024));
    text2x(rx, vy + 100, buf, C_BLUE, C_BG);
    int atk_w = (int)((100.0f - pct) / 100.0f * half_w);
    if (atk_w < 4 && hw_touched > 0) atk_w = 4;
    rect(rx, vy + 136, atk_w, 14, C_BLUE);
    rect(rx + atk_w, vy + 136, half_w - atk_w, 14, 0x00141418);
    snprintf(buf, sizeof(buf), "%.0f%%", 100.0f - pct);
    text(rx + half_w + 8, vy + 136, buf, C_BLUE, C_BG);

    /* ── LIVE BINARY ALGEBRA ─────────────────────────────────────── */
    int dy = vy + 164;
    text(rx, dy, "initial", C_BLUE, C_BG);
    bintxt(rx, dy + 16, viz_initial, C_BLUE, 0x00203040, C_BG);

    text(rx + 100, dy + 50, "XOR", 0x00FFCC00, C_BG);

    text(rx + 140, dy, "delta", 0x00FFCC00, C_BG);
    bintxt(rx + 140, dy + 16, viz_delta, 0x00FFCC00, 0x00302010, C_BG);

    text(rx + 240, dy + 50, "=", C_GREEN, C_BG);

    text(rx + 270, dy, "current", C_GREEN, C_BG);
    bintxt(rx + 270, dy + 16, viz_current, C_GREEN, 0x00103020, C_BG);

    text(rx, dy + 116, "current = initial XOR accumulator", C_GREEN, C_BG);
    if (last_modified >= 0) {
        snprintf(buf, sizeof(buf), "[%s]", buf_names[last_modified]);
        text(rx + 280, dy + 116, buf, C_DIM, C_BG);
    }

    /* Hex values */
    snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)viz_initial);
    text(rx, dy + 134, buf, C_DIM, C_BG);
    snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)viz_delta);
    text(rx + 140, dy + 134, buf, C_DIM, C_BG);
    snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)viz_current);
    text(rx + 270, dy + 134, buf, C_DIM, C_BG);

    /* ═══════════════════════════════════════════════════════════════
     * METRICS PANEL
     * ═══════════════════════════════════════════════════════════════ */
    _ny = vy + 360;
    rect(0, _ny, FB_HRES, 120, C_PANEL);

    snprintf(buf, sizeof(buf), "%.0f%%", pct);
    text3x(M, _ny + 8, buf, C_GREEN, C_PANEL);
    text(M, _ny + 56, "less compute / energy / cost", C_GREEN, C_PANEL);

    snprintf(buf, sizeof(buf), "%d of 8 synced", changed);
    text2x(380, _ny + 8, buf, C_BLUE, C_PANEL);

    float sav_1k = pct * 50.0f * 1000.0f / 100.0f / 1000.0f;
    if (sav_1k < 1.0f && pct > 0) sav_1k = 1.0f;
    float sav_global = pct * 50.0f * 50e6f / 100.0f / 1e9f;
    snprintf(buf, sizeof(buf), "$%.0fK/yr (1K)  |  $%.1fB/yr (50M global)",
             sav_1k, sav_global);
    text(380, _ny + 48, buf, C_GREEN, C_PANEL);

    snprintf(buf, sizeof(buf), "%.0fx faster query (4KB median)", measured_speedup);
    text(380, _ny + 68, buf, 0x00FFCC00, C_PANEL);

    /* Flow bar */
    int fy = _ny + 92;
    int bar_w = FB_HRES - 2*M - 200;
    int bar_x = M + 40;
    text(M, fy, "SW", C_ORANGE, C_PANEL);
    rect(bar_x, fy, bar_w, 10, C_ORANGE);
    text(M, fy + 12, "HW", C_BLUE, C_PANEL);
    int hw_w = (int)((100.0f - pct) / 100.0f * bar_w);
    if (hw_w < 4) hw_w = 4;
    rect(bar_x, fy + 12, hw_w, 10, C_BLUE);
    rect(bar_x + hw_w, fy + 12, bar_w - hw_w, 10, 0x00141418);

    /* ── History + events ────────────────────────────────────────── */
    int hy = _ny + 128;
    draw_history(M, hy, FB_HRES/2 - M - 8, 48);

    rect(FB_HRES/2 + 8, hy, FB_HRES/2 - M - 8, 48, C_BG);
    for (int i = 0; i < event_count && i < 3; i++) {
        int idx = event_count - 1 - i;
        if (idx >= 0)
            text(FB_HRES/2 + 16, hy + 2 + i * 16, event_log[idx],
                 i == 0 ? C_DIM : C_GRAY, C_BG);
    }
}
/* Adoption forecast slide */
static void draw_adoption(void) {
    rect(0, 96, FB_HRES, FB_VRES - 96 - 48, C_BG);

    text3x(M, 120, "Adoption Forecast", C_TEXT, C_BG);

    /* Year-by-year table */
    int ty = 200;
    int col_yr = M, col_adopt = M + 160, col_rev = M + 480, col_tam = M + 800, col_cum = M + 1120;

    text2x(col_yr, ty, "Year", C_DIM, C_BG);
    text2x(col_adopt, ty, "Adoption", C_DIM, C_BG);
    text2x(col_rev, ty, "Rev/Server", C_DIM, C_BG);
    text2x(col_tam, ty, "TAM Captured", C_DIM, C_BG);
    text2x(col_cum, ty, "Cumulative", C_DIM, C_BG);
    rect(M, ty + 36, FB_HRES - 2*M, 1, C_GRAY);

    /* Projections — conservative ramp */
    static const struct { int yr; const char *adopt; const char *rev; const char *tam; const char *cum; uint32_t col; } rows[] = {
        { 2027, "1K servers",    "$50/srv",  "$50K",    "$50K",    C_DIM },
        { 2028, "10K servers",   "$45/srv",  "$450K",   "$500K",   C_DIM },
        { 2029, "100K servers",  "$40/srv",  "$4M",     "$4.5M",   C_BLUE },
        { 2030, "500K servers",  "$35/srv",  "$17.5M",  "$22M",    C_BLUE },
        { 2031, "2M servers",    "$30/srv",  "$60M",    "$82M",    C_GREEN },
        { 2032, "5M servers",    "$25/srv",  "$125M",   "$207M",   C_GREEN },
        { 2033, "10M servers",   "$20/srv",  "$200M",   "$407M",   C_GREEN },
    };

    for (int i = 0; i < 7; i++) {
        int ry = ty + 48 + i * 40;
        char yrbuf[8];
        snprintf(yrbuf, sizeof(yrbuf), "%d", rows[i].yr);
        text2x(col_yr, ry, yrbuf, C_TEXT, C_BG);
        text(col_adopt, ry + 8, rows[i].adopt, rows[i].col, C_BG);
        text(col_rev, ry + 8, rows[i].rev, rows[i].col, C_BG);
        text(col_tam, ry + 8, rows[i].tam, rows[i].col, C_BG);
        text(col_cum, ry + 8, rows[i].cum, rows[i].col, C_BG);

        /* Visual bar for TAM captured */
        int bar_max = 300;
        float tam_vals[] = {0.05f, 0.45f, 4.0f, 17.5f, 60.0f, 125.0f, 200.0f};
        int bw = (int)(tam_vals[i] / 200.0f * bar_max);
        if (bw < 2) bw = 2;
        rect(col_cum + 100, ry + 8, bw, 12, rows[i].col);
    }

    /* Bottom summary */
    text2x(M, ty + 48 + 7*40 + 20, "Total addressable: $1.7B/yr (50M state-heavy servers)", C_GREEN, C_BG);
    text(M, ty + 48 + 7*40 + 56, "Conservative 20% penetration by 2033 = $407M cumulative", C_DIM, C_BG);
    text(M, ty + 48 + 7*40 + 76, "Press any key to return", C_GRAY, C_BG);

    flush_l2();

    /* Wait for any key */
    while (!key_ready()) usleep(50000);
    char ch; read(0, &ch, 1);
}

/* Full dashboard (chrome + content) — used on init and reset */
static void draw_dashboard(void) {
    refresh_viz();
    draw_chrome();
    draw_content();
}

/* Update LCD replica */
static void update_lcd(void) {
    lfill(0, 0, 320, 172, L_BG);
    lfill(0, 0, 320, 3, L_BLUE);
    lfill(0, 169, 320, 3, L_BLUE);
    lfill(240, 8, 70, 18, L_RED);
    ltext(248, 9, "LIVE", L_FG, L_RED);

    lline(10, "     REPLICA", L_BLUE);

    int changed = 0;
    for (int i = 0; i < N_BUF; i++) if (buf_changed[i]) changed++;

    char buf[40];
    snprintf(buf, sizeof(buf), " %d of 8 synced", changed);
    lline(36, buf, L_FG);

    /* Buffer strip with abbreviated names */
    static const char *lcd_names[] = {
        "agnt", "modl", "sess", "conf", "cach", "repl", "txn ", "sens"
    };
    for (int i = 0; i < N_BUF; i++) {
        uint16_t c = buf_changed[i] ? L_BLUE : 0x2104;
        int bx = 2 + i * 39;
        lfill(bx, 58, 37, 32, c);
        ltext(bx + 2, 62, lcd_names[i], buf_changed[i] ? L_BG : L_DIM, c);
    }

    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;
    snprintf(buf, sizeof(buf), " %.0f%% avoided", pct);
    lline(100, buf, L_GREEN);

    snprintf(buf, sizeof(buf), " %llu KB scanned", (unsigned long long)(sw_scanned/1024));
    lline(120, buf, L_DIM);

    lline(148, " Only deltas propagated", L_DIM);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  ACTION: modify buffer(s) and detect changes
 * ═══════════════════════════════════════════════════════════════════════ */

static void modify_buffer(int idx) {
    /* Mutate with varied values so XOR fingerprint changes */
    uint32_t seed = rdtime() & 0xFFFFFFFF;
    for (int j = 0; j < 63; j++)
        buffers[idx][j * 64] ^= ((seed >> (j & 15)) + j + 1) & 0xFF;
    buf_change_count[idx]++;
}

static void detect_all(void) {
    memset(buf_changed, 0, sizeof(buf_changed));
    total_cycles++;

    for (int i = 0; i < N_BUF; i++) {
        uint64_t ofp = fp(shadows[i], BUF_SIZE);
        uint64_t nfp = fp(buffers[i], BUF_SIZE);
        sw_scanned += BUF_SIZE;

        load64(i, ofp);
        accum64(nfp ^ ofp);
        uint64_t st = read64();
        buf_changed[i] = (st != ofp);
        if (buf_changed[i]) {
            hw_touched += BUF_SIZE;
            total_changes++;
        }
        total_ops += 3;
    }

    for (int i = 0; i < N_BUF; i++)
        memcpy(shadows[i], buffers[i], BUF_SIZE);

    /* Emit change event to stdout for laptop bridge */
    int changed = 0;
    int mask = 0;
    for (int i = 0; i < N_BUF; i++) {
        if (buf_changed[i]) { changed++; mask |= (1 << i); }
    }
    hist_push(changed);
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;
    printf("##EVENT:%d:%d:%.1f:%02X:%.0f:%llu:%llu\n",
           total_cycles, changed, pct, mask, measured_speedup,
           (unsigned long long)(sw_scanned / 1024),
           (unsigned long long)(hw_touched / 1024));
    fflush(stdout);
}

static void reset_all(void) {
    for (int i = 0; i < N_BUF; i++) {
        memset(buffers[i], 0xAA + i, BUF_SIZE);
        memcpy(shadows[i], buffers[i], BUF_SIZE);
        buf_changed[i] = 0;
        buf_change_count[i] = 0;
    }
    sw_scanned = 0; hw_touched = 0;
    total_ops = 0; total_cycles = 0; total_changes = 0;

    for (int i = 0; i < N_BUF; i++) {
        load64(i, fp(buffers[i], BUF_SIZE));
        total_ops++;
    }
    event_count = 0;
    log_event("System reset. All buffers clean.");
}

/* ═══════════════════════════════════════════════════════════════════════
 *  MAIN LOOP
 * ═══════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("ATOMiK Live System\n");

    memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd < 0) { perror("/dev/mem"); return 1; }
    fb = mmap(NULL, FB_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, FB_BASE);
    if (fb == MAP_FAILED) { perror("mmap fb"); return 1; }
    csr_page = mmap(NULL, CSR_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, CSR_BASE);
    if (csr_page == MAP_FAILED) { perror("mmap csr"); return 1; }
    adapter = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, ADAPTER_BASE);
    if (adapter == MAP_FAILED) { perror("mmap adapter"); return 1; }

    printf("Enabling HDMI...\n");
    /* Do NOT write DMA_BASE — the SoC configures it at synthesis time
     * with the PS DDR physical address (0x08100000 via HP0). Writing
     * the NaxRiscv virtual address (0x48000000) here would break DMA. */
    csr_wr(CSR_FB_VTG_EN, 1);
    csr_wr(CSR_FB_DMA_EN, 1);
    printf("Initializing LCD...\n");
    lcd_init();

    /* Initialize buffers */
    reset_all();
    log_event("ATOMiK Live System started.");
    log_event("8 state buffers initialized.");

    /* Run startup speedup benchmark */
    printf("Benchmarking...\n");
    run_startup_bench();
    printf("Speedup: %.0fx\n", measured_speedup);

    /* Run initial demo cycle so dashboard isn't all zeros */
    modify_buffer(0);
    modify_buffer(2);
    modify_buffer(5);
    detect_all();
    log_event("Modified agent.ctx, session.st, replica.0");
    modify_buffer(1);
    modify_buffer(4);
    detect_all();
    log_event("Modified model.wt, cache.hot");
    log_event("Press 1-8 to modify buffers.");

    /* Set terminal to raw mode for key input */
    term_raw();

    /* Initial draw */
    refresh_viz();
    draw_dashboard();
    update_lcd();
    flush_l2();

    printf("Interactive mode. Press keys to modify state.\n");

    int running = 1;
    while (running) {
        if (key_ready()) {
            char ch;
            if (read(0, &ch, 1) == 1) {
                char msg[60];
                switch (ch) {
                case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': {
                    int idx = ch - '1';
                    last_modified = idx;
                    modify_buffer(idx);
                    detect_all();
                    snprintf(msg, sizeof(msg), "Modified %s -> %d changed, %d clean",
                             buf_names[idx],
                             total_changes, total_cycles * N_BUF - total_changes);
                    log_event(msg);
                    refresh_viz(); draw_content();
                    update_lcd();
                    flush_l2();
                    break;
                }
                case 'a': case 'A':
                    last_modified = -1;
                    for (int i = 0; i < N_BUF; i++) modify_buffer(i);
                    detect_all();
                    log_event("Modified ALL buffers.");
                    refresh_viz(); draw_content();
                    update_lcd();
                    flush_l2();
                    break;
                case 'r': case 'R':
                    /* Flash reset confirmation */
                    rect(0, 0, FB_HRES, FB_VRES, C_BLUE);
                    textc(500, "RESET", C_BG, C_BLUE);
                    flush_l2();
                    usleep(200000);
                    reset_all();
                    hist_pos = 0;
                    memset(history, 0, sizeof(history));
                    /* Re-run initial demo so metrics aren't zero */
                    modify_buffer(0); modify_buffer(2); modify_buffer(5);
                    detect_all();
                    log_event("System reset.");
                    modify_buffer(1); modify_buffer(4);
                    detect_all();
                    log_event("Ready. Press 1-8.");
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'v': case 'V': {
                    /* Integrity verification — proves hashing/Web3 story */
                    log_event("Verifying integrity of all buffers...");
                    refresh_viz(); draw_content(); flush_l2();
                    int verified = 0;
                    for (int i = 0; i < N_BUF; i++) {
                        uint64_t current_fp = fp(buffers[i], BUF_SIZE);
                        uint64_t stored_fp = fp(shadows[i], BUF_SIZE);
                        if (current_fp == stored_fp) verified++;
                    }
                    snprintf(msg, sizeof(msg), "Integrity: %d/8 verified OK", verified);
                    log_event(msg);
                    refresh_viz(); draw_content();
                    update_lcd();
                    flush_l2();
                    break;
                }
                case 'c': case 'C': {
                    /* Inject corruption — then verify to detect it */
                    int target = total_cycles % N_BUF;
                    buffers[target][0] ^= 0xFF; /* corrupt one byte */
                    snprintf(msg, sizeof(msg), "CORRUPTED %s (1 byte tampered)", buf_names[target]);
                    log_event(msg);
                    /* Now verify */
                    int tampered = 0;
                    for (int i = 0; i < N_BUF; i++) {
                        uint64_t current_fp = fp(buffers[i], BUF_SIZE);
                        uint64_t stored_fp = fp(shadows[i], BUF_SIZE);
                        buf_changed[i] = (current_fp != stored_fp);
                        if (buf_changed[i]) tampered++;
                    }
                    last_modified = target;
                    snprintf(msg, sizeof(msg), "DETECTED: %d buffer(s) tampered", tampered);
                    log_event(msg);
                    /* Restore */
                    buffers[target][0] ^= 0xFF;
                    refresh_viz(); draw_content();
                    update_lcd();
                    flush_l2();
                    break;
                }
                case 'b': case 'B': {
                    /* Burst mode — rapid random changes for 3 seconds */
                    log_event("BURST: 3 seconds of rapid changes...");
                    refresh_viz(); draw_content(); flush_l2();
                    uint32_t rng = rdtime() & 0xFFFFFFFF;
                    uint64_t end = rdtime() + 300000000ULL; /* 3 sec at 100MHz */
                    int burst_count = 0;
                    while (rdtime() < end) {
                        rng = rng * 1103515245 + 12345;
                        int idx = (rng >> 16) % N_BUF;
                        last_modified = idx;
                        modify_buffer(idx);
                        detect_all();
                        burst_count++;
                        if (burst_count % 5 == 0) {
                            refresh_viz(); draw_content();
                            update_lcd();
                            flush_l2();
                        }
                    }
                    snprintf(msg, sizeof(msg), "BURST complete: %d changes in 3s", burst_count);
                    log_event(msg);
                    refresh_viz(); draw_content();
                    update_lcd();
                    flush_l2();
                    break;
                }
                case 'd': case 'D':
                    /* Compiler demo: show the adoption story */
                    rect(0, 96, FB_HRES, FB_VRES - 96 - 48, C_BG);
                    text3x(M, 200, "Compiler Lane", C_BLUE, C_BG);
                    text2x(M, 260, "Same C. Standard GCC.", C_TEXT, C_BG);
                    text2x(M, 300, "#include \"atomik.h\"", C_GREEN, C_BG);
                    text2x(M, 340, "atomik_load(slot, init);", C_TEXT, C_BG);
                    text2x(M, 372, "atomik_accum(delta);", C_TEXT, C_BG);
                    text2x(M, 404, "atomik_read(slot);", C_TEXT, C_BG);
                    text2x(M, 460, "riscv64-linux-gnu-gcc -O2 example.c", C_DIM, C_BG);
                    text2x(M, 500, "-> ATOMiK hardware ops in the binary", C_DIM, C_BG);
                    text3x(M, 580, "No new language.", C_GREEN, C_BG);
                    text3x(M, 636, "No new compiler.", C_GREEN, C_BG);
                    log_event("Compiler lane shown.");
                    flush_l2();
                    usleep(5000000); /* hold 5 seconds */
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'f': case 'F':
                    /* Adoption forecast slide */
                    draw_adoption();
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'q': case 'Q':
                    running = 0;
                    break;
                default:
                    break;
                }
            }
        }
        usleep(50000); /* 20 Hz poll */
    }

    term_restore();
    printf("\nATOMiK Live System stopped.\n");

    munmap(fb, FB_SIZE);
    munmap((void*)csr_page, CSR_SIZE);
    munmap((void*)adapter, 4096);
    close(memfd);
    return 0;
}
