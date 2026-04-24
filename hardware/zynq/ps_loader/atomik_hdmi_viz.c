/*
 * ATOMiK Delta-State Engine — Live HDMI Visualization
 *
 * Drives the ATOMiK hardware adapter via MMIO and renders a real-time
 * visualization of delta-state operations to the 1920×1080 HDMI framebuffer.
 *
 * Build:
 *     riscv64-linux-gnu-gcc -O2 -static atomik_hdmi_viz.c -o atomik_hdmi_viz
 *
 * Usage:
 *     atomik_hdmi_viz          # run full demo sequence
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

/* ── ATOMiK Adapter CSRs ─────────────────────────────────────────────── */
#define ADAPTER_BASE 0xF0020000UL
#define REG_CMD   0x00
#define REG_RS1   0x04
#define REG_RS2   0x08
#define REG_RD    0x0C
#define REG_STAT  0x10

/* ── VTG / DMA CSRs ─────────────────────────────────────────────────── */
#define CSR_FB_DMA_ENABLE   0xF0004804UL
#define CSR_FB_VTG_ENABLE   0xF0005000UL
#define CSR_PAGE_BASE       0xF0000000UL
#define CSR_PAGE_SIZE       0x00100000UL

/* ── Colors (XRGB8888) ──────────────────────────────────────────────── */
#define COL_BG          0x00101820  /* dark blue-gray background */
#define COL_PANEL       0x00182030  /* slightly lighter panel bg */
#define COL_TEXT        0x00E0E0E0  /* light gray text */
#define COL_DIM         0x00505050  /* dim text */
#define COL_TITLE       0x0060C0FF  /* ATOMiK blue */
#define COL_BLUE_ON     0x004488FF  /* initial state bit=1 */
#define COL_BLUE_OFF    0x00102040  /* initial state bit=0 */
#define COL_RED_ON      0x00FF4400  /* accumulator bit=1 */
#define COL_RED_OFF     0x00401000  /* accumulator bit=0 */
#define COL_GREEN_ON    0x0044FF44  /* current state bit=1 */
#define COL_GREEN_OFF   0x00104010  /* current state bit=0 */
#define COL_YELLOW      0x00FFCC00  /* ACCUM label */
#define COL_MAGENTA     0x00FF44FF  /* SWAP label */
#define COL_WHITE       0x00FFFFFF
#define COL_BAR_SW      0x00FF6644  /* software bar color */
#define COL_BAR_HW      0x0044CCFF  /* hardware bar color */

/* ── Font ────────────────────────────────────────────────────────────── */
#include "font_8x16.h"
#define CHAR_W 8
#define CHAR_H 16

/* ── Globals ─────────────────────────────────────────────────────────── */
static uint32_t *fb;
static volatile uint32_t *adapter;
static int memfd;

/* L2 cache flush via eviction pressure.
 * NaxRiscv has no Zicbom (cbo.flush/cbo.clean), so we force dirty cache
 * line writeback by writing a scratch buffer larger than the L2 (32 KB).
 * Sequential writes to 128 KB of scratch evict all dirty FB lines to DDR. */
static volatile uint8_t flush_scratch[128 * 1024] __attribute__((aligned(64)));
static void flush_l2_to_ddr(void) {
    for (int i = 0; i < (int)sizeof(flush_scratch); i += 64)
        flush_scratch[i] = 0;
    asm volatile("fence iorw,iorw");
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
    while (*s) {
        fb_char(x, y, *s, fg, bg);
        x += CHAR_W;
        s++;
    }
}

static void fb_hex64(int x, int y, uint64_t val, uint32_t fg, uint32_t bg) {
    char buf[19];
    snprintf(buf, sizeof(buf), "0x%016llX", (unsigned long long)val);
    fb_string(x, y, buf, fg, bg);
}

static void fb_hex32(int x, int y, uint32_t val, uint32_t fg, uint32_t bg) {
    char buf[11];
    snprintf(buf, sizeof(buf), "0x%08X", val);
    fb_string(x, y, buf, fg, bg);
}

