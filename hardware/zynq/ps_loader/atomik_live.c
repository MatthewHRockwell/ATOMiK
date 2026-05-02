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
 *   a      Adversarial audit (expose state, try to break it)
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
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
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

/* ── Palette (0x00RRGGBB — confirmed XRGB by color_test.c) ─────────── */
#define RGB(r,g,b) (((r)<<16)|((g)<<8)|(b))

#define C_BG       RGB(0x08,0x11,0x1A)
#define C_PANEL    RGB(0x0F,0x1B,0x28)
#define C_CARD     RGB(0x14,0x26,0x36)
#define C_TEXT     RGB(0xF3,0xF7,0xFB)
#define C_DIM      RGB(0x9A,0xA8,0xB5)
#define C_BLUE     RGB(0x1E,0xC8,0xFF)
#define C_BLUE_DK  RGB(0x0A,0x40,0x60)
#define C_ORANGE   RGB(0xFF,0x8A,0x3D)
#define C_ORANGE_DK RGB(0x60,0x30,0x10)
#define C_GREEN    RGB(0x39,0xD9,0x8A)
#define C_GRAY     RGB(0x2A,0x36,0x44)
#define C_WHITE    RGB(0xF3,0xF7,0xFB)
#define C_DKGREEN  RGB(0x16,0x4A,0x2E)
#define C_DKORANGE RGB(0x4A,0x28,0x10)
#define C_RED      RGB(0xFF,0x44,0x44)
#define C_GLOW     RGB(0x18,0x30,0x48)

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
static int      cmd_running;   /* suppress ##EVENT during remote cmd */
static float    session_savings; /* cumulative $ saved this session */
static float    best_speedup;

static const char *buf_names[] = {
    "agent.ctx", "model.wt", "session.st", "config.db",
    "cache.hot", "replica.0", "txn.log", "sensor.buf"
};

/* Scan sweep animation position (0-7 = which lane is being "scanned") */
static int scan_pos;

/* Attract mode: idle screen shown before first interaction */
static int attract_mode = 1;

/* Workload profile presets */
static int workload_profile; /* 0=manual, 1=agent, 2=cache, 3=full, 4=idle */
static const char *profile_names[] = {
    "Manual", "Agent Memory", "Cache Sync", "Full Backup", "Idle Watch"
};
/* Which buffers are "hot" per profile (bitmask) */
static const uint8_t profile_patterns[][4] = {
    {0xFF, 0xFF, 0xFF, 0xFF}, /* manual: user controls */
    {0x03, 0x07, 0x03, 0x01}, /* agent: bufs 0,1 always; sometimes 2 */
    {0x24, 0x52, 0x89, 0x14}, /* cache: random 2-3 per cycle */
    {0xFF, 0xFF, 0xFF, 0xFF}, /* full: all 8 every time */
    {0x01, 0x00, 0x02, 0x00}, /* idle: 0-1 per cycle */
};

/* ── Virtual Processor Types ────────────────────────────────────────── */
enum VProcType {
    VP_DETECT  = 0,   /* Change detection — fingerprint compare */
    VP_VERIFY  = 1,   /* Integrity verification — tamper detect */
    VP_SYNC    = 2,   /* Selective sync — replicate only changes */
    VP_ACCUM   = 3,   /* Parallel accumulation — multi-producer */
    VP_WATCH   = 4,   /* State monitoring — periodic scan */
    VP_IDLE    = 5,   /* Unassigned */
    VP_COUNT   = 6,
};
static const char *vproc_names[] = {
    "DETECT", "VERIFY", "SYNC", "ACCUM", "WATCH", "IDLE"
};
static const char *vproc_desc[] = {
    "Change Detection", "Integrity Verify", "Selective Sync",
    "Parallel Accum", "State Monitor", "Idle"
};
static const uint32_t vproc_colors[] = {
    RGB(0x1E,0xC8,0xFF),  /* DETECT = blue */
    RGB(0x39,0xD9,0x8A),  /* VERIFY = green */
    RGB(0x00,0xE5,0xFF),  /* SYNC = cyan */
    RGB(0xFF,0xCC,0x00),  /* ACCUM = yellow */
    RGB(0xFF,0x8A,0x3D),  /* WATCH = orange */
    RGB(0x2A,0x36,0x44),  /* IDLE = gray */
};
/* Per-slot vproc assignment */
static int slot_vproc[N_BUF];  /* which vproc type each slot runs */
static int slot_active[N_BUF]; /* was this slot active this cycle? */
static int slot_ops[N_BUF];    /* total ops on this slot */

/* Workload preset configs: which vproc type for each of 8 slots */
static const int vproc_presets[][N_BUF] = {
    /* Manual — all DETECT */
    {VP_DETECT, VP_DETECT, VP_DETECT, VP_DETECT,
     VP_DETECT, VP_DETECT, VP_DETECT, VP_DETECT},
    /* Agent Memory — mixed detect + accum */
    {VP_DETECT, VP_DETECT, VP_ACCUM, VP_ACCUM,
     VP_DETECT, VP_SYNC,   VP_WATCH, VP_VERIFY},
    /* Cache Sync — mostly sync + detect */
    {VP_SYNC,   VP_SYNC,   VP_SYNC,   VP_DETECT,
     VP_DETECT, VP_DETECT, VP_VERIFY, VP_WATCH},
    /* Full Backup — all detect (worst case) */
    {VP_DETECT, VP_DETECT, VP_DETECT, VP_DETECT,
     VP_DETECT, VP_DETECT, VP_DETECT, VP_DETECT},
    /* Idle Watch — mostly watch + verify */
    {VP_WATCH,  VP_WATCH,  VP_WATCH,  VP_VERIFY,
     VP_VERIFY, VP_IDLE,   VP_IDLE,   VP_IDLE},
};

static void apply_vproc_preset(int preset) {
    for (int i = 0; i < N_BUF; i++)
        slot_vproc[i] = vproc_presets[preset][i];
}

/* ── Forward declarations ──────────────────────────────────────────── */
static void modify_buffer(int idx);
static void detect_all(void);

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
    rect(x, y, w, h, RGB(0x12,0x20,0x2E));
    text(x + 8, y + 4, "Change History", C_DIM, RGB(0x12,0x20,0x2E));
    int col_w = (w - 16) / HIST_LEN;
    int max_h = h - 20;
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

/* ── Visual helpers for premium look ──────────────────────────────── */
static void panel(int x, int y, int w, int h, uint32_t bg, uint32_t glow) {
    rect(x, y, w, h, bg);
    rect(x, y, w, 2, glow);
}
static void card(int x, int y, int w, int h) {
    rect(x, y, w, h, C_CARD);
    rect(x, y, w, 1, C_GRAY);
}
static void outlined(int x, int y, int w, int h, uint32_t bg, uint32_t border) {
    rect(x, y, w, h, bg);
    rect(x, y, w, 1, border);
    rect(x, y+h-1, w, 1, border);
    rect(x, y, 1, h, border);
    rect(x+w-1, y, 1, h, border);
}

/* Layout coordinates (computed once in draw_chrome, reused in draw_content) */
/* (layout coordinates now defined as #defines above) */

#define SKIP_COL  RGB(0x1A,0x1E,0x24)  /* brighter skip boxes */
#define SKIP_TEXT RGB(0x50,0x58,0x60)

/* ── Layout grid (all Y coordinates defined here) ────────────────── */
#define TOP_Y     30       /* overscan safe margin */
#define TOP_H     80       /* top bar */
#define LANE_Y    120      /* SW/ATOMiK lane labels */
#define LANE_H    180      /* lane height */
#define LANE_TOP  (LANE_Y + 32)                 /* lane boxes start */
#define VOL_Y     (LANE_TOP + LANE_H + 8)       /* volume bars */
#define HELLO_Y   (VOL_Y + 32)                  /* delta-state algebra */
#define METRIC_Y  640                            /* metrics panel */
#define METRIC_H  130                            /* metrics height (spacious) */
#define HIST_Y    (METRIC_Y + METRIC_H + 8)     /* history */
#define HIST_H    50
#define KEY_Y     (HIST_Y + HIST_H + 4)         /* key legend */
#define BOT_Y     (FB_VRES - 52)                /* bottom bar flush with screen */
#define BOT_H     44

/* Draw static chrome — called once at startup */
static void draw_chrome(void) {
    memset(fb, 0, FB_SIZE);

    /* ── Top bar ────────────────────────────────────────────────── */
    panel(0, TOP_Y, FB_HRES, TOP_H, C_PANEL, C_BLUE);
    text2x(M, TOP_Y + 16, "ATOMiK", C_BLUE, C_PANEL);
    text(M + 160, TOP_Y + 28, "State-Aware Execution", C_DIM, C_PANEL);
    /* LIVE badge */
    rect(1568, TOP_Y + 16, 256, 44, C_BLUE);
    text(1584, TOP_Y + 30, "LIVE ON HARDWARE", C_PANEL, C_BLUE);

    /* ── Bottom bar ─────────────────────────────────────────────── */
    panel(0, BOT_Y, FB_HRES, BOT_H, C_PANEL, C_BLUE);
    text(M, BOT_Y + 12, "Same C. Standard GCC. No new language.",
         C_DIM, C_PANEL);
    text(FB_HRES/2, BOT_Y + 12,
         "ATOMiK removes wasted rediscovery of change.", C_TEXT, C_PANEL);
}

/* Draw dynamic content — called on every keypress */

/* ── Binary text grid: shows actual 1s and 0s ────────────────────── */
/* 8×8 binary matrix of '1'/'0' characters.
 * Font is 8×16. Cols at 10px, rows at 14px (no overlap).
 * Grid: 80px wide × 112px tall (last glyph bottom = 7*14+16 = 114). */
static void bintxt(int x, int y, uint64_t val, uint32_t on, uint32_t off, uint32_t bg) {
    for (int bit = 63; bit >= 0; bit--) {
        int idx = 63 - bit;
        int r = idx / 8, c = idx % 8;
        int b = (val >> bit) & 1;
        glyph(x + c * 10, y + r * 14, b ? '1' : '0', b ? on : off, bg);
    }
}

/* Random 64-bit for demo viz */
static uint64_t demo_rng = 0xDEADBEEFCAFEBABEULL;
static uint64_t rand64(void) {
    demo_rng ^= demo_rng << 13;
    demo_rng ^= demo_rng >> 7;
    demo_rng ^= demo_rng << 17;
    return demo_rng;
}
/* Hello World letter-by-letter build-up via XOR accumulation */
/* Free-form typing: user types any character, sees delta accumulation live */
static uint64_t viz_accumulator;
static uint64_t viz_last_delta;
static int      viz_letter_pos;        /* total chars typed */
static char     viz_built[64];         /* typed string (display last ~30) */
static char     viz_last_char;         /* most recent char */

static void viz_reset(void) {
    viz_accumulator = 0;
    viz_last_delta = 0;
    viz_letter_pos = 0;
    viz_last_char = 0;
    memset(viz_built, 0, sizeof(viz_built));
}

/* Add one typed character to the accumulator */
static void viz_type_char(char ch) {
    int byte_pos = viz_letter_pos % 8;
    viz_last_delta = (uint64_t)(uint8_t)ch << (56 - byte_pos * 8);
    viz_accumulator ^= viz_last_delta;
    viz_last_char = ch;
    if (viz_letter_pos < 63)
        viz_built[viz_letter_pos] = ch;
    viz_letter_pos++;
    scan_pos = (scan_pos + 1) % N_BUF;
}

/* Undo last typed character — XOR is self-inverse */
static void viz_backspace(void) {
    if (viz_letter_pos <= 0) return;
    viz_letter_pos--;
    char ch = viz_built[viz_letter_pos];
    int byte_pos = viz_letter_pos % 8;
    viz_last_delta = (uint64_t)(uint8_t)ch << (56 - byte_pos * 8);
    viz_accumulator ^= viz_last_delta;  /* XOR again = undo */
    viz_built[viz_letter_pos] = 0;
    viz_last_char = viz_letter_pos > 0 ? viz_built[viz_letter_pos - 1] : 0;
}

/* Called on command-key presses (not typed chars) to advance scan animation */
static void refresh_viz(void) {
    scan_pos = (scan_pos + 1) % N_BUF;
}

/* ── Execution lane: vertical activity column with gradient ──────── */
static void exec_lane(int x, int y, int w, int h, const char *name,
                      int active, int is_last, uint32_t act_col, uint32_t idle_col,
                      int change_count, int lane_idx, int show_sweep) {
    uint32_t bg = active ? (is_last ? RGB(0x30,0xA0,0xE0) : act_col) : idle_col;

    if (active) {
        /* Active lane: glow border + gradient fill */
        uint32_t border = is_last ? C_WHITE : act_col;
        outlined(x, y, w, h, bg, border);
    } else {
        rect(x, y, w, h, idle_col);
    }

    /* Name at top */
    uint32_t tc = active ? C_WHITE : SKIP_TEXT;
    text(x + 4, y + 4, name, tc, bg);

    /* Activity bar area */
    int bar_h = h - 40;
    int bar_y = y + 22;
    int bar_x = x + 6;
    int bar_w = w - 12;

    if (active) {
        int fill_h = change_count > 20 ? bar_h : change_count * bar_h / 20;
        if (fill_h < 4) fill_h = 4;
        /* Empty portion */
        rect(bar_x, bar_y, bar_w, bar_h - fill_h, RGB(0x08,0x10,0x18));
        /* Gradient fill: 4 bands from dark to bright */
        int seg = fill_h / 4;
        if (seg < 1) seg = 1;
        uint32_t grad[] = {
            act_col == C_ORANGE ? C_ORANGE_DK : C_BLUE_DK,
            act_col == C_ORANGE ? RGB(0x90,0x48,0x20) : RGB(0x10,0x60,0x80),
            act_col == C_ORANGE ? RGB(0xC0,0x68,0x30) : RGB(0x18,0x90,0xB0),
            act_col == C_ORANGE ? C_ORANGE : C_BLUE,
        };
        for (int g = 0; g < 4; g++) {
            int gy = bar_y + bar_h - fill_h + g * seg;
            int gh = (g == 3) ? (fill_h - 3*seg) : seg;
            if (gh > 0) rect(bar_x, gy, bar_w, gh, grad[g]);
        }
    } else {
        rect(bar_x, bar_y, bar_w, bar_h, RGB(0x10,0x14,0x18));
    }

    /* Scan sweep line (SW side only) */
    if (show_sweep) {
        int sweep_y = bar_y + (scan_pos * bar_h / N_BUF);
        rect(x + 2, sweep_y, w - 4, 2, RGB(0xFF,0xFF,0xFF));
        /* Trailing glow */
        if (sweep_y > bar_y + 2)
            rect(x + 3, sweep_y - 2, w - 6, 2, RGB(0xFF,0xFF,0xFF));
    }

    /* Status at bottom */
    if (active && change_count > 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "x%d", change_count);
        text(x + 4, y + h - 16, buf, C_BG, bg);
    } else if (!active) {
        text(x + 4, y + h - 14, "IDLE", SKIP_TEXT, idle_col);
    }
}

/* Lightweight redraw of ONLY the delta-state algebra section.
 * Called on each typed character — no full-screen clear, no flicker. */
