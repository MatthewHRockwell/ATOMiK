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

    /* Sine-wave streams — 5 horizontal flowing curves through the field
     * at different vertical offsets and phases. */
    double pulse_bright = 0.65 + 0.35 * phase;
    for (int s = 0; s < 5; s++) {
        double offset_v = -max_r + (s + 1) * (2.0 * max_r / 6.0);
        double freq     = 0.022 + s * 0.004;
        double amp      = max_r * (0.10 + 0.04 * (s % 3));
        double pha      = (now / 6000.0) * (s % 2 ? 1.0 : -1.0) + s * 0.7;
        int prev_y = -1;
        int prev_x = -1;
        for (int dx = -max_r; dx <= max_r; dx += 2) {
            int x = cx + dx;
            double dy_d = offset_v + amp * sin((cx + dx) * freq + pha);
            int y = cy + (int)dy_d;
            /* Clip to a circular mask so the field stays round. */
            if (dx * dx + (int)(dy_d * dy_d) > max_r * max_r) {
                prev_x = -1;
                continue;
            }
            if (prev_x >= 0) {
                uint8_t alpha = (uint8_t)(160 * pulse_bright);
                aaline(prev_x, prev_y, x, y, color, alpha);
                /* 1px above for thicker stroke at resolution-bump. */
                aaline(prev_x, prev_y - 1, x, y - 1, color,
                       (uint8_t)(alpha / 2));
            }
            prev_x = x; prev_y = y;
        }
    }

    /* Central anchor — bright filled disk + thin outer ring. */
    int anchor_r = 14 + (active ? 4 : 0);
    disk(cx, cy, anchor_r, color, active ? 220 : 160);
    disk(cx, cy, anchor_r - 6, ATOMIK_FG, 240);

    /* Active outer ring — saturated, 3 px thick. */
    if (active) {
        ring(cx, cy, max_r - 6, 3, color, 200);
    }
}

void hero_draw(void) {
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

    /* Three fields across the workspace, equal slots. */
    int field_w   = ws_w / 3;
    int field_r   = (field_w * 4) / 9;     /* radius ≈ 44% of slot width */
    int cx_state  = left_edge + field_w * 0 + field_w / 2;
    int cx_sync   = left_edge + field_w * 1 + field_w / 2;
    int cx_agent  = left_edge + field_w * 2 + field_w / 2;

    /* Animation phase — slow breathing pulse (0..1 across 4 s). */
    unsigned long now = anim_now_ms();
    double phase = 0.5 + 0.5 * sin(((double)(now % 4000) / 4000.0)
                                    * 2.0 * M_PI);

    personality_t active = fabric_active();

    energy_field(cx_state, cy, field_r,
                 ATOMIK_SEM_HARDWARE,                     /* cyan = STATE */
                 phase, active == PERSONALITY_STATE, now);
    energy_field(cx_sync,  cy, field_r,
                 ATOMIK_SEM_SAVINGS,                      /* green = SYNC */
                 phase, active == PERSONALITY_SYNC,  now);
    energy_field(cx_agent, cy, field_r,
                 ATOMIK_SEM_AGENT,                        /* violet = AGENT */
                 phase, active == PERSONALITY_AGENT, now);

    /* Labels below the fields — STATE / SYNC / AGENT in scale-2
     * saturated personality color.  Active personality gets brighter,
     * idle gets dim. */
    int label_y = cy + field_r + ATOMIK_GRID_L * 2;
    const char *names[3] = { "STATE", "SYNC", "AGENT" };
    int        cxs[3]    = { cx_state, cx_sync, cx_agent };
    personality_t  ps[3] = { PERSONALITY_STATE, PERSONALITY_SYNC,
                             PERSONALITY_AGENT };
    pixel_t    cols[3]   = { ATOMIK_SEM_HARDWARE,
                             ATOMIK_SEM_SAVINGS,
                             ATOMIK_SEM_AGENT };
    for (int i = 0; i < 3; i++) {
        const char *n = names[i];
        pixel_t c = (active == ps[i]) ? cols[i] : ATOMIK_FG_DIM;
        const char *sub = (i == 0) ? "memory / regions" :
                          (i == 1) ? "replica / skip"   :
                                     "context / prune";
        int name_h;
        if (font_aa_loaded(FONT_AA_UI)) {
            int tw = text_width_aa(FONT_AA_UI, n);
            draw_text_aa(FONT_AA_UI, cxs[i] - tw / 2, label_y, n, c);
            name_h = text_height_aa(FONT_AA_UI);
        } else {
            int tw = text_width(n, 2);
            draw_text(cxs[i] - tw / 2, label_y, n, 2, c);
            name_h = text_height(2);
        }
        if (font_aa_loaded(FONT_AA_LABEL)) {
            int sw = text_width_aa(FONT_AA_LABEL, sub);
            draw_text_aa(FONT_AA_LABEL, cxs[i] - sw / 2,
                         label_y + name_h + 4, sub, ATOMIK_FG_DIM);
        } else {
            int sw = text_width(sub, 1);
            draw_text(cxs[i] - sw / 2, label_y + name_h + 4,
                      sub, 1, ATOMIK_FG_DIM);
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

    anim_tick();
}
