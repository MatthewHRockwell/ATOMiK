/* atomik_asset.c — v0.36 board-side asset loader.
 *
 * Class B foundation per ChatGPT 2026-05-09 directive: a board-friendly
 * asset format (ATKA v1) that lets us blit pre-rendered art without
 * pulling libpng or zlib onto the 100 MHz NaxRiscv.  The host-side
 * tools/make_atomik_asset.py does any PNG / format conversion work
 * and writes a minimal binary file the board can mmap-style-load and
 * blit at memcpy speed.
 *
 * Critical rule (from feedback_lane2_class_a_b_directive): assets are
 * decorative — every NUMBER on screen still traces back to a real
 * producer (Class A).  This loader makes pretty backgrounds possible
 * without enabling fake metrics. */
#include "atomik_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern pixel_t *fb_back(void);

/* Helpers — read an LE uint from a byte buffer. */
static uint16_t rd_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}
static uint32_t rd_u32(const uint8_t *p) {
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) |
                      ((uint32_t)p[3] << 24));
}

int atomik_asset_load(const char *path, atomik_asset_t *out) {
    if (!out) return -1;
    out->width = out->height = 0;
    out->pixels = NULL;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    uint8_t hdr[24];
    if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr) {
        fclose(f);
        return -1;
    }
    if (memcmp(hdr, ATOMIK_ASSET_MAGIC, 4) != 0) {
        fclose(f);
        return -1;
    }
    uint16_t version  = rd_u16(hdr + 4);
    uint16_t flags    = rd_u16(hdr + 6);
    uint32_t width    = rd_u32(hdr + 8);
    uint32_t height   = rd_u32(hdr + 12);
    uint32_t pl_bytes = rd_u32(hdr + 16);

    if (version != ATOMIK_ASSET_VERSION) {
        fclose(f);
        return -1;
    }
    /* Sanity: cap dimensions at framebuffer size × 2.  An asset bigger
     * than that is almost certainly corrupted or malicious. */
    if (width == 0 || height == 0 ||
        width  > (uint32_t)FB_W * 2 ||
        height > (uint32_t)FB_H * 2) {
        fclose(f);
        return -1;
    }
    /* Cap payload at 64 MB so a corrupt header can't make us malloc the world. */
    if (pl_bytes > 64u * 1024 * 1024) {
        fclose(f);
        return -1;
    }

    /* Allocate destination pixel buffer.  Always native XRGB8888 — the
     * blit path is a straight memcpy of N bytes per scanline. */
    size_t n_pix  = (size_t)width * (size_t)height;
    pixel_t *pix  = (pixel_t *)malloc(n_pix * sizeof(pixel_t));
    if (!pix) {
        fclose(f);
        return -1;
    }

    if (flags & ATOMIK_ASSET_FLAG_RLE) {
        /* RLE: stream of (uint16 count, uint32 pixel) records.  We
         * read into a single payload buffer to avoid stdio overhead
         * per-record on the soft CPU. */
        uint8_t *payload = (uint8_t *)malloc(pl_bytes);
        if (!payload) { free(pix); fclose(f); return -1; }
        if (fread(payload, 1, pl_bytes, f) != pl_bytes) {
            free(payload); free(pix); fclose(f); return -1;
        }
        size_t consumed = 0;
        size_t produced = 0;
        while (produced < n_pix) {
            if (consumed + 6 > pl_bytes) {
                free(payload); free(pix); fclose(f); return -1;
            }
            uint16_t count = rd_u16(payload + consumed);
            uint32_t value = rd_u32(payload + consumed + 2);
            consumed += 6;
            if (count == 0) {
                free(payload); free(pix); fclose(f); return -1;
            }
            if (produced + count > n_pix) {
                free(payload); free(pix); fclose(f); return -1;
            }
            for (uint16_t i = 0; i < count; i++) {
                pix[produced + i] = (pixel_t)value;
            }
            produced += count;
        }
        free(payload);
    } else {
        /* Raw: payload is exactly width*height*4 bytes of XRGB8888. */
        size_t expect = n_pix * sizeof(pixel_t);
        if (pl_bytes != expect) {
            free(pix); fclose(f); return -1;
        }
        if (fread(pix, 1, expect, f) != expect) {
            free(pix); fclose(f); return -1;
        }
    }
    fclose(f);

    out->width  = width;
    out->height = height;
    out->pixels = pix;
    return 0;
}