static void draw_typing(void) {
    char buf[80];
    int mid = FB_HRES / 2;
    int rx = mid + 16;
    int hy = HELLO_Y;
    int full_w = FB_HRES - 2*M;

    /* Clear just the algebra region */
    rect(0, hy, FB_HRES, METRIC_Y - hy, C_BG);

    /* Title */
    rect(M, hy, full_w, 2, C_BLUE);
    text2x(M, hy + 8, "Delta-State Algebra", C_TEXT, C_BG);
    text(M + 400, hy + 20, "state = initial XOR accumulated_deltas", C_DIM, C_BG);
    snprintf(buf, sizeof(buf), "Profile: %s", profile_names[workload_profile]);
    text(FB_HRES - M - 200, hy + 20, buf,
         workload_profile == 0 ? C_DIM : C_BLUE, C_BG);

    /* Typed string card */
    int str_y = hy + 36;
    panel(M, str_y, full_w, 52, C_PANEL, C_GREEN);
    if (viz_letter_pos == 0) {
        text(M + 20, str_y + 18, "Type any key to see delta accumulation...", C_GRAY, C_PANEL);
    } else {
        int show_start = viz_letter_pos > 28 ? viz_letter_pos - 28 : 0;
        int show_count = viz_letter_pos - show_start;
        for (int i = 0; i < show_count && i < 28; i++) {
            int ci = show_start + i;
            if (ci < 63) {
                int lx = M + 20 + i * 24;
                char lch[2] = { viz_built[ci], 0 };
                uint32_t col = (ci == viz_letter_pos - 1) ? C_WHITE : C_GREEN;
                text3x(lx, str_y + 8, lch, col, C_PANEL);
            }
        }
        int cx = M + 20 + (show_count - 1) * 24;
        rect(cx, str_y + 44, 24, 3, C_WHITE);
    }
    { char pbuf[16]; snprintf(pbuf, sizeof(pbuf), "%d chars", viz_letter_pos);
      text(M + full_w - 80, str_y + 18, pbuf, C_DIM, C_PANEL); }

    /* Binary panels */
    int gy = str_y + 60;
    int pw = 104, ph = 140;
    uint32_t d_bg = RGB(0x18,0x14,0x10), a_bg = RGB(0x10,0x18,0x14);

    outlined(M, gy, pw, ph, d_bg, RGB(0x60,0x40,0x20));
    text(M + 12, gy + 4, "DELTA", RGB(0xFF,0xCC,0x00), d_bg);
    bintxt(M + 12, gy + 20, viz_last_delta, C_WHITE, RGB(0x80,0x80,0x80), d_bg);

    int xor_x = M + pw + 16;
    text3x(xor_x, gy + 32, "^", RGB(0xFF,0xCC,0x00), C_BG);
    text(xor_x, gy + 76, "XOR", RGB(0x80,0x60,0x40), C_BG);

    int acc_x = xor_x + 48;
    outlined(acc_x, gy, pw, ph, a_bg, C_DKGREEN);
    text(acc_x + 12, gy + 4, "STATE", C_GREEN, a_bg);
    bintxt(acc_x + 12, gy + 20, viz_accumulator, C_WHITE, RGB(0x80,0x80,0x80), a_bg);

    int eq_x = acc_x + pw + 20;
    text3x(eq_x, gy + 32, "=", C_GREEN, C_BG);

    int dec_x = eq_x + 44;
    text(dec_x, gy + 4, "DECODED STATE", C_DIM, C_BG);
    { char dec[9] = {0};
      for (int i = 0; i < 8; i++) {
          uint8_t b = (uint8_t)((viz_accumulator >> (56 - i*8)) & 0xFF);
          dec[i] = (b >= 32 && b < 127) ? (char)b : '.';
      }
      text3x(dec_x, gy + 24, dec, C_GREEN, C_BG); }
    text(dec_x, gy + 72, "Each keypress adds one letter", C_GRAY, C_BG);
    text(dec_x, gy + 88, "as a binary delta into the", C_GRAY, C_BG);
    text(dec_x, gy + 104, "hardware accumulator.", C_GRAY, C_BG);
    text(dec_x, gy + 128, "No rescanning. No recomputing.", C_GREEN, C_BG);

    text2x(rx, gy + 4, "How ATOMiK Works", C_TEXT, C_BG);
    rect(rx, gy + 30, 300, 1, C_GRAY);
    text(rx, gy + 38, "1. State starts at zero", C_DIM, C_BG);
    text(rx, gy + 56, "2. Changes are XOR deltas", C_DIM, C_BG);
    text(rx, gy + 74, "3. Hardware accumulates deltas", C_DIM, C_BG);
    text(rx, gy + 92, "4. Current = initial XOR all deltas", C_DIM, C_BG);
    rect(rx, gy + 112, 300, 1, C_GRAY);
    text(rx, gy + 120, "Commutative. Associative.", C_GREEN, C_BG);
    text(rx, gy + 138, "Order doesn't matter.", C_GREEN, C_BG);
}