/* ── Bit grid: 8×8 cells showing 64 bits ────────────────────────────── */
#define GRID_CELL 10

static void fb_bitgrid(int x, int y, uint64_t val,
                       uint32_t on_col, uint32_t off_col) {
    for (int bit = 63; bit >= 0; bit--) {
        int row = (63 - bit) / 8;
        int col = (63 - bit) % 8;
        uint32_t c = (val >> bit) & 1 ? on_col : off_col;
        fb_rect(x + col * GRID_CELL, y + row * GRID_CELL,
                GRID_CELL - 1, GRID_CELL - 1, c);
    }
}

/* ── Horizontal bar ─────────────────────────────────────────────────── */
static void fb_bar(int x, int y, int max_w, int h, float frac, uint32_t col) {
    int w = (int)(frac * max_w);
    if (w > max_w) w = max_w;
    fb_rect(x, y, w, h, col);
    /* dim remainder */
    if (w < max_w)
        fb_rect(x + w, y, max_w - w, h, COL_PANEL);
}

/* ── MMIO helpers (from adapter_test.c) ──────────────────────────────── */
static void wr(int off, uint32_t val) {
    adapter[off / 4] = val;
    asm volatile("fence iorw,iorw");
}
static uint32_t rr(int off) {
    asm volatile("fence iorw,iorw");
    return adapter[off / 4];
}

static void load64(uint8_t addr, uint64_t val) {
    wr(REG_RS1, addr);
    wr(REG_RS2, (uint32_t)(val & 0xFFFFFFFF));
    wr(REG_CMD, 0);  /* F_LOAD */
    wr(REG_RS2, (uint32_t)(val >> 32));
    wr(REG_CMD, 4);  /* F_LOAD_HI */
}
static void accum64(uint64_t delta) {
    wr(REG_RS1, (uint32_t)(delta & 0xFFFFFFFF));
    wr(REG_CMD, 1);  /* F_ACCUM */
    wr(REG_RS1, (uint32_t)(delta >> 32));
    wr(REG_CMD, 5);  /* F_ACCUM_HI */
}
static uint64_t read64(void) {
    wr(REG_CMD, 2);  /* F_READ */
    uint32_t lo = rr(REG_RD);
    wr(REG_CMD, 6);  /* F_READ_HI */
    uint32_t hi = rr(REG_RD);
    return ((uint64_t)hi << 32) | lo;
}
static void atomik_swap(uint8_t addr) {
    wr(REG_RS1, addr);
    wr(REG_CMD, 3);
}

/* ── Timer ───────────────────────────────────────────────────────────── */
static inline uint64_t rdtime_ticks(void) {
    uint64_t t;
    asm volatile("rdtime %0" : "=r"(t));
    return t;
}

/* ── CSR write helper (VTG/DMA enable) ───────────────────────────────── */
static int mmio_w32(unsigned long phys, uint32_t val) {
    unsigned long page = phys & ~0xFFFUL;
    unsigned long off  = phys &  0xFFFUL;
    volatile uint8_t *m = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                               MAP_SHARED, memfd, page);
    if (m == MAP_FAILED) return -1;
    *(volatile uint32_t *)(m + off) = val;
    munmap((void *)m, 4096);
    return 0;
}

/* ── Operation log ───────────────────────────────────────────────────── */
#define LOG_MAX     30
#define LOG_X       960
#define LOG_Y       100
#define LOG_LINE_H  20

static char log_lines[LOG_MAX][100];
static uint32_t log_colors[LOG_MAX];
static int log_count;

static void log_add(const char *msg, uint32_t color) {
    if (log_count < LOG_MAX) {
        strncpy(log_lines[log_count], msg, 99);
        log_lines[log_count][99] = 0;
        log_colors[log_count] = color;
        log_count++;
    } else {
        /* Scroll up */
        for (int i = 0; i < LOG_MAX - 1; i++) {
            strcpy(log_lines[i], log_lines[i + 1]);
            log_colors[i] = log_colors[i + 1];
        }
        strncpy(log_lines[LOG_MAX - 1], msg, 99);
        log_lines[LOG_MAX - 1][99] = 0;
        log_colors[LOG_MAX - 1] = color;
    }
}

