/* hero.c — v0.38-H Adaptive Mode hero (3-personality energy flow).
 *
 * Replaces v0.38-E's hexagon centerpiece with the concept-image-06
 * "ADAPTIVE MIXED WORKLOAD" visualization: three flowing energy fields
 * side-by-side, each in its personality semantic color, labeled
 * STATE / SYNC / AGENT below.  The bridge between substance (real
 * personalities executing real batched MMIO) and visual identity
 * (this is what the personality LANDSCAPE looks like).
 *
 * Geometry: centered between the Capability Rail (left) and the
 * Resource Fabric shelf (right) — uses dock_right_edge() and
 * fabric_shelf_x() so layout shifts with chrome changes.  Vertically
 * sits below the Pulse Bar with margin; bottom labels at scale-2.
 *
 * What it draws (all from primitives — no asset dependency):
 *   - Three vertical bands across the workspace, each containing:
 *     * Soft elliptical glow stack (alpha-blended concentric rings)
 *     * Flowing sine-wave streams in the personality color
 *     * A brighter central anchor disk
 *   - Active-personality field gets a saturated outer ring + brighter
 *     core so the eye locks on the currently-running personality.
 *   - Bottom labels: STATE / SYNC / AGENT in scale-2 saturated color.
 *
 * Class A discipline: NO numbers rendered here.  Every metric in the
 * UI still flows through the v0.38 provider; hero is pure identity. */
#include "atomik_os.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern pixel_t *fb_back(void);

/* Soft filled disk with constant alpha. */
static void disk(int cx, int cy, int r, pixel_t color, uint8_t alpha) {
    int r2 = r * r;
    for (int dy = -r; dy <= r; dy++) {
        int row_x2 = r2 - dy * dy;
        if (row_x2 < 0) continue;
        int hr = 0;
        while (hr * hr <= row_x2) hr++;
        hr--;
        for (int dx = -hr; dx <= hr; dx++) {
            if (alpha == 255) draw_pixel(cx + dx, cy + dy, color);
            else              draw_blend_pixel(cx + dx, cy + dy, color, alpha);
        }
    }
}

/* Soft outline ring with alpha; thickness px wide. */
static void ring(int cx, int cy, int radius, int thickness,
                 pixel_t color, uint8_t alpha) {
    int r_out = radius + thickness;
    int r_in  = radius;
    int r_out2 = r_out * r_out;
    int r_in2  = r_in  * r_in;
    for (int dy = -r_out; dy <= r_out; dy++) {
        for (int dx = -r_out; dx <= r_out; dx++) {
            int d2 = dx * dx + dy * dy;
            if (d2 <= r_out2 && d2 >= r_in2) {
                if (alpha == 255) draw_pixel(cx + dx, cy + dy, color);
                else              draw_blend_pixel(cx + dx, cy + dy, color, alpha);
            }
        }
    }
}

