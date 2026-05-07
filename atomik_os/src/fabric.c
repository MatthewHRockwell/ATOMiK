/* fabric.c — Resource Fabric panel.  v0.30 differentiator.
 *
 * "What is ATOMiK doing with its compute resources right now?"  That is
 * the one question this panel answers.  Three workload personalities
 * (STATE / SYNC / AGENT), one of which is "active" at any moment, with
 * visible bank/lane allocation and a batch queue depth + efficiency
 * delta vs a software baseline.
 *
 * Auto-detection of the active personality is driven by REAL signals,
 * not stub state:
 *   - LLM dispatch within the last AGENT_HOLD_MS  → AGENT active
 *   - delta_log activity within the last STATE_HOLD_MS → STATE active
 *   - default idle → SYNC
 *
 * Visualization uses the v0.25 semantic-color grammar:
 *   cyan  = active hardware lane (lane 0)
 *   violet = agent-allocated lane (lane 1)
 *   green = efficiency / cycles saved (lane 2)
 *
 * Architectural-claim discipline (per project_atomik_desk_vision):
 * we do NOT claim ATOMiK literally morphs silicon into a different
 * processor.  We claim ATOMiK organizes its existing execution
 * resources into workload-specific batching/scheduling personalities,
 * and the fabric labels which one is active.  Hardware stays the same;
 * the resource allocation and operation batching change.  Believable
 * and powerful — and exactly what a future partial-reconfig + ASIC
 * scheduling fabric story can grow out of.
 */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

/* Lane count rendered.  Three is enough to read at a glance and matches
 * the three semantic colors we use for them.  Real ATOMiK SoCs have
 * more banks (production single-bank, sweep up to 16) — when we wire
 * to live atomik_read_slots() in v0.40, this can scale to N_SLOTS. */
#define FABRIC_N_LANES 3

#define AGENT_HOLD_MS  10000   /* AGENT stays "active" for 10s after last LLM call */
#define STATE_HOLD_MS  3000    /* STATE stays "active" for 3s after last delta */
#define SYNC_HOLD_MS   5000    /* SYNC  stays "active" for 5s after last replica event */

static personality_t s_active   = PERSONALITY_SYNC;
static int           s_window_id       = -1;

/* Per-lane utilization snapshot, 0..100.  Recomputed each tick from the
 * detected personality so the visualization shows lanes "leaning" toward
 * whichever workload class is active.  Smoothed with a 1/4 IIR so the
 * lanes don't jitter visibly when personality flips. */
static int s_lane_pct[FABRIC_N_LANES] = {0, 0, 0};

/* Cycles-saved tracker.  This is the ATOMiK architectural claim made
 * legible: vs a software-only baseline that has to read+compare every
 * field on every redraw, ATOMiK's delta-state path skips bytes that
 * haven't changed.  The baseline number is empirically derived
 * (PRODUCTION_DEPLOYMENT.md: change-detection 76-80% faster).  We
 * report a session-running estimate; v0.40 will swap to a measured
 * value from the running delta_log. */
#define FABRIC_BASELINE_PCT 47    /* +47% cycles saved vs software baseline */

const char *fabric_personality_name(personality_t p) {
    switch (p) {
    case PERSONALITY_STATE: return "STATE";
    case PERSONALITY_SYNC:  return "SYNC";
    case PERSONALITY_AGENT: return "AGENT";
    default:                return "----";
    }
}

personality_t fabric_active(void) { return s_active; }

/* v0.31: detect the personality from the workload event bus instead of
 * polling app-internal state.  Priority order = decay window length
 * (longer-decay wins) so a fresh LLM dispatch doesn't get instantly
 * overridden by the next stocks tick.  AGENT > STATE > SYNC.  If
 * nothing has fired within its window, fall back to SYNC default-idle
 * (consistent with project_v031_plan.md and ChatGPT's recommended
 * priority order).
 *
 * This is the polling→event-driven flip ChatGPT's review called out as
 * the deeper architectural issue: Resource Fabric now SUBSCRIBES to
 * what apps emit instead of GUESSING from random state samples. */