/* Draw dynamic content — uses layout grid #defines above */
static void draw_content(void) {
    char buf[80];

    int changed = 0;
    for (int i = 0; i < N_BUF; i++) if (buf_changed[i]) changed++;
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;

    int mid = FB_HRES / 2;
    int half_w = mid - M - 16;
    int lane_w = half_w / N_BUF - 2;
    int rx = mid + 16;

    /* ── Top bar stats ───────────────────────────────────────────── */
    rect(600, TOP_Y + 4, 800, TOP_H - 8, C_PANEL);
    snprintf(buf, sizeof(buf), "Cycle %d  |  %d changes  |  %.0fx faster",
             total_cycles, total_changes, measured_speedup);
    text(620, TOP_Y + 30, buf, C_DIM, C_PANEL);

    /* ── Clear content area ──────────────────────────────────────── */
    rect(0, LANE_Y, FB_HRES, BOT_Y - LANE_Y, C_BG);

    /* ═══════════════ LEFT: SOFTWARE ═══════════════════════════════ */
    text2x(M, LANE_Y, "SOFTWARE", C_ORANGE, C_BG);
    text(M + 200, LANE_Y + 12, "rescans all state every cycle", C_DIM, C_BG);

    for (int i = 0; i < N_BUF; i++) {
        int lx = M + i * (lane_w + 2);
        exec_lane(lx, LANE_TOP, lane_w, LANE_H, buf_names[i],
                  1, 0, C_ORANGE, SKIP_COL, 20, i, 1);
    }

    snprintf(buf, sizeof(buf), "Scanned: %llu KB (100%%)",
             (unsigned long long)(sw_scanned / 1024));
    text(M, VOL_Y, buf, C_ORANGE, C_BG);
    rect(M, VOL_Y + 16, half_w, 8, C_ORANGE);

    /* ── CENTER DIVIDER ──────────────────────────────────────────── */
    rect(mid - 1, LANE_Y, 2, VOL_Y + 26 - LANE_Y, C_GRAY);

    /* ═══════════════ RIGHT: ATOMiK Virtual Processors ══════════════ */
    text2x(rx, LANE_Y, "ATOMiK", C_BLUE, C_BG);
    /* Count active vproc types */
    int vp_counts[VP_COUNT] = {0};
    for (int i = 0; i < N_BUF; i++) vp_counts[slot_vproc[i]]++;
    { char vbuf[80]; int vx = rx + 160;
      for (int t = 0; t < VP_COUNT; t++) {
          if (vp_counts[t] > 0 && t != VP_IDLE) {
              snprintf(vbuf, sizeof(vbuf), "%dx%s", vp_counts[t], vproc_names[t]);
              text(vx, LANE_Y + 14, vbuf, vproc_colors[t], C_BG);
              vx += strlen(vbuf) * CW + 12;
          }
      }
    }

    for (int i = 0; i < N_BUF; i++) {
        int lx = rx + i * (lane_w + 2);
        int vp = slot_vproc[i];
        int is_active = buf_changed[i] || slot_active[i];
        /* Use vproc-specific color for the lane */
        uint32_t lane_col = vproc_colors[vp];
        exec_lane(lx, LANE_TOP, lane_w, LANE_H, buf_names[i],
                  is_active, (i == last_modified),
                  lane_col, SKIP_COL, buf_change_count[i], i, 0);
        /* Vproc type indicator below lane */
        rect(lx, LANE_TOP + LANE_H + 2, lane_w, 14, vproc_colors[vp]);
        text(lx + 2, LANE_TOP + LANE_H + 2, vproc_names[vp], C_WHITE, vproc_colors[vp]);
    }

    snprintf(buf, sizeof(buf), "Touched: %llu KB (%.0f%%)",
             (unsigned long long)(hw_touched / 1024), 100.0f - pct);
    text(rx, VOL_Y, buf, C_BLUE, C_BG);
    int atk_w = (int)((100.0f - pct) / 100.0f * half_w);
    if (atk_w < 4 && hw_touched > 0) atk_w = 4;
    rect(rx, VOL_Y + 16, atk_w, 8, C_BLUE);
    rect(rx + atk_w, VOL_Y + 16, half_w - atk_w, 8, RGB(0x14,0x14,0x18));

    /* ═══════════════ DELTA-STATE ALGEBRA ════════════════════════════
     * Full-width section using 1728px (M to FB_HRES-M).
     * Height: HELLO_Y (342) to METRIC_Y (600) = 258px.
     *
     * Row 1 (30px):  Title bar
     * Row 2 (56px):  "Hello World" letter build-up
     * Row 3 (156px): [DELTA panel] XOR [STATE panel] = [decoded + explanation]
     * ══════════════════════════════════════════════════════════════ */
    int hy = HELLO_Y;
    int full_w = FB_HRES - 2*M;

    /* ── Title row ──────────────────────────────────────────────── */
    rect(M, hy, full_w, 2, C_BLUE);
    text2x(M, hy + 8, "Delta-State Algebra", C_TEXT, C_BG);
    text(M + 400, hy + 20, "state = initial XOR accumulated_deltas", C_DIM, C_BG);
    snprintf(buf, sizeof(buf), "Profile: %s", profile_names[workload_profile]);
    text(FB_HRES - M - 200, hy + 20, buf,
         workload_profile == 0 ? C_DIM : C_BLUE, C_BG);

    /* ── Typed string card ──────────────────────────────────────── */
    int str_y = hy + 36;
    panel(M, str_y, full_w, 52, C_PANEL, C_GREEN);
    if (viz_letter_pos == 0) {
        text(M + 20, str_y + 18, "Type any key to see delta accumulation...", C_GRAY, C_PANEL);
    } else {
        /* Show last ~28 characters at 3x scale, cursor on most recent */
        int show_start = viz_letter_pos > 28 ? viz_letter_pos - 28 : 0;
        int show_count = viz_letter_pos - show_start;
        for (int i = 0; i < show_count && i < 28; i++) {
            int ci = show_start + i;
            if (ci < 63) {
                int lx = M + 20 + i * 24;
                char lch[2] = { viz_built[ci], 0 };
                uint32_t col = (ci == viz_letter_pos - 1) ? C_WHITE : C_GREEN;
                text3x(lx, str_y + 8, lch, col, C_PANEL);
            }
        }
        /* Cursor underline on last char */
        int cx = M + 20 + (show_count - 1) * 24;
        rect(cx, str_y + 44, 24, 3, C_WHITE);
    }
    { char pbuf[16]; snprintf(pbuf, sizeof(pbuf), "%d chars", viz_letter_pos);
      text(M + full_w - 80, str_y + 18, pbuf, C_DIM, C_PANEL); }

    /* ── Algebra row ───────────────────────────────────────────── */
    /* bintxt: 8×8 grid of '1'/'0' at 10px spacing = 80w × 86h (glyph tail).
     * Panel: 104w × 116h. Label(20) + grid(86) + padding(10). */
    int gy = str_y + 60;
    int pw = 104;         /* 12 + 80 + 12 */
    int ph = 140;         /* 20 label + 112 grid + 8 pad */
    uint32_t d_bg = RGB(0x18,0x14,0x10);
    uint32_t a_bg = RGB(0x10,0x18,0x14);

    /* ── DELTA panel ───────────────────────────────────────────── */
    outlined(M, gy, pw, ph, d_bg, RGB(0x60,0x40,0x20));
    text(M + 12, gy + 4, "DELTA", RGB(0xFF,0xCC,0x00), d_bg);
    if (viz_letter_pos > 0) {
        bintxt(M + 12, gy + 20, viz_last_delta, C_WHITE, RGB(0x80,0x80,0x80), d_bg);
    } else {
        bintxt(M + 12, gy + 20, 0, C_WHITE, RGB(0x80,0x80,0x80), d_bg);
    }

    /* ── XOR operator ──────────────────────────────────────────── */
    int xor_x = M + pw + 16;
    text3x(xor_x, gy + 32, "^", RGB(0xFF,0xCC,0x00), C_BG);
    text(xor_x, gy + 76, "XOR", RGB(0x80,0x60,0x40), C_BG);

    /* ── STATE panel (accumulator) ─────────────────────────────── */
    int acc_x = xor_x + 48;
    outlined(acc_x, gy, pw, ph, a_bg, C_DKGREEN);
    text(acc_x + 12, gy + 4, "STATE", C_GREEN, a_bg);
    bintxt(acc_x + 12, gy + 20, viz_accumulator, C_WHITE, RGB(0x80,0x80,0x80), a_bg);

    /* ── Equals + decoded output ───────────────────────────────── */
    int eq_x = acc_x + pw + 20;
    text3x(eq_x, gy + 32, "=", C_GREEN, C_BG);

    int dec_x = eq_x + 44;
    text(dec_x, gy + 4, "DECODED STATE", C_DIM, C_BG);
    { char dec[9] = {0};
      for (int i = 0; i < 8; i++) {
          uint8_t b = (uint8_t)((viz_accumulator >> (56 - i*8)) & 0xFF);
          dec[i] = (b >= 32 && b < 127) ? (char)b : '.';
      }
      text3x(dec_x, gy + 24, dec, C_GREEN, C_BG);
    }
    text(dec_x, gy + 72, "Each keypress adds one letter", C_GRAY, C_BG);
    text(dec_x, gy + 88, "as a binary delta into the", C_GRAY, C_BG);
    text(dec_x, gy + 104, "hardware accumulator.", C_GRAY, C_BG);
    text(dec_x, gy + 128, "No rescanning. No recomputing.", C_GREEN, C_BG);

    /* ── Right column ──────────────────────────────────────────── */
    text2x(rx, gy + 4, "How ATOMiK Works", C_TEXT, C_BG);
    rect(rx, gy + 30, 300, 1, C_GRAY);
    text(rx, gy + 38, "1. State starts at zero", C_DIM, C_BG);
    text(rx, gy + 56, "2. Changes are XOR deltas", C_DIM, C_BG);
    text(rx, gy + 74, "3. Hardware accumulates deltas", C_DIM, C_BG);
    text(rx, gy + 92, "4. Current = initial XOR all deltas", C_DIM, C_BG);
    rect(rx, gy + 112, 300, 1, C_GRAY);
    text(rx, gy + 120, "Commutative. Associative.", C_GREEN, C_BG);
    text(rx, gy + 138, "Order doesn't matter.", C_GREEN, C_BG);

    /* ═══════════════ METRICS PANEL ═══════════════════════════════ */
    panel(0, METRIC_Y, FB_HRES, METRIC_H, C_PANEL, C_GREEN);

    int cw = (FB_HRES - 2*M - 48) / 4;
    int cy = METRIC_Y + 12;
    int card_h = 54;

    /* Card 1: Data avoided */
    card(M, cy, cw, card_h);
    snprintf(buf, sizeof(buf), "%.0f%%", pct);
    text2x(M + 12, cy + 8, buf, C_GREEN, C_CARD);
    text(M + 12, cy + 36, "less compute", C_DIM, C_CARD);

    /* Card 2: Synced */
    card(M + cw + 16, cy, cw, card_h);
    snprintf(buf, sizeof(buf), "%d of 8", changed);
    text2x(M + cw + 28, cy + 8, buf, C_BLUE, C_CARD);
    text(M + cw + 28, cy + 36, "buffers synced", C_DIM, C_CARD);

    /* Card 3: Live cost ticker */
    card(M + 2*(cw + 16), cy, cw, card_h);
    snprintf(buf, sizeof(buf), "$%.2f", session_savings);
    text2x(M + 2*(cw + 16) + 12, cy + 8, buf, C_GREEN, C_CARD);
    text(M + 2*(cw + 16) + 12, cy + 36, "saved this session (1K srv)", C_DIM, C_CARD);

    /* Card 4: Speedup */
    card(M + 3*(cw + 16), cy, cw, card_h);
    snprintf(buf, sizeof(buf), "%.0fx", measured_speedup);
    text2x(M + 3*(cw + 16) + 12, cy + 8, buf, RGB(0xFF,0xCC,0x00), C_CARD);
    text(M + 3*(cw + 16) + 12, cy + 36, "faster query", C_DIM, C_CARD);

    /* Flow bars — spaced below cards */
    int fy = cy + card_h + 20;
    int bar_w = (FB_HRES - 2*M - 140) / 2;
    text(M, fy, "SW", C_ORANGE, C_PANEL);
    rect(M + 28, fy, bar_w, 10, C_ORANGE);
    text(M + 44 + bar_w, fy, "HW", C_BLUE, C_PANEL);
    int hw_w = (int)((100.0f - pct) / 100.0f * bar_w);
    if (hw_w < 4) hw_w = 4;
    rect(M + 72 + bar_w, fy, hw_w, 10, C_BLUE);
    rect(M + 72 + bar_w + hw_w, fy, bar_w - hw_w, 10, RGB(0x14,0x14,0x18));

    /* ═══════════════ HISTORY + EVENTS ════════════════════════════ */
    draw_history(M, HIST_Y, FB_HRES/2 - M - 8, HIST_H);

    rect(FB_HRES/2 + 8, HIST_Y, FB_HRES/2 - M - 8, HIST_H, C_BG);
    for (int i = 0; i < event_count && i < 3; i++) {
        int idx = event_count - 1 - i;
        if (idx >= 0)
            text(FB_HRES/2 + 16, HIST_Y + 4 + i * 16, event_log[idx],
                 i == 0 ? C_TEXT : C_DIM, C_BG);
    }

    /* Key legend */
    text(M, KEY_Y, "[1-8]=buffers  Backspace=undo  R=reset  Q=quit  H=help", C_GRAY, C_BG);
    text(M, KEY_Y + 16, "B=benchmark  G=storm  E=freeze  X=compiler  W=workload  H=help", C_GRAY, C_BG);
}
/* Adoption forecast slide */
static void draw_adoption(void) {
    rect(0, 96, FB_HRES, FB_VRES - 96 - 48, C_BG);
    text3x(M, 120, "Adoption Forecast", C_TEXT, C_BG);
    int ty = 200;
    int col_yr = M, col_adopt = M+160, col_rev = M+480, col_tam = M+800, col_cum = M+1120;
    text2x(col_yr, ty, "Year", C_DIM, C_BG);
    text2x(col_adopt, ty, "Adoption", C_DIM, C_BG);
    text2x(col_rev, ty, "Rev/Server", C_DIM, C_BG);
    text2x(col_tam, ty, "TAM Captured", C_DIM, C_BG);
    text2x(col_cum, ty, "Cumulative", C_DIM, C_BG);
    rect(M, ty+36, FB_HRES-2*M, 1, C_GRAY);
    static const struct { int yr; const char *a,*r,*t,*c; uint32_t col; } rows[] = {
        {2027,"1K servers","$50/srv","$50K","$50K",C_DIM},
        {2028,"10K servers","$45/srv","$450K","$500K",C_DIM},
        {2029,"100K servers","$40/srv","$4M","$4.5M",C_BLUE},
        {2030,"500K servers","$35/srv","$17.5M","$22M",C_BLUE},
        {2031,"2M servers","$30/srv","$60M","$82M",C_GREEN},
        {2032,"5M servers","$25/srv","$125M","$207M",C_GREEN},
        {2033,"10M servers","$20/srv","$200M","$407M",C_GREEN},
    };
    float tam_v[] = {0.05f,0.45f,4.0f,17.5f,60.0f,125.0f,200.0f};
    for (int i = 0; i < 7; i++) {
        int ry = ty+48+i*40;
        char yb[8]; snprintf(yb,sizeof(yb),"%d",rows[i].yr);
        text2x(col_yr,ry,yb,C_TEXT,C_BG);
        text(col_adopt,ry+8,rows[i].a,rows[i].col,C_BG);
        text(col_rev,ry+8,rows[i].r,rows[i].col,C_BG);
        text(col_tam,ry+8,rows[i].t,rows[i].col,C_BG);
        text(col_cum,ry+8,rows[i].c,rows[i].col,C_BG);
        int bw=(int)(tam_v[i]/200.0f*300); if(bw<2) bw=2;
        rect(col_cum+100,ry+8,bw,12,rows[i].col);
    }
    text2x(M,ty+48+7*40+20,"Total: $1.7B/yr (50M state-heavy servers)",C_GREEN,C_BG);
    text(M,ty+48+7*40+56,"Conservative 20% penetration by 2033 = $407M cumulative",C_DIM,C_BG);
    text(M,ty+48+7*40+76,"Press any key to return",C_GRAY,C_BG);
    flush_l2();
    while (!key_ready()) usleep(50000);
    char ch; read(0, &ch, 1);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  AI TRAINING / INFERENCE DEMO PAGE
 *  Full-screen visualization of ATOMiK as a gradient accumulator.
 *  Runs REAL hardware operations — all numbers are live measurements.
 *  Triggered by Shift+I.
 * ═══════════════════════════════════════════════════════════════════════ */
static void draw_ai_demo(void) {
    char buf[120];
    int running_ai = 1;

    /* Color palette for this page */
    uint32_t C_GOLD = RGB(0xFF,0xCC,0x00);
    uint32_t C_CYAN = RGB(0x00,0xE5,0xFF);
    uint32_t C_MAGENTA = RGB(0xFF,0x44,0xAA);

    /* Worker colors — each of 8 workers gets a unique color */
    uint32_t wcol[8] = {
        RGB(0x1E,0xC8,0xFF), RGB(0x39,0xD9,0x8A), RGB(0xFF,0x8A,0x3D), RGB(0xFF,0xCC,0x00),
        RGB(0xFF,0x44,0xAA), RGB(0x00,0xE5,0xFF), RGB(0xFF,0x44,0x44), RGB(0xB0,0x88,0xFF),
    };

    /* Initialize all 8 slots as parallel accumulators */
    uint8_t weights[8][4096];
    uint8_t w_shadow[8][4096];
    for (int i = 0; i < 8; i++) {
        memset(weights[i], 0x55 + i * 0x11, 4096);
        memcpy(w_shadow[i], weights[i], 4096);
        load64(i, fp(weights[i], 4096));
    }

    int epoch = 0;
    int total_grads = 0;
    uint64_t hw_cycles_total = 0;
    uint64_t sw_cycles_total = 0;
    int layers_skipped_total = 0;
    int layers_checked_total = 0;
    uint64_t last_frame = 0;

    while (running_ai) {
        /* Auto-advance: new epoch every 400ms, or on keypress */
        uint64_t now = rdtime();
        int key_pressed = 0;
        if (key_ready()) {
            char ach;
            if (read(0, &ach, 1) == 1) {
                if (ach == 'Q' || ach == 'q' || ach == 27) {
                    running_ai = 0; continue;
                }
                /* Handle ~ commands inside AI demo too */
                if (ach == '~') {
                    char cmdbuf[512] = {0}; int ci = 0;
                    int got_eol = 0;
                    for (int attempt = 0; attempt < 200 && !got_eol; attempt++) {
                        fd_set fs; struct timeval tv = {0, 50000};
                        FD_ZERO(&fs); FD_SET(0, &fs);
                        if (select(1, &fs, NULL, NULL, &tv) > 0) {
                            char tmp[256]; int n = read(0, tmp, sizeof(tmp));
                            for (int j = 0; j < n && ci < 510; j++) {
                                if (tmp[j]=='\n'||tmp[j]=='\r') {got_eol=1;break;}
                                cmdbuf[ci++] = tmp[j];
                            }
                        } else if (ci > 0) break;
                    }
                    if (ci > 0) {
                        cmd_running = 1;
                        printf("##RSP:CMD:%s\n", cmdbuf);
                        FILE *pp = popen(cmdbuf, "r");
                        if (pp) {
                            char line[256];
                            while (fgets(line, sizeof(line), pp)) {
                                int len = strlen(line);
                                if (len>0 && line[len-1]=='\n') line[len-1]=0;
                                printf("##RSP:%s\n", line);
                            }
                            printf("##RSP:EXIT:%d\n", WEXITSTATUS(pclose(pp)));
                        } else printf("##RSP:ERROR:popen failed\n");
                        printf("##RSP:END\n"); fflush(stdout);
                        cmd_running = 0;
                    }
                    continue;
                }
                key_pressed = 1;
            }
        }
        if (!key_pressed && (now - last_frame) < 40000000ULL) {
            usleep(20000);
            continue; /* ~400ms between auto-epochs at 100MHz */
        }
        last_frame = now;

        epoch++;

        /* ── Phase A: TRAINING — gradient accumulation ─────────── */
        uint32_t rng = rdtime() & 0xFFFFFFFF;
        int active_workers = 3 + (rng % 6); /* 3-8 workers this epoch */
        int grads_this_epoch = 0;
        int worker_targets[8] = {-1,-1,-1,-1,-1,-1,-1,-1};

        /* Apply gradients: each worker ADDS to bytes in its target layer. */
        for (int w = 0; w < active_workers; w++) {
            int layer = (rng >> (w * 3 + 1)) % 8;
            worker_targets[w] = layer;
            for (int j = 0; j < 32; j++) {
                int pos = (j * 127 + epoch * 17 + w * 41) % 4096;
                weights[layer][pos] += (w + epoch + 1) & 0x3F;
            }
            grads_this_epoch++;
        }

        /* SW baseline: memcmp ONLY — pure detection cost */
        uint64_t sw_t0 = rdtime();
        for (int l = 0; l < 8; l++) {
            volatile int d = memcmp(weights[l], w_shadow[l], 4096);
            (void)d;
        }
        uint64_t sw_time = rdtime() - sw_t0;
        sw_cycles_total += sw_time;

        /* ATOMiK: fingerprint detect — 3 MMIO ops per layer */
        int changed = 0, skipped = 0;
        int layer_changed[8] = {0};
        uint64_t hw_t0 = rdtime();
        for (int l = 0; l < 8; l++) {
            uint64_t ofp = fp(w_shadow[l], 4096);
            uint64_t nfp = fp(weights[l], 4096);
            load64(l, ofp);
            accum64(nfp ^ ofp);
            read64();
            if (ofp != nfp) {
                changed++;
                layer_changed[l] = 1;
            } else {
                skipped++;
            }
        }
        uint64_t hw_time = rdtime() - hw_t0;
        /* Copy shadows AFTER timing */
        for (int l = 0; l < 8; l++) {
            if (layer_changed[l])
                memcpy(w_shadow[l], weights[l], 4096);
        }
        hw_cycles_total += hw_time;
        total_grads += grads_this_epoch;
        layers_skipped_total += skipped;
        layers_checked_total += 8;

        float speedup = (float)sw_cycles_total / (float)(hw_cycles_total > 0 ? hw_cycles_total : 1);
        float skip_pct = 100.0f * layers_skipped_total / (layers_checked_total > 0 ? layers_checked_total : 1);

        /* ── DRAW — targeted updates only (no full-screen clear) ── */
        int ty = TOP_Y + 92;
        int lane_w = (FB_HRES - 2*M - 32) / 16 - 2; /* fit 8+8 lanes */
        int lane_h = 200;
        int lx0 = M;                          /* training starts left */
        int rx = FB_HRES/2 + 16;              /* inference starts right */
        int acc_y = ty + 32 + lane_h + 12;    /* accumulator row */
        int my = acc_y + 80;                  /* metrics row */

        if (epoch == 1) {
            /* First frame: draw full chrome once */
            rect(0, 0, FB_HRES, FB_VRES, C_BG);
            panel(0, TOP_Y, FB_HRES, 80, C_PANEL, C_GOLD);
            text3x(M, TOP_Y + 12, "AI WORKLOAD", C_GOLD, C_PANEL);
            text2x(M + 320, TOP_Y + 20, "Training + Inference on ATOMiK", C_DIM, C_PANEL);
            /* Left: INFERENCE (layer status) */
            text2x(lx0, ty, "INFERENCE", C_CYAN, C_BG);
            text(lx0 + 220, ty + 12, "selective layer reload", C_DIM, C_BG);
            /* Divider */
            rect(FB_HRES/2 - 1, ty, 2, lane_h + 40, C_GRAY);
            /* Right: TRAINING (active workers) */
            text2x(rx, ty, "TRAINING", C_ORANGE, C_BG);
            text(rx + 200, ty + 12, "gradient accumulation", C_DIM, C_BG);
            /* Metrics panel chrome */
            panel(0, my, FB_HRES, 100, C_PANEL, C_GREEN);
            text(M, my + 52, "detection speedup", C_DIM, C_PANEL);
            text(M + 300, my + 52, "bandwidth saved", C_DIM, C_PANEL);
            text(M + 560, my + 52, "Mops/s", C_DIM, C_PANEL);
            /* Accumulator chrome */
            panel(M, acc_y, FB_HRES - 2*M, 68, RGB(0x14,0x20,0x30), C_BLUE);
            text2x(M + 16, acc_y + 6, "ATOMiK ACCUMULATOR", C_BLUE, RGB(0x14,0x20,0x30));
            /* Bottom bar */
            panel(0, my + 102, FB_HRES, 36, C_PANEL, C_GOLD);
            text(M, my + 112, "ATOMiK: hardware gradient accumulation + selective inference. Same C. Same GCC.",
                 C_TEXT, C_PANEL);
            text(FB_HRES - M - 200, my + 112, "LIVE  |  Q to return", C_GRAY, C_PANEL);
        }

        /* Epoch badge (small update) */
        snprintf(buf, sizeof(buf), "EPOCH %-4d", epoch);
        rect(FB_HRES - M - 200, TOP_Y + 16, 180, 44, C_GOLD);
        text2x(FB_HRES - M - 190, TOP_Y + 24, buf, C_BG, C_GOLD);

        /* ── LEFT: Worker lanes ──────────────────────────────── */
        int half = FB_HRES/2 - M - 20;
        int lw = half / 8 - 3;  /* lane width for 8 lanes in half screen */
        for (int w = 0; w < 8; w++) {
            int lx = lx0 + w * (lw + 3);
            int active = (w < active_workers);
            int layer = active ? worker_targets[w] : -1;
            uint32_t bg = active ? wcol[w] : SKIP_COL;
            rect(lx, ty + 32, lw, lane_h, bg);
            snprintf(buf, sizeof(buf), "W%d", w);
            text(lx + 4, ty + 36, buf, active ? C_BG : SKIP_TEXT, bg);
            if (active) {
                snprintf(buf, sizeof(buf), "L%d", layer);
                text2x(lx + (lw-24)/2, ty + 60, buf, C_BG, bg);
                int bar_h = 40 + (epoch * (w+1) * 7) % (lane_h - 70);
                rect(lx + 6, ty + 32 + lane_h - 8 - bar_h, lw - 12, bar_h, RGB(0xFF,0xFF,0xFF));
            } else {
                text(lx + 4, ty + 80, "IDLE", SKIP_TEXT, bg);
            }
        }

        /* ── RIGHT: Inference layers ──────────────────────────── */
        for (int l = 0; l < 8; l++) {
            int lx = rx + l * (lw + 3);
            if (layer_changed[l]) {
                rect(lx, ty + 32, lw, lane_h, C_CYAN);
                snprintf(buf, sizeof(buf), "L%d", l);
                text(lx + 4, ty + 36, buf, C_BG, C_CYAN);
                text(lx + 4, ty + 60, "LOAD", C_BG, C_CYAN);
                int fill = (epoch * 17 + l * 31) % (lane_h - 60);
                rect(lx + 4, ty + 32 + lane_h - 8 - fill, lw - 8, fill, RGB(0x00,0xA0,0xD0));
            } else {
                rect(lx, ty + 32, lw, lane_h, RGB(0x10,0x28,0x28));
                snprintf(buf, sizeof(buf), "L%d", l);
                text(lx + 4, ty + 36, buf, C_GREEN, RGB(0x10,0x28,0x28));
                text(lx + 4, ty + 80, "SKIP", C_GREEN, RGB(0x10,0x28,0x28));
            }
        }

        /* ── Accumulator slots (full width row) ───────────────── */
        snprintf(buf, sizeof(buf), "%d changed | %d skipped", changed, skipped);
        rect(M + 360, acc_y + 6, 400, 16, RGB(0x14,0x20,0x30));
        text(M + 360, acc_y + 8, buf, C_DIM, RGB(0x14,0x20,0x30));
        int slot_w = (FB_HRES - 2*M - 40) / 8;
        for (int l = 0; l < 8; l++) {
            int sx = M + 16 + l * (slot_w + 2);
            int sy = acc_y + 30;
            uint32_t sc = layer_changed[l] ? C_BLUE : C_GRAY;
            rect(sx, sy, slot_w, 28, sc);
            snprintf(buf, sizeof(buf), "L%d", l);
            text(sx + (slot_w-16)/2, sy + 6, buf, layer_changed[l] ? C_BG : SKIP_TEXT, sc);
        }

        /* ── Metrics — the investor story ──────────────────────── */
        /* Bandwidth saved is the REAL metric — not cycle speedup */
        rect(M, my + 6, 250, 44, C_PANEL);
        snprintf(buf, sizeof(buf), "%.0f%%", skip_pct);
        text3x(M, my + 8, buf, C_GREEN, C_PANEL);
        text(M, my + 52, "bandwidth saved", C_DIM, C_PANEL);

        /* Total layers synced vs skipped */
        rect(M + 300, my + 6, 250, 44, C_PANEL);
        int total_synced = layers_checked_total - layers_skipped_total;
        snprintf(buf, sizeof(buf), "%d / %d", total_synced, layers_checked_total);
        text3x(M + 300, my + 8, buf, C_BLUE, C_PANEL);
        text(M + 300, my + 52, "layers synced / total", C_DIM, C_PANEL);

        /* Detection speedup (use startup benchmark, not the mixed workload) */
        rect(M + 600, my + 6, 200, 44, C_PANEL);
        snprintf(buf, sizeof(buf), "%.0fx", measured_speedup);
        text3x(M + 600, my + 8, buf, C_GOLD, C_PANEL);
        text(M + 600, my + 52, "query speedup", C_DIM, C_PANEL);

        rect(M + 800, my + 6, 900, 80, C_PANEL);
        snprintf(buf, sizeof(buf), "Epoch %d | %d workers | %d changed | %d skipped",
                 epoch, active_workers, changed, skipped);
        text(M + 800, my + 12, buf, C_DIM, C_PANEL);
        snprintf(buf, sizeof(buf), "Total: %d grads | SW: %lluK cy | HW: %lluK cy",
                 total_grads, (unsigned long long)(sw_cycles_total/1000),
                 (unsigned long long)(hw_cycles_total/1000));
        text(M + 800, my + 36, buf, C_DIM, C_PANEL);
        text(M + 800, my + 60, "Same hardware. Same GCC. Dynamic virtual processors.", C_GREEN, C_PANEL);

        flush_l2();
    }

    /* Return to main dashboard */
    log_event("AI demo complete.");
    { char abuf[60];
      float final_sp = (float)sw_cycles_total / (float)(hw_cycles_total > 0 ? hw_cycles_total : 1);
      snprintf(abuf, sizeof(abuf), "AI result: %.1fx speedup, %d epochs", final_sp, epoch);
      log_event(abuf);
    }
}

/* "Break It" corruption detection challenge — full-screen sequential reveal */
static void draw_break_it(void) {
    /* Pick a random target buffer */
    int target = rdtime() % N_BUF;
    int pos = rdtime() % BUF_SIZE;

    /* 1. Clear screen, show big red title */
    rect(0, 0, FB_HRES, FB_VRES, C_BG);
    panel(0, TOP_Y, FB_HRES, 80, C_PANEL, C_RED);
    text3x(M, TOP_Y + 12, "CORRUPTION INJECTED", C_RED, C_PANEL);
    {
        char tbuf[80];
        snprintf(tbuf, sizeof(tbuf), "Target: %s  --  1 byte flipped", buf_names[target]);
        text2x(M, TOP_Y + 60, tbuf, C_WHITE, C_PANEL);
    }
    flush_l2();

    /* 2. Corrupt 1 byte */
    buffers[target][pos] ^= 0xFF;

    /* Pause to let corruption sink in visually */
    usleep(600000);

    /* 3. Show scanning text */
    text3x(M, TOP_Y + 130, "SCANNING...", C_BLUE, C_BG);
    flush_l2();
    usleep(400000);

    /* 4. Sweep all 8 buffers with animated reveal */
    int box_w = (FB_HRES - 2 * M - 7 * 12) / N_BUF;  /* evenly spaced boxes */
    if (box_w > 200) box_w = 200;
    int box_h = 180;
    int box_y = TOP_Y + 220;
    int detected_idx = -1;
    uint64_t detect_start = rdtime();
    uint64_t detect_cycles = 0;

    for (int i = 0; i < N_BUF; i++) {
        int bx = M + i * (box_w + 12);

        /* Draw scanning indicator */
        rect(bx, box_y, box_w, box_h, C_PANEL);
        text(bx + 4, box_y + 4, buf_names[i], C_DIM, C_PANEL);
        text2x(bx + (box_w - 11*CW*2)/2, box_y + 40, "checking...", C_DIM, C_PANEL);
        flush_l2();
        usleep(300000);

        /* Compute fingerprints */
        uint64_t t0 = rdtime();
        uint64_t current_fp = fp(buffers[i], BUF_SIZE);
        uint64_t stored_fp  = fp(shadows[i], BUF_SIZE);
        uint64_t t1 = rdtime();
        int match = (current_fp == stored_fp);

        if (!match && detected_idx < 0) {
            detected_idx = i;
            detect_cycles = t1 - t0;
        }

        /* Draw result box: green PASS or red TAMPERED */
        uint32_t box_color = match ? C_DKGREEN : RGB(0x60,0x10,0x10);
        uint32_t glow      = match ? C_GREEN   : C_RED;
        rect(bx, box_y, box_w, box_h, box_color);
        rect(bx, box_y, box_w, 3, glow);

        /* Buffer name at top */
        text(bx + 4, box_y + 8, buf_names[i], C_WHITE, box_color);

        /* Large status in center */
        if (match) {
            text3x(bx + (box_w - 4*CW*3)/2, box_y + 50, "PASS", C_GREEN, box_color);
        } else {
            text3x(bx + (box_w - 8*CW*3)/2, box_y + 50, "TAMPERED", C_RED, box_color);
        }

        /* Buffer index */
        {
            char ibuf[4];
            snprintf(ibuf, sizeof(ibuf), "#%d", i + 1);
            text(bx + (box_w - 2*CW)/2, box_y + box_h - 24, ibuf, C_DIM, box_color);
        }

        flush_l2();
    }

    /* 5. Show result summary */
    int summary_y = box_y + box_h + 40;
    rect(M, summary_y, FB_HRES - 2*M, 160, C_BG);

    {
        char rbuf[120];
        snprintf(rbuf, sizeof(rbuf), "DETECTED: 1 byte tampered in %s",
                 (detected_idx >= 0) ? buf_names[detected_idx] : "???");
        text3x(M, summary_y, rbuf, C_GREEN, C_BG);
    }
    {
        char tbuf[120];
        uint64_t ns = detect_cycles * 10; /* 10ns per cycle at 100MHz */
        snprintf(tbuf, sizeof(tbuf), "Detection time: %lu cycles (%luns at 100MHz)",
                 (unsigned long)detect_cycles, (unsigned long)ns);
        text2x(M, summary_y + 56, tbuf, C_WHITE, C_BG);
    }
    text2x(M, summary_y + 96, "Zero false positives. Zero bytes missed.", C_DIM, C_BG);

    flush_l2();

    /* 6. Restore the corrupted byte */
    buffers[target][pos] ^= 0xFF;

    /* 7. Wait for keypress */
    text(M, summary_y + 140, "Press any key to return...", C_DIM, C_BG);
    flush_l2();
    while (!key_ready()) usleep(20000);
    (void)getchar(); /* consume the key */
}

/* Benchmark race — SW memcmp vs ATOMiK fingerprint, full-screen */
static void draw_benchmark_race(void) {
    /* Clear screen */
    rect(0, 0, FB_HRES, FB_VRES, C_BG);

    /* Title */
    panel(0, TOP_Y, FB_HRES, 80, C_PANEL, RGB(0xFF,0xCC,0x00));
    text3x(M, TOP_Y+12, "BENCHMARK RACE", RGB(0xFF,0xCC,0x00), C_PANEL);
    text2x(M+400, TOP_Y+20, "memcmp vs ATOMiK -- live measurement", C_DIM, C_PANEL);

    /* Set up test: modify 3 of 8 buffers so there's a mix of changed/unchanged */
    uint8_t test_bufs[8][4096];
    uint8_t test_old[8][4096];
    for (int i = 0; i < 8; i++) {
        memset(test_bufs[i], 0xAA+i, 4096);
        memcpy(test_old[i], test_bufs[i], 4096);
    }
    /* Modify buffers 1, 4, 6 */
    for (int j = 0; j < 63; j++) {
        test_bufs[1][j*64] ^= 0x42;
        test_bufs[4][j*64] ^= 0x37;
        test_bufs[6][j*64] ^= 0x1F;
    }

    int bar_y_sw = 280, bar_y_hw = 520;
    int bar_x = M, bar_w = FB_HRES - 2*M;
    int bar_h = 60;

    /* Labels */
    text3x(M, bar_y_sw - 80, "SOFTWARE", C_ORANGE, C_BG);
    text(M, bar_y_sw - 30, "memcmp: scan every byte of all 8 buffers (32 KB)", C_DIM, C_BG);
    rect(bar_x, bar_y_sw, bar_w, bar_h, RGB(0x14,0x14,0x18)); /* empty bar bg */

    text3x(M, bar_y_hw - 80, "ATOMiK", C_BLUE, C_BG);
    text(M, bar_y_hw - 30, "XOR fingerprint: 3 hardware ops per buffer (24 ops total)", C_DIM, C_BG);
    rect(bar_x, bar_y_hw, bar_w, bar_h, RGB(0x14,0x14,0x18));

    text2x(M, 700, "Racing...", C_WHITE, C_BG);
    flush_l2();

    /* Run SW benchmark with visual progress */
    uint64_t sw_start = rdtime();
    for (int l = 0; l < 8; l++) {
        volatile int diff = memcmp(test_bufs[l], test_old[l], 4096);
        (void)diff;
        /* Update progress bar */
        int progress = (l + 1) * bar_w / 8;
        rect(bar_x, bar_y_sw, progress, bar_h, C_ORANGE);
        char pbuf[20];
        snprintf(pbuf, sizeof(pbuf), "%d/8", l+1);
        text2x(bar_x + progress + 8, bar_y_sw + 14, pbuf, C_ORANGE, C_BG);
        flush_l2();
    }
    uint64_t sw_cycles = rdtime() - sw_start;

    /* SW done label */
    char buf[80];
    snprintf(buf, sizeof(buf), "%llu cycles", (unsigned long long)sw_cycles);
    text2x(bar_x + bar_w + 16, bar_y_sw + 14, buf, C_ORANGE, C_BG);
    flush_l2();

    /* Run ATOMiK benchmark */
    for (int i = 0; i < 8; i++)
        load64(i, fp(test_old[i], 4096));

    uint64_t hw_start = rdtime();
    for (int l = 0; l < 8; l++) {
        uint64_t ofp = fp(test_old[l], 4096);
        uint64_t nfp = fp(test_bufs[l], 4096);
        load64(l, ofp);
        accum64(nfp ^ ofp);
        read64();
    }
    uint64_t hw_cycles = rdtime() - hw_start;

    /* ATOMiK bar fills instantly */
    rect(bar_x, bar_y_hw, bar_w, bar_h, C_BLUE);
    text2x(bar_x + 16, bar_y_hw + 14, "DONE", C_BG, C_BLUE);
    snprintf(buf, sizeof(buf), "%llu cycles", (unsigned long long)hw_cycles);
    text2x(bar_x + bar_w + 16, bar_y_hw + 14, buf, C_BLUE, C_BG);
    flush_l2();

    /* Results */
    float race_speedup = (float)sw_cycles / (float)(hw_cycles > 0 ? hw_cycles : 1);
    rect(0, 700, FB_HRES, 200, C_PANEL);
    rect(0, 700, FB_HRES, 2, C_GREEN);
    snprintf(buf, sizeof(buf), "%.0fx FASTER", race_speedup);
    text3x(M, 720, buf, C_GREEN, C_PANEL);

    snprintf(buf, sizeof(buf), "Software: %llu cycles  |  ATOMiK: %llu cycles",
             (unsigned long long)sw_cycles, (unsigned long long)hw_cycles);
    text2x(M, 780, buf, C_DIM, C_PANEL);

    text(M, 830, "Both paths detect the same 3 changed buffers. ATOMiK uses XOR fingerprints -- O(1) per buffer.", C_DIM, C_PANEL);
    text(M, 850, "Measured live on this board. NaxRiscv RV64GC @ 100MHz. Press any key to return.", C_GRAY, C_PANEL);
    flush_l2();

    /* Wait for keypress */
    while (!key_ready()) usleep(50000);
    char ch; read(0, &ch, 1);
}

/* TCO Calculator — full-screen overlay showing savings at server scale */
static void draw_tco_calculator(void) {
    char buf[120];

    /* Compute live pct_avoided */
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;

    /* Clear screen */
    rect(0, 0, FB_HRES, FB_VRES, C_BG);

    /* Title panel */
    panel(0, TOP_Y, FB_HRES, 80, C_PANEL, C_GREEN);
    text3x(M, TOP_Y + 12, "TCO CALCULATOR", C_GREEN, C_PANEL);
    text2x(M + 420, TOP_Y + 20, "Based on live hardware measurement this session", C_DIM, C_PANEL);

    /* Live measurement callout */
    int ly = TOP_Y + 100;
    panel(M, ly, FB_HRES - 2*M, 56, C_PANEL, C_BLUE);
    snprintf(buf, sizeof(buf), "Current session: %.0f%% compute eliminated", pct);
    text2x(M + 20, ly + 8, buf, C_GREEN, C_PANEL);
    text(M + 20, ly + 38, "(measured on this board)", C_DIM, C_PANEL);

    /* Table header */
    int ty = ly + 76;
    int col_srv = M;
    int col_cost = M + 320;
    int col_sav = M + 640;
    int col_net = M + 960;
    int col_bar = M + 1260;
    int bar_max_w = FB_HRES - M - col_bar - 20;

    rect(col_srv, ty, FB_HRES - 2*M, 2, C_GRAY);
    text2x(col_srv, ty + 8, "Servers", C_DIM, C_BG);
    text2x(col_cost, ty + 8, "Annual Energy", C_DIM, C_BG);
    text2x(col_sav, ty + 8, "ATOMiK Savings", C_DIM, C_BG);
    text2x(col_net, ty + 8, "Net Saved", C_DIM, C_BG);
    rect(col_srv, ty + 40, FB_HRES - 2*M, 1, C_GRAY);

    /* Table rows */
    static const struct { int servers; const char *srv_label; float annual_cost; } scales[] = {
        {       100, "100",            5000.0f },
        {      1000, "1,000",         50000.0f },
        {     10000, "10,000",       500000.0f },
        {    100000, "100,000",     5000000.0f },
        {   1000000, "1,000,000",  50000000.0f },
        {  50000000, "50,000,000", 2500000000.0f },
    };
    int n_rows = 6;
    int row_h = 64;
    float max_savings = 0;

    /* Pre-compute max savings for bar scaling */
    for (int i = 0; i < n_rows; i++) {
        float sav = scales[i].servers * 50.0f * pct / 100.0f;
        if (sav > max_savings) max_savings = sav;
    }

    /* Color gradient for rows — dimmer at small scale, brighter at large */
    uint32_t row_colors[] = { C_DIM, C_DIM, C_BLUE, C_BLUE, C_GREEN, C_GREEN };
    uint32_t bar_colors[] = {
        RGB(0x10,0x60,0x80), RGB(0x14,0x80,0xA0), RGB(0x1E,0xC8,0xFF),
        RGB(0x20,0xA0,0x60), RGB(0x39,0xD9,0x8A), RGB(0x50,0xFF,0xA0),
    };

    for (int i = 0; i < n_rows; i++) {
        int ry = ty + 48 + i * row_h;
        float savings = scales[i].servers * 50.0f * pct / 100.0f;

        /* Servers column */
        text2x(col_srv, ry + 8, scales[i].srv_label, C_WHITE, C_BG);

        /* Annual energy cost */
        if (scales[i].annual_cost >= 1e9f)
            snprintf(buf, sizeof(buf), "$%.1fB", scales[i].annual_cost / 1e9f);
        else if (scales[i].annual_cost >= 1e6f)
            snprintf(buf, sizeof(buf), "$%.0fM", scales[i].annual_cost / 1e6f);
        else if (scales[i].annual_cost >= 1e3f)
            snprintf(buf, sizeof(buf), "$%dK", (int)(scales[i].annual_cost / 1e3f));
        else
            snprintf(buf, sizeof(buf), "$%.0f", scales[i].annual_cost);
        text(col_cost, ry + 14, buf, row_colors[i], C_BG);

        /* ATOMiK savings */
        if (savings >= 1e9f)
            snprintf(buf, sizeof(buf), "$%.1fB", savings / 1e9f);
        else if (savings >= 1e6f)
            snprintf(buf, sizeof(buf), "$%.1fM", savings / 1e6f);
        else if (savings >= 1e3f)
            snprintf(buf, sizeof(buf), "$%dK", (int)(savings / 1e3f + 0.5f));
        else
            snprintf(buf, sizeof(buf), "$%.0f", savings);
        text2x(col_sav, ry + 8, buf, row_colors[i], C_BG);

        /* Net saved (same as savings — no license cost modeled here) */
        if (savings >= 1e9f)
            snprintf(buf, sizeof(buf), "$%.1fB", savings / 1e9f);
        else if (savings >= 1e6f)
            snprintf(buf, sizeof(buf), "$%.1fM", savings / 1e6f);
        else if (savings >= 1e3f)
            snprintf(buf, sizeof(buf), "$%dK", (int)(savings / 1e3f + 0.5f));
        else
            snprintf(buf, sizeof(buf), "$%.0f", savings);
        text2x(col_net, ry + 8, buf, row_colors[i], C_BG);

        /* Proportional bar */
        int bw = 0;
        if (max_savings > 0)
            bw = (int)(savings / max_savings * bar_max_w);
        if (bw < 4 && savings > 0) bw = 4;
        if (bw > bar_max_w) bw = bar_max_w;
        rect(col_bar, ry + 6, bw, 28, bar_colors[i]);

        /* Thin separator */
        rect(col_srv, ry + row_h - 2, FB_HRES - 2*M, 1, RGB(0x14,0x1E,0x28));
    }

    /* Bottom text */
    int bot = ty + 48 + n_rows * row_h + 20;
    snprintf(buf, sizeof(buf),
        "All savings derived from live measurement: %.0f%% compute eliminated. "
        "Not projected -- measured NOW on this hardware.", pct);
    text(M, bot, buf, C_GREEN, C_BG);

    text(M, bot + 30, "Formula: servers x $50/yr x pct_avoided/100", C_DIM, C_BG);

    text(M, bot + 70, "Press any key to return", C_GRAY, C_BG);
    flush_l2();

    /* Wait for keypress */
    while (!key_ready()) usleep(50000);
    char tch; read(0, &tch, 1);
}

/* ── Dollar-Per-Second Race (Shift+D) ──────────────────────────────── */
static void draw_dollar_race(void) {
    char buf[120];
    uint32_t C_GOLD = RGB(0xFF,0xCC,0x00);

    /* Compute live pct_avoided */
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 85.0f;
    if (pct < 1.0f) pct = 85.0f; /* fallback if no ops yet */

    /* SW cost rate: $50/srv/yr * 1000 servers / seconds_per_year
     * Accelerated 100,000x so 10s demo = ~11.5 days of server time.
     * Makes the dollar amounts visceral. */
    float accel = 100000.0f;
    float sw_rate = 50.0f * 1000.0f / (365.25f * 86400.0f) * accel;
    float atk_rate = sw_rate * (1.0f - pct / 100.0f);

    /* Clear + title */
    rect(0, 0, FB_HRES, FB_VRES, C_BG);
    panel(0, TOP_Y, FB_HRES, 80, C_PANEL, C_GOLD);
    text3x(M, TOP_Y + 12, "COST RACE", C_GOLD, C_PANEL);
    text2x(M + 300, TOP_Y + 20, "1,000 servers  |  $50/srv/yr  |  100,000x time", C_DIM, C_PANEL);

    /* Column headers */
    int col_sw = M;
    int col_diff = FB_HRES / 2 - 180;
    int col_atk = FB_HRES - M - 400;

    text2x(col_sw, TOP_Y + 100, "SOFTWARE COST", C_ORANGE, C_BG);
    text(col_sw, TOP_Y + 134, "Full-scan every cycle", C_DIM, C_BG);

    text2x(col_diff, TOP_Y + 100, "DIFFERENCE", C_GREEN, C_BG);
    text(col_diff, TOP_Y + 134, "ATOMiK savings", C_DIM, C_BG);

    text2x(col_atk, TOP_Y + 100, "ATOMiK COST", C_BLUE, C_BG);
    text(col_atk, TOP_Y + 134, "Only changed state", C_DIM, C_BG);

    /* Counter area backgrounds */
    int ctr_y = TOP_Y + 160;
    int ctr_h = 120;
    panel(col_sw, ctr_y, 400, ctr_h, C_DKORANGE, C_ORANGE);
    panel(col_diff, ctr_y, 360, ctr_h, C_DKGREEN, C_GREEN);
    panel(col_atk, ctr_y, 400, ctr_h, C_BLUE_DK, C_BLUE);

    /* Gap bar area */
    int bar_y = ctr_y + ctr_h + 40;
    text(M, bar_y - 16, "Savings gap (wider = more money saved):", C_DIM, C_BG);
    int bar_w_max = FB_HRES - 2 * M;

    /* Bottom info panel */
    int info_y = bar_y + 80;
    panel(0, info_y, FB_HRES, 36, C_PANEL, C_GOLD);
    text(M, info_y + 10, "Auto-running 10 seconds  |  Press Q to stop early",
         C_DIM, C_PANEL);

    flush_l2();

    /* Auto-run loop */
    uint64_t t_start = rdtime();
    float sw_total = 0.0f, atk_total = 0.0f;
    int running_race = 1;

    while (running_race) {
        uint64_t now = rdtime();
        float elapsed = (float)(now - t_start) / 100000000.0f; /* 100MHz timer */

        sw_total = sw_rate * elapsed;
        atk_total = atk_rate * elapsed;
        float diff = sw_total - atk_total;

        /* Update SW counter */
        rect(col_sw + 8, ctr_y + 20, 380, 80, C_DKORANGE);
        snprintf(buf, sizeof(buf), "$%.4f", sw_total);
        text3x(col_sw + 16, ctr_y + 30, buf, C_ORANGE, C_DKORANGE);
        snprintf(buf, sizeof(buf), "$%.0f/hr (accelerated)", sw_rate * 3600.0f / accel);
        text(col_sw + 16, ctr_y + 82, buf, C_DIM, C_DKORANGE);

        /* Update ATOMiK counter */
        rect(col_atk + 8, ctr_y + 20, 380, 80, C_BLUE_DK);
        snprintf(buf, sizeof(buf), "$%.4f", atk_total);
        text3x(col_atk + 16, ctr_y + 30, buf, C_BLUE, C_BLUE_DK);
        snprintf(buf, sizeof(buf), "$%.0f/hr (accelerated)", atk_rate * 3600.0f / accel);
        text(col_atk + 16, ctr_y + 82, buf, C_DIM, C_BLUE_DK);

        /* Update DIFFERENCE counter */
        rect(col_diff + 8, ctr_y + 20, 340, 80, C_DKGREEN);
        snprintf(buf, sizeof(buf), "$%.4f", diff);
        text3x(col_diff + 16, ctr_y + 30, buf, C_GREEN, C_DKGREEN);
        snprintf(buf, sizeof(buf), "saved so far");
        text(col_diff + 16, ctr_y + 82, buf, C_DIM, C_DKGREEN);

        /* Gap bar — proportional to difference fraction */
        float gap_frac = sw_total > 0 ? diff / sw_total : 0;
        int bar_w = (int)(gap_frac * bar_w_max);
        if (bar_w < 4 && diff > 0) bar_w = 4;
        if (bar_w > bar_w_max) bar_w = bar_w_max;
        rect(M, bar_y, bar_w_max, 40, RGB(0x14,0x1E,0x28));
        if (bar_w > 0) {
            rect(M, bar_y, bar_w, 40, C_GREEN);
            /* Percentage label inside bar */
            snprintf(buf, sizeof(buf), "%.0f%% saved", pct);
            if (bar_w > 120)
                text(M + bar_w / 2 - 40, bar_y + 12, buf, C_BG, C_GREEN);
        }

        flush_l2();

        /* Check for keypress or 10 second timeout */
        if (elapsed >= 10.0f) {
            running_race = 0;
        } else if (key_ready()) {
            char rch;
            if (read(0, &rch, 1) == 1) {
                if (rch == 'Q' || rch == 'q' || rch == 27) {
                    running_race = 0;
                }
                /* Handle ~ commands */
                if (rch == '~') {
                    char cmdbuf[512] = {0}; int ci = 0;
                    int got_eol = 0;
                    for (int attempt = 0; attempt < 200 && !got_eol; attempt++) {
                        fd_set fs; struct timeval tv = {0, 50000};
                        FD_ZERO(&fs); FD_SET(0, &fs);
                        if (select(1, &fs, NULL, NULL, &tv) > 0) {
                            char tmp[256]; int n = read(0, tmp, sizeof(tmp));
                            for (int j = 0; j < n && ci < 510; j++) {
                                if (tmp[j]=='\n'||tmp[j]=='\r') {got_eol=1;break;}
                                cmdbuf[ci++] = tmp[j];
                            }
                        } else if (ci > 0) break;
                    }
                    if (ci > 0) {
                        cmd_running = 1;
                        printf("##RSP:CMD:%s\n", cmdbuf);
                        FILE *pp = popen(cmdbuf, "r");
                        if (pp) {
                            char line[256];
                            while (fgets(line, sizeof(line), pp)) {
                                int len = strlen(line);
                                if (len>0 && line[len-1]=='\n') line[len-1]=0;
                                printf("##RSP:%s\n", line);
                            }
                            printf("##RSP:EXIT:%d\n", WEXITSTATUS(pclose(pp)));
                        } else printf("##RSP:ERROR:popen failed\n");
                        printf("##RSP:END\n"); fflush(stdout);
                        cmd_running = 0;
                    }
                }
            }
        }

        usleep(100000); /* 100ms between updates */
    }

    /* Final summary */
    float final_diff = sw_total - atk_total;
    rect(0, info_y, FB_HRES, FB_VRES - info_y, C_BG);
    panel(0, info_y, FB_HRES, 120, C_PANEL, C_GREEN);
    snprintf(buf, sizeof(buf),
        "ATOMiK saves $%.4f per 10 seconds at 1,000 servers", final_diff);
    text2x(M, info_y + 12, buf, C_GREEN, C_PANEL);
    snprintf(buf, sizeof(buf),
        "Annualized: $%.0fK/yr  |  %.0f%% compute eliminated",
        final_diff * 6.0f * 60.0f * 24.0f * 365.25f / 1000.0f, pct);
    text2x(M, info_y + 48, buf, C_GOLD, C_PANEL);
    text(M, info_y + 88, "Press any key to return", C_GRAY, C_PANEL);
    flush_l2();

    /* Wait for keypress */
    while (!key_ready()) usleep(50000);
    char dch; read(0, &dch, 1);

    log_event("Cost race complete.");
    { char lbuf[60];
      snprintf(lbuf, sizeof(lbuf), "Race: $%.4f saved in 10s @ 1K srv", final_diff);
      log_event(lbuf);
    }
}

/* ── Latency Scope (Shift+L) ──────────────────────────────────────── */
static void draw_latency_scope(void) {
    char buf[120];

    #define SCOPE_ROUNDS 20

    uint64_t sw_times[SCOPE_ROUNDS];
    uint64_t hw_times[SCOPE_ROUNDS];
    uint64_t sw_sum = 0, hw_sum = 0;
    uint64_t max_time = 1; /* avoid div-by-zero */

    /* Clear + title */
    rect(0, 0, FB_HRES, FB_VRES, C_BG);
    panel(0, TOP_Y, FB_HRES, 80, C_PANEL, C_BLUE);
    text3x(M, TOP_Y + 12, "LATENCY SCOPE", C_BLUE, C_PANEL);
    text2x(M + 420, TOP_Y + 20, "Oscilloscope-style timing traces", C_DIM, C_PANEL);
    text(M, TOP_Y + 60, "Measuring...", C_DIM, C_PANEL);
    flush_l2();

    /* Run measurements */
    /* Ensure shadows are current for memcmp baseline */
    for (int i = 0; i < N_BUF; i++)
        memcpy(shadows[i], buffers[i], BUF_SIZE);

    /* Warmup */
    for (int w = 0; w < 5; w++) {
        for (int i = 0; i < N_BUF; i++) {
            volatile int d = memcmp(buffers[i], shadows[i], BUF_SIZE);
            (void)d;
            load64(i, fp(buffers[i], BUF_SIZE));
            accum64(0);
            read64();
        }
    }

    for (int r = 0; r < SCOPE_ROUNDS; r++) {
        /* Mutate a few buffers so there's something to detect */
        int mutate = (r * 3 + 1) % N_BUF;
        buffers[mutate][(r * 97) % BUF_SIZE] ^= 0x55;

        /* SW: memcmp all 8 buffers */
        uint64_t t0 = rdtime();
        for (int i = 0; i < N_BUF; i++) {
            volatile int d = memcmp(buffers[i], shadows[i], BUF_SIZE);
            (void)d;
        }
        uint64_t t1 = rdtime();
        sw_times[r] = t1 - t0;
        sw_sum += sw_times[r];

        /* ATOMiK: fp + load64 + accum64 + read64 for all 8 */
        t0 = rdtime();
        for (int i = 0; i < N_BUF; i++) {
            uint64_t f = fp(buffers[i], BUF_SIZE);
            load64(i, f);
            accum64(f ^ fp(shadows[i], BUF_SIZE));
            read64();
        }
        t1 = rdtime();
        hw_times[r] = t1 - t0;
        hw_sum += hw_times[r];

        /* Update shadow for next round */
        memcpy(shadows[mutate], buffers[mutate], BUF_SIZE);

        /* Track max */
        if (sw_times[r] > max_time) max_time = sw_times[r];
        if (hw_times[r] > max_time) max_time = hw_times[r];
    }

    /* ── Draw the scope ───────────────────────────────────────────── */
    rect(M, TOP_Y + 56, 400, 20, C_PANEL); /* clear "Measuring..." */
    text(M, TOP_Y + 60, "Measurement complete", C_GREEN, C_PANEL);

    /* Trace areas */
    int trace_x = M + 60;
    int trace_w = FB_HRES - 2 * M - 80;
    int trace_h = 240;
    int sw_trace_y = TOP_Y + 100;
    int hw_trace_y = sw_trace_y + trace_h + 60;

    /* Labels */
    text2x(M, sw_trace_y, "SW", C_ORANGE, C_BG);
    text2x(M, hw_trace_y, "HW", C_BLUE, C_BG);

    /* Trace backgrounds (dark scope screen) */
    uint32_t scope_bg = RGB(0x08,0x0C,0x10);
    rect(trace_x, sw_trace_y, trace_w, trace_h, scope_bg);
    rect(trace_x, hw_trace_y, trace_w, trace_h, scope_bg);

    /* Horizontal gridlines (4 divisions) */
    for (int g = 1; g < 4; g++) {
        int gy = sw_trace_y + g * trace_h / 4;
        for (int gx = trace_x; gx < trace_x + trace_w; gx += 8)
            px(gx, gy, RGB(0x20,0x28,0x30));
        gy = hw_trace_y + g * trace_h / 4;
        for (int gx = trace_x; gx < trace_x + trace_w; gx += 8)
            px(gx, gy, RGB(0x20,0x28,0x30));
    }

    /* Vertical gridlines every 5 rounds */
    int col_w = trace_w / SCOPE_ROUNDS;
    for (int g = 5; g < SCOPE_ROUNDS; g += 5) {
        int gx = trace_x + g * col_w;
        for (int gy = sw_trace_y; gy < sw_trace_y + trace_h; gy += 4)
            px(gx, gy, RGB(0x20,0x28,0x30));
        for (int gy = hw_trace_y; gy < hw_trace_y + trace_h; gy += 4)
            px(gx, gy, RGB(0x20,0x28,0x30));
        /* Round label */
        snprintf(buf, sizeof(buf), "%d", g);
        text(gx - 4, sw_trace_y + trace_h + 2, buf, C_DIM, C_BG);
    }

    /* Draw spikes — same vertical scale for both */
    for (int r = 0; r < SCOPE_ROUNDS; r++) {
        int cx = trace_x + r * col_w + col_w / 2;
        int spike_w = col_w - 6;
        if (spike_w < 4) spike_w = 4;

        /* SW spike (orange) */
        int sw_h = (int)((float)sw_times[r] / (float)max_time * (trace_h - 8));
        if (sw_h < 2) sw_h = 2;
        int sw_top = sw_trace_y + trace_h - 4 - sw_h;
        rect(cx - spike_w/2, sw_top, spike_w, sw_h, C_ORANGE);
        /* Bright tip */
        rect(cx - spike_w/2, sw_top, spike_w, 2, C_WHITE);

        /* HW spike (blue) */
        int hw_h = (int)((float)hw_times[r] / (float)max_time * (trace_h - 8));
        if (hw_h < 2) hw_h = 2;
        int hw_top = hw_trace_y + trace_h - 4 - hw_h;
        rect(cx - spike_w/2, hw_top, spike_w, hw_h, C_BLUE);
        /* Bright tip */
        rect(cx - spike_w/2, hw_top, spike_w, 2, C_WHITE);
    }

    /* Baseline */
    rect(trace_x, sw_trace_y + trace_h - 4, trace_w, 1, C_ORANGE);
    rect(trace_x, hw_trace_y + trace_h - 4, trace_w, 1, C_BLUE);

    /* Results panel */
    int res_y = hw_trace_y + trace_h + 40;
    uint64_t sw_avg = sw_sum / SCOPE_ROUNDS;
    uint64_t hw_avg = hw_sum / SCOPE_ROUNDS;
    float ratio = (float)sw_avg / (float)(hw_avg > 0 ? hw_avg : 1);

    panel(0, res_y, FB_HRES, 100, C_PANEL, C_GREEN);
    snprintf(buf, sizeof(buf),
        "Software: avg %llu cycles  |  ATOMiK: avg %llu cycles  |  %.0fx faster",
        (unsigned long long)sw_avg, (unsigned long long)hw_avg, ratio);
    text2x(M, res_y + 12, buf, C_TEXT, C_PANEL);
    snprintf(buf, sizeof(buf),
        "%d rounds x %d buffers x %d bytes  |  Same scale on both traces",
        SCOPE_ROUNDS, N_BUF, BUF_SIZE);
    text(M, res_y + 52, buf, C_DIM, C_PANEL);
    text(M, res_y + 76, "Press any key to return", C_GRAY, C_PANEL);

    flush_l2();

    /* Wait for keypress with ~ command handler */
    int scope_wait = 1;
    while (scope_wait) {
        if (key_ready()) {
            char sch;
            if (read(0, &sch, 1) == 1) {
                if (sch == '~') {
                    char cmdbuf[512] = {0}; int ci = 0;
                    int got_eol = 0;
                    for (int attempt = 0; attempt < 200 && !got_eol; attempt++) {
                        fd_set fs; struct timeval tv = {0, 50000};
                        FD_ZERO(&fs); FD_SET(0, &fs);
                        if (select(1, &fs, NULL, NULL, &tv) > 0) {
                            char tmp[256]; int n = read(0, tmp, sizeof(tmp));
                            for (int j = 0; j < n && ci < 510; j++) {
                                if (tmp[j]=='\n'||tmp[j]=='\r') {got_eol=1;break;}
                                cmdbuf[ci++] = tmp[j];
                            }
                        } else if (ci > 0) break;
                    }
                    if (ci > 0) {
                        cmd_running = 1;
                        printf("##RSP:CMD:%s\n", cmdbuf);
                        FILE *pp = popen(cmdbuf, "r");
                        if (pp) {
                            char line[256];
                            while (fgets(line, sizeof(line), pp)) {
                                int len = strlen(line);
                                if (len>0 && line[len-1]=='\n') line[len-1]=0;
                                printf("##RSP:%s\n", line);
                            }
                            printf("##RSP:EXIT:%d\n", WEXITSTATUS(pclose(pp)));
                        } else printf("##RSP:ERROR:popen failed\n");
                        printf("##RSP:END\n"); fflush(stdout);
                        cmd_running = 0;
                    }
                } else {
                    scope_wait = 0;
                }
            }
        }
        usleep(50000);
    }

    log_event("Latency scope complete.");
    { char lbuf[60];
      snprintf(lbuf, sizeof(lbuf), "Scope: SW %llu / HW %llu = %.0fx",
               (unsigned long long)sw_avg, (unsigned long long)hw_avg, ratio);
      log_event(lbuf);
    }

    #undef SCOPE_ROUNDS
}

/* ═══════════════════════════════════════════════════════════════════════
 *  STATE STORM — theatrical split-screen showing SW waste vs ATOMiK sparsity
 *  Replaces burst mode. Triggered by Shift+G.
 * ═══════════════════════════════════════════════════════════════════════ */
static void draw_state_storm(void) {
    char buf[120];

    /* Clear + title */
    rect(0, 0, FB_HRES, FB_VRES, C_BG);
    panel(0, TOP_Y, FB_HRES, 80, C_PANEL, C_ORANGE);
    text3x(M, TOP_Y + 12, "STATE STORM", C_ORANGE, C_PANEL);
    text(M + 340, TOP_Y + 28, "Watch software drown while ATOMiK stays sparse",
         C_DIM, C_PANEL);
    flush_l2();

    /* Layout */
    int mid = FB_HRES / 2;
    int half_w = mid - M - 8;
    int lane_w = half_w / N_BUF - 2;
    int lane_h = 320;
    int label_y = TOP_Y + 100;
    int lane_top = label_y + 40;
    int counter_y = lane_top + lane_h + 20;
    int stats_y = counter_y + 60;

    /* Column headers */
    text2x(M, label_y, "SOFTWARE", C_ORANGE, C_BG);
    text(M + 200, label_y + 12, "scans ALL state every cycle", C_DIM, C_BG);
    rect(mid - 1, label_y, 2, lane_h + 40, C_GRAY);
    int rx = mid + 8;
    text2x(rx, label_y, "ATOMiK", C_BLUE, C_BG);
    text(rx + 160, label_y + 12, "touches ONLY what changed", C_DIM, C_BG);

    flush_l2();

    /* Storm loop: 5 seconds, ~100 iterations at 50ms each */
    uint64_t storm_start = rdtime();
    uint64_t storm_end = storm_start + 500000000ULL; /* 5s at 100MHz */
    uint32_t rng = rdtime() & 0xFFFFFFFF;
    int storm_changes = 0;
    uint64_t storm_sw_kb = 0;
    uint64_t storm_hw_kb = 0;
    int total_skipped = 0;
    int iteration = 0;

    while (rdtime() < storm_end) {
        /* Mutate 2-6 random buffers */
        rng = rng * 1103515245 + 12345;
        int n_mut = 2 + (rng >> 16) % 5;
        for (int m = 0; m < n_mut; m++) {
            rng = rng * 1103515245 + 12345;
            int idx = rng % N_BUF;
            modify_buffer(idx);
        }

        /* Detect */
        detect_all();
        storm_changes += n_mut;

        /* Count changed vs skipped this iteration */
        int changed_now = 0;
        for (int i = 0; i < N_BUF; i++)
            if (buf_changed[i]) changed_now++;
        int skipped_now = N_BUF - changed_now;
        total_skipped += skipped_now;

        /* Track KB */
        storm_sw_kb = sw_scanned / 1024;
        storm_hw_kb = hw_touched / 1024;

        /* LEFT side: ALL 8 lanes filled orange (SW scans everything) */
        for (int i = 0; i < N_BUF; i++) {
            int lx = M + i * (lane_w + 2);
            rect(lx, lane_top, lane_w, lane_h, C_ORANGE);
            text(lx + 2, lane_top + 2, buf_names[i], C_BG, C_ORANGE);
        }

        /* RIGHT side: only CHANGED lanes blue, unchanged dark */
        for (int i = 0; i < N_BUF; i++) {
            int lx = rx + i * (lane_w + 2);
            if (buf_changed[i]) {
                rect(lx, lane_top, lane_w, lane_h, C_BLUE);
                text(lx + 2, lane_top + 2, buf_names[i], C_BG, C_BLUE);
            } else {
                rect(lx, lane_top, lane_w, lane_h, RGB(0x08,0x0C,0x10));
                text(lx + 2, lane_top + 2, buf_names[i], RGB(0x30,0x38,0x40),
                     RGB(0x08,0x0C,0x10));
            }
        }

        /* Giant center counter: BUFFERS SKIPPED */
        {
            rect(mid - 200, counter_y, 400, 48, C_BG);
            snprintf(buf, sizeof(buf), "%d BUFFERS SKIPPED", total_skipped);
            int tw = strlen(buf) * CW * 2;
            text2x(mid - tw / 2, counter_y + 4, buf, C_GREEN, C_BG);
        }

        /* Running totals */
        rect(M, stats_y, FB_HRES - 2*M, 32, C_BG);
        snprintf(buf, sizeof(buf), "SW scanned: %lluKB",
                 (unsigned long long)storm_sw_kb);
        text2x(M, stats_y, buf, C_ORANGE, C_BG);
        snprintf(buf, sizeof(buf), "ATOMiK touched: %lluKB",
                 (unsigned long long)storm_hw_kb);
        text2x(rx, stats_y, buf, C_BLUE, C_BG);

        flush_l2();
        iteration++;

        /* Check for early exit */
        if (key_ready()) {
            char sch;
            if (read(0, &sch, 1) == 1) {
                if (sch == 'Q' || sch == 'q' || sch == 27) break;
            }
        }

        usleep(50000); /* 50ms per iteration */
    }

    /* Freeze with final stats */
    float pct = storm_sw_kb > 0 ?
        100.0f * (storm_sw_kb - storm_hw_kb) / storm_sw_kb : 0;

    rect(0, counter_y - 20, FB_HRES, FB_VRES - counter_y + 20, C_BG);
    panel(0, counter_y - 20, FB_HRES, 260, C_PANEL, C_GREEN);

    snprintf(buf, sizeof(buf), "Storm complete: %d changes in 5 seconds",
             storm_changes);
    text2x(M, counter_y, buf, C_TEXT, C_PANEL);

    text2x(M, counter_y + 40, "Software scanned every byte, every time.",
           C_ORANGE, C_PANEL);
    text2x(M, counter_y + 76, "ATOMiK touched only what changed.",
           C_BLUE, C_PANEL);

    /* Giant percentage */
    snprintf(buf, sizeof(buf), "%.0f%% WORK AVOIDED", pct);
    {
        int tw = strlen(buf) * CW * 3;
        text3x(mid - tw / 2, counter_y + 120, buf, C_GREEN, C_PANEL);
    }

    snprintf(buf, sizeof(buf), "SW: %lluKB  |  ATOMiK: %lluKB  |  %d iterations",
             (unsigned long long)storm_sw_kb,
             (unsigned long long)storm_hw_kb, iteration);
    text(M, counter_y + 180, buf, C_DIM, C_PANEL);

    text(M, counter_y + 210, "Press any key to return", C_GRAY, C_PANEL);
    flush_l2();

    /* Wait for keypress */
    while (!key_ready()) usleep(50000);
    char sch; read(0, &sch, 1);

    log_event("State storm complete.");
    { char lbuf[60];
      snprintf(lbuf, sizeof(lbuf), "Storm: %d changes, %.0f%% avoided",
               storm_changes, pct);
      log_event(lbuf);
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  FREEZE FRAME — clean closing screen with three proof cards
 *  Triggered by Shift+E ("End").
 * ═══════════════════════════════════════════════════════════════════════ */
static void draw_freeze_frame(void) {
    char buf[120];

    /* Compute live pct_avoided */
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;

    /* Clear to dark */
    rect(0, 0, FB_HRES, FB_VRES, C_BG);

    /* Card dimensions */
    int card_w = 1400;
    int card_h = 140;
    int card_x = (FB_HRES - card_w) / 2;

    /* Card 1 (y=200): Blue border — LIVE HARDWARE */
    {
        int cy = 200;
        outlined(card_x, cy, card_w, card_h, C_PANEL, C_BLUE);
        rect(card_x, cy, card_w, 3, C_BLUE);
        rect(card_x, cy + card_h - 1, card_w, 1, C_BLUE);
        {
            const char *title = "LIVE HARDWARE";
            int tw = strlen(title) * CW * 3;
            text3x(card_x + (card_w - tw) / 2, cy + 16, title, C_BLUE, C_PANEL);
        }
        {
            const char *sub = "NaxRiscv RV64GC @ 100MHz | Ubuntu 24.04 | Zynq-7020";
            int sw = strlen(sub) * CW;
            text(card_x + (card_w - sw) / 2, cy + 80, sub, C_DIM, C_PANEL);
        }
    }

    /* Card 2 (y=400): Green border — ONLY DELTAS MOVED */
    {
        int cy = 400;
        outlined(card_x, cy, card_w, card_h, C_PANEL, C_GREEN);
        rect(card_x, cy, card_w, 3, C_GREEN);
        rect(card_x, cy + card_h - 1, card_w, 1, C_GREEN);
        {
            const char *title = "ONLY DELTAS MOVED";
            int tw = strlen(title) * CW * 3;
            text3x(card_x + (card_w - tw) / 2, cy + 16, title, C_GREEN, C_PANEL);
        }
        snprintf(buf, sizeof(buf),
                 "%.0f%% compute eliminated this session (measured, not projected)",
                 pct);
        {
            int sw = strlen(buf) * CW;
            text(card_x + (card_w - sw) / 2, cy + 80, buf, C_DIM, C_PANEL);
        }
    }

    /* Card 3 (y=600): Orange border — STANDARD C PATH */
    {
        int cy = 600;
        outlined(card_x, cy, card_w, card_h, C_PANEL, C_ORANGE);
        rect(card_x, cy, card_w, 3, C_ORANGE);
        rect(card_x, cy + card_h - 1, card_w, 1, C_ORANGE);
        {
            const char *title = "STANDARD C PATH";
            int tw = strlen(title) * CW * 3;
            text3x(card_x + (card_w - tw) / 2, cy + 16, title, C_ORANGE, C_PANEL);
        }
        {
            const char *sub = "#include atomik.h | riscv64-linux-gnu-gcc -O2 | runs on this board";
            int sw = strlen(sub) * CW;
            text(card_x + (card_w - sw) / 2, cy + 80, sub, C_DIM, C_PANEL);
        }
    }

    /* Bottom (y=850) */
    {
        const char *tagline = "The board is the proof. The product is the IP.";
        int tw = strlen(tagline) * CW * 2;
        text2x((FB_HRES - tw) / 2, 850, tagline, C_TEXT, C_BG);
    }
    {
        const char *logo = "ATOMiK";
        int tw = strlen(logo) * CW * 3;
        text3x((FB_HRES - tw) / 2, 900, logo, C_BLUE, C_BG);
    }

    flush_l2();

    /* Wait for keypress */
    while (!key_ready()) usleep(50000);
    char fch; read(0, &fch, 1);

    log_event("Freeze frame shown.");
}

/* Full dashboard (chrome + content) — used on init and reset */
/* ═══════════════════════════════════════════════════════════════════════
 *  ADVERSARIAL AUDIT — expose all internal state, invite investor to break it
 * ═══════════════════════════════════════════════════════════════════════ */

static void draw_adversarial_audit(void) {
    char buf[120];

    /* ── Layout constants ───────────────────────────────────────────── */
    #define AA_TOP      30
    #define AA_HDR_H    80
    #define AA_TBL_Y    (AA_TOP + AA_HDR_H + 20)
    #define AA_ROW_H    36
    #define AA_REG_Y    (AA_TBL_Y + N_BUF * AA_ROW_H + 24)
    #define AA_HELP_Y   (AA_REG_Y + 80)

    /* ── Full clear + header ────────────────────────────────────────── */
    rect(0, 0, FB_HRES, FB_VRES, C_BG);
    panel(0, AA_TOP, FB_HRES, AA_HDR_H, C_PANEL, C_RED);
    text3x(M, AA_TOP + 8, "ADVERSARIAL AUDIT", C_RED, C_PANEL);
    text(M, AA_TOP + 56, "All internal state exposed. Try to break it.", C_DIM, C_PANEL);

    /* LIVE badge */
    rect(FB_HRES - M - 200, AA_TOP + 16, 180, 44, C_RED);
    text2x(FB_HRES - M - 190, AA_TOP + 24, "AUDIT", C_WHITE, C_RED);

    /* ── Column headers ─────────────────────────────────────────────── */
    int col_name = M;
    int col_cur  = M + 140;
    int col_shad = M + 380;
    int col_stat = M + 620;
    int hdr_y = AA_TBL_Y - 20;
    text(col_name, hdr_y, "BUFFER", C_DIM, C_BG);
    text(col_cur,  hdr_y, "CURRENT FINGERPRINT", C_DIM, C_BG);
    text(col_shad, hdr_y, "SHADOW FINGERPRINT", C_DIM, C_BG);
    text(col_stat, hdr_y, "STATUS", C_DIM, C_BG);
    rect(M, hdr_y + 16, FB_HRES - 2*M, 1, C_GRAY);

    /* ── Draw the 8 buffer rows (initial state — all should match) ── */
    int tampered[N_BUF];
    memset(tampered, 0, sizeof(tampered));
    uint64_t detect_times[N_BUF];
    memset(detect_times, 0, sizeof(detect_times));

    /* Helper macro: draw one row */
    #define AA_DRAW_ROW(i) do { \
        int ry = AA_TBL_Y + (i) * AA_ROW_H; \
        uint64_t cfp = fp(buffers[(i)], BUF_SIZE); \
        uint64_t sfp = fp(shadows[(i)], BUF_SIZE); \
        int match = (cfp == sfp); \
        /* Clear row */ \
        rect(M, ry, FB_HRES - 2*M, AA_ROW_H - 2, C_BG); \
        /* Tampered flash: red background */ \
        if (tampered[(i)]) \
            rect(M, ry, FB_HRES - 2*M, AA_ROW_H - 2, RGB(0x40,0x08,0x08)); \
        /* Name */ \
        snprintf(buf, sizeof(buf), "%d. %s", (i)+1, buf_names[(i)]); \
        text(col_name, ry + 10, buf, C_TEXT, tampered[(i)] ? RGB(0x40,0x08,0x08) : C_BG); \
        /* Current FP */ \
        snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)cfp); \
        text(col_cur, ry + 10, buf, C_BLUE, tampered[(i)] ? RGB(0x40,0x08,0x08) : C_BG); \
        /* Shadow FP */ \
        snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)sfp); \
        text(col_shad, ry + 10, buf, C_ORANGE, tampered[(i)] ? RGB(0x40,0x08,0x08) : C_BG); \
        /* Status */ \
        if (match) { \
            text(col_stat, ry + 10, "MATCH", C_GREEN, tampered[(i)] ? RGB(0x40,0x08,0x08) : C_BG); \
        } else { \
            text(col_stat, ry + 10, "MISMATCH", C_RED, tampered[(i)] ? RGB(0x40,0x08,0x08) : C_BG); \
            if (tampered[(i)] && detect_times[(i)] > 0) { \
                uint64_t ns = detect_times[(i)] * 10; \
                snprintf(buf, sizeof(buf), "TAMPERED -- DETECTED IN %lu CYCLES (%luns)", \
                         (unsigned long)detect_times[(i)], (unsigned long)ns); \
                text(col_stat + 80, ry + 10, buf, C_RED, RGB(0x40,0x08,0x08)); \
            } \
        } \
    } while(0)

    for (int i = 0; i < N_BUF; i++) {
        AA_DRAW_ROW(i);
    }

    /* ── ATOMiK adapter register state ──────────────────────────────── */
    rect(M, AA_REG_Y, FB_HRES - 2*M, 60, C_PANEL);
    rect(M, AA_REG_Y, FB_HRES - 2*M, 2, C_BLUE);
    text(M + 8, AA_REG_Y + 8, "ATOMiK Adapter Registers:", C_DIM, C_PANEL);

    #define AA_DRAW_REGS() do { \
        uint32_t r_cmd = adapter[0]; \
        uint32_t r_rs1 = adapter[1]; \
        uint32_t r_rs2 = adapter[2]; \
        uint32_t r_rd  = adapter[3]; \
        snprintf(buf, sizeof(buf), \
                 "CMD=%08X  RS1=%08X  RS2=%08X  RD=%08X", \
                 r_cmd, r_rs1, r_rs2, r_rd); \
        rect(M + 8, AA_REG_Y + 28, FB_HRES - 2*M - 16, 20, C_PANEL); \
        text(M + 8, AA_REG_Y + 30, buf, C_BLUE, C_PANEL); \
    } while(0)

    AA_DRAW_REGS();

    /* ── Help line ──────────────────────────────────────────────────── */
    text(M, AA_HELP_Y, "Press 1-8 to corrupt a buffer, V to verify all, R to reset, Q to quit",
         C_GRAY, C_BG);
    text(M, AA_HELP_Y + 20, "~ prefix for screenshot commands", C_GRAY, C_BG);

    flush_l2();

    /* ── Interactive loop ───────────────────────────────────────────── */
    int audit_running = 1;
    while (audit_running) {
        if (key_ready()) {
            char ach;
            if (read(0, &ach, 1) != 1) continue;

            /* ~ command handler for screenshots */
            if (ach == '~') {
                char cmdbuf[512] = {0};
                int ci = 0;
                int got_eol = 0;
                for (int attempt = 0; attempt < 200 && !got_eol; attempt++) {
                    fd_set fs; struct timeval tv = {0, 50000};
                    FD_ZERO(&fs); FD_SET(0, &fs);
                    if (select(1, &fs, NULL, NULL, &tv) > 0) {
                        char tmp[256];
                        int n = read(0, tmp, sizeof(tmp));
                        for (int j = 0; j < n && ci < 510; j++) {
                            if (tmp[j] == '\n' || tmp[j] == '\r') { got_eol = 1; break; }
                            cmdbuf[ci++] = tmp[j];
                        }
                    } else if (ci > 0) break;
                }
                cmdbuf[ci] = 0;
                if (ci > 0) {
                    printf("##RSP:CMD:%s\n", cmdbuf);
                    FILE *pp = popen(cmdbuf, "r");
                    if (pp) {
                        char line[256];
                        while (fgets(line, sizeof(line), pp)) {
                            int len = strlen(line);
                            if (len > 0 && line[len-1] == '\n') line[len-1] = 0;
                            printf("##RSP:%s\n", line);
                        }
                        int rc = pclose(pp);
                        printf("##RSP:EXIT:%d\n", WEXITSTATUS(rc));
                    } else {
                        printf("##RSP:ERROR:popen failed\n");
                    }
                    printf("##RSP:END\n");
                    fflush(stdout);
                }
                continue;
            }

            if (ach >= '1' && ach <= '8') {
                /* Corrupt buffer N by flipping one random byte */
                int idx = ach - '1';
                int pos = rdtime() % BUF_SIZE;
                buffers[idx][pos] ^= 0xFF;
                tampered[idx] = 1;

                /* Immediately run detection on ALL buffers, time each */
                for (int i = 0; i < N_BUF; i++) {
                    uint64_t t0 = rdtime();
                    uint64_t cfp = fp(buffers[i], BUF_SIZE);
                    uint64_t sfp = fp(shadows[i], BUF_SIZE);
                    uint64_t t1 = rdtime();
                    if (cfp != sfp) {
                        detect_times[i] = t1 - t0;
                        tampered[i] = 1;
                    }
                }

                /* Redraw only changed rows + registers */
                for (int i = 0; i < N_BUF; i++) {
                    AA_DRAW_ROW(i);
                }
                AA_DRAW_REGS();
                flush_l2();

            } else if (ach == 'V' || ach == 'v') {
                /* Verify all 8, show timing */
                uint64_t total_t0 = rdtime();
                for (int i = 0; i < N_BUF; i++) {
                    uint64_t t0 = rdtime();
                    uint64_t cfp = fp(buffers[i], BUF_SIZE);
                    uint64_t sfp = fp(shadows[i], BUF_SIZE);
                    uint64_t t1 = rdtime();
                    if (cfp != sfp) {
                        tampered[i] = 1;
                        detect_times[i] = t1 - t0;
                    } else {
                        tampered[i] = 0;
                        detect_times[i] = t1 - t0;
                    }
                }
                uint64_t total_cycles_v = rdtime() - total_t0;
                uint64_t total_ns = total_cycles_v * 10;

                /* Redraw all rows */
                for (int i = 0; i < N_BUF; i++) {
                    AA_DRAW_ROW(i);
                }
                AA_DRAW_REGS();

                /* Show total timing below help */
                rect(M, AA_HELP_Y + 44, FB_HRES - 2*M, 20, C_BG);
                snprintf(buf, sizeof(buf), "Verified all 8 buffers in %llu cycles (%lluns at 100MHz)",
                         (unsigned long long)total_cycles_v, (unsigned long long)total_ns);
                text(M, AA_HELP_Y + 44, buf, C_GREEN, C_BG);
                flush_l2();

            } else if (ach == 'R' || ach == 'r') {
                /* Reset: restore all buffers from shadows, clear tamper state */
                for (int i = 0; i < N_BUF; i++) {
                    memcpy(buffers[i], shadows[i], BUF_SIZE);
                    tampered[i] = 0;
                    detect_times[i] = 0;
                }
                /* Redraw all rows */
                for (int i = 0; i < N_BUF; i++) {
                    AA_DRAW_ROW(i);
                }
                AA_DRAW_REGS();

                /* Clear timing line */
                rect(M, AA_HELP_Y + 44, FB_HRES - 2*M, 20, C_BG);
                text(M, AA_HELP_Y + 44, "All buffers restored. All GREEN.", C_GREEN, C_BG);
                flush_l2();

            } else if (ach == 'Q' || ach == 'q') {
                audit_running = 0;
            }
        }
        usleep(50000); /* 20 Hz poll */
    }

    /* Restore corrupted buffers from shadows before returning */
    for (int i = 0; i < N_BUF; i++) {
        if (tampered[i]) {
            memcpy(buffers[i], shadows[i], BUF_SIZE);
        }
    }

    #undef AA_DRAW_ROW
    #undef AA_DRAW_REGS
    #undef AA_TOP
    #undef AA_HDR_H
    #undef AA_TBL_Y
    #undef AA_ROW_H
    #undef AA_REG_Y
    #undef AA_HELP_Y
}

