/* fb2png.c — capture /dev/fb0 to a PNG file.
 *
 * Standalone utility shipped alongside atomik_os. mmap's the framebuffer
 * (1920x1080 XRGB8888 by default), writes a PNG with NO external deps:
 * we emit a zlib stream of "stored" (uncompressed) deflate blocks plus
 * an Adler-32 checksum. This makes the output PNG ~3× larger than a
 * compressed one, but the binary cross-compiles against any libc and
 * doesn't need zlib on the board. Files are still readable by every
 * standard PNG decoder. */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define DEFAULT_W 1920
#define DEFAULT_H 1080
#define DEFAULT_BPP 4

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
    /* CRC over type + data */
    uint8_t *buf = malloc(4 + n);
    memcpy(buf, type, 4);
    if (data && n) memcpy(buf + 4, data, n);
    uint32_t c = crc(buf, 4 + n);
    free(buf);
    w_be32(f, c);
}

int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "/tmp/shot.png";
    int W = argc > 2 ? atoi(argv[2]) : DEFAULT_W;
    int H = argc > 3 ? atoi(argv[3]) : DEFAULT_H;
    int B = DEFAULT_BPP;

    int fb = open("/dev/fb0", O_RDONLY);
    if (fb < 0) { perror("open /dev/fb0"); return 1; }
    size_t bytes = (size_t)W * H * B;
    uint8_t *src = mmap(NULL, bytes, PROT_READ, MAP_SHARED, fb, 0);
    if (src == MAP_FAILED) { perror("mmap fb0"); return 1; }

    /* Build raw scanline buffer: PNG IDAT format = filter byte (0=None) +
     * RGB bytes per pixel. We convert XRGB little-endian (BGRA in memory)
     * to RGB, dropping alpha. */
    size_t row_bytes = 1 + (size_t)W * 3;
    uint8_t *raw = malloc(row_bytes * (size_t)H);
    if (!raw) { perror("malloc raw"); return 1; }
    for (int y = 0; y < H; y++) {
        uint8_t *r = raw + y * row_bytes;
        r[0] = 0;  /* filter: None */
        const uint8_t *s = src + (size_t)y * W * B;
        for (int x = 0; x < W; x++) {
            uint8_t b = s[x*4 + 0];
            uint8_t g = s[x*4 + 1];
            uint8_t rd= s[x*4 + 2];
            r[1 + x*3 + 0] = rd;
            r[1 + x*3 + 1] = g;
            r[1 + x*3 + 2] = b;
        }
    }

    /* "Compress" by wrapping the raw stream in a zlib container of stored
     * (BTYPE=00) deflate blocks. Each block carries up to 65535 bytes of
     * literal data preceded by 5-byte header (BFINAL flag + LEN + NLEN). */
    size_t raw_n = row_bytes * (size_t)H;
    /* Capacity: 2-byte zlib header + ceil(raw_n/65535)*5 + raw_n + 4 (Adler-32). */
    size_t blocks   = (raw_n + 65534) / 65535;
    if (blocks == 0) blocks = 1;
    size_t comp_cap = 2 + blocks * 5 + raw_n + 4;
    uint8_t *comp = malloc(comp_cap);
    if (!comp) { perror("malloc comp"); return 1; }
    uint8_t *o = comp;
    /* zlib header: CMF=0x78 (deflate, 32K), FLG=0x01 (no preset dict, fastest, fcheck so 0x7801 is divisible by 31). */
    *o++ = 0x78;
    *o++ = 0x01;
    size_t pos = 0;
    while (pos < raw_n) {
        size_t chunk = raw_n - pos;
        if (chunk > 65535) chunk = 65535;
        int last = (pos + chunk == raw_n);
        *o++ = last ? 0x01 : 0x00;       /* BFINAL bit + BTYPE=00 (stored) */
        *o++ = chunk & 0xff;
        *o++ = (chunk >> 8) & 0xff;
        *o++ = (~chunk) & 0xff;
        *o++ = ((~chunk) >> 8) & 0xff;
        memcpy(o, raw + pos, chunk);
        o   += chunk;
        pos += chunk;
    }
    /* Adler-32 over the raw uncompressed data */
    uint32_t s1 = 1, s2 = 0;
    for (size_t i = 0; i < raw_n; i++) {
        s1 = (s1 + raw[i]) % 65521;
        s2 = (s2 + s1)     % 65521;
    }
    uint32_t adler = (s2 << 16) | s1;
    *o++ = (adler >> 24) & 0xff;
    *o++ = (adler >> 16) & 0xff;
    *o++ = (adler >>  8) & 0xff;
    *o++ = (adler >>  0) & 0xff;
    size_t comp_len = (size_t)(o - comp);

    /* Write PNG */
    FILE *f = fopen(out, "wb");
    if (!f) { perror("fopen out"); return 1; }
    static const uint8_t SIG[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    fwrite(SIG, 1, 8, f);

    /* IHDR */
    uint8_t ihdr[13];
    ihdr[0] = (W >> 24); ihdr[1] = (W >> 16); ihdr[2] = (W >> 8); ihdr[3] = W;
    ihdr[4] = (H >> 24); ihdr[5] = (H >> 16); ihdr[6] = (H >> 8); ihdr[7] = H;
    ihdr[8]  = 8;   /* bit depth */
    ihdr[9]  = 2;   /* color type RGB */
    ihdr[10] = 0;   /* compression */
    ihdr[11] = 0;   /* filter */
    ihdr[12] = 0;   /* interlace */
    write_chunk(f, "IHDR", ihdr, 13);
    /* IDAT */
    write_chunk(f, "IDAT", comp, comp_len);
    /* IEND */
    write_chunk(f, "IEND", NULL, 0);
    fclose(f);

    free(comp); free(raw); munmap(src, bytes); close(fb);
    fprintf(stdout, "wrote %s (%dx%d)\n", out, W, H);
    return 0;
}