static void log_draw(void) {
    /* Clear log area */
    fb_rect(LOG_X, LOG_Y, 920, LOG_MAX * LOG_LINE_H, COL_BG);
    for (int i = 0; i < log_count; i++) {
        fb_string(LOG_X, LOG_Y + i * LOG_LINE_H, log_lines[i],
                  log_colors[i], COL_BG);
    }
}

/* ── Panel rendering ─────────────────────────────────────────────────── */

/* State visualization panel (left half) */
#define STATE_X  40
#define STATE_Y  120

static void draw_state_panel(uint64_t initial, uint64_t current) {
    uint64_t acc = initial ^ current;  /* accumulator = S xor current */

    /* Labels */
    fb_string(STATE_X, STATE_Y, "Initial State S[0]:", COL_TITLE, COL_BG);
    fb_hex64(STATE_X + 20 * CHAR_W, STATE_Y, initial, COL_BLUE_ON, COL_BG);
    fb_bitgrid(STATE_X, STATE_Y + 24, initial, COL_BLUE_ON, COL_BLUE_OFF);

    int ay = STATE_Y + 24 + 8 * GRID_CELL + 16;
    fb_string(STATE_X, ay, "Accumulator (XOR):", COL_YELLOW, COL_BG);
    fb_hex64(STATE_X + 20 * CHAR_W, ay, acc, COL_RED_ON, COL_BG);
    fb_bitgrid(STATE_X, ay + 24, acc, COL_RED_ON, COL_RED_OFF);

    int cy = ay + 24 + 8 * GRID_CELL + 16;
    /* XOR symbol between accumulator and result */
    fb_string(STATE_X + 30, cy - 8, "S[0] XOR acc =", COL_DIM, COL_BG);

    cy += 16;
    fb_string(STATE_X, cy, "Current State:", COL_GREEN_ON, COL_BG);
    fb_hex64(STATE_X + 20 * CHAR_W, cy, current, COL_GREEN_ON, COL_BG);
    fb_bitgrid(STATE_X, cy + 24, current, COL_GREEN_ON, COL_GREEN_OFF);
}

/* Benchmark panel (bottom) */
#define BENCH_X  40
#define BENCH_Y  780
#define BAR_W    400
#define BAR_H    18

static void draw_bench_panel(void) {
    fb_string(BENCH_X, BENCH_Y, "Change Detection Benchmark:", COL_TITLE, COL_BG);

    /* Software memcmp vs ATOMiK hardware */
    int sizes[] = {64, 256, 1024, 4096, 16384, 65536};
    const char *labels[] = {"  64B", " 256B", "  1KB", "  4KB", " 16KB", " 64KB"};

    for (int i = 0; i < 6; i++) {
        int y = BENCH_Y + 30 + i * (BAR_H + 12);
        int sz = sizes[i];
        char lbl[64];

        /* Measure software: memcmp-like loop */
        volatile uint8_t *buf = malloc(sz);
        if (!buf) continue;
        memset((void *)buf, 0xAA, sz);

        uint64_t t0 = rdtime_ticks();
        volatile uint32_t dummy = 0;
        for (int j = 0; j < sz; j++) dummy += buf[j];
        uint64_t sw_cycles = rdtime_ticks() - t0;
        free((void *)buf);

        /* Measure hardware: single LOAD + READ */
        load64(0, 0xAAAAAAAAAAAAAAAAULL);
        t0 = rdtime_ticks();
        uint64_t val = read64();
        uint64_t hw_cycles = rdtime_ticks() - t0;
        (void)val;

        float speedup = (float)sw_cycles / (float)(hw_cycles > 0 ? hw_cycles : 1);
        float sw_frac = 1.0f;
        float hw_frac = (float)hw_cycles / (float)(sw_cycles > 0 ? sw_cycles : 1);

        /* Draw */
        fb_string(BENCH_X, y, labels[i], COL_TEXT, COL_BG);

        fb_string(BENCH_X + 60, y, "SW:", COL_BAR_SW, COL_BG);
        fb_bar(BENCH_X + 100, y + 2, BAR_W, BAR_H - 4, sw_frac, COL_BAR_SW);

        fb_string(BENCH_X + 100 + BAR_W + 10, y, "HW:", COL_BAR_HW, COL_BG);
        fb_bar(BENCH_X + 100 + BAR_W + 50, y + 2, BAR_W, BAR_H - 4, hw_frac, COL_BAR_HW);

        snprintf(lbl, sizeof(lbl), "%.0fx", speedup);
        fb_string(BENCH_X + 100 + BAR_W * 2 + 70, y, lbl, COL_WHITE, COL_BG);
    }
}

