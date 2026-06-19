/* fb.c — /dev/fb0 mmap, double-buffered software compositor. */
#include "atomik_os.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define LITEX_FB_DMA_ENABLE  0xF0002004UL
#define LITEX_FB_VTG_ENABLE  0xF0002800UL

static int        s_fd        = -1;
static int        s_mem_fd    = -1;
static pixel_t   *s_fb_map    = NULL;   /* /dev/fb0 mmap (presented surface) */
static pixel_t   *s_back      = NULL;   /* heap-allocated back buffer */

/* Tiny /dev/mem MMIO helper for the LiteX VTG/DMA enable CSRs. */
static int mmio_w32(unsigned long phys, uint32_t val) {
    if (s_mem_fd < 0) {
        s_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (s_mem_fd < 0) return -1;
    }
    unsigned long page = phys & ~0xFFFUL;
    unsigned long off  = phys &  0xFFFUL;
    volatile uint8_t *m = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED,
                               s_mem_fd, page);
    if (m == MAP_FAILED) return -1;
    *(volatile uint32_t *)(m + off) = val;
    munmap((void *)m, 4096);
    return 0;
}

int fb_open(void) {
    s_fd = open("/dev/fb0", O_RDWR);
    if (s_fd < 0) { perror("open /dev/fb0"); return -1; }
    s_fb_map = mmap(NULL, FB_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED,
                    s_fd, 0);
    if (s_fb_map == MAP_FAILED) { perror("mmap fb0"); return -1; }
    s_back = aligned_alloc(64, FB_BYTES);
    if (!s_back) { perror("alloc back"); return -1; }
    return 0;
}

void fb_close(void) {
    if (s_fb_map && s_fb_map != MAP_FAILED) munmap(s_fb_map, FB_BYTES);
    if (s_fd >= 0) close(s_fd);
    if (s_mem_fd >= 0) close(s_mem_fd);
    free(s_back);
    s_fb_map = NULL; s_back = NULL; s_fd = -1; s_mem_fd = -1;
}

pixel_t *fb_back(void) { return s_back; }

void fb_present(void) {
    /* Single 8.3 MiB memcpy. The simple-framebuffer driver handles tearing
     * at vsync inside the kernel via simplefb_pan; we just copy. */
    memcpy(s_fb_map, s_back, FB_BYTES);
}

void fb_clear(pixel_t color) {
    pixel_t *p = s_back;
    for (size_t i = 0; i < (size_t)FB_W * FB_H; i++) p[i] = color;
}

/* v0.40 in-OS auto-capture: write the current back buffer to a PNG.  Lets
 * atomik_os snapshot its own screen WITHOUT any host/UART command (no console
 * input bleed, no shell during render). */
void fb_write_png(const char *path) {
    if (s_back) png_write_xrgb(path, (const uint32_t *)s_back, FB_W, FB_H);
}

void fb_enable_scanout(int enable) {
    /* DMA must be enabled BEFORE VTG, disabled AFTER VTG, to avoid showing
     * a torn first frame. */
    uint32_t v = enable ? 1 : 0;
    if (enable) {
        mmio_w32(LITEX_FB_DMA_ENABLE, v);
        mmio_w32(LITEX_FB_VTG_ENABLE, v);
    } else {
        mmio_w32(LITEX_FB_VTG_ENABLE, 0);
        mmio_w32(LITEX_FB_DMA_ENABLE, 0);
    }
}
