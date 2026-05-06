/* dock.c — bottom dock with adaptive ordering.
 *
 * v0.3: each icon is bound to an agent action_t. The dock asks the agent
 * for each icon's score and renders LEFT-TO-RIGHT in DESCENDING score
 * order, so the user's most-likely-next action floats to the left edge
 * (closest to the App-launcher key). This is the first user-visible piece
 * of the agentic-OS direction. */
#include "atomik_os.h"
#include <string.h>

#define DOCK_HEIGHT      88
#define DOCK_PADDING_X   24
#define DOCK_PADDING_Y   12
#define ICON_SIZE        56
#define ICON_GAP         16
#define DOCK_RADIUS      18

static const struct {
    const char *label;
    pixel_t     color;
    action_t    action;     /* which agent action does clicking this icon log? */
} ICONS[] = {
    { "About",     rgb(0xA8, 0xB2, 0xC4), ACT_OPEN_ABOUT    },
    { "Monitor",   rgb(0x6E, 0xC4, 0x6E), ACT_OPEN_MONITOR  },
    { "Terminal",  rgb(0x36, 0x44, 0x60), ACT_OPEN_TERMINAL },
    { "Files",     rgb(0xC9, 0x8C, 0x3C), ACT_OPEN_FILES    },
    { "Notes",     rgb(0x4F, 0xC3, 0xFF), ACT_OPEN_NOTES    },
    { "ATOMiK",    rgb(0xFF, 0x6F, 0x91), ACT_NONE          },
};
#define N_ICONS ((int)(sizeof(ICONS) / sizeof(ICONS[0])))

int dock_count(void) { return N_ICONS; }

static int dock_x0(void) {
    int total_w = N_ICONS * ICON_SIZE + (N_ICONS - 1) * ICON_GAP +
                  2 * DOCK_PADDING_X;
    return (FB_W - total_w) / 2;
}

static int dock_y0(void) {
    /* visionOS-style floating dock: ATOMIK_DOCK_GAP px from the screen
     * edge. Smaller gap reads as "floating slab" instead of "edge-pinned
     * tray".  Was 16 px; v0.25 adopts the design-system default of 8. */
    return FB_H - DOCK_HEIGHT - ATOMIK_DOCK_GAP;
}

/* Cached order from the last dock_draw, so dock_action_for_slot can
 * resolve a visible-slot index back to an icon without recomputing. */
static int s_last_order[N_ICONS] = {0,1,2,3,4,5};

/* Compute the icon order based on agent score. Higher score = closer to
 * the left edge of the dock. Ties broken by original index for stability. */
static void compute_order(int order[N_ICONS]) {
    double scores[N_ICONS];
    for (int i = 0; i < N_ICONS; i++) {
        order[i]  = i;
        scores[i] = ICONS[i].action == ACT_NONE ? 0.0
                                                : agent_score(ICONS[i].action);
    }
    /* Insertion sort by descending score (small N). */
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

void dock_draw(int hover_index) {
    int total_w = N_ICONS * ICON_SIZE + (N_ICONS - 1) * ICON_GAP +
                  2 * DOCK_PADDING_X;
    int dx = dock_x0();
    int dy = dock_y0();

    /* Dock background panel — translucent dark, rounded. */
    draw_rect_rounded(dx, dy, total_w, DOCK_HEIGHT, DOCK_RADIUS,
                      ATOMIK_DOCK_BG & 0xFFFFFF);

    /* 1px border highlight */
    draw_rect_rounded(dx, dy, total_w, 1,             DOCK_RADIUS,
                      ATOMIK_DOCK_BORDER);

    int order[N_ICONS];
    compute_order(order);
    memcpy(s_last_order, order, sizeof order);

    int ix = dx + DOCK_PADDING_X;
    int iy = dy + DOCK_PADDING_Y;
    for (int slot = 0; slot < N_ICONS; slot++) {
        int i     = order[slot];
        int hover = (slot == hover_index);
        int size  = hover ? ICON_SIZE + 8 : ICON_SIZE;
        int off   = (ICON_SIZE - size) / 2;
        draw_rect_rounded(ix + off, iy + off, size, size, 12, ICONS[i].color);

        /* First-letter chip for now — real glyph icons later. */
        char ch[2] = { ICONS[i].label[0], 0 };
        int  scale = 3;
        int  tw    = text_width(ch, scale);
        int  th    = text_height(scale);
        draw_text(ix + off + (size - tw) / 2,
                  iy + off + (size - th) / 2,
                  ch, scale, ATOMIK_FG);

        /* If this icon's action is the agent's predicted next, draw a
         * pulsing dot under it.  v0.25: use ATOMIK_SEM_AGENT (violet)
         * instead of raw cyan — agent prediction is AGENT in the
         * semantic grammar, not HARDWARE.  The bug the previous version
         * carried was that prediction-pulse and focus-indicator both
         * used cyan, making it impossible to read at a glance whether
         * a glowing element meant "active state" or "agent suggests
         * this".  Violet for AGENT, cyan stays for HARDWARE/active. */
        if (ICONS[i].action != ACT_NONE && agent_predict() == ICONS[i].action) {
            unsigned long t  = anim_now_ms();
            /* 1.5 s cycle, alpha 0.7..1.0.  4-term Taylor sin so we
             * stay libm-free in the per-pixel hot path. */
            double phase = ((t % 1500) / 1500.0) * 6.2831853;
            double s = phase;
            double s3 = s*s*s, s5 = s3*s*s;
            double sinv = s - s3/6.0 + s5/120.0;
            if (sinv >  1.0) sinv =  1.0;
            if (sinv < -1.0) sinv = -1.0;
            double bright = 0.7 + 0.3 * sinv;
            if (bright < 0.0) bright = 0.0;
            if (bright > 1.0) bright = 1.0;

            /* Decompose ATOMIK_SEM_AGENT (#9B7EE0) into channels and
             * scale by `bright` to get the pulse color. */
            uint8_t r = (uint8_t)(0x9B * bright);
            uint8_t g = (uint8_t)(0x7E * bright);
            uint8_t b = (uint8_t)(0xE0 * bright);
            pixel_t pulse_color = rgb(r, g, b);

            int dot_x = ix + ICON_SIZE / 2;
            int dot_y = iy + ICON_SIZE + 4;
            int radius = 3;
            for (int dy2 = -radius; dy2 <= radius; dy2++)
                for (int dx2 = -radius; dx2 <= radius; dx2++)
                    if (dx2*dx2 + dy2*dy2 <= radius*radius)
                        draw_pixel(dot_x + dx2, dot_y + dy2, pulse_color);
            anim_tick();   /* keep frame loop alive while pulse animates */
        }

        ix += ICON_SIZE + ICON_GAP;
    }
}

action_t dock_action_for_slot(int slot) {
    if (slot < 0 || slot >= N_ICONS) return ACT_NONE;
    int icon_index = s_last_order[slot];
    return ICONS[icon_index].action;
}

int dock_hit_test(int mouse_x, int mouse_y) {
    int dy = dock_y0();
    if (mouse_y < dy || mouse_y > dy + DOCK_HEIGHT) return -1;
    int ix = dock_x0() + DOCK_PADDING_X;
    int iy = dy + DOCK_PADDING_Y;
    for (int i = 0; i < N_ICONS; i++) {
        if (mouse_x >= ix && mouse_x < ix + ICON_SIZE &&
            mouse_y >= iy && mouse_y < iy + ICON_SIZE) return i;
        ix += ICON_SIZE + ICON_GAP;
    }
    return -1;
}
