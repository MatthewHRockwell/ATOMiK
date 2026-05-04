/* font.c — minimal embedded 8x16 VGA font, scaled with simple AA so larger
 * sizes look acceptable for headlines on a 1080p display. The glyph table is
 * a compact pre-baked subset of standard CP437; ASCII printables only.
 *
 * NOTE: this file pulls in the same font_8x16.h that the legacy atomik_live
 * uses, so we get parity on character coverage with the existing demo
 * without re-licensing or re-typing 4 KB of glyph data. */
#include "atomik_os.h"

/* Pull in the existing glyph table sitting next to the ps_loader sources. */
#include "../../hardware/zynq/ps_loader/font_8x16.h"

int font_init(void) { return 0; }
int text_height(int scale) { return 16 * scale; }

int text_width(const char *s, int scale) {
    int w = 0;
    while (*s++) w += 8 * scale;
    return w;
}

static void draw_glyph(int x, int y, unsigned char c, int scale, pixel_t color) {
    if (c < 32 || c > 126) c = '?';
    /* Table is a flat 4096-byte array: 16 rows per glyph, 256 glyphs. */
    const unsigned char *g = &font_8x16[(unsigned)c * 16];
    for (int row = 0; row < 16; row++) {
        unsigned char bits = g[row];
        for (int col = 0; col < 8; col++) {
            if (!(bits & (0x80 >> col))) continue;
            /* Scaled pixel block. AA edges by walking sub-pixel rows when
             * scale >= 2 — for now we just upscale solid; AA is a v0.1
             * improvement once we move to a real font rasterizer. */
            int px = x + col * scale;
            int py = y + row * scale;
            for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++)
                    draw_pixel(px + sx, py + sy, color);
        }
    }
}

void draw_text(int x, int y, const char *s, int scale, pixel_t color) {
    int cx = x;
    while (*s) {
        draw_glyph(cx, y, (unsigned char)*s, scale, color);
        cx += 8 * scale;
        s++;
    }
}
