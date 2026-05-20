/* dock.c — Capability Rail (left-anchored vertical dock).
 *
 * v0.34-A introduced the rail.  v0.38-J ran a polish pass that left
 * candy-colored solid squares with giant pixel-font letters — by
 * v0.38-K3 that was the weakest visible surface vs the rest of the
 * premium shell.  ChatGPT 2026-05-16 directive: "The rail should
 * feel like a capability system, not a shortcut column."
 *
 * v0.38-L1 rewrite — concept-aligned capability rail:
 *   - Dark glass cells (rgb(0x10,0x18,0x2C) body) instead of saturated
 *     color squares.  Lane color stays as the cell's BORDER + letter
 *     glyph color, not its FILL.
 *   - AA UI letter glyph (FONT_AA_UI 18 px) replaces the giant
 *     pixel-font scale-4 letter.  Deliberate placeholder for proper
 *     line-style iconography in a later slice.
 *   - Larger breathing room between cells (24 px gap).
 *   - Active app: 2 px outer alpha halo + saturated cell border.
 *   - Open app: small filled dot on the right edge of the cell.
 *   - Atom (Assistant) placeholder cell — visible 6th slot in violet,
 *     action=ACT_NONE until v0.39-A wires the overlay.
 *
 * v0.39-F — product-nav pass:
 *   - Replace single-letter placeholder with simple line-art icons
 *     drawn from draw_rect / draw_blend_pixel primitives.  One
 *     symbol per app (i-in-circle, bar chart, prompt, folder, lines,
 *     atom rosette).  Layered-stroke compliant — every visible
 *     stroke gets a soft alpha halo around it.
 *   - Add per-cell name label below the icon (AA atomik_14 font).
 *   - Cell taller (80×100) to fit icon + label.
 *   - Rail wider accordingly (112 px).  All other v0.38-L1 behavior
 *     preserved.
 */
#include "atomik_os.h"
#include <string.h>

/* === geometry === */
#define RAIL_X_MARGIN    ATOMIK_GRID_L
#define RAIL_TOP_MARGIN  (ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + ATOMIK_GRID_L * 2)
#define ICON_SIZE        80      /* cell width  (v0.39-F: 72 → 80) */
#define CELL_HEIGHT      100     /* cell height (v0.39-F: square → tall) */
#define GLYPH_REGION_H   52      /* upper portion holds the icon       */
#define LABEL_REGION_H   28      /* lower portion holds the AA label   */
#define ICON_GAP         18      /* v0.39-F: 24 → 18, taller cells already breathe */
#define RAIL_PADDING     ATOMIK_GRID_L
#define RAIL_RADIUS      18
#define LABEL_GAP        ATOMIK_GRID_S

#define ACTIVE_HALO      3       /* alpha halo around active cell      */
#define OPEN_DOT_R       4       /* radius of the open-app right dot   */

static const struct {
    const char *label;
    const char *title;
    pixel_t     color;
    action_t    action;
} ICONS[] = {
    { "About",     "About ATOMiK OS", rgb(0xA8, 0xB2, 0xC4), ACT_OPEN_ABOUT    },
    { "Monitor",   "ATOMiK Monitor",  rgb(0x6E, 0xC4, 0x6E), ACT_OPEN_MONITOR  },
    { "Terminal",  "Terminal",        rgb(0x9D, 0xB7, 0xE8), ACT_OPEN_TERMINAL },
    { "Files",     "Files",           rgb(0xE0, 0xB0, 0x66), ACT_OPEN_FILES    },
    { "Notes",     "Notes",           rgb(0x4F, 0xC3, 0xFF), ACT_OPEN_NOTES    },
    { "?Atom",     "Atom",            rgb(0xB0, 0x8C, 0xFF), ACT_OPEN_ASSISTANT },
};
#define N_ICONS ((int)(sizeof(ICONS) / sizeof(ICONS[0])))

int dock_count(void) { return N_ICONS; }

int dock_left_edge(void) { return RAIL_X_MARGIN; }
int dock_right_edge(void) {
    return RAIL_X_MARGIN + RAIL_PADDING * 2 + ICON_SIZE;
}

static int rail_x(void) { return RAIL_X_MARGIN; }
static int rail_y(void) { return RAIL_TOP_MARGIN; }
static int rail_w(void) { return RAIL_PADDING * 2 + ICON_SIZE; }
static int rail_h(void) {
    return RAIL_PADDING * 2 + N_ICONS * CELL_HEIGHT + (N_ICONS - 1) * ICON_GAP;
}

