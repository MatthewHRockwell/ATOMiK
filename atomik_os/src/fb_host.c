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

/* --- minimal PNG writer (stored-deflate, no zlib), mirrors fb2png.c --- */
static uint32_t s_crc_tab[256];
static int s_crc_init = 0;
static void crc_init(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? (0xedb88320u ^ (c >> 1)) : (c >> 1);
        s_crc_tab[n] = c;
    }
    s_crc_init = 1;
}
static uint32_t crc32(const uint8_t *b, size_t n) {
    if (!s_crc_init) crc_init();
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = s_crc_tab[(c ^ b[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
static void be32(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v };
    fwrite(b, 1, 4, f);
}
static void chunk(FILE *f, const char *type, const uint8_t *data, size_t n) {
    be32(f, (uint32_t)n);
    fwrite(type, 1, 4, f);
    if (data && n) fwrite(data, 1, n, f);
    uint8_t *buf = malloc(4 + n);
    memcpy(buf, type, 4);
    if (data && n) memcpy(buf + 4, data, n);
    be32(f, crc32(buf, 4 + n));
    free(buf);
}

static void write_png(const char *path, const pixel_t *fb, int W, int H) {
    size_t row_bytes = 1 + (size_t)W * 3;
    uint8_t *raw = malloc(row_bytes * (size_t)H);
    if (!raw) return;
    const uint8_t *src = (const uint8_t *)fb;
    for (int y = 0; y < H; y++) {
        uint8_t *r = raw + (size_t)y * row_bytes;
        r[0] = 0;
        const uint8_t *s = src + (size_t)y * W * 4;
        for (int x = 0; x < W; x++) {
            uint8_t b = s[x*4 + 0], g = s[x*4 + 1], rd = s[x*4 + 2];
            r[1 + x*3 + 0] = rd; r[1 + x*3 + 1] = g; r[1 + x*3 + 2] = b;
        }
    }
    size_t raw_n = row_bytes * (size_t)H;
    size_t blocks = (raw_n + 65534) / 65535; if (!blocks) blocks = 1;
    uint8_t *comp = malloc(2 + blocks * 5 + raw_n + 4), *o = comp;
    *o++ = 0x78; *o++ = 0x01;
    size_t pos = 0;
    while (pos < raw_n) {
        size_t ck = raw_n - pos; if (ck > 65535) ck = 65535;
        int last = (pos + ck == raw_n);
        *o++ = last ? 1 : 0;
        *o++ = ck & 0xff; *o++ = (ck >> 8) & 0xff;
        *o++ = (~ck) & 0xff; *o++ = ((~ck) >> 8) & 0xff;
        memcpy(o, raw + pos, ck); o += ck; pos += ck;
    }
    uint32_t s1 = 1, s2 = 0;
    for (size_t i = 0; i < raw_n; i++) { s1 = (s1 + raw[i]) % 65521; s2 = (s2 + s1) % 65521; }
    uint32_t adler = (s2 << 16) | s1;
    *o++ = (adler >> 24) & 0xff; *o++ = (adler >> 16) & 0xff;
    *o++ = (adler >> 8) & 0xff; *o++ = adler & 0xff;
    size_t comp_len = (size_t)(o - comp);

    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen shot"); free(raw); free(comp); return; }
    static const uint8_t SIG[8] = { 137,80,78,71,13,10,26,10 };
    fwrite(SIG, 1, 8, f);
    uint8_t ihdr[13] = {
        (uint8_t)(W>>24),(uint8_t)(W>>16),(uint8_t)(W>>8),(uint8_t)W,
        (uint8_t)(H>>24),(uint8_t)(H>>16),(uint8_t)(H>>8),(uint8_t)H,
        8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", comp, comp_len);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw); free(comp);
    fprintf(stderr, "[host] wrote %s (%dx%d)\n", path, W, H);
}

void fb_present(void) {
    const char *path = getenv("ATOMIK_SHOT");
    if (!path) path = "/tmp/atomik_host_shot.png";
    write_png(path, s_back, FB_W, FB_H);
}
