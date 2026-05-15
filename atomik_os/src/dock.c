/* dock.c — Capability Rail (left-anchored vertical dock).
 *
 * v0.34-A introduced the rail.  v0.38-I reinstates the v0.38-D polish
 * that was stashed during the kernel-oops recovery 2026-05-10:
 *   - ICON_SIZE 64 → 80 (+25% touch target, more visual weight)
 *   - ICON_GAP  16 → 20 (more breathing room between rows)
 *   - Letter glyph scale 3 → 4 (proportional with new size)
 *   - Per-icon "open" indicator (small filled bar above the icon when
 *     the corresponding app has a visible window)
 *   - Per-icon "focused" glow (3-px outer ring around the icon when
 *     its app is the currently-topmost window)
 *
 * Window-state detection is title-based — apps register their
 * windows via wm_open() with a known title, and we scan wm_get()
 * once per draw to find matches. */
#include "atomik_os.h"
#include <string.h>

/* === geometry === */
#define RAIL_X_MARGIN    ATOMIK_GRID_L
#define RAIL_TOP_MARGIN  (ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + ATOMIK_GRID_L * 2)
#define ICON_SIZE        80
#define ICON_GAP         20
#define RAIL_PADDING     ATOMIK_GRID_M
#define RAIL_RADIUS      14
#define LABEL_GAP        ATOMIK_GRID_S

#define GLOW_THICKNESS   3
#define OPEN_BAR_W       (ICON_SIZE / 2)
#define OPEN_BAR_H       3

static const struct {
    const char *label;
    const char *title;
    pixel_t     color;
    action_t    action;
} ICONS[] = {
    { "About",     "About ATOMiK OS", rgb(0xA8, 0xB2, 0xC4), ACT_OPEN_ABOUT    },
    { "Monitor",   "ATOMiK Monitor",  rgb(0x6E, 0xC4, 0x6E), ACT_OPEN_MONITOR  },
    { "Terminal",  "Terminal",        rgb(0x36, 0x44, 0x60), ACT_OPEN_TERMINAL },
    { "Files",     "Files",           rgb(0xC9, 0x8C, 0x3C), ACT_OPEN_FILES    },
    { "Notes",     "Notes",           rgb(0x4F, 0xC3, 0xFF), ACT_OPEN_NOTES    },
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

static int s_last_order[N_ICONS] = {0,1,2,3,4};

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

/* Draw an outer ring around (x, y, w, h), thickness px wide. */
static void draw_ring(int x, int y, int w, int h, int thickness,
                      pixel_t color) {
    for (int t = 0; t < thickness; t++) {
        draw_rect(x - t, y - t, w + 2*t, 1, color);
        draw_rect(x - t, y + h - 1 + t, w + 2*t, 1, color);
        draw_rect(x - t, y - t, 1, h + 2*t, color);
        draw_rect(x + w - 1 + t, y - t, 1, h + 2*t, color);
    }
}

void dock_draw(int hover_index) {
    int rx = rail_x();
    int ry = rail_y();
    int rw = rail_w();
    int rh = rail_h();

    /* Rail background — translucent dark slab with a subtle border. */
    draw_rect_rounded(rx, ry, rw, rh, RAIL_RADIUS,
                      ATOMIK_DOCK_BG & 0xFFFFFF);
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
        int size     = hover ? ICON_SIZE + 6 : ICON_SIZE;
        int off_x    = (ICON_SIZE - size) / 2;
        int off_y    = (ICON_SIZE - size) / 2;
        int icon_x   = ix + off_x;
        int icon_y   = iy + off_y;

        /* Focused-app glow ring (3-px outer rim, lane color). */
        if (focused) {
            draw_ring(icon_x, icon_y, size, size, GLOW_THICKNESS,
                      ICONS[i].color);
        }

        /* Icon body — rounded rect filled with the app color. */
        draw_rect_rounded(icon_x, icon_y, size, size, 12,
                          ICONS[i].color);

        /* First-letter glyph at scale-4 (proportional to ICON_SIZE 80). */
        char ch[2] = { ICONS[i].label[0], 0 };
        int  scale = 4;
        int  tw    = text_width(ch, scale);
        int  th    = text_height(scale);
        draw_text(icon_x + (size - tw) / 2,
                  icon_y + (size - th) / 2,
                  ch, scale, ATOMIK_FG);

        /* Open-app cap — small filled bar above the icon. */
        if (open) {
            int bar_x = icon_x + (size - OPEN_BAR_W) / 2;
            int bar_y = icon_y - OPEN_BAR_H - 2;
            draw_rect(bar_x, bar_y, OPEN_BAR_W, OPEN_BAR_H,
                      ICONS[i].color);
        }

        /* Predicted-next pulse — violet dot right of the icon. */
        if (ICONS[i].action != ACT_NONE && agent_predict() == ICONS[i].action) {
            unsigned long t  = anim_now_ms();
            double phase = ((t % 1500) / 1500.0) * 6.2831853;
            double s = phase;
            double s3 = s*s*s, s5 = s3*s*s;
            double sinv = s - s3/6.0 + s5/120.0;
            if (sinv >  1.0) sinv =  1.0;
            if (sinv < -1.0) sinv = -1.0;
            double bright = 0.7 + 0.3 * sinv;
            if (bright < 0.0) bright = 0.0;
            if (bright > 1.0) bright = 1.0;
            uint8_t r = (uint8_t)(0xB0 * bright);
            uint8_t g = (uint8_t)(0x8C * bright);
            uint8_t b = (uint8_t)(0xFF * bright);
            pixel_t pulse_color = rgb(r, g, b);
            int dot_x = ix + ICON_SIZE + LABEL_GAP + 2;
            int dot_y = iy + ICON_SIZE / 2;
            int radius = 4;
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