static int s_last_order[N_ICONS] = {0,1,2,3,4,5};

static void compute_order(int order[N_ICONS]) {
    double scores[N_ICONS];
    for (int i = 0; i < N_ICONS; i++) {
        order[i]  = i;
        scores[i] = ICONS[i].action == ACT_NONE ? 0.0
                                                : agent_score(ICONS[i].action);
    }
    for (int i = 1; i < N_ICONS; i++) {
        int    cur_i = order[i];
        double cur_s = scores[cur_i];
        int    j     = i - 1;
        while (j >= 0 && scores[order[j]] < cur_s) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = cur_i;
    }
}

static int find_window_by_title(const char *title) {
    if (!title) return -1;
    for (int i = 0; i < wm_count(); i++) {
        const window_t *w = wm_get(i);
        if (!w || !w->visible) continue;
        if (strncmp(w->title, title, sizeof w->title) == 0) return w->id;
    }
    return -1;
}

static int app_is_open(int icon_idx) {
    return find_window_by_title(ICONS[icon_idx].title) >= 0;
}

static int app_is_focused(int icon_idx) {
    const window_t *top = wm_topmost();
    if (!top) return 0;
    return strncmp(top->title, ICONS[icon_idx].title,
                   sizeof top->title) == 0;
}

/* v0.39-F line-icon helpers.  Each icon paints a 36×36 line-art
 * symbol centered in (cx, cy).  Strokes are LAYERED — every 1 px
 * line carries a 1 px alpha-50 halo on each side so the icons read
 * as solid weights at 1080p, never as naked single-pixel lines.
 * Layered-stroke discipline; ChatGPT 2026-05-15. */
static void stroke_h(int x, int y, int w, pixel_t color) {
    draw_rect(x, y, w, 1, color);
    for (int dx = 0; dx < w; dx++) {
        draw_blend_pixel(x + dx, y - 1, color, 90);
        draw_blend_pixel(x + dx, y + 1, color, 90);
    }
}
static void stroke_v(int x, int y, int h, pixel_t color) {
    draw_rect(x, y, 1, h, color);
    for (int dy = 0; dy < h; dy++) {
        draw_blend_pixel(x - 1, y + dy, color, 90);
        draw_blend_pixel(x + 1, y + dy, color, 90);
    }
}
static void stroke_circle(int cx, int cy, int radius, pixel_t color) {
    /* Bresenham circle + per-octant halo neighbours. */
    int x = radius, y = 0, err = 0;
    while (x >= y) {
        const int pts[8][2] = {
            { cx + x, cy + y }, { cx - x, cy + y },
            { cx + x, cy - y }, { cx - x, cy - y },
            { cx + y, cy + x }, { cx - y, cy + x },
            { cx + y, cy - x }, { cx - y, cy - x },
        };
        for (int p = 0; p < 8; p++) {
            draw_pixel(pts[p][0], pts[p][1], color);
            draw_blend_pixel(pts[p][0] + 1, pts[p][1],     color, 70);
            draw_blend_pixel(pts[p][0] - 1, pts[p][1],     color, 70);
            draw_blend_pixel(pts[p][0],     pts[p][1] + 1, color, 70);
            draw_blend_pixel(pts[p][0],     pts[p][1] - 1, color, 70);
        }
        y += 1;
        if (err <= 0) { err += 2*y + 1; }
        if (err >  0) { x -= 1; err -= 2*x + 1; }
    }
}
static void stroke_disk(int cx, int cy, int radius, pixel_t color) {
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= r2) {
                draw_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}

/* === Icons ===
 * Each icon paints into a 36×36 box centred on (cx, cy).  Glyphs use
 * a single `color` (mode-aware: dim slate when unfocused, lane color
 * when focused). */