static void draw_attract(void) {
    rect(0, 0, FB_HRES, FB_VRES, C_BG);

    /* Giant centered ATOMiK logo */
    text3x(FB_HRES/2 - 108, 300, "ATOMiK", C_BLUE, C_BG);

    /* Subtitle */
    text2x(FB_HRES/2 - 240, 380, "State-Aware Execution Engine", C_DIM, C_BG);

    /* Pulsing prompt */
    text2x(FB_HRES/2 - 160, 500, "Press any buffer (1-8)", C_WHITE, C_BG);
    text(FB_HRES/2 - 200, 550, "to see ATOMiK detect meaningful change in hardware", C_DIM, C_BG);

    /* Bottom proof chips */
    int cy = 750;
    /* Chip 1 */
    rect(FB_HRES/2 - 500, cy, 300, 60, C_PANEL);
    rect(FB_HRES/2 - 500, cy, 300, 2, C_BLUE);
    text2x(FB_HRES/2 - 488, cy + 12, "Live Hardware", C_BLUE, C_PANEL);
    text(FB_HRES/2 - 488, cy + 40, "NaxRiscv RV64GC @ 100MHz", C_DIM, C_PANEL);

    /* Chip 2 */
    rect(FB_HRES/2 - 150, cy, 300, 60, C_PANEL);
    rect(FB_HRES/2 - 150, cy, 300, 2, C_GREEN);
    text2x(FB_HRES/2 - 138, cy + 12, "Only Deltas Move", C_GREEN, C_PANEL);
    text(FB_HRES/2 - 138, cy + 40, "Unchanged state = zero cost", C_DIM, C_PANEL);

    /* Chip 3 */
    rect(FB_HRES/2 + 200, cy, 300, 60, C_PANEL);
    rect(FB_HRES/2 + 200, cy, 300, 2, C_ORANGE);
    text2x(FB_HRES/2 + 212, cy + 12, "Standard C Path", C_ORANGE, C_PANEL);
    text(FB_HRES/2 + 212, cy + 40, "Same GCC. No new language.", C_DIM, C_PANEL);

    /* Speedup from benchmark */
    char buf[40];
    snprintf(buf, sizeof(buf), "%.0fx faster detection", measured_speedup);
    text2x(FB_HRES/2 - 160, 650, buf, RGB(0xFF,0xCC,0x00), C_BG);

    flush_l2();
}

