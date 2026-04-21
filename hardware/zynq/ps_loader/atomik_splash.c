/*
 * ATOMiK Boot Splash — draws branding to 1920×1080 HDMI framebuffer.
 *
 * Called early in the init sequence to display an ATOMiK splash screen
 * before the Linux console takes over. Enables VTG+DMA if not already on.
 *
 * Build:
 *     riscv64-linux-gnu-gcc -O2 -static atomik_splash.c -o atomik_splash
 *
 * Usage:
 *     atomik_splash          # draw splash and exit
 *     atomik_splash --hold   # draw splash and wait for keypress
 * ========================================================================= */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define FB_BASE     0x48000000UL
#define FB_HRES     1920
#define FB_VRES     1080
#define FB_BPP      4
#define FB_SIZE     (FB_HRES * FB_VRES * FB_BPP)

#define CSR_FB_DMA_ENABLE   0xF0002004UL
#define CSR_FB_VTG_ENABLE   0xF0002800UL

/* Colors */
#define COL_BG       0x00080C14   /* very dark navy */
#define COL_ACCENT   0x004488FF   /* ATOMiK blue */
#define COL_WHITE    0x00FFFFFF
#define COL_GRAY     0x00A0A0A0
#define COL_DIM      0x00404050
#define COL_GREEN    0x0044FF88
#define COL_DARK_ACC 0x00102040   /* dark blue accent */

#include "font_8x16.h"
#define CHAR_W 8
#define CHAR_H 16

static uint32_t *fb;
static int memfd;

/* L2 flush via eviction (NaxRiscv has no Zicbom) */
static volatile uint8_t flush_scratch[128 * 1024] __attribute__((aligned(64)));
static void flush_l2(void) {
    for (int i = 0; i < (int)sizeof(flush_scratch); i += 64)
        flush_scratch[i] = 0;
    asm volatile("fence iorw,iorw");
}

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

/* Draw string centered horizontally */
static void fb_center(int y, const char *s, uint32_t fg, uint32_t bg) {
    int len = strlen(s);
    int x = (FB_HRES - len * CHAR_W) / 2;
    fb_string(x, y, s, fg, bg);
}

/* Draw a large character (scaled 4x) */
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

static void fb_center_big(int y, const char *s, uint32_t fg, uint32_t bg, int scale) {
    int len = strlen(s);
    int x = (FB_HRES - len * CHAR_W * scale) / 2;
    fb_string_big(x, y, s, fg, bg, scale);
}

/* CSR write */
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

/* ── Splash screen layout ───────────────────────────────────────────── */
static void draw_splash(void) {
    /* Background */
    memset(fb, 0, FB_SIZE);

    /* Subtle horizontal gradient stripe behind the title */
    for (int y = 340; y < 500; y++) {
        for (int x = 0; x < FB_HRES; x++) {
            /* Fade in from edges */
            int dist = abs(x - FB_HRES / 2);
            int alpha = 255 - (dist * 255 / (FB_HRES / 2));
            if (alpha < 0) alpha = 0;
            int r = (0x10 * alpha) / 255;
            int g = (0x20 * alpha) / 255;
            int b = (0x40 * alpha) / 255;
            fb[y * FB_HRES + x] = (r << 16) | (g << 8) | b;
        }
    }

    /* ── Title: ATOMiK (large, 4x scale) ──────────────────────────── */
    fb_center_big(370, "ATOMiK", COL_ACCENT, 0 /* transparent - use existing bg */, 5);

    /* Actually, drawing with bg=0 will overwrite the gradient with black.
     * Let's draw text on top by only writing foreground pixels. */

    /* Redraw title properly — only set fg pixels, skip bg */
    {
        const char *title = "ATOMiK";
        int scale = 5;
        int len = strlen(title);
        int x0 = (FB_HRES - len * CHAR_W * scale) / 2;
        int y0 = 370;
        for (int ci = 0; ci < len; ci++) {
            const uint8_t *glyph = &font_8x16[(unsigned char)title[ci] * 16];
            for (int row = 0; row < 16; row++)
                for (int col = 0; col < 8; col++)
                    if (glyph[row] & (0x80 >> col))
                        fb_rect(x0 + ci * CHAR_W * scale + col * scale,
                                y0 + row * scale, scale, scale, COL_WHITE);
        }
    }

    /* Tagline */
    fb_center(470, "Delta-State Compute Architecture", COL_ACCENT, COL_BG);

    /* Horizontal rule */
    fb_rect(FB_HRES / 2 - 300, 500, 600, 2, COL_DIM);

    /* System info */
    fb_center(530, "NaxRiscv RV64GC | 100 MHz | XC7Z020-2", COL_GRAY, COL_BG);
    fb_center(555, "Linux 6.9 | Ubuntu 24.04 | 1920x1080 HDMI", COL_GRAY, COL_BG);

    /* Core formula */
    fb_center(600, "current_state = initial_state XOR accumulator", COL_GREEN, COL_BG);

    /* Bottom credits */
    fb_center(700, "Hardware-validated delta-state algebra", COL_DIM, COL_BG);
    fb_center(725, "108 Lean4 theorems | 353 SDK tests | 80/80 hardware tests", COL_DIM, COL_BG);

    flush_l2();
}

int main(int argc, char **argv) {
    int hold = (argc > 1 && strcmp(argv[1], "--hold") == 0);

    memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd < 0) { perror("open /dev/mem"); return 1; }

    fb = mmap(NULL, FB_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
              memfd, FB_BASE);
    if (fb == MAP_FAILED) { perror("mmap fb"); return 1; }

    /* Enable scan-out */
    mmio_w32(CSR_FB_VTG_ENABLE, 1);
    mmio_w32(CSR_FB_DMA_ENABLE, 1);

    draw_splash();

    if (hold) {
        printf("ATOMiK splash displayed. Press Enter to continue boot...\n");
        getchar();
    }

    munmap(fb, FB_SIZE);
    close(memfd);
    return 0;
}