void atomik_asset_free(atomik_asset_t *a) {
    if (!a) return;
    if (a->pixels) free(a->pixels);
    a->pixels = NULL;
    a->width = a->height = 0;
}

/* Compute clipped src/dst rects so a partial off-screen blit
 * draws only the visible portion.  Returns 1 if any pixels are
 * visible, 0 if entirely off-screen. */
static int clip(const atomik_asset_t *a, int dx, int dy,
                int *out_dx, int *out_dy,
                int *out_sx, int *out_sy,
                int *out_w,  int *out_h) {
    int sx = 0, sy = 0;
    int w  = (int)a->width;
    int h  = (int)a->height;
    if (dx < 0) { sx = -dx; w -= sx; dx = 0; }
    if (dy < 0) { sy = -dy; h -= sy; dy = 0; }
    if (dx + w > FB_W) w = FB_W - dx;
    if (dy + h > FB_H) h = FB_H - dy;
    if (w <= 0 || h <= 0) return 0;
    *out_dx = dx; *out_dy = dy;
    *out_sx = sx; *out_sy = sy;
    *out_w  = w;  *out_h  = h;
    return 1;
}

void atomik_asset_blit(const atomik_asset_t *a, int dx, int dy) {
    if (!a || !a->pixels) return;
    int rdx, rdy, rsx, rsy, rw, rh;
    if (!clip(a, dx, dy, &rdx, &rdy, &rsx, &rsy, &rw, &rh)) return;
    pixel_t *bb = fb_back();
    size_t row_bytes = (size_t)rw * sizeof(pixel_t);
    for (int row = 0; row < rh; row++) {
        const pixel_t *src = a->pixels + (size_t)(rsy + row) * a->width + rsx;
        pixel_t       *dst = bb + (size_t)(rdy + row) * FB_W + rdx;
        memcpy(dst, src, row_bytes);
    }
}

/* Per-channel alpha blend: dst = (src*alpha + dst*(255-alpha)) / 255.
 * Used for subtle background overlays at controlled opacity. */
static inline pixel_t blend(pixel_t src, pixel_t dst, uint8_t a) {
    uint32_t inv = 255u - a;
    uint32_t sr = (src >> 16) & 0xFF;
    uint32_t sg = (src >> 8)  & 0xFF;
    uint32_t sb =  src        & 0xFF;
    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >> 8)  & 0xFF;
    uint32_t db =  dst        & 0xFF;
    uint32_t r = (sr * a + dr * inv) / 255;
    uint32_t g = (sg * a + dg * inv) / 255;
    uint32_t b = (sb * a + db * inv) / 255;
    return (pixel_t)((r << 16) | (g << 8) | b);
}

void atomik_asset_blit_tiled(const atomik_asset_t *a,
                             int dx, int dy, int w, int h) {
    if (!a || !a->pixels || w <= 0 || h <= 0) return;
    /* Iterate over destination by asset-sized tiles.  Each tile call
     * goes through atomik_asset_blit which already clips to the
     * framebuffer, so partial-tiles at the right/bottom edge are
     * handled for free. */
    int aw = (int)a->width;
    int ah = (int)a->height;
    if (aw <= 0 || ah <= 0) return;
    for (int ty = 0; ty < h; ty += ah) {
        for (int tx = 0; tx < w; tx += aw) {
            atomik_asset_blit(a, dx + tx, dy + ty);
        }
    }
}

void atomik_asset_blit_alpha(const atomik_asset_t *a, int dx, int dy,
                             uint8_t alpha) {
    if (!a || !a->pixels) return;
    if (alpha == 0) return;
    if (alpha == 255) { atomik_asset_blit(a, dx, dy); return; }
    int rdx, rdy, rsx, rsy, rw, rh;
    if (!clip(a, dx, dy, &rdx, &rdy, &rsx, &rsy, &rw, &rh)) return;
    pixel_t *bb = fb_back();
    for (int row = 0; row < rh; row++) {
        const pixel_t *src = a->pixels + (size_t)(rsy + row) * a->width + rsx;
        pixel_t       *dst = bb + (size_t)(rdy + row) * FB_W + rdx;
        for (int col = 0; col < rw; col++) {
            dst[col] = blend(src[col], dst[col], alpha);
        }
    }
}
