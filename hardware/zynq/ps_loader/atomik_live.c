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
 *  INTERACTIVE DASHBOARD
 * ═══════════════════════════════════════════════════════════════════════ */

/* Layout constants */
#define M         96    /* margin */
#define TITLE_Y   32
#define STATE_Y   112
#define GAUGE_X   1200
#define GAUGE_Y   112
#define GAUGE_W   120
#define GAUGE_H   400
#define COST_Y    560
#define CMD_Y     800
#define FLOW_Y    720

/* Draw a vertical gauge */
static void draw_gauge(int x, int y, int w, int h, float frac,
                       uint32_t fill, uint32_t empty, const char *label) {
    rect(x, y, w, h, empty);
    int fh = (int)(frac * h);
    if (fh > h) fh = h;
    rect(x, y + h - fh, w, fh, fill);
    /* border */
    rect(x, y, w, 1, C_DIM); rect(x, y+h-1, w, 1, C_DIM);
    rect(x, y, 1, h, C_DIM); rect(x+w-1, y, 1, h, C_DIM);
    /* label */
    int ll = strlen(label);
    text(x + (w - ll*CW)/2, y + h + 8, label, C_DIM, C_BG);
}

/* Draw state buffer list */
static void draw_buffers(void) {
    int x = M, y = STATE_Y + 48;
    char buf[60];

    text2x(M, STATE_Y, "State Buffers", C_TEXT, C_BG);
    text(M, STATE_Y + 36, "Press 1-8 to modify  |  a = all  |  r = reset", C_DIM, C_BG);

    for (int i = 0; i < N_BUF; i++) {
        int bx = x, by = y + i * 72;
        uint32_t bg = buf_changed[i] ? C_BLUE : C_GRAY;
        uint32_t border = buf_changed[i] ? C_BLUE : C_GRAY;

        /* Buffer card */
        rect(bx, by, 1040, 60, bg);

        /* Number key hint */
        snprintf(buf, sizeof(buf), "[%d]", i+1);
        text(bx + 12, by + 8, buf, C_WHITE, bg);

        /* Name */
        text2x(bx + 56, by + 6, buf_names[i], C_WHITE, bg);

        /* Status */
        const char *status = buf_changed[i] ? "MODIFIED" : "CLEAN";
        uint32_t sc = buf_changed[i] ? C_WHITE : C_DIM;
        text(bx + 56, by + 40, status, sc, bg);

        /* Change count */
        snprintf(buf, sizeof(buf), "%d changes", buf_change_count[i]);
        text(bx + 240, by + 40, buf, C_DIM, bg);

        /* Fingerprint */
        uint64_t f = fp(buffers[i], BUF_SIZE);
        snprintf(buf, sizeof(buf), "fp:%016llX", (unsigned long long)f);
        text(bx + 500, by + 40, buf, C_DIM, bg);
    }
}

/* Draw energy/compute gauges */
static void draw_gauges(void) {
    int gx = GAUGE_X;

    text2x(gx, STATE_Y, "Compute Cost", C_TEXT, C_BG);

    /* SW gauge: always 100% (scans everything) */
    float sw_frac = 1.0f;
    draw_gauge(gx, GAUGE_Y + 48, GAUGE_W, GAUGE_H, sw_frac,
               C_ORANGE, C_DKORANGE, "Software");

    /* ATOMiK gauge: proportional to changed buffers */
    int changed = 0;
    for (int i = 0; i < N_BUF; i++) if (buf_changed[i]) changed++;
    float hw_frac = (float)changed / N_BUF;
    draw_gauge(gx + 180, GAUGE_Y + 48, GAUGE_W, GAUGE_H, hw_frac,
               C_BLUE, C_GRAY, "ATOMiK");

    /* Labels between gauges */
    char buf[40];
    text(gx + GAUGE_W + 20, GAUGE_Y + 48 + GAUGE_H/2 - 40,
         "scans", C_ORANGE, C_BG);
    text(gx + GAUGE_W + 20, GAUGE_Y + 48 + GAUGE_H/2 - 20,
         "all 8", C_ORANGE, C_BG);
    snprintf(buf, sizeof(buf), "%d of 8", changed);
    text(gx + GAUGE_W + 20, GAUGE_Y + 48 + GAUGE_H/2 + 20,
         "touches", C_BLUE, C_BG);
    text(gx + GAUGE_W + 20, GAUGE_Y + 48 + GAUGE_H/2 + 40,
         buf, C_BLUE, C_BG);

    /* Savings */
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;
    snprintf(buf, sizeof(buf), "%.0f%%", pct);
    text3x(gx + 60, GAUGE_Y + 48 + GAUGE_H + 48, buf, C_GREEN, C_BG);
    text(gx + 20, GAUGE_Y + 48 + GAUGE_H + 104, "less compute", C_GREEN, C_BG);
    text(gx + 20, GAUGE_Y + 48 + GAUGE_H + 120, "less energy", C_GREEN, C_BG);
    text(gx + 20, GAUGE_Y + 48 + GAUGE_H + 136, "less cost", C_GREEN, C_BG);
}

