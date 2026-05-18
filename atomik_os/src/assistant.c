/* assistant.c — v0.39-A summoned ATOMiK Assistant ("Atom") overlay.
 *
 * Per ChatGPT 2026-05-16 + Matt directive: silicon-octopus character
 * that EXPLAINS ATOMiK but never BECOMES ATOMiK.  Class B visual
 * asset + Class A event/metric context.  The character art is
 * pre-rendered (assets/assistant/assistant_idle_160.atomik_asset).
 * The speech is generated from deterministic templates that route
 * through fabric_active() + metric_get() — never invented.
 *
 * MVP behavior (v0.39-A):
 *   - Summon: 'O' on Atom rail cell (via dock action) OR '?'/'H' keys.
 *   - Render: glass speech bubble in lower-left workspace, 160 px
 *     character bust on the left, 2-line AA explanation on the right.
 *   - Dismiss: Esc, another summon, or auto after 12 s.
 *   - Never covers the Resource Fabric (right shelf).
 *
 * Speech templates:
 *   - SYNC active:   "SYNC is active. ATOMiK emitted N deltas and
 *                     skipped unchanged replica state."
 *   - STATE active:  "STATE is active. Repeated writes were
 *                     coalesced before commit."
 *   - AGENT active:  "AGENT is active. Hot context retained, cold
 *                     context skipped."
 *   - (no active):   "ATOMiK is idle, waiting for workload."
 *
 * Deterministic — no LLM.  Future v0.39-B will wire event-aware
 * triggers; v0.39-C will add sprite states.  This slice is the
 * static foundation. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

#define ASSIST_AUTO_DISMISS_MS  12000
#define ASSIST_BUBBLE_W         640
#define ASSIST_BUBBLE_H         220
#define ASSIST_AVATAR_PX        160
#define ASSIST_MARGIN           ATOMIK_GRID_L

static atomik_asset_t s_avatar;
static int            s_avatar_loaded = 0;
static int            s_visible       = 0;
static unsigned long  s_shown_ms      = 0;
/* Last frame's bubble rect — used to mark dirty on dismiss so the
 * desktop is repainted cleanly. */
static int s_last_x, s_last_y, s_last_w, s_last_h;

int assistant_init(void) {
    if (s_avatar_loaded) return 1;
    int rc = atomik_asset_load(
        "/tmp/atomik_assets/assistant_idle_160.atomik_asset",
        &s_avatar);
    if (rc != 0) {
        memset(&s_avatar, 0, sizeof s_avatar);
        return 0;
    }
    s_avatar_loaded = 1;
    return 1;
}

void assistant_summon(void) {
    if (s_visible) {
        /* Toggle off if already showing. */
        assistant_dismiss();
        return;
    }
    /* Ensure asset is loaded — first summon may happen after assets
     * landed on the board mid-session. */
    if (!s_avatar_loaded) assistant_init();
    s_visible  = 1;
    s_shown_ms = anim_now_ms();
}

void assistant_dismiss(void) {
    if (!s_visible) return;
    s_visible = 0;
    /* Mark the previous bubble rect dirty so the desktop redraws
     * cleanly behind it on the next frame. */
    if (s_last_w > 0 && s_last_h > 0) {
        dirty_rect(s_last_x, s_last_y, s_last_w, s_last_h);
    }
}

int assistant_visible(void) { return s_visible; }

void assistant_tick(void) {
    if (!s_visible) return;
    unsigned long now = anim_now_ms();
    if (now - s_shown_ms > ASSIST_AUTO_DISMISS_MS) {
        assistant_dismiss();
    }
}

/* Build deterministic two-line explanation for the current active
 * personality + a real metric value.  Class A discipline: every
 * number flows from metric_get() / atomik_event_total(); never
 * fabricated.  When the producer is WAITING we say so explicitly. */
static void compose_message(char *line1, size_t l1cap,
                            char *line2, size_t l2cap) {
    personality_t p = fabric_active();
    line1[0] = line2[0] = 0;
    switch (p) {
    case PERSONALITY_STATE: {
        const atomik_metric_t *coal = metric_get("state.coalesce_pct");
        snprintf(line1, l1cap, "STATE is active.");
        if (coal && coal->source != METRIC_WAITING) {
            snprintf(line2, l2cap,
                     "Repeated writes coalesced %.0f%% before commit.",
                     coal->value);
        } else {
            snprintf(line2, l2cap,
                     "Repeated writes will be coalesced before commit.");
        }
        break;
    }
    case PERSONALITY_SYNC: {
        const atomik_metric_t *ops = metric_get("sync.ops_issued");
        snprintf(line1, l1cap, "SYNC is active.");
        if (ops && ops->source != METRIC_WAITING && ops->value > 0) {
            snprintf(line2, l2cap,
                     "ATOMiK emitted %u deltas and skipped "
                     "unchanged replica state.",
                     (unsigned)ops->value);
        } else {
            snprintf(line2, l2cap,
                     "ATOMiK is replica-tracking. Unchanged regions "
                     "will be skipped.");
        }
        break;
    }
    case PERSONALITY_AGENT: {
        const atomik_metric_t *ret = metric_get("agent.ops_issued");
        snprintf(line1, l1cap, "AGENT is active.");
        if (ret && ret->source != METRIC_WAITING && ret->value > 0) {
            snprintf(line2, l2cap,
                     "Hot context: %u regions retained by relevance; "
                     "cold pruned.",
                     (unsigned)ret->value);
        } else {
            snprintf(line2, l2cap,
                     "Hot context will be retained by relevance; "
                     "cold context pruned.");
        }
        break;
    }
    default:
        snprintf(line1, l1cap, "ATOMiK is idle.");
        snprintf(line2, l2cap, "Waiting for a workload to fire.");
        break;
    }
}

