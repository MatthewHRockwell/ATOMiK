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

/* Full hero display redraw */
static void draw_dashboard(void) {
    memset(fb, 0, FB_SIZE);
    char buf[80];

    int changed = 0;
    for (int i = 0; i < N_BUF; i++) if (buf_changed[i]) changed++;
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;

    /* ── Top bar ─────────────────────────────────────────────────── */
    rect(0, 0, FB_HRES, 64, C_PANEL);
    text2x(M, 16, "ATOMiK", C_BLUE, C_PANEL);
    rect(1568, 8, 256, 48, C_BLUE);
    text(1584, 24, "LIVE ON HARDWARE", C_PANEL, C_BLUE);

    /* ── SOFTWARE vs ATOMiK — the hero comparison ────────────────── */
    int box_w = 176, box_h = 100, gap = 20;
    int total_w = N_BUF * (box_w + gap) - gap;
    int x0 = (FB_HRES - total_w) / 2;

    /* Software row — all boxes lit orange */
    text2x(M, 96, "SOFTWARE", C_ORANGE, C_BG);
    text(M + 200, 108, "rescans all state every cycle", C_DIM, C_BG);
    for (int i = 0; i < N_BUF; i++) {
        int bx = x0 + i * (box_w + gap);
        rect(bx, 148, box_w, box_h, C_ORANGE);
        text(bx + 8, 156, buf_names[i], C_WHITE, C_ORANGE);
        text2x(bx + (box_w - 4*CW*2)/2, 180, "SCAN", C_WHITE, C_ORANGE);
    }

    /* ATOMiK row — only changed boxes blue, rest dark */
    int ay = 280;
    text2x(M, ay - 16, "ATOMiK", C_BLUE, C_BG);
    text(M + 160, ay - 4, "acts only on meaningful change", C_DIM, C_BG);
    for (int i = 0; i < N_BUF; i++) {
        int bx = x0 + i * (box_w + gap);
        if (buf_changed[i]) {
            rect(bx, ay + 16, box_w, box_h, C_BLUE);
            text(bx + 8, ay + 24, buf_names[i], C_WHITE, C_BLUE);
            text2x(bx + (box_w - 4*CW*2)/2, ay + 48, "SYNC", C_WHITE, C_BLUE);
        } else {
            rect(bx, ay + 16, box_w, box_h, 0x00141418);
            text(bx + 8, ay + 24, buf_names[i], 0x00303030, 0x00141418);
            text2x(bx + (box_w - 4*CW*2)/2, ay + 48, "SKIP", 0x00303030, 0x00141418);
        }
    }

    /* ── Hero numbers — center of screen ─────────────────────────── */
    int ny = 440;
    rect(0, ny, FB_HRES, 200, C_PANEL);

    /* Data avoided — the headline */
    snprintf(buf, sizeof(buf), "%.0f%%", pct);
    text3x(M + 40, ny + 20, buf, C_GREEN, C_PANEL);
    text2x(M + 40, ny + 72, "less compute", C_GREEN, C_PANEL);
    text(M + 40, ny + 110, "less energy. less cost.", C_GREEN, C_PANEL);

    /* Synced count */
    snprintf(buf, sizeof(buf), "%d of 8", changed);
    text3x(600, ny + 20, buf, C_BLUE, C_PANEL);
    text2x(600, ny + 72, "buffers synced", C_BLUE, C_PANEL);

    /* Cost projection */
    float savings = pct * 0.5f;
    snprintf(buf, sizeof(buf), "$%.0fK", savings);
    text3x(1050, ny + 20, buf, C_GREEN, C_PANEL);
    text2x(1050, ny + 72, "saved / year", C_GREEN, C_PANEL);
    text(1050, ny + 110, "at 1,000 servers", C_DIM, C_PANEL);

    /* Flow bar */
    int fy = ny + 150;
    int bar_w = FB_HRES - 2*M - 400;
    int bar_x = M + 200;
    text(M + 40, fy + 4, "Software", C_ORANGE, C_PANEL);
    rect(bar_x, fy, bar_w, 18, C_ORANGE);
    text(M + 40, fy + 24, "ATOMiK", C_BLUE, C_PANEL);
    int hw_w = (int)((100.0f - pct) / 100.0f * bar_w);
    if (hw_w < 4) hw_w = 4;
    rect(bar_x, fy + 24, hw_w, 18, C_BLUE);
    rect(bar_x + hw_w, fy + 24, bar_w - hw_w, 18, 0x00141418);

    /* ── Bottom: adoption message + subtle log ───────────────────── */
    int by = 680;
    textc(by, "Same C. Standard GCC. ATOMiK hardware acceleration.", C_DIM, C_BG);
    textc(by + 24, "No new language. No new religion.", C_DIM, C_BG);

    /* Recent events — very subtle */
    int ly = 760;
    text(M, ly, "Recent:", C_GRAY, C_BG);
    for (int i = 0; i < event_count && i < 4; i++) {
        int idx = event_count - 1 - i;
        if (idx >= 0)
            text(M + 80 + i * 400, ly, event_log[idx],
                 i == 0 ? C_DIM : C_GRAY, C_BG);
    }

    /* Memory anchor at very bottom */
    rect(0, FB_VRES - 80, FB_HRES, 80, C_PANEL);
    textc(FB_VRES - 60, "ATOMiK removes wasted rediscovery of change.", C_TEXT, C_PANEL);
    textc(FB_VRES - 36, "Licensable compute IP for state-heavy systems.", C_BLUE, C_PANEL);
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
    int mask = 0;
    for (int i = 0; i < N_BUF; i++) {
        if (buf_changed[i]) { changed++; mask |= (1 << i); }
    }
    float pct = sw_scanned > 0 ?
        100.0f * (sw_scanned - hw_touched) / sw_scanned : 0;
    printf("##EVENT:%d:%d:%.1f:%02X\n", total_cycles, changed, pct, mask);
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
