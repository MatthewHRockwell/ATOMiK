/* fbcrop.c — capture a SUB-RECTANGLE of /dev/fb0 to a PNG.
 *
 * Same dependency-free stored-deflate PNG encoder as fb2png, but dumps only
 * [x0,y0,w,h] so the output is tiny and survives a degraded UART link (a small
 * mostly-dark crop gzips to a few tens of KB).  Used to prove Atom's float bob
 * when the full 1920x1080 frame won't transfer.
 *
 *   fbcrop <out.png> <x0> <y0> <w> <h>
 *   riscv64-linux-gnu-gcc -O2 fbcrop.c -o fbcrop
 */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define FB_W 1920
#define FB_H 1080
#define BPP  4

static uint32_t crc_table[256];
static int crc_table_init = 0;
static void make_crc_table(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? (0xedb88320U ^ (c >> 1)) : (c >> 1);
        crc_table[n] = c;
    }
    crc_table_init = 1;
}
static uint32_t crc(const uint8_t *buf, size_t n) {
    if (!crc_table_init) make_crc_table();
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
static void w_be32(FILE *f, uint32_t v) {
    uint8_t b[4] = { v >> 24, v >> 16, v >> 8, v };
    fwrite(b, 1, 4, f);
}
static void write_chunk(FILE *f, const char *type, const uint8_t *data, size_t n) {
    w_be32(f, (uint32_t)n);
    fwrite(type, 1, 4, f);
    if (data && n) fwrite(data, 1, n, f);
    uint8_t *buf = malloc(4 + n);
    memcpy(buf, type, 4);
    if (data && n) memcpy(buf + 4, data, n);
    uint32_t c = crc(buf, 4 + n);
    free(buf);
    w_be32(f, c);
}

int main(int argc, char **argv) {
    if (argc < 6) { fprintf(stderr, "usage: %s out x0 y0 w h\n", argv[0]); return 2; }
    const char *out = argv[1];
    int x0 = atoi(argv[2]), y0 = atoi(argv[3]);
    int W  = atoi(argv[4]), H  = atoi(argv[5]);
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x0 + W > FB_W) W = FB_W - x0;
    if (y0 + H > FB_H) H = FB_H - y0;
    if (W <= 0 || H <= 0) { fprintf(stderr, "empty rect\n"); return 2; }

    int fb = open("/dev/fb0", O_RDONLY);
    if (fb < 0) { perror("open /dev/fb0"); return 1; }
    size_t bytes = (size_t)FB_W * FB_H * BPP;
    uint8_t *src = mmap(NULL, bytes, PROT_READ, MAP_SHARED, fb, 0);
    if (src == MAP_FAILED) { perror("mmap fb0"); return 1; }

    size_t row_bytes = 1 + (size_t)W * 3;
    uint8_t *raw = malloc(row_bytes * (size_t)H);
    if (!raw) { perror("malloc raw"); return 1; }
    for (int y = 0; y < H; y++) {
        uint8_t *r = raw + (size_t)y * row_bytes;
        r[0] = 0;  /* filter: None */
        const uint8_t *s = src + (size_t)(y0 + y) * FB_W * BPP + (size_t)x0 * BPP;
        for (int x = 0; x < W; x++) {
            uint8_t b = s[x*4 + 0], g = s[x*4 + 1], rd = s[x*4 + 2];
            r[1 + x*3 + 0] = rd;
            r[1 + x*3 + 1] = g;
            r[1 + x*3 + 2] = b;
        }
    }

    size_t raw_n = row_bytes * (size_t)H;
    size_t blocks = (raw_n + 65534) / 65535; if (blocks == 0) blocks = 1;
    uint8_t *comp = malloc(2 + blocks * 5 + raw_n + 4);
    if (!comp) { perror("malloc comp"); return 1; }
    uint8_t *o = comp;
    *o++ = 0x78; *o++ = 0x01;
    size_t pos = 0;
    while (pos < raw_n) {
        size_t chunk = raw_n - pos; if (chunk > 65535) chunk = 65535;
        int last = (pos + chunk == raw_n);
        *o++ = last ? 0x01 : 0x00;
        *o++ = chunk & 0xff; *o++ = (chunk >> 8) & 0xff;
        *o++ = (~chunk) & 0xff; *o++ = ((~chunk) >> 8) & 0xff;
        memcpy(o, raw + pos, chunk); o += chunk; pos += chunk;
    }
    uint32_t s1 = 1, s2 = 0;
    for (size_t i = 0; i < raw_n; i++) { s1 = (s1 + raw[i]) % 65521; s2 = (s2 + s1) % 65521; }
    uint32_t adler = (s2 << 16) | s1;
    *o++ = (adler >> 24) & 0xff; *o++ = (adler >> 16) & 0xff;
    *o++ = (adler >> 8) & 0xff;  *o++ = (adler) & 0xff;
    size_t comp_len = (size_t)(o - comp);

    FILE *f = fopen(out, "wb");
    if (!f) { perror("fopen out"); return 1; }
    static const uint8_t SIG[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    fwrite(SIG, 1, 8, f);
    uint8_t ihdr[13];
    ihdr[0]=(W>>24); ihdr[1]=(W>>16); ihdr[2]=(W>>8); ihdr[3]=W;
    ihdr[4]=(H>>24); ihdr[5]=(H>>16); ihdr[6]=(H>>8); ihdr[7]=H;
    ihdr[8]=8; ihdr[9]=2; ihdr[10]=0; ihdr[11]=0; ihdr[12]=0;
    write_chunk(f, "IHDR", ihdr, 13);
    write_chunk(f, "IDAT", comp, comp_len);
    write_chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(comp); free(raw); munmap(src, bytes); close(fb);
    fprintf(stdout, "wrote %s (%dx%d)\n", out, W, H);
    return 0;
}
