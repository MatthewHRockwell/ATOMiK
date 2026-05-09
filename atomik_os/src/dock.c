/* dock.c — Capability Rail (left-anchored vertical dock).
 *
 * v0.34 (ChatGPT 2026-05-09 visual shell): the dock moves from a
 * bottom-centered horizontal strip to a left-anchored vertical
 * column.  The visual grammar of ATOMiK Desk is:
 *
 *   Pulse Bar              ← top, full width
 *   Capability Rail (this) ← left, vertical column
 *   Main Workspace         ← center, where surfaces live
 *   Resource Fabric        ← right, pinned shelf
 *
 * Same five capabilities (About / Monitor / Terminal / Files / Notes),
 * same agent-driven score-based reordering, same predicted-next
 * violet pulse.  Just oriented vertically, with more breathing room
 * and a cleaner card-style chrome.
 *
 * The hit-test, ordering, slot-action lookup APIs are unchanged — only
 * the geometry flipped — so main.c's number-key launcher still works
 * via dock_action_for_slot().
 *
 * Per ChatGPT 2026-05-09 hard rule: every visual element must answer
 * "what changed / what did ATOMiK avoid / which personality / why
 * adapt / what would software pay" — anything else is decoration.
 * The Rail's answer: "which capability the agent predicts next, and
 * which is currently focused."  That's why the violet pulse exists
 * (predictive-AGENT signal) and why hover-grow exists (which the user
 * is about to launch).  Both are state-aware affordances. */
#include "atomik_os.h"
#include <string.h>

/* === geometry === */
/* Card-style icons: square, larger touch target than the prior 56-px
 * bottom-dock chips, with vertical spacing.  Total rail width is
 * ATOMIK_GRID_L (left margin) + icon + ATOMIK_GRID_L (right margin)
 * = 64 + 16 + 16 = ~96 px.  That carves a thin strip from the left
 * side of the workspace; everything else has more horizontal room. */
#define RAIL_X_MARGIN    ATOMIK_GRID_L      /* 16 px from screen left */
#define RAIL_TOP_MARGIN  (ATOMIK_SAFE_TOP + 32 + ATOMIK_GRID_L * 2)
                                            /* below Pulse Bar + breathing */
#define ICON_SIZE        64
#define ICON_GAP         ATOMIK_GRID_L      /* 16 px between icons */
#define RAIL_PADDING     ATOMIK_GRID_M      /* 8 px inside rail border */
#define RAIL_RADIUS      12
#define LABEL_GAP        ATOMIK_GRID_S      /* 4 px between icon and pulse */

static const struct {
    const char *label;
    pixel_t     color;
    action_t    action;
} ICONS[] = {
    { "About",     rgb(0xA8, 0xB2, 0xC4), ACT_OPEN_ABOUT    },
    { "Monitor",   rgb(0x6E, 0xC4, 0x6E), ACT_OPEN_MONITOR  },
    { "Terminal",  rgb(0x36, 0x44, 0x60), ACT_OPEN_TERMINAL },
    { "Files",     rgb(0xC9, 0x8C, 0x3C), ACT_OPEN_FILES    },
    { "Notes",     rgb(0x4F, 0xC3, 0xFF), ACT_OPEN_NOTES    },
};
#define N_ICONS ((int)(sizeof(ICONS) / sizeof(ICONS[0])))

int dock_count(void) { return N_ICONS; }

/* The Rail's outer left edge.  Consumed by other modules (e.g. main.c
 * workspace_cx() should account for the Rail like it accounts for
 * the right-side Resource Fabric shelf). */
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

/* Cached agent-score order so dock_action_for_slot resolves visible
 * positions back to original icon indices without recomputing. */
static int s_last_order[N_ICONS] = {0,1,2,3,4};

static void compute_order(int order[N_ICONS]) {
    double scores[N_ICONS];
    for (int i = 0; i < N_ICONS; i++) {
        order[i]  = i;
        scores[i] = ICONS[i].action == ACT_NONE ? 0.0
                                                : agent_score(ICONS[i].action);
    }
    /* Insertion sort — top score floats to slot 0 (top of the Rail).
     * Reads naturally because the user's eye starts at the top. */
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
    int rx = rail_x();
    int ry = rail_y();
    int rw = rail_w();
    int rh = rail_h();

    /* Rail background — translucent dark slab with a subtle border.
     * Same visual weight as the Resource Fabric shelf on the right
     * so the layout reads as bilaterally balanced chrome. */
    draw_rect_rounded(rx, ry, rw, rh, RAIL_RADIUS,
                      ATOMIK_DOCK_BG & 0xFFFFFF);
    /* 1-px border accent at top + bottom for definition.  Single
     * draw_rect calls so we don't pay the rounded-rect cost twice. */
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
        int i     = order[slot];
        int hover = (slot == hover_index);
        int size  = hover ? ICON_SIZE + 6 : ICON_SIZE;
        int off_x = (ICON_SIZE - size) / 2;
        int off_y = (ICON_SIZE - size) / 2;
        draw_rect_rounded(ix + off_x, iy + off_y, size, size, 10,
                          ICONS[i].color);

        /* First-letter chip — same convention as the prior bottom dock.
         * v0.40 swap to real glyph icons (per the design north star). */
        char ch[2] = { ICONS[i].label[0], 0 };
        int  scale = 3;
        int  tw    = text_width(ch, scale);
        int  th    = text_height(scale);
        draw_text(ix + off_x + (size - tw) / 2,
                  iy + off_y + (size - th) / 2,
                  ch, scale, ATOMIK_FG);

        /* Predicted-next pulse: violet dot to the RIGHT of the icon
         * (was below, in the bottom-dock layout).  Right-of-icon reads
         * as "what's next" given Western reading direction.  v0.32
         * established this as the AGENT semantic signal. */
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
            uint8_t r = (uint8_t)(0x9B * bright);
            uint8_t g = (uint8_t)(0x7E * bright);
            uint8_t b = (uint8_t)(0xE0 * bright);
            pixel_t pulse_color = rgb(r, g, b);
            /* Pulse dot to the right of the icon, vertically centered. */
            int dot_x = ix + ICON_SIZE + LABEL_GAP + 2;
            int dot_y = iy + ICON_SIZE / 2;
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
