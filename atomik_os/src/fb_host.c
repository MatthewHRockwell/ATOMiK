/* fb_host.c — HOST (laptop) framebuffer backend for the atomik_os preview
 * build.  Compiled INSTEAD of fb.c when -DATOMIK_HOST is set (see Makefile
 * `host` target).  All UI drawing goes into a heap back buffer exactly as on
 * the board; the only difference is fb_present() writes a PNG of the frame
 * instead of memcpy-ing it to /dev/fb0.  This lets us render the REAL UI code
 * to an image on the laptop in seconds, with no board.
 *
 * Honest-data note: a host preview cannot read the ATOMiK adapter, so any
 * on-screen numbers come from scenario/seed paths — this build is for LAYOUT
 * and VISUAL verification only.  Investor/proof captures still come from a
 * real board fb2png (see feedback_screenshot_rb_swap, deploy_screenshot_contract).
 *
 * PNG path: $ATOMIK_SHOT or /tmp/atomik_host_shot.png.  Uses the same
 * dependency-free stored-deflate PNG writer as tools/fb2png.c, and the same
 * little-endian XRGB8888 -> RGB channel extraction (byte0=B, byte1=G, byte2=R)
 * so the preview's colors match the board exactly. */
#include "atomik_os.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pixel_t *s_back = NULL;

pixel_t *fb_back(void) { return s_back; }

int fb_open(void) {
    s_back = aligned_alloc(64, FB_BYTES);
    if (!s_back) { perror("alloc back"); return -1; }
    memset(s_back, 0, FB_BYTES);
    return 0;
}

void fb_close(void) { free(s_back); s_back = NULL; }

void fb_clear(pixel_t color) {
    for (size_t i = 0; i < (size_t)FB_W * FB_H; i++) s_back[i] = color;
}

void fb_enable_scanout(int enable) { (void)enable; }   /* no MMIO on host */

/* PNG writing is shared with the board backend via png_write.c. */
void fb_present(void) {
    const char *path = getenv("ATOMIK_SHOT");
    if (!path) path = "/tmp/atomik_host_shot.png";
    png_write_xrgb(path, (const uint32_t *)s_back, FB_W, FB_H);
    fprintf(stderr, "[host] wrote %s (%dx%d)\n", path, FB_W, FB_H);
}

/* In-OS auto-capture parity on host (writes wherever requested). */
void fb_write_png(const char *path) {
    if (s_back) png_write_xrgb(path, (const uint32_t *)s_back, FB_W, FB_H);
}
