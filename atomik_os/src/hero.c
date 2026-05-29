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

void hero_draw(void) {
    int left_edge  = dock_right_edge() + ATOMIK_GRID_L * 2;
    int right_edge = fabric_shelf_x() - ATOMIK_GRID_L * 2;
    int ws_w  = right_edge - left_edge;
    int ws_cx = (left_edge + right_edge) / 2;
    int top   = ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + ATOMIK_GRID_L * 3;
    unsigned long now = anim_now_ms();
    personality_t act = fabric_active();

    /* ---- title block ---- */
    int y = top + 36;
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
    const char *tag = "INTELLIGENT    ADAPTIVE    SUBSTRATE";
    if (font_aa_loaded(FONT_AA_LABEL)) {
        int tw = text_width_aa(FONT_AA_LABEL, tag);
        draw_text_aa(FONT_AA_LABEL, ws_cx - tw / 2, y, tag, ATOMIK_ACCENT_DIM);
        y += text_height_aa(FONT_AA_LABEL) + 16;
    } else {
        int tw = text_width(tag, 1);
        draw_text(ws_cx - tw / 2, y, tag, 1, ATOMIK_ACCENT_DIM);
        y += text_height(1) + 16;
    }

    /* status line — honest: ACTIVE only when a workload event is recent. */
    int busy = 0;
    if (atomik_event_total() > 0) {
        unsigned long t1 = atomik_event_last_ts(EVT_STATE_DELTA);
        unsigned long t2 = atomik_event_last_ts(EVT_SYNC_REPLICA);
        unsigned long t3 = atomik_event_last_ts(EVT_AGENT_CONTEXT);
        if ((t1 && now - t1 < 3000) || (t2 && now - t2 < 3000) ||
            (t3 && now - t3 < 3000)) busy = 1;
    }
    char status[48];
    if (busy) snprintf(status, sizeof status, "%s   /   ACTIVE",
                       fabric_personality_name(act));
    else      snprintf(status, sizeof status, "IDLE   /   READY");
    pixel_t st_c = busy ? ATOMIK_SEM_SAVINGS : ATOMIK_ACCENT;
    if (font_aa_loaded(FONT_AA_UI)) {
        int tw = text_width_aa(FONT_AA_UI, status);
        int sh = text_height_aa(FONT_AA_UI);
        label_halo(ws_cx, y, tw, sh, st_c, busy);
        draw_text_aa(FONT_AA_UI, ws_cx - tw / 2, y, status, st_c);
        y += sh + ATOMIK_GRID_L * 2;
    } else {
        int tw = text_width(status, 2);
        draw_text(ws_cx - tw / 2, y, status, 2, st_c);
        y += text_height(2) + ATOMIK_GRID_L * 2;
    }

    /* ---- twin glass panels ---- */
    int gap   = ATOMIK_GRID_L;
    int pw    = (ws_w - gap) / 2;
    int avail = (FB_H - ATOMIK_GRID_L * 2) - y;   /* vertical space below title */
    int ph    = avail - ATOMIK_GRID_L * 2;
    if (ph > 340) ph = 340;
    if (ph < 160) ph = 160;
    int py    = y + (avail - ph) / 2;             /* center in lower workspace */
    if (py < y) py = y;
    int lx = left_edge;
    int rx = left_edge + pw + gap;

    /* Left — SYSTEM OVERVIEW (real facts). */
    glass_panel(lx, py, pw, ph);
    {
        int ix = lx + ATOMIK_GRID_L, iw = pw - ATOMIK_GRID_L * 2;
        int ty = py + ATOMIK_GRID_M;
        if (font_aa_loaded(FONT_AA_UI)) {
            draw_text_aa(FONT_AA_UI, ix, ty, "SYSTEM OVERVIEW", ATOMIK_FG);
            ty += text_height_aa(FONT_AA_UI) + ATOMIK_GRID_M;
        } else {
            draw_text(ix, ty, "SYSTEM OVERVIEW", 2, ATOMIK_FG);
            ty += text_height(2) + ATOMIK_GRID_M;
        }
        int rowh = (font_aa_loaded(FONT_AA_LABEL) ? text_height_aa(FONT_AA_LABEL)
                                                  : text_height(1)) + 9;
        double u = hero_uptime();
        char up[24];
        snprintf(up, sizeof up, "%02d:%02d:%02d",
                 (int)u / 3600, ((int)u % 3600) / 60, (int)u % 60);
        hero_stat(ix, ty, iw, "CORE",    "RV64 NaxRiscv"); ty += rowh;
        hero_stat(ix, ty, iw, "ISA",     "RV64GC");        ty += rowh;
        hero_stat(ix, ty, iw, "ADAPTER", "0xF0020000");    ty += rowh;
        hero_stat(ix, ty, iw, "ACTIVE",  fabric_personality_name(act)); ty += rowh;
        hero_stat(ix, ty, iw, "UPTIME",  up);              ty += rowh;
        /* Fill the lower panel with the ATOMiK orbital motif (concept-01's
         * System-Overview circular viz) + a divider above it. */
        if (py + ph - ty > 130) {
            draw_rect(ix, ty + ATOMIK_GRID_S, iw, 1, wm_card_border());
            core_node(lx + pw / 2, ty + (py + ph - ty) / 2 + 8, now);
        }
    }

    /* Right — DELTA COALESCING gauge (REAL perf_last_for STATE). */
    glass_panel(rx, py, pw, ph);
    {
        int ix = rx + ATOMIK_GRID_L;
        int ty = py + ATOMIK_GRID_M;
        const char *title = "DELTA COALESCING";
        if (font_aa_loaded(FONT_AA_UI)) {
            draw_text_aa(FONT_AA_UI, ix, ty, title, ATOMIK_FG);
        } else {
            draw_text(ix, ty, title, 2, ATOMIK_FG);
        }
        const perf_sample_t *s = perf_last_for(PERSONALITY_STATE);
        int have = (s && s->ops_logical > 0);
        int pct  = have ? (int)(100u * (s->ops_logical - s->ops_issued)
                                 / s->ops_logical) : 0;
        /* real raw counts under the title (honest provenance for the gauge) */
        char sub[28];
        if (have) snprintf(sub, sizeof sub, "%u -> %u ops",
                           (unsigned)s->ops_logical, (unsigned)s->ops_issued);
        else      snprintf(sub, sizeof sub, "awaiting workload");
        int sty = py + ATOMIK_GRID_M +
                  (font_aa_loaded(FONT_AA_UI) ? text_height_aa(FONT_AA_UI)
                                              : text_height(2)) + 4;
        if (font_aa_loaded(FONT_AA_LABEL))
            draw_text_aa(FONT_AA_LABEL, ix, sty, sub, ATOMIK_FG_DIM);
        else
            draw_text(ix, sty, sub, 1, ATOMIK_FG_DIM);
        int gcx = rx + pw / 2;
        int gcy = py + ph / 2 + 10;
        int gr  = (ph < 170) ? 44 : 54;
        hero_gauge(gcx, gcy, gr, pct, ATOMIK_SEM_HARDWARE);
        /* center value */
        char pc[8];
        if (have) snprintf(pc, sizeof pc, "%d", pct);
        else      snprintf(pc, sizeof pc, "--");
        if (font_aa_loaded(FONT_AA_DISPLAY)) {
            int nw = text_width_aa(FONT_AA_DISPLAY, pc);
            int nh = text_height_aa(FONT_AA_DISPLAY);
            draw_text_aa(FONT_AA_DISPLAY, gcx - nw / 2, gcy - nh / 2, pc,
                         ATOMIK_SEM_HARDWARE);
        } else {
            int nw = text_width(pc, 3);
            draw_text(gcx - nw / 2, gcy - text_height(3) / 2, pc, 3,
                      ATOMIK_SEM_HARDWARE);
        }
        /* unit + honest source tag under the gauge */
        const char *unit = "% ops coalesced";
        const char *src  = have ? "live perf-bench" : "waiting for workload";
        if (font_aa_loaded(FONT_AA_LABEL)) {
            int uw = text_width_aa(FONT_AA_LABEL, unit);
            draw_text_aa(FONT_AA_LABEL, gcx - uw / 2, gcy + gr + 6, unit,
                         ATOMIK_FG_DIM);
            int sw = text_width_aa(FONT_AA_LABEL, src);
            draw_text_aa(FONT_AA_LABEL, gcx - sw / 2,
                         gcy + gr + 6 + text_height_aa(FONT_AA_LABEL) + 2, src,
                         have ? ATOMIK_SEM_SAVINGS : ATOMIK_FG_DIM);
        } else {
            int uw = text_width(unit, 1);
            draw_text(gcx - uw / 2, gcy + gr + 6, unit, 1, ATOMIK_FG_DIM);
        }
    }

    dirty_rect(left_edge, top, ws_w, (py + ph) - top);
    anim_tick();
}