static void icon_about(int cx, int cy, pixel_t color) {
    /* "i" in circle. */
    stroke_circle(cx, cy, 17, color);
    /* Dot above the stem (the i's tittle). */
    stroke_disk(cx, cy - 7, 2, color);
    /* Vertical stem. */
    stroke_v(cx, cy - 2, 11, color);
}
static void icon_monitor(int cx, int cy, pixel_t color) {
    /* Three-bar vertical chart, increasing heights. */
    int base_y = cy + 10;
    int bw = 5;
    int gx = 8;
    int heights[3] = { 9, 14, 19 };
    for (int i = 0; i < 3; i++) {
        int bx = cx - gx + i * gx;
        int bh = heights[i];
        /* Filled rect for the bar (no halo — keep it solid). */
        draw_rect(bx - bw/2, base_y - bh, bw, bh, color);
        /* Faint alpha lift on each side so the bar reads layered. */
        for (int dy = 0; dy < bh; dy++) {
            draw_blend_pixel(bx - bw/2 - 1, base_y - bh + dy, color, 80);
            draw_blend_pixel(bx + bw/2,     base_y - bh + dy, color, 80);
        }
    }
    /* Baseline. */
    stroke_h(cx - 14, base_y + 2, 28, color);
}
static void icon_terminal(int cx, int cy, pixel_t color) {
    /* ">" arrow + underscore. */
    int x = cx - 11;
    /* Two diagonals making the > glyph. */
    for (int t = 0; t < 6; t++) {
        draw_pixel(x + t,         cy - 5 + t, color);
        draw_blend_pixel(x + t + 1, cy - 5 + t, color, 80);
        draw_blend_pixel(x + t,     cy - 5 + t + 1, color, 80);
        draw_pixel(x + t,         cy + 5 - t, color);
        draw_blend_pixel(x + t + 1, cy + 5 - t, color, 80);
        draw_blend_pixel(x + t,     cy + 5 - t + 1, color, 80);
    }
    /* Trailing underscore. */
    stroke_h(cx + 1, cy + 6, 11, color);
}
static void icon_files(int cx, int cy, pixel_t color) {
    /* Folder outline: tab + body. */
    int top    = cy - 11;
    int bottom = cy + 11;
    int left   = cx - 14;
    int right  = cx + 14;
    /* Tab on the left half of the top edge. */
    stroke_h(left,           top - 4, 12, color);
    stroke_v(left,           top - 4, 4,  color);
    stroke_v(left + 12,      top - 4, 4,  color);
    /* Body — rounded-ish rectangle (just 4 strokes). */
    stroke_h(left,           top, right - left,    color);
    stroke_h(left,           bottom, right - left, color);
    stroke_v(left,           top, bottom - top,    color);
    stroke_v(right,          top, bottom - top,    color);
}
static void icon_notes(int cx, int cy, pixel_t color) {
    /* Three stacked horizontal lines (note rule lines), inside a
     * thin page outline. */
    int left   = cx - 12;
    int right  = cx + 12;
    int top    = cy - 13;
    int bottom = cy + 13;
    stroke_h(left,  top,    right - left, color);
    stroke_h(left,  bottom, right - left, color);
    stroke_v(left,  top,    bottom - top, color);
    stroke_v(right, top,    bottom - top, color);
    /* Three rule lines inside. */
    stroke_h(left + 4, cy - 6, right - left - 8, color);
    stroke_h(left + 4, cy,     right - left - 8, color);
    stroke_h(left + 4, cy + 6, right - left - 8, color);
}
static void icon_atom(int cx, int cy, pixel_t color) {
    /* 3-dot rosette around a central node — placeholder for the
     * eventual chromakey'd Atom asset.  Reads as the assistant. */
    stroke_disk(cx, cy, 3, color);
    /* Three satellite dots arranged at 120° offsets. */
    /* Angles 90°, 210°, 330° → (0,1), (-0.866,-0.5), (0.866,-0.5).  */
    int dx_top = 0;     int dy_top = -12;
    int dx_bl  = -10;   int dy_bl  = 6;
    int dx_br  = 10;    int dy_br  = 6;
    stroke_disk(cx + dx_top, cy + dy_top, 2, color);
    stroke_disk(cx + dx_bl,  cy + dy_bl,  2, color);
    stroke_disk(cx + dx_br,  cy + dy_br,  2, color);
    /* Thin connector lines (single pixel + halo) from centre to
     * each satellite, gives the orbital feel. */
    for (int t = 4; t < 11; t++) {
        draw_blend_pixel(cx,     cy - t, color, 110);
        draw_blend_pixel(cx - t, cy + t * 6 / 10, color, 110);
        draw_blend_pixel(cx + t, cy + t * 6 / 10, color, 110);
    }
}

