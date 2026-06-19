/* png_write.c — dependency-free XRGB8888 -> PNG writer, shared by the board
 * framebuffer backend (fb.c), the host preview backend (fb_host.c), and the
 * in-OS auto-capture path (main.c).  Emits a zlib stream of "stored"
 * (uncompressed) deflate blocks + Adler-32 — no zlib dependency, readable by
 * every standard PNG decoder.  Channel extraction matches tools/fb2png.c:
 * little-endian 0x00RRGGBB in memory => byte0=B, byte1=G, byte2=R. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t s_crc_tab[256];
static int s_crc_init = 0;
static void crc_init(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? (0xedb88320u ^ (c >> 1)) : (c >> 1);
        s_crc_tab[n] = c;
    }
    s_crc_init = 1;
}
static uint32_t crc32_buf(const uint8_t *b, size_t n) {
    if (!s_crc_init) crc_init();
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = s_crc_tab[(c ^ b[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
static void be32(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v >> 24), (uint8_t)(v >> 16),
                     (uint8_t)(v >> 8), (uint8_t)v };
    fwrite(b, 1, 4, f);
}
static void chunk(FILE *f, const char *type, const uint8_t *data, size_t n) {
    be32(f, (uint32_t)n);
    fwrite(type, 1, 4, f);
    if (data && n) fwrite(data, 1, n, f);
    uint8_t *buf = malloc(4 + n);
    if (!buf) return;
    memcpy(buf, type, 4);
    if (data && n) memcpy(buf + 4, data, n);
    be32(f, crc32_buf(buf, 4 + n));
    free(buf);
}

/* Write a w*h XRGB8888 buffer to `path` as an RGB PNG.  Returns 0 on success. */
int png_write_xrgb(const char *path, const uint32_t *px, int w, int h) {
    const uint8_t *src = (const uint8_t *)px;
    size_t row_bytes = 1 + (size_t)w * 3;
    uint8_t *raw = malloc(row_bytes * (size_t)h);
    if (!raw) return -1;
    for (int y = 0; y < h; y++) {
        uint8_t *r = raw + (size_t)y * row_bytes;
        r[0] = 0;                                  /* filter: None */
        const uint8_t *s = src + (size_t)y * w * 4;
        for (int x = 0; x < w; x++) {
            uint8_t b = s[x*4 + 0], g = s[x*4 + 1], rd = s[x*4 + 2];
            r[1 + x*3 + 0] = rd; r[1 + x*3 + 1] = g; r[1 + x*3 + 2] = b;
        }
    }
    size_t raw_n  = row_bytes * (size_t)h;
    size_t blocks = (raw_n + 65534) / 65535; if (!blocks) blocks = 1;
    uint8_t *comp = malloc(2 + blocks * 5 + raw_n + 4);
    if (!comp) { free(raw); return -1; }
    uint8_t *o = comp;
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
    if (!f) { free(raw); free(comp); return -1; }
    static const uint8_t SIG[8] = { 137,80,78,71,13,10,26,10 };
    fwrite(SIG, 1, 8, f);
    uint8_t ihdr[13] = {
        (uint8_t)(w>>24),(uint8_t)(w>>16),(uint8_t)(w>>8),(uint8_t)w,
        (uint8_t)(h>>24),(uint8_t)(h>>16),(uint8_t)(h>>8),(uint8_t)h,
        8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", comp, comp_len);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw); free(comp);
    return 0;
}
