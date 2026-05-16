/* font_aa.c — v0.38-K anti-aliased bitmap font atlas loader.
 *
 * Reads .atomik_font files produced by tools/font_pack.py (format
 * documented in that script).  Each atlas is mmap()'d (or fread into
 * heap when mmap isn't available — fbcon-mode hosts) once at startup;
 * glyph entries are indexed by codepoint via linear scan over the
 * directory (95 entries for ASCII 32-126 — branch-friendly).
 *
 * Render path is a straight mask-blit: for each glyph, walk its
 * width*height alpha bytes and call draw_blend_pixel(x+dx, y+dy,
 * color, mask_alpha).  No SIMD, no scaling, no rotation.  Glyph cache
 * isn't needed — the masks are already pre-rendered and live in heap.
 *
 * Files live at /tmp/atomik_fonts/atomik_<size>.atomik_font; deploy
 * ships them alongside .atomik_asset blits.  If a font is missing,
 * draw_text_aa() falls back to the pixel font silently — keeps the
 * board usable on first boot before assets land. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* Header layout (32 bytes, little-endian) per font_pack.py. */
typedef struct __attribute__((packed)) {
    char     magic[4];      /* "AFNT" */
    uint16_t version;
    uint16_t pixel_size;
    int16_t  ascender;
    int16_t  descender;
    uint16_t line_height;
    uint16_t glyph_count;
    uint32_t dir_offset;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t flags;
} afnt_header_t;

/* Glyph dirent layout (20 bytes per entry) per font_pack.py. */
typedef struct __attribute__((packed)) {
    uint32_t codepoint;
    uint16_t w;
    uint16_t h;
    int16_t  advance;
    int16_t  offset_x;
    int16_t  offset_y;
    uint16_t reserved;
    uint32_t data_offset;
} afnt_glyph_t;

typedef struct {
    int             loaded;
    afnt_header_t   hdr;
    afnt_glyph_t   *glyphs;     /* directory in heap                 */
    uint8_t        *masks;      /* mask blob in heap                 */
} font_atlas_t;

static font_atlas_t s_atlases[FONT_AA_COUNT];

static const char *atlas_path(font_aa_id_t id) {
    switch (id) {
    case FONT_AA_LABEL:   return "/tmp/atomik_fonts/atomik_14.atomik_font";
    case FONT_AA_UI:      return "/tmp/atomik_fonts/atomik_18.atomik_font";
    case FONT_AA_DISPLAY: return "/tmp/atomik_fonts/atomik_28.atomik_font";
    default:              return NULL;
    }
}

static int load_one(font_aa_id_t id) {
    const char *path = atlas_path(id);
    if (!path) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    font_atlas_t *a = &s_atlases[id];
    memset(a, 0, sizeof *a);

    if (fread(&a->hdr, 1, sizeof a->hdr, f) != sizeof a->hdr) {
        fclose(f); return 0;
    }
    if (memcmp(a->hdr.magic, "AFNT", 4) != 0 || a->hdr.version != 1) {
        fclose(f); return 0;
    }

    size_t dir_bytes = (size_t)a->hdr.glyph_count * sizeof(afnt_glyph_t);
    a->glyphs = malloc(dir_bytes);
    if (!a->glyphs) { fclose(f); return 0; }
    if (fseek(f, a->hdr.dir_offset, SEEK_SET) != 0 ||
        fread(a->glyphs, 1, dir_bytes, f) != dir_bytes) {
        free(a->glyphs); a->glyphs = NULL;
        fclose(f); return 0;
    }

    a->masks = malloc(a->hdr.data_size);
    if (!a->masks) { free(a->glyphs); a->glyphs = NULL;
                     fclose(f); return 0; }
    if (fseek(f, a->hdr.data_offset, SEEK_SET) != 0 ||
        fread(a->masks, 1, a->hdr.data_size, f) != a->hdr.data_size) {
        free(a->masks);  a->masks  = NULL;
        free(a->glyphs); a->glyphs = NULL;
        fclose(f); return 0;
    }

    a->loaded = 1;
    fclose(f);
    return 1;
}

int font_aa_init(void) {
    int n = 0;
    for (int i = 0; i < FONT_AA_COUNT; i++) {
        if (load_one((font_aa_id_t)i)) n++;
    }
    return n;
}

int font_aa_loaded(font_aa_id_t id) {
    if (id < 0 || id >= FONT_AA_COUNT) return 0;
    return s_atlases[id].loaded;
}

/* Linear lookup over the directory.  95 entries → cheap.  Returns
 * NULL for glyphs we don't have (caller falls back to space-width). */
static const afnt_glyph_t *find_glyph(const font_atlas_t *a, uint32_t cp) {
    for (uint16_t i = 0; i < a->hdr.glyph_count; i++) {
        if (a->glyphs[i].codepoint == cp) return &a->glyphs[i];
    }
    return NULL;
}

int text_width_aa(font_aa_id_t id, const char *s) {
    if (!s) return 0;
    if (id < 0 || id >= FONT_AA_COUNT || !s_atlases[id].loaded) {
        return text_width(s, 1);    /* pixel-font fallback */
    }
    const font_atlas_t *a = &s_atlases[id];
    int w = 0;
    for (const char *p = s; *p; p++) {
        const afnt_glyph_t *g = find_glyph(a, (uint8_t)*p);
        w += g ? g->advance : (a->hdr.pixel_size / 2);
    }
    return w;
}

int text_height_aa(font_aa_id_t id) {
    if (id < 0 || id >= FONT_AA_COUNT || !s_atlases[id].loaded) {
        return text_height(1);
    }
    return s_atlases[id].hdr.line_height;
}

int text_ascender_aa(font_aa_id_t id) {
    if (id < 0 || id >= FONT_AA_COUNT || !s_atlases[id].loaded) {
        return text_height(1);
    }
    return s_atlases[id].hdr.ascender;
}

void draw_text_aa(font_aa_id_t id, int x, int y, const char *s,
                  pixel_t color) {
    if (!s) return;
    if (id < 0 || id >= FONT_AA_COUNT || !s_atlases[id].loaded) {
        /* Pixel font fallback so missing atlases don't blank the UI. */
        int scale = (id == FONT_AA_DISPLAY) ? 3
                  : (id == FONT_AA_UI)      ? 2 : 1;
        draw_text(x, y, s, scale, color);
        return;
    }
    const font_atlas_t *a  = &s_atlases[id];
    /* draw_text contract is "y is the top of the cell, baseline auto".
     * For consistency we treat (x, y) as the top of the cell too,
     * so the caller can swap draw_text → draw_text_aa with no math. */
    int baseline = y + a->hdr.ascender;
    int pen      = x;
    for (const char *p = s; *p; p++) {
        const afnt_glyph_t *g = find_glyph(a, (uint8_t)*p);
        if (!g) {
            pen += a->hdr.pixel_size / 2;
            continue;
        }
        int gx = pen + g->offset_x;
        int gy = baseline + g->offset_y;
        const uint8_t *mask = a->masks + g->data_offset;
        for (int py = 0; py < g->h; py++) {
            for (int px = 0; px < g->w; px++) {
                uint8_t alpha = mask[py * g->w + px];
                if (alpha == 0) continue;
                if (alpha == 255) draw_pixel(gx + px, gy + py, color);
                else              draw_blend_pixel(gx + px, gy + py,
                                                   color, alpha);
            }
        }
        pen += g->advance;
    }
}
