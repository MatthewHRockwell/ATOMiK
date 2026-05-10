/* font.c — minimal embedded 8x16 VGA font + v0.38-B glyph cache.
 *
 * Original behavior: draw_glyph walked the 8-bit-per-row glyph table,
 * extracted each bit, and called draw_pixel up to scale^2 times per
 * set bit.  For a string of 30 chars at scale 1, that's ~2,000+
 * draw_pixel calls — each with bounds checks + index multiply +
 * function-call overhead.  Text is everywhere in the UI; this was
 * the single largest hot path for text-heavy frames.
 *
 * v0.38-B (per ChatGPT 2026-05-09 perf directive): pre-render every
 * printable ASCII char at scales 1/2/3 into a byte-per-pixel mask
 * once at boot.  Drawing becomes a tight loop over the mask with
 * direct fb writes — no bit extraction, no per-pixel bounds check
 * (clipped at the rect level), no nested scale loop.  Glyph counts
 * are published into the metric provider so the "X% glyph cache hit"
 * win is part of the live VISUAL story.
 *
 * Memory: 8x16 + 16x32 + 24x48 = 128 + 512 + 1152 = 1792 bytes per
 * char × 95 printables = ~170 KB.  Statically allocated so init is
 * free, no malloc, no fragmentation. */
#include "atomik_os.h"
#include <string.h>

/* Pull in the existing glyph table sitting next to the ps_loader sources. */
#include "../../hardware/zynq/ps_loader/font_8x16.h"

extern pixel_t *fb_back(void);

#define GLYPH_FIRST  32
#define GLYPH_LAST   126
#define GLYPH_COUNT  (GLYPH_LAST - GLYPH_FIRST + 1)

#define MAX_CACHED_SCALE 3

/* Statically-sized cache.  s_mask[scale-1][char-32][row*gw + col]. */
static uint8_t s_mask_s1[GLYPH_COUNT][8  * 16];
static uint8_t s_mask_s2[GLYPH_COUNT][16 * 32];
static uint8_t s_mask_s3[GLYPH_COUNT][24 * 48];

static int s_cache_built = 0;

/* Per-frame counters published to metric provider. */
static unsigned long s_glyphs_drawn   = 0;
static unsigned long s_glyphs_cached  = 0;
static unsigned long s_glyphs_clipped = 0;   /* fell off-screen, partial draw */

unsigned long font_glyphs_drawn(void)   { return s_glyphs_drawn; }
unsigned long font_glyphs_cached(void)  { return s_glyphs_cached; }
unsigned long font_glyphs_clipped(void) { return s_glyphs_clipped; }

static void build_one(int scale, unsigned char c, uint8_t *out) {
    if (c < GLYPH_FIRST || c > GLYPH_LAST) c = '?';
    const unsigned char *g = &font_8x16[(unsigned)c * 16];
    int gw = 8  * scale;
    int gh = 16 * scale;
    memset(out, 0, (size_t)gw * gh);
    for (int row = 0; row < 16; row++) {
        unsigned char bits = g[row];
        for (int col = 0; col < 8; col++) {
            if (!(bits & (0x80 >> col))) continue;
            for (int sy = 0; sy < scale; sy++) {
                int py = row * scale + sy;
                uint8_t *r = out + py * gw;
                for (int sx = 0; sx < scale; sx++) {
                    r[col * scale + sx] = 1;
                }
            }
        }
    }
}

int font_init(void) {
    if (s_cache_built) return 0;
    for (int c = GLYPH_FIRST; c <= GLYPH_LAST; c++) {
        int idx = c - GLYPH_FIRST;
        build_one(1, (unsigned char)c, s_mask_s1[idx]);
        build_one(2, (unsigned char)c, s_mask_s2[idx]);
        build_one(3, (unsigned char)c, s_mask_s3[idx]);
    }
    s_cache_built = 1;
    return 0;
}

int text_height(int scale) { return 16 * scale; }

int text_width(const char *s, int scale) {
    int w = 0;
    while (*s++) w += 8 * scale;
    return w;
}

/* Resolve cache slot for (scale, char).  Returns NULL for unsupported
 * scales — caller falls back to slow path. */
static const uint8_t *cached_mask(int scale, unsigned char c) {
    if (!s_cache_built) return NULL;
    if (c < GLYPH_FIRST || c > GLYPH_LAST) c = '?';
    int idx = c - GLYPH_FIRST;
    switch (scale) {
    case 1: return s_mask_s1[idx];
    case 2: return s_mask_s2[idx];
    case 3: return s_mask_s3[idx];
    default: return NULL;
    }
}

/* Slow path: original per-pixel draw via draw_pixel.  Used for scales
 * outside the cache range OR when the glyph rect is partially off
 * screen (the fast path's clip check would corrupt the mask walk). */
static void draw_glyph_slow(int x, int y, unsigned char c, int scale,
                            pixel_t color) {
    if (c < 32 || c > 126) c = '?';
    const unsigned char *g = &font_8x16[(unsigned)c * 16];
    for (int row = 0; row < 16; row++) {
        unsigned char bits = g[row];
        for (int col = 0; col < 8; col++) {
            if (!(bits & (0x80 >> col))) continue;
            int px = x + col * scale;
            int py = y + row * scale;
            for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++)
                    draw_pixel(px + sx, py + sy, color);
        }
    }
}

static void draw_glyph_fast(int x, int y, unsigned char c, int scale,
                            pixel_t color, const uint8_t *mask) {
    int gw = 8  * scale;
    int gh = 16 * scale;
    /* Trivial reject: entirely off-screen. */
    if (x + gw <= 0 || y + gh <= 0 || x >= FB_W || y >= FB_H) {
        s_glyphs_clipped++;
        return;
    }
    /* If the rect is partially off-screen, fall back to the per-pixel
     * path so we don't write outside the framebuffer.  Common case
     * (text fully on screen) hits the tight inner loop. */
    if (x < 0 || y < 0 || x + gw > FB_W || y + gh > FB_H) {
        s_glyphs_clipped++;
        draw_glyph_slow(x, y, c, scale, color);
        return;
    }
    pixel_t *bb = fb_back();
    for (int sy = 0; sy < gh; sy++) {
        pixel_t        *row  = bb   + (y + sy) * FB_W + x;
        const uint8_t  *mrow = mask + sy * gw;
        for (int sx = 0; sx < gw; sx++) {
            if (mrow[sx]) row[sx] = color;
        }
    }
}

static void draw_glyph(int x, int y, unsigned char c, int scale,
                       pixel_t color) {
    s_glyphs_drawn++;
    const uint8_t *mask = cached_mask(scale, c);
    if (mask) {
        s_glyphs_cached++;
        draw_glyph_fast(x, y, c, scale, color, mask);
    } else {
        draw_glyph_slow(x, y, c, scale, color);
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
