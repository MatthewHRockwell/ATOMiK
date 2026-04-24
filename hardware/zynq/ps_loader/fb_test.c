/*
 * Phase 9.2 framebuffer pixel test — runs on Linux on NaxRiscv.
 *
 * Writes a pattern directly into the framebuffer via /dev/mem. Also enables
 * the VideoTimingGenerator and framebuffer DMA via CSR writes — they default
 * to disabled so the DMA master stays silent during BIOS/kernel boot.
 *
 * Framebuffer:        NaxRiscv 0x48000000, 640x480, XRGB8888 (1.2 MB)
 * Framebuffer DMA CSR: 0xF000200C (base:F0002000 + offset 0x0C) -- enable
 * VTG enable CSR:      0xF0002800 -- enable
 *
 * Build (host cross):
 *     riscv64-linux-gnu-gcc -O2 -static fb_test.c -o fb_test
 *
 * Usage:
 *     fb_test red        # solid red
 *     fb_test green      # solid green
 *     fb_test blue       # solid blue
 *     fb_test white      # solid white
 *     fb_test black      # solid black
 *     fb_test checker    # 32-pixel checkerboard
 *     fb_test hbars      # R/G/B/W vertical bars
 *     fb_test gradient   # horizontal red gradient
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

/* LiteX CSR addresses (from csr.csv of the fb bitstream) */
#define CSR_FB_DMA_BASE     0xF0004800UL    /* 32-bit: base address */
#define CSR_FB_DMA_ENABLE   0xF0004804UL    /* 1: enable fetch */
#define CSR_FB_VTG_ENABLE   0xF0005000UL    /* 1: enable video timing generator */
/* CSR page is one 4 KiB region starting at 0xF0000000 */
#define CSR_PAGE_BASE       0xF0000000UL
#define CSR_PAGE_SIZE       0x00100000UL

/* XRGB8888: 0x00RRGGBB. With endianness="big" in WishboneDMAReader (no
 * byte-swap), the 32-bit word passes through unchanged: R at bits[16:24],
 * G at bits[8:16], B at bits[0:8]. Pixel unpack in wb_framebuffer.py
 * maps these to the PHY's r/g/b channels. */
static uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void fill_solid(uint32_t *fb, uint32_t px) {
    for (long i = 0; i < FB_HRES * FB_VRES; i++) fb[i] = px;
}

static void fill_checker(uint32_t *fb) {
    for (int y = 0; y < FB_VRES; y++) {
        for (int x = 0; x < FB_HRES; x++) {
            int cx = (x >> 5) & 1;
            int cy = (y >> 5) & 1;
            fb[y * FB_HRES + x] = (cx ^ cy) ? rgb(255,255,255) : rgb(0,0,0);
        }
    }
}

static void fill_hbars(uint32_t *fb) {
    const uint32_t colors[4] = {
        rgb(255, 0, 0), rgb(0, 255, 0), rgb(0, 0, 255), rgb(255, 255, 255),
    };
    for (int y = 0; y < FB_VRES; y++) {
        for (int x = 0; x < FB_HRES; x++) {
            fb[y * FB_HRES + x] = colors[(x * 4) / FB_HRES];
        }
    }
}

static void fill_gradient(uint32_t *fb) {
    for (int y = 0; y < FB_VRES; y++) {
        for (int x = 0; x < FB_HRES; x++) {
            uint8_t v = (uint8_t)(x * 255 / (FB_HRES - 1));
            fb[y * FB_HRES + x] = rgb(v, 0, 0);
        }
    }
}

/* Write a 32-bit word to a physical MMIO address via /dev/mem. */
static int mmio_w32(int memfd, unsigned long phys, uint32_t val) {
    unsigned long page = phys & ~0xFFFUL;
    unsigned long off  = phys &  0xFFFUL;
    volatile uint8_t *m = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED,
                               memfd, page);
    if (m == MAP_FAILED) { perror("mmap mmio"); return -1; }
    *(volatile uint32_t *)(m + off) = val;
    munmap((void *)m, 4096);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s {red|green|blue|white|black|checker|hbars|gradient|disable}\n", argv[0]);
        return 2;
    }

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open /dev/mem"); return 1; }

    /* Handle "disable" specially — turn off VTG+DMA without writing pixels. */
    if (!strcmp(argv[1], "disable")) {
        mmio_w32(fd, CSR_FB_DMA_ENABLE, 0);
        mmio_w32(fd, CSR_FB_VTG_ENABLE, 0);
        close(fd);
        printf("video DMA + VTG disabled\n");
        return 0;
    }

    uint32_t *fb = mmap(NULL, FB_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fd, FB_BASE);
    if (fb == MAP_FAILED) { perror("mmap fb"); return 1; }

    const char *pat = argv[1];
    if      (!strcmp(pat, "red"))      fill_solid(fb, rgb(255, 0, 0));
    else if (!strcmp(pat, "green"))    fill_solid(fb, rgb(0, 255, 0));
    else if (!strcmp(pat, "blue"))     fill_solid(fb, rgb(0, 0, 255));
    else if (!strcmp(pat, "white"))    fill_solid(fb, rgb(255, 255, 255));
    else if (!strcmp(pat, "black"))    fill_solid(fb, 0);
    else if (!strcmp(pat, "checker"))  fill_checker(fb);
    else if (!strcmp(pat, "hbars"))    fill_hbars(fb);
    else if (!strcmp(pat, "gradient")) fill_gradient(fb);
    else {
        fprintf(stderr, "unknown pattern: %s\n", pat);
        return 2;
    }

    munmap(fb, FB_SIZE);

    /* Enable scan-out AFTER the frame has been written so the monitor does
     * not flicker through uninitialised memory. */
    mmio_w32(fd, CSR_FB_VTG_ENABLE, 1);
    mmio_w32(fd, CSR_FB_DMA_ENABLE, 1);

    close(fd);
    printf("wrote %s pattern to FB @ 0x%08lx (%d x %d, %d bpp), enabled scan-out\n",
           pat, FB_BASE, FB_HRES, FB_VRES, FB_BPP * 8);
    return 0;
}