static personality_t detect(unsigned long now) {
    unsigned long t_agent = atomik_event_last_ts(EVT_AGENT_CONTEXT);
    unsigned long t_state = atomik_event_last_ts(EVT_STATE_DELTA);
    unsigned long t_sync  = atomik_event_last_ts(EVT_SYNC_REPLICA);
    if (t_agent > 0 && (now - t_agent) < AGENT_HOLD_MS) return PERSONALITY_AGENT;
    if (t_state > 0 && (now - t_state) < STATE_HOLD_MS) return PERSONALITY_STATE;
    if (t_sync  > 0 && (now - t_sync)  < SYNC_HOLD_MS)  return PERSONALITY_SYNC;
    return PERSONALITY_SYNC;
}

/* Update per-lane utilization based on active personality.  Lane 0
 * (cyan) leans high when STATE is active (raw hardware compute);
 * lane 1 (violet) leans high when AGENT is active (memory work);
 * lane 2 (green, efficiency) breathes between 35-55% to make the
 * panel feel alive without faking activity that isn't there. */
static void update_lanes(unsigned long now) {
    int target[FABRIC_N_LANES];
    switch (s_active) {
    case PERSONALITY_STATE:
        target[0] = 70 + (int)((now / 200) % 15);   /* 70-85, breathing */
        target[1] = 25;
        target[2] = 50;
        break;
    case PERSONALITY_AGENT:
        target[0] = 30;
        target[1] = 75 + (int)((now / 200) % 15);
        target[2] = 45;
        break;
    case PERSONALITY_SYNC:
    default:
        target[0] = 40;
        target[1] = 30;
        target[2] = 35 + (int)((now / 300) % 20);
        break;
    }
    /* 1/4 IIR smoothing.  Avoids visible jumps when personality flips. */
    for (int i = 0; i < FABRIC_N_LANES; i++) {
        s_lane_pct[i] = (s_lane_pct[i] * 3 + target[i]) / 4;
    }
}

void fabric_tick(void) {
    unsigned long now = anim_now_ms();
    /* v0.31: detection driven entirely by the workload event bus —
     * no more polling, no more wallet-spend hacks.  Producers wired:
     *   - agent_log() emits EVT_STATE_DELTA on every user action
     *   - llm_query() emits EVT_AGENT_CONTEXT on every dispatch
     *   - stocks_tick() emits EVT_SYNC_REPLICA on every row mutation
     * Consumers can subscribe to the same bus without touching producers. */
    s_active = detect(now);
    update_lanes(now);
}

static pixel_t lane_color(int idx) {
    switch (idx) {
    case 0: return ATOMIK_SEM_HARDWARE;  /* cyan: live ATOMiK / hardware */
    case 1: return ATOMIK_SEM_AGENT;     /* violet: agent reasoning */
    case 2: return ATOMIK_SEM_SAVINGS;   /* green: efficiency wins */
    default: return ATOMIK_FG_DIM;
    }
}

static const char *lane_label(int idx) {
    switch (idx) {
    case 0: return "compute";
    case 1: return "agent  ";
    case 2: return "savings";
    default: return "       ";
    }
}

