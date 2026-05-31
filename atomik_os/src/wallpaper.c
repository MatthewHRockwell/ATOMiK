/* wallpaper.c — background. Vertical gradient + a subtle radial vignette
 * + a centered ATOMiK wordmark, ATOMiK accent color glow.
 *
 * The vignette work (per-frame ellipse blending) is too expensive at 100 MHz
 * to redraw every frame. We render the whole wallpaper ONCE into the back
 * buffer, then memcpy it to a heap cache. On subsequent frames we memcpy
 * the cache straight back to the back buffer. wallpaper_invalidate()
 * forces a re-render when settings change.
 *
 * v0.36: try to load /tmp/atomik_assets/topology_wallpaper.atomik_asset
 * before falling back to the procedural render.  Class B per ChatGPT
 * 2026-05-09 — pre-rendered art, deployed alongside the binary, shown
 * UNDER the Class A telemetry rather than instead of it. */
#include "atomik_os.h"
#include <stdlib.h>
#include <string.h>

extern pixel_t *fb_back(void);

static pixel_t *s_cache  = NULL;
static int      s_cached = 0;

void wallpaper_invalidate(void) { s_cached = 0; }

/* --- v0.40 layered procedural background helpers (concept-01 depth) --- */
static void bg_line(int x0, int y0, int x1, int y1, pixel_t c, uint8_t a) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx - dy;
    for (;;) {
        draw_blend_pixel(x0, y0, c, a);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}
static void bg_circle(int cx, int cy, int r, pixel_t c, uint8_t a) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        draw_blend_pixel(cx+x, cy+y, c, a); draw_blend_pixel(cx-x, cy+y, c, a);
        draw_blend_pixel(cx+x, cy-y, c, a); draw_blend_pixel(cx-x, cy-y, c, a);
        draw_blend_pixel(cx+y, cy+x, c, a); draw_blend_pixel(cx-y, cy+x, c, a);
        draw_blend_pixel(cx+y, cy-x, c, a); draw_blend_pixel(cx-y, cy-x, c, a);
        y++;
        if (err < 0) err += 2*y + 1; else { x--; err += 2*(y - x) + 1; }
    }
}

/* Soft additive radial glow with quadratic falloff (a = peak*(1 - d^2/R^2)).
 * Bounded box; only blends inside the disc.  The workhorse for the nebula. */
static void soft_glow(int cx, int cy, int R, pixel_t c, uint8_t peak) {
    if (R <= 0) return;
    long R2 = (long)R * R;
    for (int dy = -R; dy <= R; dy++) {
        int y = cy + dy; if (y < 0 || y >= FB_H) continue;
        for (int dx = -R; dx <= R; dx++) {
            int x = cx + dx; if (x < 0 || x >= FB_W) continue;
            long d2 = (long)dx * dx + (long)dy * dy;
            if (d2 >= R2) continue;
            uint8_t a = (uint8_t)((long)peak * (R2 - d2) / R2);
            if (a) draw_blend_pixel(x, y, c, a);
        }
    }
}

/* Atmospheric "deep space / nebula" background (concept-01): smooth dark navy
 * with a soft blue central glow behind the hero, off-center nebula clouds for
 * asymmetric depth, faint rings, and a sparse star field — NO perspective
 * grid.  Rendered ONCE into the wallpaper cache (no per-frame cost) with a
 * FIXED seed so it is screenshot-stable.  Pure decorative Class B chrome. */