/* Dispatch table — by icon_idx (matches ICONS[] order). */
static void draw_icon(int icon_idx, int cx, int cy, pixel_t color) {
    switch (icon_idx) {
    case 0: icon_about(cx, cy, color);    break;
    case 1: icon_monitor(cx, cy, color);  break;
    case 2: icon_terminal(cx, cy, color); break;
    case 3: icon_files(cx, cy, color);    break;
    case 4: icon_notes(cx, cy, color);    break;
    case 5: icon_atom(cx, cy, color);     break;
    default:                              break;
    }
}

/* Outer alpha halo around (x, y, w, h) — `thickness` rings, each at a
 * decreasing alpha.  Used for the active-cell glow and hover lift. */
static void halo_ring(int x, int y, int w, int h, int thickness,
                      pixel_t color, uint8_t base_alpha) {
    for (int t = 1; t <= thickness; t++) {
        uint8_t a = (uint8_t)(base_alpha - (t - 1) * (base_alpha / (thickness + 1)));
        for (int sx = 0; sx < w + 2 * t; sx++) {
            draw_blend_pixel(x - t + sx, y - t,           color, a);
            draw_blend_pixel(x - t + sx, y + h - 1 + t,   color, a);
        }
        for (int sy = 0; sy < h + 2 * t; sy++) {
            draw_blend_pixel(x - t,           y - t + sy, color, a);
            draw_blend_pixel(x + w - 1 + t,   y - t + sy, color, a);
        }
    }
}