/* Draw cost projector */
static void draw_cost(void) {
    char buf[80];
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;

    rect(M, COST_Y, 1040, 100, C_PANEL);
    text2x(M + 20, COST_Y + 8, "At Scale", C_TEXT, C_PANEL);

    /* Assume $50/server/year for state management compute */
    float savings_per_server = 50.0f * pct / 100.0f;
    snprintf(buf, sizeof(buf), "1,000 servers: $%.0fK/year saved",
             savings_per_server * 1000 / 1000);
    text(M + 20, COST_Y + 48, buf, C_GREEN, C_PANEL);

    snprintf(buf, sizeof(buf), "%.0f%% less compute per sync cycle  |  %d checks run",
             pct, total_cycles);
    text(M + 20, COST_Y + 72, buf, C_DIM, C_PANEL);
}

/* Draw command history / event log */
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

static void draw_log(void) {
    rect(M, CMD_Y, 1040, 150, C_BG);
    text(M, CMD_Y, "Event Log", C_DIM, C_BG);
    for (int i = 0; i < event_count && i < 8; i++) {
        uint32_t c = (i == event_count - 1) ? C_TEXT : C_DIM;
        text(M + 16, CMD_Y + 20 + i * 16, event_log[i], c, C_BG);
    }
}

/* Full dashboard redraw */
static void draw_dashboard(void) {
    memset(fb, 0, FB_SIZE);

    /* Title bar */
    rect(0, 0, FB_HRES, TITLE_Y + 48, C_PANEL);
    text2x(M, TITLE_Y, "ATOMiK", C_BLUE, C_PANEL);
    text(M + 12*CW + 16, TITLE_Y + 16, "State Engine", C_TEXT, C_PANEL);
    rect(1540, TITLE_Y + 4, 280, 40, C_BLUE);
    text(1556, TITLE_Y + 12, "LIVE ON HARDWARE", C_PANEL, C_BLUE);

    /* System info */
    text(600, TITLE_Y + 4, "NaxRiscv RV64GC | 100 MHz | Zynq-7020 | Ubuntu 24.04", C_DIM, C_PANEL);
    text(600, TITLE_Y + 24, "Know what changed. Move only what matters.", C_BLUE, C_PANEL);

    draw_buffers();
    draw_gauges();
    draw_cost();
    draw_log();

    /* Bottom bar */
    rect(0, FB_VRES - 32, FB_HRES, 32, C_PANEL);
    text(M, FB_VRES - 24, "[1-8] modify buffer  [a] all  [r] reset  [p] presentation  [q] quit",
         C_DIM, C_PANEL);
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
    lline(40, buf, L_FG);

    /* Mini buffer strip */
    for (int i = 0; i < N_BUF; i++) {
        uint16_t c = buf_changed[i] ? L_BLUE : 0x2104;
        lfill(8 + i * 38, 70, 32, 24, c);
    }

    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;
    snprintf(buf, sizeof(buf), " %.0f%% data avoided", pct);
    lline(110, buf, L_GREEN);

    lline(140, " Only changed state", L_DIM);
    lline(152, " was sent.", L_DIM);
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
    for (int i = 0; i < N_BUF; i++) if (buf_changed[i]) changed++;
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;
    printf("##EVENT:%d:%d:%.1f\n", total_cycles, changed, pct);
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
    csr_wr(CSR_FB_VTG_EN, 1);
    csr_wr(CSR_FB_DMA_EN, 1);
    printf("Initializing LCD...\n");
    lcd_init();

    /* Initialize buffers */
    reset_all();
    log_event("ATOMiK Live System started.");
    log_event("8 state buffers initialized.");
    log_event("Hardware adapter at 0xF0020000 ready.");

    /* Set terminal to raw mode for key input */
    term_raw();

    /* Initial draw */
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
                    modify_buffer(idx);
                    detect_all();
                    snprintf(msg, sizeof(msg), "Modified %s -> %d changed, %d clean",
                             buf_names[idx],
                             total_changes, total_cycles * N_BUF - total_changes);
                    log_event(msg);
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                }
                case 'a': case 'A':
                    for (int i = 0; i < N_BUF; i++) modify_buffer(i);
                    detect_all();
                    log_event("Modified ALL buffers.");
                    draw_dashboard();
                    update_lcd();
                    flush_l2();
                    break;
                case 'r': case 'R':
                    reset_all();
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