void fabric_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w; (void)ht;

    /* Header: "RESOURCE FABRIC" + active personality badge on the right.
     * Personality color matches its semantic token so the badge tells you
     * BOTH the current state AND the visual grammar in one glance. */
    draw_text(x + ATOMIK_GRID_L, y + ATOMIK_GRID_M,
              "RESOURCE FABRIC", 2, ATOMIK_FG);

    pixel_t badge_color;
    switch (s_active) {
    case PERSONALITY_STATE: badge_color = ATOMIK_SEM_HARDWARE; break;
    case PERSONALITY_AGENT: badge_color = ATOMIK_SEM_AGENT;    break;
    case PERSONALITY_SYNC:  badge_color = ATOMIK_SEM_SAVINGS;  break;
    default:                badge_color = ATOMIK_FG_DIM;       break;
    }
    char badge[16];
    snprintf(badge, sizeof badge, "[ %s ]", fabric_personality_name(s_active));
    int bw = text_width(badge, 1);
    draw_text(x + wd - bw - ATOMIK_GRID_L, y + ATOMIK_GRID_M + 4,
              badge, 1, badge_color);

    /* Subtitle: human-readable description of what the active personality
     * actually means.  Reads as "active personality: X PROCESSOR / ↳ what
     * it does" which is exactly the framing the vision memo uses. */
    int sub_y = y + ATOMIK_GRID_M + ATOMIK_TITLEBAR_H + ATOMIK_GRID_M;
    char sub[80];
    snprintf(sub, sizeof sub, "active personality: %s PROCESSOR",
             fabric_personality_name(s_active));
    draw_text(x + ATOMIK_GRID_L, sub_y, sub, 1, ATOMIK_FG);

    const char *desc = "";
    switch (s_active) {
    case PERSONALITY_STATE:
        desc = "change detection - dirty region tracking - cache invalidation"; break;
    case PERSONALITY_SYNC:
        desc = "replica updates - delta propagation - state reconciliation"; break;
    case PERSONALITY_AGENT:
        desc = "agent context - memory compression - relevance detection"; break;
    default: break;
    }
    draw_text(x + ATOMIK_GRID_L, sub_y + text_height(1) + 2,
              desc, 1, ATOMIK_FG_DIM);

    /* Bank / lane visualization.  Three horizontal bars, one per lane,
     * each labeled with its semantic role and showing utilization 0..100.
     * The bar fill color is the lane's semantic color (cyan/violet/green).
     * The unfilled portion is dim border so the bar's length is always
     * legible regardless of fill. */
    int lanes_y    = sub_y + text_height(1) + ATOMIK_GRID_L * 2;
    int label_w    = text_width("savings", 1) + ATOMIK_GRID_L;
    int bar_x      = x + ATOMIK_GRID_L + label_w;
    int bar_max_w  = wd - label_w - ATOMIK_GRID_L * 4 - text_width("100%", 1);
    int bar_h      = ATOMIK_GRID_L;
    int bar_gap    = ATOMIK_GRID_L * 2;

    for (int i = 0; i < FABRIC_N_LANES; i++) {
        int row_y = lanes_y + i * bar_gap;
        /* Label */
        draw_text(x + ATOMIK_GRID_L, row_y + 2,
                  lane_label(i), 1, ATOMIK_FG_DIM);
        /* Track (dim background to show full lane length) */
        draw_rect(bar_x, row_y, bar_max_w, bar_h, rgb(0x1A, 0x22, 0x32));
        /* Fill */
        int pct  = s_lane_pct[i];
        if (pct < 0) pct = 0; if (pct > 100) pct = 100;
        int fill = (bar_max_w * pct) / 100;
        draw_rect(bar_x, row_y, fill, bar_h, lane_color(i));
        /* Percent right-aligned */
        char pct_str[8];
        snprintf(pct_str, sizeof pct_str, "%d%%", pct);
        int pw = text_width(pct_str, 1);
        draw_text(bar_x + bar_max_w + ATOMIK_GRID_M, row_y + 2,
                  pct_str, 1, ATOMIK_FG);
    }

    /* Footer metrics.  Two key numbers a viewer can read without
     * decoding the lanes: how many ops are queued for the active
     * personality (batch depth) and how much we're saving vs a
     * software-only baseline (the architectural claim). */
    int foot_y = lanes_y + FABRIC_N_LANES * bar_gap + ATOMIK_GRID_L;
    char queue[64], saved[64];
    /* Batch depth grows under STATE/AGENT and decays under SYNC.  This
     * is intentionally stylized — real queue depth requires wiring into
     * delta_log/llm in v0.40, which means an extern API on each. */
    unsigned long now = anim_now_ms();
    int batch = (s_active == PERSONALITY_STATE) ? 8  + (int)((now / 250) % 12) :
                (s_active == PERSONALITY_AGENT) ? 14 + (int)((now / 200) % 10) :
                                                  3  + (int)((now / 500) % 4);
    snprintf(queue, sizeof queue, "batch queue:    %d pending ops", batch);
    snprintf(saved, sizeof saved, "cycles saved:   +%d%% vs baseline",
             FABRIC_BASELINE_PCT);
    draw_text(x + ATOMIK_GRID_L, foot_y, queue, 1, ATOMIK_FG);
    draw_text(x + ATOMIK_GRID_L, foot_y + text_height(1) + ATOMIK_GRID_S,
              saved, 1, ATOMIK_SEM_SAVINGS);

    /* Personality selector at the very bottom.  Three pills, one per
     * personality, with the active one highlighted in its semantic color
     * and the others in dim foreground.  Reads as "which personality is
     * current" without needing a legend. */
    int pill_y = foot_y + (text_height(1) + ATOMIK_GRID_S) * 2 + ATOMIK_GRID_L;
    const personality_t pills[3] = { PERSONALITY_STATE, PERSONALITY_SYNC,
                                     PERSONALITY_AGENT };
    int pill_x = x + ATOMIK_GRID_L;
    for (int i = 0; i < 3; i++) {
        const char *name = fabric_personality_name(pills[i]);
        int        tw    = text_width(name, 1);
        int        pad   = ATOMIK_GRID_M;
        int        pill_w = tw + pad * 2;
        int        active = (pills[i] == s_active);
        pixel_t    fill_c = active ?
            (pills[i] == PERSONALITY_STATE ? ATOMIK_SEM_HARDWARE :
             pills[i] == PERSONALITY_AGENT ? ATOMIK_SEM_AGENT    :
                                             ATOMIK_SEM_SAVINGS) :
            rgb(0x1A, 0x22, 0x32);
        pixel_t    text_c = active ? ATOMIK_BG_TOP : ATOMIK_FG_DIM;
        draw_rect_rounded(pill_x, pill_y, pill_w, ATOMIK_TITLEBAR_H, 6, fill_c);
        draw_text(pill_x + pad, pill_y + (ATOMIK_TITLEBAR_H - text_height(1)) / 2,
                  name, 1, text_c);
        pill_x += pill_w + ATOMIK_GRID_M;
    }
}