void assistant_draw(void) {
    if (!s_visible) return;

    /* Position: lower-left of the workspace, anchored above the
     * bottom-of-screen edge.  Never overlaps the Fabric shelf. */
    int x = dock_right_edge() + ATOMIK_GRID_L * 2;
    int y = FB_H - ASSIST_BUBBLE_H - ATOMIK_GRID_L * 2;
    int w = ASSIST_BUBBLE_W;
    int h = ASSIST_BUBBLE_H;
    if (x + w > fabric_shelf_x() - ATOMIK_GRID_L) {
        w = fabric_shelf_x() - ATOMIK_GRID_L - x;
        if (w < 320) w = 320;
    }

    s_last_x = x; s_last_y = y; s_last_w = w; s_last_h = h;

    /* Glass bubble body — same dark navy palette as other chrome. */
    pixel_t body   = rgb(0x10, 0x18, 0x2C);
    pixel_t accent = ATOMIK_SEM_AGENT;         /* violet for the assistant */
    int radius = 18;
    draw_rect_rounded(x, y, w, h, radius, body);

    /* Layered alpha halo around the bubble — gives the "summoned"
     * lift without being a hard outline. */
    for (int g = 1; g <= 4; g++) {
        uint8_t a = (uint8_t)(36 - (g - 1) * 8);
        for (int sx = 0; sx < w + 2 * g; sx++) {
            draw_blend_pixel(x - g + sx, y - g,           accent, a);
            draw_blend_pixel(x - g + sx, y + h - 1 + g,   accent, a);
        }
        for (int sy = 0; sy < h + 2 * g; sy++) {
            draw_blend_pixel(x - g,           y - g + sy, accent, a);
            draw_blend_pixel(x + w - 1 + g,   y - g + sy, accent, a);
        }
    }
    /* Bright 1 px violet border around the rounded body. */
    draw_rect(x + radius, y,              w - radius * 2, 1, accent);
    draw_rect(x + radius, y + h - 1,      w - radius * 2, 1, accent);
    draw_rect(x,              y + radius, 1, h - radius * 2, accent);
    draw_rect(x + w - 1,      y + radius, 1, h - radius * 2, accent);

    /* Avatar — 160 px Atom bust on the left, vertically centered. */
    int av_x = x + ATOMIK_GRID_L + 4;
    int av_y = y + (h - ASSIST_AVATAR_PX) / 2;
    if (s_avatar_loaded) {
        atomik_asset_blit(&s_avatar, av_x, av_y);
    } else {
        /* Fallback — violet placeholder rectangle with "Atom" text. */
        int aw = ASSIST_AVATAR_PX;
        int ah = ASSIST_AVATAR_PX;
        draw_rect_rounded(av_x, av_y, aw, ah, 12,
                          rgb(0x1E, 0x16, 0x40));
        const char *nm = "ATOM";
        if (font_aa_loaded(FONT_AA_UI)) {
            int tw = text_width_aa(FONT_AA_UI, nm);
            int th = text_height_aa(FONT_AA_UI);
            draw_text_aa(FONT_AA_UI, av_x + (aw - tw) / 2,
                         av_y + (ah - th) / 2, nm, accent);
        }
    }

    /* Text block — name + 2-line explanation, right of avatar. */
    char line1[96], line2[160];
    compose_message(line1, sizeof line1, line2, sizeof line2);
    int tx     = av_x + ASSIST_AVATAR_PX + ATOMIK_GRID_L * 2;
    int text_w = (x + w) - tx - ATOMIK_GRID_L;
    (void)text_w;

    int row_y = y + ATOMIK_GRID_L + 6;
    if (font_aa_loaded(FONT_AA_DISPLAY)) {
        const char *who = "Atom";
        draw_text_aa(FONT_AA_DISPLAY, tx, row_y, who, accent);
        row_y += text_height_aa(FONT_AA_DISPLAY) + ATOMIK_GRID_S;
    } else {
        draw_text(tx, row_y, "Atom", 3, accent);
        row_y += text_height(3) + ATOMIK_GRID_S;
    }
    /* Line 1 — bold-ish (UI atlas). */
    if (font_aa_loaded(FONT_AA_UI)) {
        draw_text_aa(FONT_AA_UI, tx, row_y, line1, ATOMIK_FG);
        row_y += text_height_aa(FONT_AA_UI) + ATOMIK_GRID_S;
    } else {
        draw_text(tx, row_y, line1, 2, ATOMIK_FG);
        row_y += text_height(2) + ATOMIK_GRID_S;
    }
    /* Line 2 — label atlas, dim FG.  Manual wrap at character
     * boundary — keeps the bubble layout simple. */
    if (font_aa_loaded(FONT_AA_LABEL)) {
        draw_text_aa(FONT_AA_LABEL, tx, row_y, line2, ATOMIK_FG_DIM);
        row_y += text_height_aa(FONT_AA_LABEL) + 6;
    } else {
        draw_text(tx, row_y, line2, 1, ATOMIK_FG_DIM);
        row_y += text_height(1) + 6;
    }

    /* Bottom hint — "Esc dismiss · O to cycle personality" small. */
    if (font_aa_loaded(FONT_AA_LABEL)) {
        const char *hint = "Esc to dismiss";
        draw_text_aa(FONT_AA_LABEL, tx,
                     y + h - text_height_aa(FONT_AA_LABEL) - 8,
                     hint, rgb(0x5A, 0x66, 0x82));
    }

    /* Mark the bubble area dirty so the renderer composites it. */
    dirty_rect(x - 6, y - 6, w + 12, h + 12);
}