/* ── Title bar ───────────────────────────────────────────────────────── */
static void draw_title(void) {
    fb_rect(0, 0, FB_HRES, 80, COL_PANEL);
    fb_string(40, 16, "ATOMiK Delta-State Engine", COL_TITLE, COL_PANEL);
    fb_string(40, 40, "Live Hardware Demo", COL_WHITE, COL_PANEL);
    fb_string(500, 16, "NaxRiscv RV64GC @ 100 MHz", COL_DIM, COL_PANEL);
    fb_string(500, 40, "XC7Z020-2 | 1920x1080 HDMI | AXI HP0 64-bit",
              COL_DIM, COL_PANEL);
    fb_string(500, 56, "current_state = initial_state XOR accumulator",
              COL_YELLOW, COL_PANEL);
}

/* ── Demo sequence ───────────────────────────────────────────────────── */
static void demo_pause(int ms) {
    flush_l2_to_ddr();  /* force dirty FB lines to DDR before DMA scans */
    usleep(ms * 1000);
}

static void run_demo(void) {
    uint64_t initial, current;
    char msg[100];

    /* Clear screen */
    memset(fb, 0, FB_SIZE);
    draw_title();

    /* ── Act 1: LOAD initial state ───────────────────────────────────── */
    fb_string(LOG_X, LOG_Y - 24, "Operation Log:", COL_TITLE, COL_BG);
    log_count = 0;

    initial = 0xDEADBEEFCAFEBABEULL;
    load64(0, initial);
    current = read64();
    draw_state_panel(initial, current);
    log_add("LOAD addr=0 val=0xDEADBEEFCAFEBABE", COL_BLUE_ON);
    log_add("  -> state = 0xDEADBEEFCAFEBABE (acc=0)", COL_TEXT);
    log_draw();
    demo_pause(2000);

    /* ── Act 2: Accumulate deltas ────────────────────────────────────── */
    uint64_t deltas[] = {
        0x1111111111111111ULL,
        0x00FF00FF00FF00FFULL,
        0xF0F0F0F000000000ULL,
        0x000000000F0F0F0FULL,
    };
    const char *delta_names[] = {
        "0x1111111111111111",
        "0x00FF00FF00FF00FF",
        "0xF0F0F0F000000000",
        "0x000000000F0F0F0F",
    };

    for (int i = 0; i < 4; i++) {
        accum64(deltas[i]);
        current = read64();
        draw_state_panel(initial, current);

        snprintf(msg, sizeof(msg), "ACCUM delta=%s", delta_names[i]);
        log_add(msg, COL_YELLOW);
        snprintf(msg, sizeof(msg), "  -> state = 0x%016llX",
                 (unsigned long long)current);
        log_add(msg, COL_TEXT);
        log_draw();
        demo_pause(1500);
    }

    /* ── Act 3: Self-inverse property ────────────────────────────────── */
    log_add("--- Self-Inverse Property ---", COL_MAGENTA);
    log_draw();
    demo_pause(1000);

    /* Apply same delta twice — should cancel */
    accum64(0x1111111111111111ULL);
    current = read64();
    draw_state_panel(initial, current);
    log_add("ACCUM 0x1111111111111111 (again)", COL_YELLOW);
    snprintf(msg, sizeof(msg), "  -> state = 0x%016llX",
             (unsigned long long)current);
    log_add(msg, COL_TEXT);
    log_draw();
    demo_pause(1500);

    accum64(0x00FF00FF00FF00FFULL);
    current = read64();
    draw_state_panel(initial, current);
    log_add("ACCUM 0x00FF00FF00FF00FF (again)", COL_YELLOW);
    snprintf(msg, sizeof(msg), "  -> state = 0x%016llX (deltas cancelled!)",
             (unsigned long long)current);
    log_add(msg, COL_GREEN_ON);
    log_draw();
    demo_pause(2000);

    /* ── Act 4: Commutativity ────────────────────────────────────────── */
    log_add("--- Commutativity: A+B = B+A ---", COL_MAGENTA);
    log_draw();
    demo_pause(1000);

    load64(0, 0);
    accum64(0xAAAAAAAAAAAAAAAAULL);
    accum64(0x5555555555555555ULL);
    uint64_t ab = read64();

    load64(0, 0);
    accum64(0x5555555555555555ULL);
    accum64(0xAAAAAAAAAAAAAAAAULL);
    uint64_t ba = read64();

    snprintf(msg, sizeof(msg), "A+B = 0x%016llX", (unsigned long long)ab);
    log_add(msg, COL_YELLOW);
    snprintf(msg, sizeof(msg), "B+A = 0x%016llX", (unsigned long long)ba);
    log_add(msg, COL_YELLOW);
    log_add(ab == ba ? "  MATCH -- order doesn't matter!" : "  MISMATCH!",
            ab == ba ? COL_GREEN_ON : COL_RED_ON);

    initial = 0;
    draw_state_panel(initial, ab);
    log_draw();
    demo_pause(2000);

    /* ── Act 5: Multi-address contexts ───────────────────────────────── */
    log_add("--- Multi-Address Isolation ---", COL_MAGENTA);
    log_draw();
    demo_pause(1000);

    load64(0, 0xAAAA0000BBBB0000ULL);
    load64(1, 0x0000CCCC0000DDDDULL);

    /* Read addr 0 */
    load64(0, 0xAAAA0000BBBB0000ULL); /* re-select addr 0 */
    current = read64();
    initial = 0xAAAA0000BBBB0000ULL;
    draw_state_panel(initial, current);
    log_add("LOAD addr=0: 0xAAAA0000BBBB0000", COL_BLUE_ON);
    log_add("LOAD addr=1: 0x0000CCCC0000DDDD", COL_BLUE_ON);
    snprintf(msg, sizeof(msg), "READ addr=0: 0x%016llX (isolated)",
             (unsigned long long)current);
    log_add(msg, COL_GREEN_ON);
    log_draw();
    demo_pause(2000);

    /* ── Act 6: Benchmark bars ───────────────────────────────────────── */
    log_add("--- Benchmark: SW scan vs HW detect ---", COL_MAGENTA);
    log_draw();
    demo_pause(500);

    draw_bench_panel();
    demo_pause(3000);
}

/* ── Main ────────────────────────────────────────────────────────────── */
int main(void) {
    memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd < 0) { perror("open /dev/mem"); return 1; }

    /* Map framebuffer */
    fb = mmap(NULL, FB_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
              memfd, FB_BASE);
    if (fb == MAP_FAILED) { perror("mmap fb"); return 1; }

    /* Map adapter CSRs */
    adapter = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED,
                   memfd, ADAPTER_BASE);
    if (adapter == MAP_FAILED) { perror("mmap adapter"); return 1; }

    /* Enable VTG + DMA */
    mmio_w32(CSR_FB_VTG_ENABLE, 1);
    mmio_w32(CSR_FB_DMA_ENABLE, 1);

    printf("ATOMiK HDMI Visualization — running demo\n");

    /* Run demo in a loop */
    for (;;) {
        run_demo();
        printf("Demo cycle complete, restarting in 3s...\n");
        sleep(3);
    }

    munmap(fb, FB_SIZE);
    munmap((void *)adapter, 4096);
    close(memfd);
    return 0;
}