static void wallpaper_full_render(void) {
    uint32_t seed = 0xA70A1CBAu;
#define BG_RND (seed ^= seed << 13, seed ^= seed >> 17, seed ^= seed << 5, seed)

    /* Hero center: workspace midpoint (rail .. Fabric) so the glow sits behind
     * the hero, not the screen center. */
    int rr = dock_right_edge(), fs = fabric_shelf_x();
    int cx = (rr > 0 && fs > rr) ? (rr + fs) / 2 : FB_W / 2;
    int cy = (FB_H * 38) / 100;
    pixel_t cyan = ATOMIK_ACCENT;

    /* 1. base gradient — deep navy, very slightly lifted up top (where the
     * nebula core sits), deepening toward the bottom. */
    draw_gradient_v(0, 0, FB_W, FB_H, rgb(0x07, 0x0C, 0x18), rgb(0x04, 0x07, 0x10));

    /* 2. off-center nebula clouds — large, very soft, low alpha, for an
     * asymmetric "asteroid field / deep space" depth (blue + a hint of
     * violet).  Drawn before the core so the core reads brightest. */
    soft_glow((FB_W * 22) / 100, (FB_H * 72) / 100, 520, rgb(0x10, 0x22, 0x4C), 20);
    soft_glow((FB_W * 82) / 100, (FB_H * 22) / 100, 460, rgb(0x22, 0x1A, 0x44), 16);
    soft_glow((FB_W * 70) / 100, (FB_H * 80) / 100, 420, rgb(0x0E, 0x26, 0x46), 14);

    /* 3. nebula core behind the hero — a wide blue halo + a tighter cyan core,
     * additive, smooth quadratic falloff.  This is the concept's central glow. */
    soft_glow(cx, cy, 600, rgb(0x14, 0x34, 0x60), 30);
    soft_glow(cx, cy, 340, rgb(0x2A, 0x6A, 0xA8), 30);
    soft_glow(cx, cy, 180, cyan, 22);

    /* 4. faint concentric rings behind the hero (the concept's subtle halo). */
    bg_circle(cx, cy, 232, cyan, 12);
    bg_circle(cx, cy, 318, cyan, 8);
    bg_circle(cx, cy, 404, cyan, 5);

    /* 5. star field — sparse points across the WHOLE field, varied brightness,
     * mostly cool white with occasional cyan/violet.  A handful of brighter
     * "hero" stars get a 1px cross bloom. */
    for (int p = 0; p < 280; p++) {
        int x = (int)(BG_RND % (uint32_t)FB_W);
        int y = (int)(BG_RND % (uint32_t)FB_H);
        uint8_t a = (uint8_t)(10 + (BG_RND % 46u));
        uint32_t pick = BG_RND % 10u;
        pixel_t c = pick < 7u ? rgb(0xC8, 0xD6, 0xF0)
                  : pick < 9u ? cyan : rgb(0x9A, 0x86, 0xFF);
        draw_blend_pixel(x, y, c, a);
    }
    for (int p = 0; p < 26; p++) {
        int x = (int)(BG_RND % (uint32_t)FB_W);
        int y = (int)(BG_RND % (uint32_t)FB_H);
        draw_blend_pixel(x, y, rgb(0xE6, 0xEE, 0xFF), 200);
        draw_blend_pixel(x - 1, y, cyan, 60); draw_blend_pixel(x + 1, y, cyan, 60);
        draw_blend_pixel(x, y - 1, cyan, 60); draw_blend_pixel(x, y + 1, cyan, 60);
    }

    /* 6. soft edge vignette — darken toward all four edges, gentle. */
    int vb = 260;
    for (int b = 0; b < vb; b++) {
        uint8_t a = (uint8_t)(60 * (vb - b) / vb);
        for (int x = 0; x < FB_W; x++) {
            draw_blend_pixel(x, b, 0, a);
            draw_blend_pixel(x, FB_H - 1 - b, 0, a);
        }
    }
    for (int b = 0; b < vb; b++) {
        uint8_t a = (uint8_t)(60 * (vb - b) / vb);
        for (int y = 0; y < FB_H; y++) {
            draw_blend_pixel(b, y, 0, a);
            draw_blend_pixel(FB_W - 1 - b, y, 0, a);
        }
    }
#undef BG_RND
}

void wallpaper_draw(void) {
    pixel_t *bb = fb_back();
    /* v0.38-A: wallpaper paints the entire framebuffer every frame
     * (whole-screen memcpy from cache).  Honest dirty mark = all
     * 2040 tiles.  v0.38-B will teach this path to memcpy only the
     * tiles that other layers will actually overwrite, dropping the
     * dirty count to whatever the per-surface union is. */
    dirty_all();
    if (!s_cache) {
        s_cache = aligned_alloc(64, FB_BYTES);
        if (!s_cache) {
            /* No cache available — render direct every frame. */
            wallpaper_full_render();
            return;
        }
    }
    if (!s_cached) {
        /* First call (or after invalidate): render into back buffer, then
         * snapshot to cache. Subsequent frames just memcpy from cache. */
        wallpaper_full_render();
        memcpy(s_cache, bb, FB_BYTES);
        s_cached = 1;
        return;
    }
    memcpy(bb, s_cache, FB_BYTES);
}
