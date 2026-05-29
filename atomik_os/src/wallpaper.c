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

/* Layered hero background (concept-01): deep gradient -> perspective particle
 * terrain + horizon glow -> concentric rings + soft radial center glow behind
 * the hero -> edge vignette.  Rendered ONCE into the wallpaper cache (no per-
 * frame cost) with a FIXED seed so the scene is screenshot-stable.  Pure
 * decorative Class B chrome — no telemetry. */
static void wallpaper_full_render(void) {
    uint32_t seed = 0xA70A1CBAu;
#define BG_RND (seed ^= seed << 13, seed ^= seed >> 17, seed ^= seed << 5, seed)

    /* Hero center: workspace midpoint (rail .. Fabric) so the rings/glow sit
     * behind the hero, not the screen center. */
    int rr = dock_right_edge(), fs = fabric_shelf_x();
    int cx = (rr > 0 && fs > rr) ? (rr + fs) / 2 : FB_W / 2;
    int cy_glow = (FB_H * 30) / 100;
    int horizon = (FB_H * 60) / 100;
    pixel_t cyan = ATOMIK_ACCENT;

    /* 1. base gradient — very dark navy, deepening downward. */
    draw_gradient_v(0, 0, FB_W, FB_H, rgb(0x06, 0x09, 0x12), rgb(0x0B, 0x10, 0x1E));

    /* 2. perspective floor grid below the horizon (data-fabric plane). */
    for (int i = -12; i <= 12; i++)
        bg_line(cx, horizon, cx + i * (FB_W / 10), FB_H, cyan, 22);
    for (int k = 1; k <= 16; k++) {
        int yy = horizon + (FB_H - horizon) * k * k / (16 * 16);
        uint8_t a = (uint8_t)(38 - k * 2); if (a < 7) a = 7;
        for (int x = 0; x < FB_W; x++) draw_blend_pixel(x, yy, cyan, a);
    }

    /* 3. horizon glow band — soft bright line where the plane meets space. */
    for (int g = -6; g <= 6; g++) {
        int gg = g < 0 ? -g : g;
        uint8_t a = (uint8_t)(64 - 9 * gg);
        for (int x = 0; x < FB_W; x++) draw_blend_pixel(x, horizon + g, cyan, a);
    }

    /* 4. particle terrain — dense near the horizon, fading downward. */
    int band = FB_H - horizon;
    for (int p = 0; p < 1100; p++) {
        int x = (int)(BG_RND % (uint32_t)FB_W);
        int depth = (int)(BG_RND % (uint32_t)band);   /* 0 at horizon */
        int y = horizon + depth;
        uint8_t a = (uint8_t)(150 - depth * 132 / band); if (a < 12) a = 12;
        pixel_t c = (BG_RND & 3u) ? cyan : rgb(0x80, 0x6C, 0xFF); /* mostly cyan, some violet */
        draw_blend_pixel(x, y, c, a);
        draw_blend_pixel(x + 1, y, c, a / 2);
        draw_blend_pixel(x, y + 1, c, a / 2);
    }

    /* 5. sparse micro-particles in the upper field. */
    for (int p = 0; p < 160; p++) {
        int x = (int)(BG_RND % (uint32_t)FB_W);
        int y = (int)(BG_RND % (uint32_t)horizon);
        draw_blend_pixel(x, y, cyan, (uint8_t)(18 + (BG_RND % 38u)));
    }

    /* 6. concentric rings behind the hero — thin, smooth, low alpha. */
    for (int r = 120; r <= 360; r += 48)
        bg_circle(cx, cy_glow, r, cyan, 15);

    /* 7. soft radial center glow behind the hero (bounded box, additive). */
    int R = 360; long R2 = (long)R * R;
    for (int dy = -R; dy <= R; dy++) {
        int y = cy_glow + dy; if (y < 0 || y >= FB_H) continue;
        for (int dx = -R; dx <= R; dx++) {
            int x = cx + dx; if (x < 0 || x >= FB_W) continue;
            long d2 = (long)dx * dx + (long)dy * dy;
            if (d2 >= R2) continue;
            uint8_t a = (uint8_t)(42 * (R2 - d2) / R2);
            if (a) draw_blend_pixel(x, y, cyan, a);
        }
    }

    /* 8. edge vignette — darken toward all four edges. */
    int vb = 230;
    for (int b = 0; b < vb; b++) {
        uint8_t a = (uint8_t)(72 * (vb - b) / vb);
        for (int x = 0; x < FB_W; x++) {
            draw_blend_pixel(x, b, 0, a);
            draw_blend_pixel(x, FB_H - 1 - b, 0, a);
        }
    }
    for (int b = 0; b < vb; b++) {
        uint8_t a = (uint8_t)(72 * (vb - b) / vb);
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
