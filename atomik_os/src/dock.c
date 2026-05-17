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
 *     action=ACT_NONE until v0.39-A wires the overlay. */
#include "atomik_os.h"
#include <string.h>

/* === geometry === */
#define RAIL_X_MARGIN    ATOMIK_GRID_L
#define RAIL_TOP_MARGIN  (ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + ATOMIK_GRID_L * 2)
#define ICON_SIZE        72
#define ICON_GAP         24
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
    { "Atom",      "Atom",            rgb(0xB0, 0x8C, 0xFF), ACT_NONE          },
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
    return RAIL_PADDING * 2 + N_ICONS * ICON_SIZE + (N_ICONS - 1) * ICON_GAP;
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
        int size     = ICON_SIZE;
        pixel_t accent = ICONS[i].color;

        /* === Glass cell body === */
        pixel_t cell_body = rgb(0x14, 0x1E, 0x34);
        draw_rect_rounded(icon_x, icon_y, size, size, 14, cell_body);

        /* === Focused glow (subtle outer halo) === */
        if (focused) {
            halo_ring(icon_x, icon_y, size, size, ACTIVE_HALO,
                      accent, 60);
        }

        /* === Hover lift === */
        if (hover && !focused) {
            halo_ring(icon_x, icon_y, size, size, 2, accent, 36);
        }

        /* === Cell border (1 px) — focused gets the bright accent;
         * idle gets a quiet slate that still reads as a cell. */
        pixel_t border = focused ? accent : rgb(0x2C, 0x36, 0x52);
        int radius = 14;
        draw_rect(icon_x + radius, icon_y,              size - radius * 2, 1, border);
        draw_rect(icon_x + radius, icon_y + size - 1,   size - radius * 2, 1, border);
        draw_rect(icon_x,              icon_y + radius, 1, size - radius * 2, border);
        draw_rect(icon_x + size - 1,   icon_y + radius, 1, size - radius * 2, border);

        /* === Glyph — AA UI letter in accent color (deliberate
         * placeholder for proper line iconography in a later slice).
         * Falls back to pixel-font scale-3 if the atlas hasn't
         * shipped yet. */
        char ch[2] = { ICONS[i].label[0], 0 };
        pixel_t glyph_col = focused ? accent : rgb(0x9A, 0xA6, 0xC0);
        if (font_aa_loaded(FONT_AA_DISPLAY)) {
            int tw = text_width_aa(FONT_AA_DISPLAY, ch);
            int th = text_height_aa(FONT_AA_DISPLAY);
            draw_text_aa(FONT_AA_DISPLAY,
                         icon_x + (size - tw) / 2,
                         icon_y + (size - th) / 2 - 2,
                         ch, glyph_col);
        } else {
            int tw = text_width(ch, 3);
            int th = text_height(3);
            draw_text(icon_x + (size - tw) / 2,
                      icon_y + (size - th) / 2,
                      ch, 3, glyph_col);
        }

        /* === Open-app indicator — small filled dot on the right
         * edge.  Lane color, half-radius outer halo for soft glow. */
        if (open) {
            int dot_x = icon_x + size + 4;
            int dot_y = icon_y + size / 2;
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
            int dot_x = icon_x + size + LABEL_GAP + 12;
            int dot_y = icon_y + size / 2;
            int radius = 3;
            for (int dy2 = -radius; dy2 <= radius; dy2++)
                for (int dx2 = -radius; dx2 <= radius; dx2++)
                    if (dx2*dx2 + dy2*dy2 <= radius*radius)
                        draw_pixel(dot_x + dx2, dot_y + dy2, pulse_color);
            anim_tick();
        }

        iy += ICON_SIZE + ICON_GAP;
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
        if (mouse_y >= iy && mouse_y < iy + ICON_SIZE) return i;
        iy += ICON_SIZE + ICON_GAP;
    }
    return -1;
}
