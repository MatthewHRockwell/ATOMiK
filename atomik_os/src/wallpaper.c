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

/* v0.36-A: tile a SMALL (480×270) asset across the full framebuffer,
 * then let the procedural overlay handle vignette/glow/wordmark.
 * Smaller asset → ~9 MB heap peak instead of the ~17 MB that hung
 * v0.36 #1 on AX7020.  Tile dimensions (480×270) divide 1920×1080
 * evenly (4×4) so there are no partial-tile edges. */
static int try_tile_asset(void) {
    const char *paths[] = {
        "/tmp/atomik_assets/topology_tile.atomik_asset",
        "/root/atomik_assets/topology_tile.atomik_asset",
        "/usr/share/atomik_os/topology_tile.atomik_asset",
        NULL
    };
    atomik_asset_t a;
    for (int i = 0; paths[i]; i++) {
        if (atomik_asset_load(paths[i], &a) == 0) {
            atomik_asset_blit_tiled(&a, 0, 0, FB_W, FB_H);
            atomik_asset_free(&a);
            return 1;
        }
    }
    return 0;
}

static void wallpaper_full_render(void) {
    /* v0.36-A: tile the small (480×270) topology asset across the full
     * screen.  If the asset isn't present, draw the procedural gradient
     * as a fallback.  Either way, the vignette, accent glow, and
     * wordmark are layered on top procedurally — they're cheap and
     * keep the wordmark sharp regardless of asset presence. */
    if (!try_tile_asset()) {
        draw_gradient_v(0, 0, FB_W, FB_H, ATOMIK_BG_TOP, ATOMIK_BG_BOT);
    }

    /* Soft accent vignette in the center: a few translucent ellipses. */
    int cx = FB_W / 2;
    int cy = FB_H / 2 - 60;
    for (int r = 600; r >= 200; r -= 50) {
        /* Cheap circle fill with low alpha. We sample sparsely on big radii
         * for speed — full fill at small radii, scanline-skipping at large. */
        int step = r > 400 ? 4 : (r > 300 ? 2 : 1);
        int alpha = 8 + (600 - r) / 12;
        if (alpha > 60) alpha = 60;
        for (int dy = -r; dy <= r; dy += step) {
            int span = (int)((r * r - dy * dy) > 0 ? r * r - dy * dy : 0);
            int xspan = 0;
            while (xspan * xspan < span) xspan++;
            for (int dx = -xspan; dx <= xspan; dx += step) {
                draw_blend_pixel(cx + dx, cy + dy, ATOMIK_ACCENT, (uint8_t)alpha);
            }
        }
    }

    /* Wordmark — ATOMiK in big text, with a subtle drop shadow. */
    const char *mark = "ATOMiK";
    int scale = 6;
    int tw = text_width(mark, scale);
    int th = text_height(scale);
    int tx = (FB_W - tw) / 2;
    int ty = (FB_H - th) / 2 - 40;

    /* Drop shadow */
    draw_text(tx + 3, ty + 4, mark, scale, rgb(0x00, 0x00, 0x00));
    /* Main text */
    draw_text(tx, ty, mark, scale, ATOMIK_FG);

    /* Tagline below */
    const char *tag = "Delta-State Desktop";
    int tag_scale = 2;
    int tag_w = text_width(tag, tag_scale);
    draw_text((FB_W - tag_w) / 2, ty + th + 24, tag, tag_scale, ATOMIK_ACCENT);
}

void wallpaper_draw(void) {
    pixel_t *bb = fb_back();
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
