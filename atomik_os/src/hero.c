/* hero.c — v0.38-E procedural centerpiece visualization.
 *
 * Closes the biggest concept-image gap: the iridescent 3D fabric
 * visualization that anchors `docs/design/concept_images/01_atomik_desk_home.png`.
 * We don't have pre-rendered art for it yet (asset pipeline shipped
 * but no hero asset generated); this renders an approximation from
 * primitives so the OS has a real visual anchor instead of the
 * About text card sitting in the middle of the screen.
 *
 * What it draws (all from draw_pixel + draw_blend_pixel + draw_rect):
 *   - Soft alpha-blended radial halo (cyan fading outward)
 *   - 4 concentric hexagon outlines (different rotations)
 *   - 6 radial spokes connecting inner to outer
 *   - Pulsing central dot (slow 2-Hz breathing)
 *
 * Performance budget: bounded.  ~400 px diameter × outlines (no fill)
 * = a few thousand draw_pixel calls.  Acceptable on 100 MHz NaxRiscv
 * given we only redraw on frame-dirty + dirty-tile system will
 * eventually skip clean tiles entirely.
 *
 * Class A discipline: this is decorative chrome only.  No numbers
 * rendered here, no metric values surfaced.  All telemetry stays
 * in Pulse Bar / Fabric / State Watch / Replica Flow — which read
 * from the metric provider.  Hero is pure visual identity. */
#include "atomik_os.h"
#include <math.h>

extern pixel_t *fb_back(void);

static int g_cx(void) { return FB_W / 2 + 60; }    /* shift slightly toward
                                                       Fabric shelf for balance */
static int g_cy(void) { return FB_H / 2 + 60; }    /* below screen center to
                                                       avoid Pulse Bar */

/* Bresenham line into back buffer with optional alpha. */
static void hero_line(int x0, int y0, int x1, int y1,
                      pixel_t color, uint8_t alpha) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        if (alpha == 255) draw_pixel(x0, y0, color);
        else              draw_blend_pixel(x0, y0, color, alpha);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

/* Hexagon outline at (cx, cy) with given radius and rotation in radians.
 * Stroke is `thickness` pixels (parallel offsets, cheap). */
static void hex_at(int cx, int cy, double radius, double rotation,
                   pixel_t color, uint8_t alpha, int thickness) {
    int xs[7], ys[7];
    for (int i = 0; i < 6; i++) {
        double a = rotation + i * (M_PI / 3.0);
        xs[i] = cx + (int)(radius * cos(a));
        ys[i] = cy + (int)(radius * sin(a));
    }
    xs[6] = xs[0]; ys[6] = ys[0];
    for (int t = 0; t < thickness; t++) {
        for (int i = 0; i < 6; i++) {
            hero_line(xs[i], ys[i] + t, xs[i+1], ys[i+1] + t, color, alpha);
            if (t > 0) {
                hero_line(xs[i], ys[i] - t, xs[i+1], ys[i+1] - t, color, alpha);
            }
        }
    }
}

/* Soft halo: concentric circles drawn as outline rings at sparse
 * radii with linearly-fading alpha.  Cheap — sparse sample. */
static void hero_halo(int cx, int cy, int outer_r, pixel_t color) {
    for (int r = outer_r; r > outer_r / 6; r -= 12) {
        double t = (double)r / outer_r;
        uint8_t alpha = (uint8_t)(60.0 * (1.0 - t) * (1.0 - t) + 8);
        /* Walk the circle's circumference, stepping in degrees. */
        for (int deg = 0; deg < 360; deg += 2) {
            double a = deg * M_PI / 180.0;
            int x = cx + (int)(r * cos(a));
            int y = cy + (int)(r * sin(a));
            draw_blend_pixel(x, y, color, alpha);
        }
    }
}

/* Filled disk at (cx, cy) radius r.  Used for the pulsing center. */
static void hero_disk(int cx, int cy, int r, pixel_t color, uint8_t alpha) {
    int r2 = r * r;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r2) {
                if (alpha == 255) draw_pixel(cx + dx, cy + dy, color);
                else              draw_blend_pixel(cx + dx, cy + dy, color, alpha);
            }
        }
    }
}

void hero_draw(void) {
    int cx = g_cx();
    int cy = g_cy();

    unsigned long now = anim_now_ms();
    /* Slow pulse: full cycle every ~4 s. */
    double phase = (double)(now % 4000) / 4000.0;
    double pulse = 0.5 + 0.5 * sin(phase * 2.0 * M_PI);

    /* Soft outer halo — pre-multiplied cyan with fade. */
    pixel_t cyan = ATOMIK_ACCENT;
    hero_halo(cx, cy, 360, cyan);

    /* Inner halo, brighter, denser. */
    for (int r = 60; r < 220; r += 18) {
        double t = (r - 60.0) / 160.0;
        uint8_t alpha = (uint8_t)(45 * (1.0 - t));
        for (int deg = 0; deg < 360; deg += 1) {
            double a = deg * M_PI / 180.0;
            int x = cx + (int)(r * cos(a));
            int y = cy + (int)(r * sin(a));
            draw_blend_pixel(x, y, cyan, alpha);
        }
    }

    /* Three nested hexagon rings at different rotations.  Outer ring
     * dimmer; inner ring brighter.  Rotation crawls slowly with time. */
    double base_rot = (double)(now % 16000) / 16000.0 * 2.0 * M_PI;
    hex_at(cx, cy, 320, base_rot,            cyan, 60,  1);
    hex_at(cx, cy, 240, base_rot + 0.2,      cyan, 110, 1);
    hex_at(cx, cy, 160, base_rot + 0.4,      cyan, 180, 1);
    hex_at(cx, cy, 100, base_rot + 0.6,      cyan, 230, 2);

    /* Six radial spokes from inner-ring radius to mid-ring radius.
     * Brighten with pulse so the whole viz feels alive. */
    uint8_t spoke_alpha = (uint8_t)(80 + 60 * pulse);
    for (int i = 0; i < 6; i++) {
        double a = base_rot + i * (M_PI / 3.0);
        int x0 = cx + (int)(100 * cos(a));
        int y0 = cy + (int)(100 * sin(a));
        int x1 = cx + (int)(240 * cos(a));
        int y1 = cy + (int)(240 * sin(a));
        hero_line(x0, y0, x1, y1, cyan, spoke_alpha);
    }

    /* Central anchor: nested disks, brightest in the middle, with the
     * outer ring pulsing in size.  Marks the OS identity at the
     * geometric heart of the screen. */
    int outer_d = 22 + (int)(4 * pulse);
    hero_disk(cx, cy, outer_d, cyan, 90);
    hero_disk(cx, cy, 14,      cyan, 200);
    hero_disk(cx, cy, 6,       ATOMIK_FG, 255);

    /* v0.38-A: register the dirty area.  The hero covers a ~720×720
     * region centered on (cx, cy) — mark those tiles dirty so the
     * upcoming tile-skip path knows to repaint when animation steps. */
    dirty_rect(cx - 360, cy - 360, 720, 720);

    /* Drive animation forward so the pulse keeps advancing. */
    anim_tick();
}