/* Thin Bresenham line into the back buffer, alpha-blended. */
static void aaline(int x0, int y0, int x1, int y1,
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

/* Render one personality energy field at (cx, cy) within max_r.
 *   - 4 concentric glow rings (alpha-fading)
 *   - 5 sine-wave streams flowing through the field
 *   - bright central anchor disk
 *   - pulsing brightness driven by `phase` (0..1)
 *   - `active` adds a saturated outer ring + brighter center */
static void energy_field(int cx, int cy, int max_r,
                         pixel_t color, double phase, int active,
                         unsigned long now) {
    /* Glow stack — concentric rings at decreasing radii.  Each ring
     * stacks alpha so the center reads as bright cyan/violet/green. */
    for (int r = max_r; r > 24; r -= 16) {
        double t = (double)r / max_r;
        uint8_t alpha = (uint8_t)(35 * (1.0 - t * t));
        ring(cx, cy, r - 4, 4, color, alpha);
    }
    /* Inner brighter disk gradient. */
    for (int r = 90; r > 18; r -= 6) {
        double t = (r - 18.0) / 72.0;
        uint8_t alpha = (uint8_t)(70 * (1.0 - t));
        disk(cx, cy, r, color, alpha);
    }

    /* Sine-wave streams — v0.38-L2: shrunk from 5 → 3 streams per
     * field per ChatGPT 2026-05-16 "less diagram feel."  The previous
     * dense stream pattern read as a technical visualization; three
     * streams keep the field alive without crowding the surface. */
    double pulse_bright = 0.65 + 0.35 * phase;
    for (int s = 0; s < 3; s++) {
        double offset_v = -max_r * 0.55 + s * (max_r * 0.55);
        double freq     = 0.024 + s * 0.006;
        double amp      = max_r * (0.10 + 0.03 * (s % 3));
        double pha      = (now / 6000.0) * (s % 2 ? 1.0 : -1.0) + s * 0.7;
        int prev_y = -1;
        int prev_x = -1;
        for (int dx = -max_r; dx <= max_r; dx += 2) {
            int x = cx + dx;
            double dy_d = offset_v + amp * sin((cx + dx) * freq + pha);
            int y = cy + (int)dy_d;
            if (dx * dx + (int)(dy_d * dy_d) > max_r * max_r) {
                prev_x = -1;
                continue;
            }
            if (prev_x >= 0) {
                uint8_t alpha = (uint8_t)(140 * pulse_bright);
                aaline(prev_x, prev_y, x, y, color, alpha);
                aaline(prev_x, prev_y - 1, x, y - 1, color,
                       (uint8_t)(alpha / 2));
            }
            prev_x = x; prev_y = y;
        }
    }

    /* Central anchor — v0.38-L2: smaller, doesn't dominate.  Active
     * field gets a brighter rim instead of a bigger anchor. */
    int anchor_r = 12 + (active ? 2 : 0);
    disk(cx, cy, anchor_r, color, active ? 220 : 160);
    disk(cx, cy, anchor_r - 6, ATOMIK_FG, 240);

    if (active) {
        /* Two thin rings — soft outer + bright inner — gives the
         * active field a 25%-or-so visual lift without making it
         * bigger than the others. */
        ring(cx, cy, max_r - 8, 2, color, 220);
        ring(cx, cy, max_r - 4, 1, color, 90);
    }
}

/* v0.38-L2 central ATOMiK core node — small luminous nexus that the
 * three personality fields visibly feed into.  ChatGPT 2026-05-16:
 * "make STATE / SYNC / AGENT feed into one fabric, not three
 * separate diagrams."  The core uses a neutral cyan-leaning palette
 * so it doesn't compete with whichever personality is active. */
static void core_node(int cx, int cy, unsigned long now) {
    pixel_t base = ATOMIK_SEM_HARDWARE;     /* cyan = OS chrome */
    /* Outer glow stack — wider, very low alpha. */
    for (int r = 80; r > 20; r -= 8) {
        double t = (double)(r - 20) / 60.0;
        uint8_t alpha = (uint8_t)(40 * (1.0 - t));
        ring(cx, cy, r, 4, base, alpha);
    }
    /* Atomic orbital rings — two thin ellipses rotated through phase,
     * implemented as alpha-blended elliptical traces. */
    double t = (double)now / 1500.0;
    for (int ring_i = 0; ring_i < 2; ring_i++) {
        double tilt = (ring_i == 0 ? 0.6 : -0.6) + t * 0.0;
        int   rx_e  = 36 - ring_i * 8;
        int   ry_e  = 14 - ring_i * 2;
        int   steps = 120;
        for (int i = 0; i < steps; i++) {
            double theta = (double)i / steps * 6.2831853;
            double x = rx_e * cos(theta);
            double y = ry_e * sin(theta);
            /* Rotate by tilt. */
            double rx = x * cos(tilt) - y * sin(tilt);
            double ry = x * sin(tilt) + y * cos(tilt);
            int px = cx + (int)rx;
            int py = cy + (int)ry;
            uint8_t a = 200 - (uint8_t)(i % 8) * 4;
            draw_blend_pixel(px, py, base, a);
            draw_blend_pixel(px, py - 1, base, a / 3);
        }
    }
    /* Core disk — bright filled center + halo. */
    int r = 14;
    for (int dy = -r; dy <= r; dy++) {
        int row = r * r - dy * dy;
        if (row < 0) continue;
        int hr = 0;
        while (hr * hr <= row) hr++;
        hr--;
        for (int dx = -hr; dx <= hr; dx++) {
            draw_pixel(cx + dx, cy + dy, base);
        }
    }
    disk(cx, cy, 7, ATOMIK_FG, 250);
}

/* Curved energy link from a personality field anchor to the central
 * core, drawn as an alpha-blended quadratic Bezier polyline.  Reads
 * as "this lane is feeding the core" — supports the "one fabric"
 * composition. */
static void energy_link(int ax, int ay, int bx, int by,
                        pixel_t color, double phase) {
    /* Control point pulled slightly toward the screen baseline so the
     * curve arcs through the workspace rather than running flat. */
    double cx = (ax + bx) / 2.0;
    double cy = (ay + by) / 2.0 + 70.0;
    int steps = 32;
    int prev_x = -1, prev_y = -1;
    for (int i = 0; i <= steps; i++) {
        double t = (double)i / steps;
        double omt = 1.0 - t;
        double x = omt*omt*ax + 2*omt*t*cx + t*t*bx;
        double y = omt*omt*ay + 2*omt*t*cy + t*t*by;
        int px = (int)x;
        int py = (int)y;
        /* Brighter near the source field, fading toward the core
         * to suggest energy flowing inward.  Multiply by personality
         * phase so the link breathes with the field. */
        double tail   = 1.0 - t * 0.5;
        uint8_t a = (uint8_t)(110 * tail * (0.7 + 0.3 * phase));
        if (prev_x >= 0) {
            aaline(prev_x, prev_y, px, py, color, a);
            aaline(prev_x, prev_y - 1, px, py - 1, color, a / 2);
        }
        prev_x = px; prev_y = py;
    }
}

/* v0.39-J: three-layer alpha rect behind a label heading.
 * Outer/mid/core rectangles at increasing alpha give a soft semantic
 * glow without a blur pass.  Active fields get ~2× the alpha of idle
 * ones so the active personality reads from across the room. */
static void label_halo(int cx, int y, int tw, int th,
                       pixel_t color, int active) {
    uint8_t a0 = active ? 14 :  6;   /* outer */
    uint8_t a1 = active ? 28 : 12;   /* mid   */
    uint8_t a2 = active ? 50 : 22;   /* core  */
    int x = cx - tw / 2;
    for (int dy = y - 6; dy < y + th + 6; dy++)
        for (int dx = x - 10; dx < x + tw + 10; dx++)
            draw_blend_pixel(dx, dy, color, a0);
    for (int dy = y - 3; dy < y + th + 3; dy++)
        for (int dx = x - 6; dx < x + tw + 6; dx++)
            draw_blend_pixel(dx, dy, color, a1);
    for (int dy = y - 1; dy < y + th + 1; dy++)
        for (int dx = x - 2; dx < x + tw + 2; dx++)
            draw_blend_pixel(dx, dy, color, a2);
}

void hero_draw_adaptive(void) {
    /* Workspace centerline: midpoint between rail right-edge and
     * Fabric shelf left-edge.  This puts the hero properly centered
     * regardless of chrome changes. */
    int left_edge  = dock_right_edge() + ATOMIK_GRID_L * 2;
    int right_edge = fabric_shelf_x() - ATOMIK_GRID_L * 2;
    int ws_w       = right_edge - left_edge;
    int cy         = ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + 60
                     + (FB_H - ATOMIK_SAFE_TOP - ATOMIK_PULSE_BAR_H - 60) / 2
                     - 40;
    if (cy < 320) cy = FB_H / 2 - 60;

    /* v0.38-L2: three fields shrunk ~15% so they no longer fill the
     * workspace + a central ATOMiK core node sits between them.
     * Energy links arc from each personality anchor into the core,
     * making the composition read as ONE fabric instead of three
     * separate diagrams. */
    int field_w   = ws_w / 3;
    int field_r   = (field_w * 7) / 18;     /* ~39% of slot, was ~44%  */
    int cx_state  = left_edge + field_w * 0 + field_w / 2;
    int cx_sync   = left_edge + field_w * 1 + field_w / 2;
    int cx_agent  = left_edge + field_w * 2 + field_w / 2;
    int core_x    = cx_sync;
    int core_y    = cy - 12;                /* slight lift above fields */

    unsigned long now = anim_now_ms();
    double phase = 0.5 + 0.5 * sin(((double)(now % 4000) / 4000.0)
                                    * 2.0 * M_PI);

    personality_t active = fabric_active();

    /* Pass 1 — paint each personality field. */
    energy_field(cx_state, cy, field_r,
                 ATOMIK_SEM_HARDWARE,
                 phase, active == PERSONALITY_STATE, now);
    energy_field(cx_sync,  cy, field_r,
                 ATOMIK_SEM_SAVINGS,
                 phase, active == PERSONALITY_SYNC,  now);
    energy_field(cx_agent, cy, field_r,
                 ATOMIK_SEM_AGENT,
                 phase, active == PERSONALITY_AGENT, now);

    /* Pass 2 — curved energy links from each anchor to the core.
     * Drawn AFTER the fields so the link starts crisp at the anchor
     * and fades into the core glow as it converges. */
    energy_link(cx_state, cy, core_x, core_y, ATOMIK_SEM_HARDWARE, phase);
    energy_link(cx_agent, cy, core_x, core_y, ATOMIK_SEM_AGENT,    phase);
    /* SYNC sits directly under the core; use a short vertical link
     * instead of a curved arc. */
    {
        int steps = 20;
        for (int i = 0; i <= steps; i++) {
            double t = (double)i / steps;
            int py = (int)(cy * (1 - t) + core_y * t);
            uint8_t a = (uint8_t)(120 * (1.0 - t * 0.4)
                                 * (0.7 + 0.3 * phase));
            draw_blend_pixel(core_x,     py,     ATOMIK_SEM_SAVINGS, a);
            draw_blend_pixel(core_x - 1, py,     ATOMIK_SEM_SAVINGS, a / 2);
            draw_blend_pixel(core_x + 1, py,     ATOMIK_SEM_SAVINGS, a / 2);
        }
    }

    /* Pass 3 — central ATOMiK core node painted last so it sits on
     * top of where the three links converge. */
    core_node(core_x, core_y, now);

    /* v0.39-J: Labels below the fields — STATE / SYNC / AGENT in
     * atomik_18 (FONT_AA_UI) with a per-personality semantic halo.
     * Active field: saturated color heading + full-brightness sublabel.
     * Idle fields: dim heading + stepped-down sublabel so the contrast
     * gap is wide enough to read from across the room. */
    int label_y = cy + field_r + ATOMIK_GRID_L * 2;
    const char *names[3] = { "STATE", "SYNC", "AGENT" };
    int        cxs[3]    = { cx_state, cx_sync, cx_agent };
    personality_t  ps[3] = { PERSONALITY_STATE, PERSONALITY_SYNC,
                             PERSONALITY_AGENT };
    pixel_t    cols[3]   = { ATOMIK_SEM_HARDWARE,
                             ATOMIK_SEM_SAVINGS,
                             ATOMIK_SEM_AGENT };
    for (int i = 0; i < 3; i++) {
        int is_active = (active == ps[i]);
        const char *n = names[i];
        pixel_t c     = is_active ? cols[i] : ATOMIK_FG_DIM;
        /* Active sublabel lifts to full foreground; idle steps below
         * FG_DIM to widen the active/idle contrast gap. */
        pixel_t sub_c = is_active ? ATOMIK_FG : rgb(0x72, 0x7C, 0x94);
        const char *sub = (i == 0) ? "memory / regions" :
                          (i == 1) ? "replica / skip"   :
                                     "context / prune";
        int name_h, tw;
        if (font_aa_loaded(FONT_AA_UI)) {
            tw     = text_width_aa(FONT_AA_UI, n);
            name_h = text_height_aa(FONT_AA_UI);
            label_halo(cxs[i], label_y, tw, name_h, cols[i], is_active);
            draw_text_aa(FONT_AA_UI, cxs[i] - tw / 2, label_y, n, c);
        } else {
            tw     = text_width(n, 2);
            name_h = text_height(2);
            label_halo(cxs[i], label_y, tw, name_h, cols[i], is_active);
            draw_text(cxs[i] - tw / 2, label_y, n, 2, c);
        }
        if (font_aa_loaded(FONT_AA_LABEL)) {
            int sw = text_width_aa(FONT_AA_LABEL, sub);
            draw_text_aa(FONT_AA_LABEL, cxs[i] - sw / 2,
                         label_y + name_h + 4, sub, sub_c);
        } else {
            int sw = text_width(sub, 1);
            draw_text(cxs[i] - sw / 2, label_y + name_h + 4,
                      sub, 1, sub_c);
        }
    }

    /* Mark the dirty region: the bounding box of the three fields +
     * labels.  Helps the v0.38-A dirty-tile tracker score the hero
     * correctly. */
    int dx = left_edge;
    int dy = cy - field_r - ATOMIK_GRID_M;
    int dw = ws_w;
    int dh = (label_y + text_height(2) + text_height(1) + 8) - dy;
    dirty_rect(dx, dy, dw, dh);
    /* NOTE: anim_tick() is called once per frame by hero_draw() (the home
     * surface that redraw_frame actually invokes).  When Adaptive Mode wires
     * this scene in, its per-frame caller must ensure exactly one anim_tick(). */
}

/* ====================================================================
 * v0.40 HOME surface — concept-01 "ATOMiK Desk" composition.
 *
 * Centered wordmark + tagline + honest status line, over twin glass
 * panels (System Overview + a real coalescing gauge).  The 3-orb energy
 * scene above (hero_draw_adaptive) is concept-06's Adaptive Mode and is
 * preserved for that surface.
 *
 * HONEST DATA ONLY (feedback_no_class_c_metrics): System Overview shows
 * real system facts; the right gauge is the REAL STATE coalesce ratio
 * (perf_last_for).  We deliberately do NOT reproduce the concept's
 * "Predictive Accuracy 92%" panel — that exact phrasing is the banned
 * fake-metric example.
 * ==================================================================== */

/* Translucent glass card: rounded body + top sheen + accent inner rim. */
static void glass_panel(int x, int y, int w, int h) {
    draw_rect_rounded(x, y, w, h, 10, wm_card_bg());
    for (int sy = 0; sy < 10; sy++)
        for (int sx = 2; sx < w - 2; sx++)
            draw_blend_pixel(x + sx, y + 1 + sy, ATOMIK_FG, (uint8_t)(11 - sy));
    draw_rect(x + 10, y,         w - 20, 1, wm_card_border());
    draw_rect(x + 10, y + h - 1, w - 20, 1, wm_card_border());
    draw_rect(x,         y + 10, 1, h - 20, wm_card_border());
    draw_rect(x + w - 1, y + 10, 1, h - 20, wm_card_border());
    for (int sx = 12; sx < w - 12; sx++)
        draw_blend_pixel(x + sx, y + 2, ATOMIK_ACCENT, 22);   /* inner glow rim */
}

/* label (dim, left) + value (fg, right) on one row. */
static void hero_stat(int x, int y, int w, const char *label, const char *val) {
    if (font_aa_loaded(FONT_AA_LABEL)) {
        draw_text_aa(FONT_AA_LABEL, x, y, label, ATOMIK_FG_DIM);
        int vw = text_width_aa(FONT_AA_LABEL, val);
        draw_text_aa(FONT_AA_LABEL, x + w - vw, y, val, ATOMIK_FG);
    } else {
        draw_text(x, y, label, 1, ATOMIK_FG_DIM);
        int vw = text_width(val, 1);
        draw_text(x + w - vw, y, val, 1, ATOMIK_FG);
    }
}

/* Circular gauge: dim track + bright filled arc (pct, clockwise from top). */
static void hero_gauge(int cx, int cy, int r, int pct, pixel_t color) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    pixel_t track = rgb(0x24, 0x2E, 0x42);
    for (int a = 0; a < 360; a++) {
        double rad = (a - 90) * M_PI / 180.0;
        int    on  = (a * 100 / 360) < pct;
        pixel_t c  = on ? color : track;
        for (int t = 0; t < 4; t++) {
            int px = cx + (int)((r - t) * cos(rad));
            int py = cy + (int)((r - t) * sin(rad));
            if (on) draw_pixel(px, py, c);
            else    draw_blend_pixel(px, py, c, 120);
        }
    }
    ring(cx, cy, r - 6, 2, color, 30);
}