void dock_draw(int hover_index) {
    int rx = rail_x();
    int ry = rail_y();
    int rw = rail_w();
    int rh = rail_h();

    /* Rail background — translucent dark slab.  v0.38-L1: use the
     * same dark-glass body color as the Pulse Bar pills + Fabric
     * card body so the chrome reads as one design system. */
    pixel_t rail_body = rgb(0x10, 0x18, 0x2C);
    draw_rect_rounded(rx, ry, rw, rh, RAIL_RADIUS, rail_body);
    /* Subtle border — 1 px dim cyan along the rail edges. */
    draw_rect(rx + RAIL_RADIUS / 2, ry, rw - RAIL_RADIUS, 1,
              ATOMIK_DOCK_BORDER);
    draw_rect(rx + RAIL_RADIUS / 2, ry + rh - 1, rw - RAIL_RADIUS, 1,
              ATOMIK_DOCK_BORDER);

    int order[N_ICONS];
    compute_order(order);
    memcpy(s_last_order, order, sizeof order);

    int ix = rx + RAIL_PADDING;
    int iy = ry + RAIL_PADDING;
    for (int slot = 0; slot < N_ICONS; slot++) {
        int i        = order[slot];
        int hover    = (slot == hover_index);
        int focused  = app_is_focused(i);
        int open     = app_is_open(i);
        int icon_x   = ix;
        int icon_y   = iy;
        int cw       = ICON_SIZE;
        int ch       = CELL_HEIGHT;
        pixel_t accent = ICONS[i].color;

        /* === Glass cell body === */
        pixel_t cell_body = rgb(0x14, 0x1E, 0x34);
        draw_rect_rounded(icon_x, icon_y, cw, ch, 14, cell_body);

        /* === Focused glow (subtle outer halo) === */
        if (focused) {
            halo_ring(icon_x, icon_y, cw, ch, ACTIVE_HALO,
                      accent, 60);
        }

        /* === Hover lift === */
        if (hover && !focused) {
            halo_ring(icon_x, icon_y, cw, ch, 2, accent, 36);
        }

        /* === Cell border (1 px) — focused gets the bright accent;
         * idle gets a quiet slate that still reads as a cell. */
        pixel_t border = focused ? accent : rgb(0x2C, 0x36, 0x52);
        int radius = 14;
        draw_rect(icon_x + radius, icon_y,              cw - radius * 2, 1, border);
        draw_rect(icon_x + radius, icon_y + ch - 1,     cw - radius * 2, 1, border);
        draw_rect(icon_x,              icon_y + radius, 1, ch - radius * 2, border);
        draw_rect(icon_x + cw - 1,     icon_y + radius, 1, ch - radius * 2, border);

        /* === v0.39-F: line-art icon centred in the upper portion
         * of the cell.  Replaces the v0.38-L1 single-letter
         * placeholder glyph. */
        pixel_t glyph_col = focused ? accent : rgb(0x9A, 0xA6, 0xC0);
        int icon_cx = icon_x + cw / 2;
        int icon_cy = icon_y + GLYPH_REGION_H / 2 + 4;
        draw_icon(i, icon_cx, icon_cy, glyph_col);

        /* === v0.39-F: per-cell label below the icon.  Strip the
         * leading '?' that the rail uses internally for Atom (so the
         * "?Atom" first-letter glyph showed `?` in the prior slice).
         * Now that we have proper icons, just show "Atom". */
        const char *lab = ICONS[i].label;
        if (lab[0] == '?') lab++;
        pixel_t label_col = focused ? accent : rgb(0x7A, 0x86, 0xA0);
        if (font_aa_loaded(FONT_AA_LABEL)) {
            int tw  = text_width_aa(FONT_AA_LABEL, lab);
            int lab_y = icon_y + GLYPH_REGION_H +
                        (LABEL_REGION_H - text_height_aa(FONT_AA_LABEL)) / 2;
            draw_text_aa(FONT_AA_LABEL,
                         icon_x + (cw - tw) / 2,
                         lab_y, lab, label_col);
        } else {
            int tw = text_width(lab, 1);
            int lab_y = icon_y + GLYPH_REGION_H +
                        (LABEL_REGION_H - text_height(1)) / 2;
            draw_text(icon_x + (cw - tw) / 2, lab_y, lab, 1, label_col);
        }

        /* === Open-app indicator — small filled dot on the right
         * edge.  Lane color, half-radius outer halo for soft glow. */
        if (open) {
            int dot_x = icon_x + cw + 4;
            int dot_y = icon_y + GLYPH_REGION_H / 2;
            int r = OPEN_DOT_R;
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (dx*dx + dy*dy <= r*r) {
                        draw_pixel(dot_x + dx, dot_y + dy, accent);
                    }
                }
            }
            for (int dy = -r - 2; dy <= r + 2; dy++) {
                for (int dx = -r - 2; dx <= r + 2; dx++) {
                    int d2 = dx*dx + dy*dy;
                    if (d2 > r*r && d2 <= (r + 2) * (r + 2)) {
                        draw_blend_pixel(dot_x + dx, dot_y + dy,
                                         accent, 80);
                    }
                }
            }
        }

        /* === Predicted-next pulse — small violet dot to the right.
         * Kept from v0.34-A but smaller and dimmer to suit the new
         * cell scale. */
        if (ICONS[i].action != ACT_NONE && agent_predict() == ICONS[i].action) {
            unsigned long t  = anim_now_ms();
            double phase = ((t % 1500) / 1500.0) * 6.2831853;
            double s = phase;
            double s3 = s*s*s, s5 = s3*s*s;
            double sinv = s - s3/6.0 + s5/120.0;
            if (sinv >  1.0) sinv =  1.0;
            if (sinv < -1.0) sinv = -1.0;
            double bright = 0.6 + 0.4 * sinv;
            if (bright < 0.0) bright = 0.0;
            if (bright > 1.0) bright = 1.0;
            uint8_t r = (uint8_t)(0xB0 * bright);
            uint8_t g = (uint8_t)(0x8C * bright);
            uint8_t b = (uint8_t)(0xFF * bright);
            pixel_t pulse_color = rgb(r, g, b);
            int dot_x = icon_x + cw + LABEL_GAP + 12;
            int dot_y = icon_y + GLYPH_REGION_H / 2;
            int radius = 3;
            for (int dy2 = -radius; dy2 <= radius; dy2++)
                for (int dx2 = -radius; dx2 <= radius; dx2++)
                    if (dx2*dx2 + dy2*dy2 <= radius*radius)
                        draw_pixel(dot_x + dx2, dot_y + dy2, pulse_color);
            anim_tick();
        }

        iy += CELL_HEIGHT + ICON_GAP;
    }
}

action_t dock_action_for_slot(int slot) {
    if (slot < 0 || slot >= N_ICONS) return ACT_NONE;
    int icon_index = s_last_order[slot];
    return ICONS[icon_index].action;
}

int dock_hit_test(int mouse_x, int mouse_y) {
    int rx = rail_x();
    int ry = rail_y();
    if (mouse_x < rx + RAIL_PADDING ||
        mouse_x >= rx + RAIL_PADDING + ICON_SIZE) return -1;
    int iy = ry + RAIL_PADDING;
    for (int i = 0; i < N_ICONS; i++) {
        if (mouse_y >= iy && mouse_y < iy + CELL_HEIGHT) return i;
        iy += CELL_HEIGHT + ICON_GAP;
    }
    return -1;
}
