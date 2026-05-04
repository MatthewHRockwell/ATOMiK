/* wallpaper.c — background. Vertical gradient + a subtle radial vignette
 * + a centered ATOMiK wordmark, ATOMiK accent color glow. */
#include "atomik_os.h"

void wallpaper_draw(void) {
    /* Smooth vertical gradient covering the whole screen. */
    draw_gradient_v(0, 0, FB_W, FB_H, ATOMIK_BG_TOP, ATOMIK_BG_BOT);

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