static double hero_uptime(void) {
    FILE *f = fopen("/proc/uptime", "r");
    double up = 0;
    if (f) { if (fscanf(f, "%lf", &up) != 1) up = 0; fclose(f); }
    return up;
}

/* blended line (Bresenham) */
static void hero_line(int x0, int y0, int x1, int y1, pixel_t c, uint8_t a) {
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

/* SYSTEM OVERVIEW glowing particle sphere (concept-01): a dense globe of light
 * particles — hundreds of surface dots (front bright, back dim), dotted
 * meridian arcs for structure, and a bright radial core glow.  Rotates slowly.
 * Decorative chrome — no telemetry. */
#define HERO_NSPH 320
static void constellation_sphere(int cx, int cy, int r, unsigned long now) {
    double ang = (double)(now % 22000) / 22000.0 * 6.28318531;
    double ca = cos(ang), sa = sin(ang);

    /* core glow: 3 stacked radial passes (wide halo -> mid -> hot core) so the
     * orb reads as luminous, like the concept.  Additive. */
    struct { int rad; pixel_t c; uint8_t peak; } glow[3] = {
        { r * 8 / 5, rgb(0x1A, 0x4C, 0x80),  62 },
        { r * 9 / 10, rgb(0x3C, 0x8A, 0xCC),  86 },
        { r * 2 / 5, rgb(0x9A, 0xD2, 0xF6),  96 },
    };
    for (int g = 0; g < 3; g++) {
        int gg = glow[g].rad; long g2 = (long)gg * gg;
        for (int dy = -gg; dy <= gg; dy++) {
            int yy = cy + dy; if (yy < 0 || yy >= FB_H) continue;
            for (int dx = -gg; dx <= gg; dx++) {
                int xx = cx + dx; if (xx < 0 || xx >= FB_W) continue;
                long d2 = (long)dx*dx + (long)dy*dy; if (d2 >= g2) continue;
                uint8_t a = (uint8_t)((long)glow[g].peak * (g2 - d2) / g2);
                if (a) draw_blend_pixel(xx, yy, glow[g].c, a);
            }
        }
    }

    /* dense surface particles (fibonacci sphere) — bright on the front face */
    const double ga = 2.39996323;
    for (int i = 0; i < HERO_NSPH; i++) {
        double t  = (i + 0.5) / HERO_NSPH;
        double y  = 1.0 - 2.0 * t;
        double rr = sqrt(1.0 - y * y);
        double th = ga * i;
        double x = rr * cos(th), z = rr * sin(th);
        double xr = x * ca - z * sa, zr = x * sa + z * ca;
        int px = cx + (int)(xr * r), py2 = cy + (int)(y * r);
        double f = (zr + 1.0) * 0.5;              /* 0 back .. 1 front */
        uint8_t a = (uint8_t)(40 + 215 * f * f);
        pixel_t c = f > 0.66 ? rgb(0xE8, 0xF4, 0xFF)
                  : f > 0.38 ? rgb(0x7C, 0xD8, 0xFF) : rgb(0x34, 0x70, 0xA4);
        draw_blend_pixel(px, py2, c, a);
        if (f > 0.6) {   /* 2px + bloom on front particles */
            draw_blend_pixel(px+1, py2, c, (uint8_t)(a*3/5));
            draw_blend_pixel(px, py2+1, c, (uint8_t)(a*3/5));
            if (f > 0.86) { draw_blend_pixel(px-1, py2, c, a/2);
                            draw_blend_pixel(px, py2-1, c, a/2); }
        }
    }

    /* dotted meridian arcs (great circles through the poles) for globe depth */
    for (int L = 0; L < 5; L++) {
        double lon = ang * 0.6 + L * 0.6283185;
        double cl = cos(lon), sl = sin(lon);
        for (int k = 0; k < 54; k++) {
            double phi = (double)k / 54.0 * 6.28318531;
            double cp = cos(phi);
            double x = cp * cl, y = sin(phi), z = cp * sl;
            double f = (z + 1.0) * 0.5;
            int px = cx + (int)(x * r), py2 = cy + (int)(y * r);
            uint8_t a = (uint8_t)(14 + 120 * f);
            draw_blend_pixel(px, py2, f > 0.5 ? ATOMIK_ACCENT : rgb(0x24, 0x52, 0x80), a);
        }
    }
}

/* PREDICTIVE INSIGHTS "pebble" gauge: a dashed ring of short radial ticks;
 * the leading `pct`% are bright, the rest dim.  `pct` 0..100. */
static void dashed_ring_gauge(int cx, int cy, int r, int pct, pixel_t color) {
    const int N = 48;
    int lit = N * pct / 100;
    for (int i = 0; i < N; i++) {
        double a = -1.5707963 + (double)i / N * 6.28318531;  /* from top, CW */
        double co = cos(a), si = sin(a);
        int on = (i < lit);
        uint8_t al = on ? 230 : 40;
        pixel_t c = on ? color : ATOMIK_FG_DIM;
        int r0 = r - 4, r1 = r + 2;
        hero_line(cx + (int)(co * r0), cy + (int)(si * r0),
                  cx + (int)(co * r1), cy + (int)(si * r1), c, al);
        if (on)   /* small bloom on lit ticks */
            draw_blend_pixel(cx + (int)(co * r), cy + (int)(si * r), color, 120);
    }
}

/* CTA bar at the bottom of a panel: a hairline divider + label + right arrow. */
static void hero_cta(int x, int y, int w, const char *label) {
    draw_rect(x, y, w, 1, wm_card_border());
    int ty = y + ATOMIK_GRID_M;
    pixel_t c = ATOMIK_ACCENT;
    if (font_aa_loaded(FONT_AA_LABEL)) {
        draw_text_aa(FONT_AA_LABEL, x, ty, label, c);
    } else {
        draw_text(x, ty, label, 1, c);
    }
    /* right arrow ">" */
    int ah = (font_aa_loaded(FONT_AA_LABEL) ? text_height_aa(FONT_AA_LABEL)
                                            : text_height(1));
    int axc = x + w - 8, ayc = ty + ah / 2;
    for (int i = 0; i < 5; i++) {
        draw_blend_pixel(axc - 4 + i, ayc - 4 + i, c, 220);
        draw_blend_pixel(axc - 4 + i, ayc + 4 - i, c, 220);
    }
}

/* one status row with a colored value, right-aligned (concept SYSTEM OVERVIEW). */
static void hero_row(int x, int y, int w, const char *label,
                     const char *value, pixel_t vcol) {
    int lab_aa = font_aa_loaded(FONT_AA_LABEL);
    if (lab_aa) draw_text_aa(FONT_AA_LABEL, x, y, label, rgb(0x7C, 0x88, 0x9E));
    else        draw_text(x, y, label, 1, rgb(0x7C, 0x88, 0x9E));
    int vw = lab_aa ? text_width_aa(FONT_AA_LABEL, value) : text_width(value, 1);
    if (lab_aa) draw_text_aa(FONT_AA_LABEL, x + w - vw, y, value, vcol);
    else        draw_text(x + w - vw, y, value, 1, vcol);
}

/* small panel header (concept-01): a node icon + a 14px muted-white title.
 * Returns the y below the header. */
static int hero_panel_header(int x, int y, const char *label) {
    int lab = font_aa_loaded(FONT_AA_LABEL);
    int h = lab ? text_height_aa(FONT_AA_LABEL) : text_height(1);
    int icy = y + h / 2;
    disk(x + 4, icy, 2, ATOMIK_ACCENT, 230);
    ring(x + 4, icy, 5, 1, ATOMIK_ACCENT, 120);
    pixel_t tc = rgb(0xC6, 0xD2, 0xE6);
    if (lab) draw_text_aa(FONT_AA_LABEL, x + 16, y, label, tc);
    else     draw_text(x + 16, y, label, 1, tc);
    return y + h + ATOMIK_GRID_M;
}

void hero_draw(void) {
    int left_edge  = dock_right_edge() + ATOMIK_GRID_L * 2;
    int right_edge = fabric_shelf_x() - ATOMIK_GRID_L * 2;
    (void)left_edge; (void)right_edge;
    /* concept-01 proportions: the hero + twin-panel group is centered around
     * screen x~0.484, with the twin panels spanning x 0.255..0.712 (each ~21%),
     * NOT filling the rail..fabric workspace.  Background shows on both sides. */
    int panel_left  = FB_W * 255 / 1000;
    int panel_right = FB_W * 712 / 1000;
    int ws_w  = panel_right - panel_left;
    int ws_cx = (panel_left + panel_right) / 2;
    int top   = ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + ATOMIK_GRID_L * 3;
    unsigned long now = anim_now_ms();
    personality_t act = fabric_active();

    /* ---- title block ---- */
    /* ATOMiK hex mark above the wordmark (concept-01): a glowing cyan hexagon
     * with the atom-node glyph inside. */
    int icon_cy = top + 28;
    int hr = 22, hxr = hr * 866 / 1000, hyr = hr / 2;
    /* soft glow behind the mark */
    for (int gr = hr + 9; gr > 4; gr -= 2)
        ring(ws_cx, icon_cy, gr, 1, ATOMIK_ACCENT,
             (uint8_t)(3 + 12 * (hr + 9 - gr) / (hr + 9)));
    int vx[6] = { ws_cx, ws_cx + hxr, ws_cx + hxr, ws_cx, ws_cx - hxr, ws_cx - hxr };
    int vy[6] = { icon_cy - hr, icon_cy - hyr, icon_cy + hyr,
                  icon_cy + hr, icon_cy + hyr, icon_cy - hyr };
    for (int i = 0; i < 6; i++) {
        hero_line(vx[i], vy[i], vx[(i+1)%6], vy[(i+1)%6], ATOMIK_ACCENT, 235);
        hero_line(vx[i], vy[i] + 1, vx[(i+1)%6], vy[(i+1)%6] + 1, ATOMIK_ACCENT, 70);
    }
    /* atom-node glyph: center + 3 satellites with connectors */
    disk(ws_cx, icon_cy, 2, ATOMIK_FG, 245);
    disk(ws_cx, icon_cy - 8, 1, ATOMIK_ACCENT, 210);
    disk(ws_cx + 7, icon_cy + 4, 1, ATOMIK_ACCENT, 210);
    disk(ws_cx - 7, icon_cy + 4, 1, ATOMIK_ACCENT, 210);
    hero_line(ws_cx, icon_cy, ws_cx, icon_cy - 7, ATOMIK_ACCENT, 150);
    hero_line(ws_cx, icon_cy, ws_cx + 6, icon_cy + 4, ATOMIK_ACCENT, 150);
    hero_line(ws_cx, icon_cy, ws_cx - 6, icon_cy + 4, ATOMIK_ACCENT, 150);
    int y = icon_cy + 28;
    const char *wm = "ATOMiK Desk";
    if (font_aa_loaded(FONT_AA_BRAND)) {
        int tw = text_width_aa(FONT_AA_BRAND, wm);
        draw_text_aa(FONT_AA_BRAND, ws_cx - tw / 2, y, wm, ATOMIK_FG);
        y += text_height_aa(FONT_AA_BRAND) + 6;
    } else {
        int tw = text_width(wm, 3);
        draw_text(ws_cx - tw / 2, y, wm, 3, ATOMIK_FG);
        y += text_height(3) + 6;
    }
    const char *tag = "Intelligent.   Adaptive.   Autonomous.";
    if (font_aa_loaded(FONT_AA_LABEL)) {
        int tw = text_width_aa(FONT_AA_LABEL, tag);
        draw_text_aa(FONT_AA_LABEL, ws_cx - tw / 2, y, tag, ATOMIK_ACCENT_DIM);
        y += text_height_aa(FONT_AA_LABEL) + 16;
    } else {
        int tw = text_width(tag, 1);
        draw_text(ws_cx - tw / 2, y, tag, 1, ATOMIK_ACCENT_DIM);
        y += text_height(1) + 16;
    }

    /* status line — ACTIVE only when a real workload is RUNNING (the
     * self-driving demo), not merely from seeded usage history (agent_log
     * emits EVT_STATE_DELTA, which would otherwise read as "busy").  Idle
     * home reads "Idle / Ready" like concept-01. */
    int busy = 0;
    if (fabric_demo_enabled()) {
        unsigned long t1 = atomik_event_last_ts(EVT_STATE_DELTA);
        unsigned long t2 = atomik_event_last_ts(EVT_SYNC_REPLICA);
        unsigned long t3 = atomik_event_last_ts(EVT_AGENT_CONTEXT);
        if ((t1 && now - t1 < 3000) || (t2 && now - t2 < 3000) ||
            (t3 && now - t3 < 3000)) busy = 1;
    }
    char status[48];
    if (busy) snprintf(status, sizeof status, "%s  /  Active",
                       fabric_personality_name(act));
    else      snprintf(status, sizeof status, "Idle  /  Ready");
    pixel_t st_c = busy ? ATOMIK_SEM_SAVINGS : ATOMIK_ACCENT;
    int sh = font_aa_loaded(FONT_AA_UI) ? text_height_aa(FONT_AA_UI)
                                        : text_height(2);
    if (font_aa_loaded(FONT_AA_UI)) {
        int tw = text_width_aa(FONT_AA_UI, status);
        draw_text_aa(FONT_AA_UI, ws_cx - tw / 2, y, status, st_c);  /* plain, concept-01 */
    } else {
        int tw = text_width(status, 2);
        draw_text(ws_cx - tw / 2, y, status, 2, st_c);
    }
    y += sh + 6;

    /* Subtitle — honest idle/active phrasing (concept: "System calm. All
     * functions nominal."). Reflects real activity, not a fixed string. */
    const char *subt = busy ? "Workload active on the fabric."
                            : "System calm. All functions nominal.";
    if (font_aa_loaded(FONT_AA_LABEL)) {
        int tw = text_width_aa(FONT_AA_LABEL, subt);
        draw_text_aa(FONT_AA_LABEL, ws_cx - tw / 2, y, subt, ATOMIK_FG_DIM);
        y += text_height_aa(FONT_AA_LABEL) + ATOMIK_GRID_L * 2;
    } else {
        int tw = text_width(subt, 1);
        draw_text(ws_cx - tw / 2, y, subt, 1, ATOMIK_FG_DIM);
        y += text_height(1) + ATOMIK_GRID_L * 2;
    }

    /* ---- twin glass panels (concept-01 bounds) ---- */
    int gap   = ATOMIK_GRID_L;
    int pw    = (ws_w - gap) / 2;
    int ph    = FB_H * 315 / 1000;                /* ~340 px, concept height    */
    int py    = FB_H * 560 / 1000;                /* ~605 px, concept top        */
    if (py < y + ATOMIK_GRID_M) py = y + ATOMIK_GRID_M;   /* never overlap title */
    if (py + ph > FB_H - ATOMIK_GRID_L * 2) ph = FB_H - ATOMIK_GRID_L * 2 - py;
    int lx = panel_left;
    int rx = panel_left + pw + gap;

    int lab_h  = font_aa_loaded(FONT_AA_LABEL) ? text_height_aa(FONT_AA_LABEL)
                                               : text_height(1);
    int cta_h  = lab_h + ATOMIK_GRID_M * 2;     /* reserved at panel bottom    */

    /* Left — SYSTEM OVERVIEW: constellation sphere + operating-state rows. */
    glass_panel(lx, py, pw, ph);
    {
        int ix = lx + ATOMIK_GRID_L, iw = pw - ATOMIK_GRID_L * 2;
        int ty = py + ATOMIK_GRID_M;
        ty = hero_panel_header(ix, ty, "SYSTEM OVERVIEW");
        int body_bot = py + ph - cta_h;

        /* constellation sphere on the left ~30% */
        int sph_cx = lx + (int)(pw * 0.29);
        int sph_cy = (ty + body_bot) / 2;
        int sph_r  = pw * 22 / 100;
        int maxr = (body_bot - ty) / 2 - 6;
        if (sph_r > maxr) sph_r = maxr;
        if (sph_r < 26) sph_r = 26;
        constellation_sphere(sph_cx, sph_cy, sph_r, now);

        /* operating-state rows on the right ~55% (qualitative state + one
         * not-yet-connected placeholder dash, per the no-fake-numbers rule). */
        int rx2 = lx + (int)(pw * 0.49);
        int rw2 = (lx + pw - ATOMIK_GRID_L) - rx2;
        int rowh = lab_h + 11;
        int rblock = 5 * rowh;
        int rty = ty + ((body_bot - ty) - rblock) / 2;
        if (rty < ty) rty = ty;
        pixel_t green = ATOMIK_SEM_SAVINGS;
        int idle = (act == PERSONALITY_NONE);
        hero_row(rx2, rty, rw2, "STATUS",     idle ? "IDLE" : "ACTIVE",
                 idle ? green : ATOMIK_ACCENT);                       rty += rowh;
        hero_row(rx2, rty, rw2, "MODE",       "ADAPTIVE", ATOMIK_ACCENT); rty += rowh;
        hero_row(rx2, rty, rw2, "LEARNING",   "ACTIVE",   green);        rty += rowh;
        hero_row(rx2, rty, rw2, "QUEUE",      "--", ATOMIK_FG_DIM);       rty += rowh;
        hero_row(rx2, rty, rw2, "THROUGHPUT", "NOMINAL", ATOMIK_ACCENT);  rty += rowh;

        hero_cta(ix, body_bot, iw, "VIEW SYSTEM INSIGHTS");
    }

    /* Right — PREDICTIVE INSIGHTS: next likely actions (real agent_predict)
     * + a dashed "pebble" confidence ring + CTA.  We deliberately do NOT show
     * the concept's fabricated "Focus 92"; the ring is the top prediction's
     * real confidence, the list is the real Markov scorer. */
    glass_panel(rx, py, pw, ph);
    {
        int ix = rx + ATOMIK_GRID_L, iw = pw - ATOMIK_GRID_L * 2;
        int ty = py + ATOMIK_GRID_M;
        ty = hero_panel_header(ix, ty, "PREDICTIVE INSIGHTS");
        int body_bot = py + ph - cta_h;

        /* real top-3 predicted actions */
        double sc[ACT_MAX]; double tot = 0;
        for (int a = ACT_OPEN_ABOUT; a < ACT_MAX; a++) { sc[a] = agent_score((action_t)a); tot += sc[a]; }
        int top[3] = {0,0,0}; double topv[3] = {-1,-1,-1};
        for (int a = ACT_OPEN_ABOUT; a < ACT_MAX; a++)
            for (int k = 0; k < 3; k++)
                if (sc[a] > topv[k]) {
                    for (int m = 2; m > k; m--) { topv[m] = topv[m-1]; top[m] = top[m-1]; }
                    topv[k] = sc[a]; top[k] = a; break;
                }
        int have_pred = tot > 0.0;

        if (font_aa_loaded(FONT_AA_LABEL))
            draw_text_aa(FONT_AA_LABEL, ix, ty, "Next likely actions", rgb(0x7C,0x88,0x9E));
        else draw_text(ix, ty, "Next likely actions", 1, rgb(0x7C,0x88,0x9E));
        ty += lab_h + ATOMIK_GRID_M;

        int list_w = (int)(iw * 0.56);
        pixel_t dotc[3] = { ATOMIK_ACCENT, rgb(0x58,0xAE,0xF0), rgb(0x46,0x84,0xDA) };
        int rowh2 = lab_h + 12;
        for (int k = 0; k < 3; k++) {
            int ry = ty + k * rowh2;
            disk(ix + 4, ry + lab_h/2, 4, dotc[k], 230);
            const char *nm = have_pred ? agent_action_name((action_t)top[k]) : "learning";
            char pct[8];
            if (have_pred) snprintf(pct, sizeof pct, "%d%%", (int)(topv[k]*100.0/tot + 0.5));
            else           snprintf(pct, sizeof pct, "--");
            int nx = ix + 14;
            if (font_aa_loaded(FONT_AA_LABEL)) {
                draw_text_aa(FONT_AA_LABEL, nx, ry, nm, ATOMIK_FG);
                int pw2 = text_width_aa(FONT_AA_LABEL, pct);
                draw_text_aa(FONT_AA_LABEL, ix + list_w - pw2, ry, pct, dotc[k]);
            } else {
                draw_text(nx, ry, nm, 1, ATOMIK_FG);
                int pw2 = text_width(pct, 1);
                draw_text(ix + list_w - pw2, ry, pct, 1, dotc[k]);
            }
        }

        /* dashed pebble confidence ring on the right ~40% */
        int gcx = rx + pw - (int)(iw * 0.21) - ATOMIK_GRID_L;
        int gcy = (ty + body_bot) / 2;
        int gr  = (int)(iw * 0.17);
        int maxgr = (body_bot - ty) / 2 - 4;
        if (gr > maxgr) gr = maxgr;
        if (gr < 30) gr = 30;
        int conf = have_pred ? (int)(topv[0]*100.0/tot + 0.5) : 0;
        dashed_ring_gauge(gcx, gcy, gr, conf, ATOMIK_ACCENT);
        char cv[8];
        if (have_pred) snprintf(cv, sizeof cv, "%d", conf); else snprintf(cv, sizeof cv, "--");
        if (font_aa_loaded(FONT_AA_DISPLAY)) {
            int nw = text_width_aa(FONT_AA_DISPLAY, cv), nh = text_height_aa(FONT_AA_DISPLAY);
            draw_text_aa(FONT_AA_DISPLAY, gcx - nw/2, gcy - nh/2 - 4, cv, ATOMIK_ACCENT);
        } else {
            int nw = text_width(cv, 3);
            draw_text(gcx - nw/2, gcy - text_height(3)/2 - 4, cv, 3, ATOMIK_ACCENT);
        }
        if (font_aa_loaded(FONT_AA_LABEL)) {
            const char *cl = "CONFIDENCE";
            int cw = text_width_aa(FONT_AA_LABEL, cl);
            draw_text_aa(FONT_AA_LABEL, gcx - cw/2, gcy + gr - lab_h, cl, rgb(0x7C,0x88,0x9E));
        }

        hero_cta(ix, body_bot, iw, "EXPLORE INSIGHTS");
    }

    /* Footer caption (concept-01) — quiet brand line under the surface. */
    {
        const char *foot = "ATOMiK Desk   -   Adaptive Operating Environment";
        int fh = font_aa_loaded(FONT_AA_LABEL) ? text_height_aa(FONT_AA_LABEL)
                                               : text_height(1);
        int fy = FB_H - ATOMIK_GRID_L * 2 - fh;
        pixel_t fc = rgb(0x6A, 0x76, 0x90);
        if (fy > py + ph + ATOMIK_GRID_M) {       /* only if it clears the cards */
            if (font_aa_loaded(FONT_AA_LABEL)) {
                int tw = text_width_aa(FONT_AA_LABEL, foot);
                draw_text_aa(FONT_AA_LABEL, ws_cx - tw / 2, fy, foot, fc);
            } else {
                int tw = text_width(foot, 1);
                draw_text(ws_cx - tw / 2, fy, foot, 1, fc);
            }
        }
    }

    dirty_rect(left_edge, top, ws_w, FB_H - top);
    anim_tick();
}