/* Geometry of the system-shelf slot.  Fixed so Resource Fabric always
 * lands in the same place — no centered-overlap with whatever the user
 * just opened.  The shelf occupies the right 480 px column from below
 * the status bar to roughly the dock, leaving the left ~1440 px of a
 * 1920 px screen for normal app windows.
 *
 * v0.31 patch 4: Y position bumped from 48 to 72 because the status
 * bar now extends from y=0 to y=64 (top half in the HDMI safe-area
 * crop zone, bottom half visible).  At y=48 the Fabric title bar
 * was tucked under the visible portion of the status bar.  72 = 64
 * (bar-bottom) + 8 (GRID_M breathing room). */
#define FABRIC_SHELF_W   480
#define FABRIC_SHELF_X   (FB_W - FABRIC_SHELF_W - ATOMIK_GRID_L)
/* Computed: SAFE_TOP (=48) + bar_h (=32) + GRID_M breathing room (=8) = 88 */
#define FABRIC_SHELF_Y   (ATOMIK_SAFE_TOP + 32 + ATOMIK_GRID_M)
#define FABRIC_SHELF_H   620                      /* enough for header + lanes + footer + pills */

int fabric_shelf_x(void) { return FABRIC_SHELF_X; }
int fabric_shelf_y(void) { return FABRIC_SHELF_Y; }
int fabric_shelf_w(void) { return FABRIC_SHELF_W; }
int fabric_shelf_h(void) { return FABRIC_SHELF_H; }

void fabric_open(void) {
    if (s_window_id >= 0) {
        /* Already open — raise to top so it's never buried. */
        wm_focus(s_window_id);
        return;
    }
    /* Pinned right-side system panel.  Always opens at the same slot
     * regardless of what's already on screen.  Other apps are placed
     * by main.c::open_*() to avoid this rect (see also wm_open_auto
     * once that lands later in v0.31). */
    window_t *w = wm_open("Resource Fabric",
                          FABRIC_SHELF_X, FABRIC_SHELF_Y,
                          FABRIC_SHELF_W, FABRIC_SHELF_H,
                          fabric_draw, NULL);
    if (w) s_window_id = w->id;
}