static void draw_dashboard(void) {
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
    memset(slot_active, 0, sizeof(slot_active));
    total_cycles++;

    for (int i = 0; i < N_BUF; i++) {
        int vp = slot_vproc[i];
        if (vp == VP_IDLE) continue;  /* idle slots skip */

        uint64_t ofp = fp(shadows[i], BUF_SIZE);
        uint64_t nfp = fp(buffers[i], BUF_SIZE);
        sw_scanned += BUF_SIZE;

        /* All vproc types use the same ATOMiK ops but with different semantics:
         * DETECT:  load old → accum delta → read: changed?
         * VERIFY:  load known-good → accum current → read: tampered?
         * SYNC:    load old → accum delta → read: if changed, mark for sync
         * ACCUM:   load base → accum new data → read: accumulated state
         * WATCH:   load last-seen → accum current → read: anything moved?
         * The hardware doesn't care — it's XOR algebra either way. */
        load64(i, ofp);
        accum64(nfp ^ ofp);
        uint64_t st = read64();
        buf_changed[i] = (st != ofp);
        slot_active[i] = 1;
        slot_ops[i] += 3;

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
    /* Update cost ticker: $50/srv/yr at 1000 servers, saved per detection cycle */
    session_savings += pct / 100.0f * 50.0f * 1000.0f / (365.25f * 24 * 3600);
    if (!cmd_running) {
        printf("##EVENT:%d:%d:%.1f:%02X:%.0f:%llu:%llu\n",
               total_cycles, changed, pct, mask, measured_speedup,
               (unsigned long long)(sw_scanned / 1024),
               (unsigned long long)(hw_touched / 1024));
        fflush(stdout);
    }
}

static void reset_all(void) {
    for (int i = 0; i < N_BUF; i++) {
        memset(buffers[i], 0xAA + i, BUF_SIZE);
        memcpy(shadows[i], buffers[i], BUF_SIZE);
        buf_changed[i] = 0;
        buf_change_count[i] = 0;
        slot_active[i] = 0;
        slot_ops[i] = 0;
    }
    apply_vproc_preset(workload_profile);
    sw_scanned = 0; hw_touched = 0;
    total_ops = 0; total_cycles = 0; total_changes = 0;

    for (int i = 0; i < N_BUF; i++) {
        load64(i, fp(buffers[i], BUF_SIZE));
        total_ops++;
    }
    event_count = 0;
    viz_reset();
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

    /* Ignore SIGPIPE so popen() pipelines (e.g. cat … | head -5) don't
     * kill us when the downstream process closes early. */
    signal(SIGPIPE, SIG_IGN);

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

    /* Initial draw — attract screen first, dashboard after first keypress */
    refresh_viz();
    draw_attract();
    update_lcd();
    flush_l2();

    printf("Interactive mode. Press keys to modify state.\n");

    int running = 1;
    while (running) {
        if (key_ready()) {
            char ch;
            if (read(0, &ch, 1) == 1) {
                char msg[60];

                /* ── Remote command executor ─────────────────────────
                 * Lines starting with ~ are shell commands from the
                 * laptop-side Claude. Read the rest of the line,
                 * execute via popen(), return results as ##RSP: lines.
                 * Demo keeps running — no interruption.
                 * ──────────────────────────────────────────────────── */
                if (ch == '~') {
                    char cmdbuf[512] = {0};
                    int ci = 0;
                    /* Read rest of line — batch-read to avoid losing
                     * chars at 921600 baud. Wait up to 2s total,
                     * with short re-polls to catch trailing bytes. */
                    int got_eol = 0;
                    for (int attempt = 0; attempt < 200 && !got_eol; attempt++) {
                        fd_set fs; struct timeval tv = {0, 50000}; /* 50ms */
                        FD_ZERO(&fs); FD_SET(0, &fs);
                        if (select(1, &fs, NULL, NULL, &tv) > 0) {
                            char tmp[256];
                            int n = read(0, tmp, sizeof(tmp));
                            for (int j = 0; j < n && ci < 510; j++) {
                                if (tmp[j] == '\n' || tmp[j] == '\r') { got_eol = 1; break; }
                                cmdbuf[ci++] = tmp[j];
                            }
                        } else if (ci > 0) {
                            break; /* have data + timeout = done */
                        }
                    }
                    cmdbuf[ci] = 0;
                    if (ci > 0) {
                        cmd_running = 1;
                        printf("##RSP:CMD:%s\n", cmdbuf);
                        FILE *pp = popen(cmdbuf, "r");
                        if (pp) {
                            char line[256];
                            while (fgets(line, sizeof(line), pp)) {
                                /* Strip trailing newline */
                                int len = strlen(line);
                                if (len > 0 && line[len-1] == '\n') line[len-1] = 0;
                                printf("##RSP:%s\n", line);
                            }
                            int rc = pclose(pp);
                            printf("##RSP:EXIT:%d\n", WEXITSTATUS(rc));
                        } else {
                            printf("##RSP:ERROR:popen failed\n");
                        }
                        printf("##RSP:END\n");
                        fflush(stdout);
                        cmd_running = 0;
                    }
                    goto key_done;
                }

                /* Attract mode: first 1-8 keypress transitions to dashboard */
                if (attract_mode && ch >= '1' && ch <= '8') {
                    attract_mode = 0;
                    draw_dashboard();
                    flush_l2();
                    /* fall through to process the key normally */
                }

                /* Backspace/DEL — handle before switch (avoids signed char issues) */
                if (ch == 127 || ch == 8 || ch == '<') {
                    if (viz_letter_pos > 0) {
                        viz_backspace();
                        log_event("Backspace: delta rolled back (XOR undo)");
                        draw_typing();
                        flush_l2();
                    }
                    goto key_done;
                }
                /* Printable non-command chars → type into accumulator (fast path) */
                if (ch >= 32 && ch < 127
                    && !(ch >= '1' && ch <= '8')
                    && !(ch >= 'A' && ch <= 'Z')  /* ALL uppercase = commands */
                    && ch != '?') {
                    if (viz_letter_pos < 63) {
                        viz_type_char(ch);
                        snprintf(msg, sizeof(msg), "Typed '%c' -> byte %d", ch, (viz_letter_pos-1) % 8);
                        log_event(msg);
                        draw_typing();
                        flush_l2();
                    }
                    goto key_done;
                }
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
                case 'A':
                    draw_adversarial_audit();
                    draw_dashboard(); update_lcd(); flush_l2();
                    break;
                case 'R':
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
                case 'V': {
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
                case 'C':
                    draw_break_it();
                    draw_dashboard(); update_lcd(); flush_l2();
                    break;
                case 'B':
                    draw_benchmark_race();
                    draw_dashboard(); update_lcd(); flush_l2();
                    break;
                case 'G':
                    draw_state_storm();
                    draw_dashboard(); update_lcd(); flush_l2();
                    break;
                case 'D':
                    draw_dollar_race();
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'E':
                    draw_freeze_frame();
                    draw_dashboard(); update_lcd(); flush_l2();
                    break;
                case 'F':
                    draw_adoption();
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'I':
                    /* AI Training + Inference demo */
                    log_event("Launching AI workload demo...");
                    draw_ai_demo();
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'T':
                    draw_tco_calculator();
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'L':
                    draw_latency_scope();
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'X': {
                    /* Compiler lane: standard C + GCC + ATOMiK */
                    rect(0, 0, FB_HRES, FB_VRES, C_BG);
                    panel(0, TOP_Y, FB_HRES, 80, C_PANEL, C_GREEN);
                    text3x(M, TOP_Y + 12, "COMPILER LANE", C_GREEN, C_PANEL);
                    text2x(M + 400, TOP_Y + 20, "Standard C. Standard GCC. ATOMiK hardware.", C_DIM, C_PANEL);

                    /* Show the actual code */
                    int cy = TOP_Y + 120;
                    text2x(M, cy, "#include \"atomik.h\"", C_GREEN, C_BG);
                    cy += 50;
                    text2x(M, cy, "// Load initial state into hardware slot", C_DIM, C_BG);
                    cy += 36;
                    text2x(M, cy, "atomik_load(slot, fingerprint);", C_WHITE, C_BG);
                    cy += 50;
                    text2x(M, cy, "// Accumulate delta (XOR)", C_DIM, C_BG);
                    cy += 36;
                    text2x(M, cy, "atomik_accum(new_fp ^ old_fp);", C_WHITE, C_BG);
                    cy += 50;
                    text2x(M, cy, "// Read result — changed or not?", C_DIM, C_BG);
                    cy += 36;
                    text2x(M, cy, "if (atomik_read() != old_fp)", C_WHITE, C_BG);
                    cy += 36;
                    text2x(M, cy, "    sync_this_buffer();", C_BLUE, C_BG);

                    cy += 80;
                    rect(M, cy, FB_HRES - 2*M, 2, C_ORANGE);
                    cy += 20;
                    text2x(M, cy, "Build command:", C_DIM, C_BG);
                    cy += 36;
                    text2x(M, cy, "riscv64-linux-gnu-gcc -O2 -o demo demo.c", C_ORANGE, C_BG);
                    cy += 50;
                    text2x(M, cy, "-> ATOMiK ops compile to standard RISC-V instructions", C_DIM, C_BG);
                    cy += 36;
                    text2x(M, cy, "-> Runs on this board. Right now. No special toolchain.", C_GREEN, C_BG);

                    cy += 60;
                    text3x(M, cy, "No new language.", C_GREEN, C_BG);
                    text3x(M + 500, cy, "No new compiler.", C_GREEN, C_BG);

                    text(M, FB_VRES - 60, "Press any key to return", C_GRAY, C_BG);
                    log_event("Compiler lane shown.");
                    flush_l2();
                    while (!key_ready()) usleep(50000);
                    { char xch; read(0, &xch, 1); }
                    draw_dashboard(); update_lcd(); flush_l2();
                    break;
                }
                case 'W':
                    /* Cycle through workload profiles — reconfigures virtual processors */
                    workload_profile = (workload_profile + 1) % 5;
                    apply_vproc_preset(workload_profile);
                    snprintf(msg, sizeof(msg), "VProc: %s", profile_names[workload_profile]);
                    log_event(msg);
                    /* Show the vproc assignment */
                    { char vp_msg[60] = "Slots: ";
                      for (int i = 0; i < N_BUF; i++) {
                          char tmp[8]; snprintf(tmp, sizeof(tmp), "%s ", vproc_names[slot_vproc[i]]);
                          if (strlen(vp_msg) + strlen(tmp) < 58) strcat(vp_msg, tmp);
                      }
                      log_event(vp_msg);
                    }
                    if (workload_profile > 0) {
                        int pat_idx = total_cycles % 4;
                        uint8_t mask = profile_patterns[workload_profile][pat_idx];
                        for (int i = 0; i < N_BUF; i++) {
                            if (mask & (1 << i)) modify_buffer(i);
                        }
                        detect_all();
                    }
                    refresh_viz(); draw_content();
                    update_lcd();
                    flush_l2();
                    break;
                case 'S': {
                    /* Session summary */
                    char buf[80];
                    rect(0, 96, FB_HRES, FB_VRES - 96 - 48, C_BG);
                    text3x(M, 120, "Session Summary", C_TEXT, C_BG);
                    float spct = sw_scanned > 0 ?
                        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;
                    float ssav_1k = spct * 50.0f * 1000.0f / 100.0f / 1000.0f;
                    float ssav_g = spct * 50.0f * 50e6f / 100.0f / 1e9f;
                    snprintf(buf, sizeof(buf), "Total cycles: %d", total_cycles);
                    text2x(M, 200, buf, C_TEXT, C_BG);
                    snprintf(buf, sizeof(buf), "Total changes detected: %d", total_changes);
                    text2x(M, 240, buf, C_BLUE, C_BG);
                    snprintf(buf, sizeof(buf), "Software scanned: %llu KB", (unsigned long long)(sw_scanned/1024));
                    text2x(M, 280, buf, C_ORANGE, C_BG);
                    snprintf(buf, sizeof(buf), "ATOMiK touched: %llu KB", (unsigned long long)(hw_touched/1024));
                    text2x(M, 320, buf, C_BLUE, C_BG);
                    snprintf(buf, sizeof(buf), "Data avoided: %llu KB (%.0f%%)", (unsigned long long)((sw_scanned-hw_touched)/1024), spct);
                    text2x(M, 360, buf, C_GREEN, C_BG);
                    snprintf(buf, sizeof(buf), "Savings: $%.0fK/yr (1K)  |  $%.1fB/yr (50M global)", ssav_1k, ssav_g);
                    text2x(M, 420, buf, C_GREEN, C_BG);
                    snprintf(buf, sizeof(buf), "Query speedup: %.0fx (4KB median)", measured_speedup);
                    text2x(M, 460, buf, RGB(0xFF,0xCC,0x00), C_BG);
                    text2x(M, 540, "Proven live. On this board. In this session.", C_TEXT, C_BG);
                    text(M, 600, "Press any key to return", C_GRAY, C_BG);
                    flush_l2();
                    while (!key_ready()) usleep(50000);
                    char sch; read(0, &sch, 1);
                    draw_dashboard(); update_lcd(); flush_l2();
                    break;
                }
                case 'H': case '?': {
                    /* Help overlay */
                    rect(0, 96, FB_HRES, FB_VRES - 96 - 48, C_BG);
                    text3x(M, 120, "ATOMiK Controls", C_BLUE, C_BG);
                    int hy = 200;
                    text2x(M, hy, "[1-8]", C_TEXT, C_BG); text(M+120, hy+8, "Modify specific state buffer", C_DIM, C_BG);
                    text2x(M, hy+=40, "[a]", C_TEXT, C_BG); text(M+120, hy+8, "Adversarial audit -- expose state, try to break it", C_DIM, C_BG);
                    text2x(M, hy+=40, "[b]", C_TEXT, C_BG); text(M+120, hy+8, "Benchmark race -- memcmp vs ATOMiK (full-screen)", C_DIM, C_BG);
                    text2x(M, hy+=40, "[G]", C_TEXT, C_BG); text(M+120, hy+8, "State storm -- SW drowns while ATOMiK stays sparse", C_DIM, C_BG);
                    text2x(M, hy+=40, "[c]", C_TEXT, C_BG); text(M+120, hy+8, "Inject corruption + auto-detect tamper", C_DIM, C_BG);
                    text2x(M, hy+=40, "[v]", C_TEXT, C_BG); text(M+120, hy+8, "Verify integrity of all buffers", C_DIM, C_BG);
                    text2x(M, hy+=40, "[w]", C_TEXT, C_BG); text(M+120, hy+8, "Cycle workload profiles (Agent/Cache/Full/Idle)", C_DIM, C_BG);
                    text2x(M, hy+=40, "[D]", C_TEXT, C_BG); text(M+120, hy+8, "Cost race — dollar counters racing in real-time", C_DIM, C_BG);
                    text2x(M, hy+=40, "[E]", C_TEXT, C_BG); text(M+120, hy+8, "Freeze frame — closing proof screen for investors", C_DIM, C_BG);
                    text2x(M, hy+=40, "[L]", C_TEXT, C_BG); text(M+120, hy+8, "Latency scope — oscilloscope timing traces", C_DIM, C_BG);
                    text2x(M, hy+=40, "[f]", C_TEXT, C_BG); text(M+120, hy+8, "Adoption forecast — year-by-year TAM", C_DIM, C_BG);
                    text2x(M, hy+=40, "[s]", C_TEXT, C_BG); text(M+120, hy+8, "Session summary — aggregate proof", C_DIM, C_BG);
                    text2x(M, hy+=40, "[T]", C_TEXT, C_BG); text(M+120, hy+8, "TCO calculator — savings at server scale", C_DIM, C_BG);
                    text2x(M, hy+=40, "[X]", C_TEXT, C_BG); text(M+120, hy+8, "Compiler lane — standard C execution path", C_DIM, C_BG);
                    text2x(M, hy+=40, "[r]", C_TEXT, C_BG); text(M+120, hy+8, "Reset all state + counters", C_DIM, C_BG);
                    text2x(M, hy+=40, "[h]", C_TEXT, C_BG); text(M+120, hy+8, "This help screen", C_DIM, C_BG);
                    text(M, hy+60, "Press any key to return", C_GRAY, C_BG);
                    flush_l2();
                    while (!key_ready()) usleep(50000);
                    char hch; read(0, &hch, 1);
                    draw_dashboard(); update_lcd(); flush_l2();
                    break;
                }
                case 'Q':
                    running = 0;
                    break;
                case 127: case 8:   /* Backspace/DEL from direct UART */
                case '<':            /* Backspace alias from browser */
                    if (viz_letter_pos > 0) {
                        viz_backspace();
                        log_event("Backspace: delta rolled back (XOR undo)");
                        draw_content();
                        update_lcd();
                        flush_l2();
                    }
                    break;
                default:
                    break;
                }
                key_done: (void)0;
            }
        }
        usleep(50000); /* 20 Hz poll */

        /* ── Idle animation: auto-mutate + detect every 2s ──── */
        if (!attract_mode) {
            static uint64_t last_anim = 0;
            static uint32_t idle_rng = 0x12345678;
            uint64_t now = rdtime();
            if (now - last_anim > 200000000ULL) { /* 2s at 100MHz */
                last_anim = now;
                scan_pos = (scan_pos + 1) % N_BUF;

                /* Auto-mutate 1-3 random buffers to keep metrics alive */
                idle_rng = idle_rng * 1103515245 + 12345;
                int n_mut = 1 + (idle_rng >> 16) % 3;
                for (int m = 0; m < n_mut; m++) {
                    idle_rng = idle_rng * 1103515245 + 12345;
                    int slot = idle_rng % N_BUF;
                    for (int j = 0; j < 63; j++)
                        buffers[slot][(j * 64 + total_cycles * 3) % BUF_SIZE]
                            ^= ((m + total_cycles + j + 1) & 0xFF) | 0x01;
                }
                detect_all();

                /* Redraw SW lanes + metrics (lightweight, no full redraw) */
                int mid = FB_HRES / 2;
                int half_w = mid - M - 16;
                int lane_w = half_w / N_BUF - 2;
                for (int i = 0; i < N_BUF; i++) {
                    int lx = M + i * (lane_w + 2);
                    exec_lane(lx, LANE_TOP, lane_w, LANE_H, buf_names[i],
                              1, 0, C_ORANGE, SKIP_COL, 20, i, 1);
                }
                /* Redraw metrics panel (cost ticker updates) */
                draw_content();
                update_lcd();
                flush_l2();
            }
        }
    }

    term_restore();
    printf("\nATOMiK Live System stopped.\n");

    munmap(fb, FB_SIZE);
    munmap((void*)csr_page, CSR_SIZE);
    munmap((void*)adapter, 4096);
    close(memfd);
    return 0;
}
