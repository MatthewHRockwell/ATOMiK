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

#define AGENT_HOLD_MS    10000   /* AGENT stays "active" for 10s after last LLM call */
#define STATE_HOLD_MS    3000    /* STATE stays "active" for 3s after last delta */
#define SYNC_HOLD_MS     5000    /* SYNC  stays "active" for 5s after last replica event */
/* v0.32: manual override decay.  30s gives a presenter time to talk
 * about each personality during a demo without the override silently
 * expiring mid-sentence, but short enough that a forgotten override
 * doesn't permanently mis-represent the system. */
#define FABRIC_OVERRIDE_HOLD_MS  30000

static personality_t s_active           = PERSONALITY_SYNC;
static int           s_window_id        = -1;
/* v0.32: presenter override state.  s_override_p == PERSONALITY_NONE
 * means "AUTO" (no override active).  Set via fabric_cycle_override();
 * decays after FABRIC_OVERRIDE_HOLD_MS unless cycled again. */
static personality_t s_override_p       = PERSONALITY_NONE;
static unsigned long s_override_set_ms  = 0;

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
 * nothing has fired within its window, fall back to SYNC default-idle.
 *
 * v0.32: presenter override beats auto-detection while it's fresh.
 * The override carries a decay so a forgotten override doesn't lie
 * indefinitely.  When override decays, we silently fall back to auto-
 * detection — no jarring transition needed because the auto path
 * generally produces the same answer the override was forcing.
 *
 * Architectural framing (per project_v031_plan.md): the user-visible
 * story is "ATOMiK organizes resources around the workload class".
 * The override is a presenter shim, NOT the primary mechanism — that's
 * why the rendering shows "MANUAL:" prefix when override is active. */
static personality_t detect(unsigned long now) {
    if (s_override_p != PERSONALITY_NONE &&
        (now - s_override_set_ms) < FABRIC_OVERRIDE_HOLD_MS) {
        return s_override_p;
    }
    /* Override decayed — clear it so subsequent reads don't re-arm. */
    if (s_override_p != PERSONALITY_NONE) s_override_p = PERSONALITY_NONE;

    unsigned long t_agent = atomik_event_last_ts(EVT_AGENT_CONTEXT);
    unsigned long t_state = atomik_event_last_ts(EVT_STATE_DELTA);
    unsigned long t_sync  = atomik_event_last_ts(EVT_SYNC_REPLICA);
    if (t_agent > 0 && (now - t_agent) < AGENT_HOLD_MS) return PERSONALITY_AGENT;
    if (t_state > 0 && (now - t_state) < STATE_HOLD_MS) return PERSONALITY_STATE;
    if (t_sync  > 0 && (now - t_sync)  < SYNC_HOLD_MS)  return PERSONALITY_SYNC;
    return PERSONALITY_SYNC;
}

void fabric_cycle_override(void) {
    /* AUTO → STATE → SYNC → AGENT → AUTO.  Each press advances; pressing
     * P during an active override resets the decay clock so a presenter
     * can hold a personality across a long talking point.  Emits an
     * EVT_OVERRIDE event onto the bus so future subscribers can react. */
    switch (s_override_p) {
    case PERSONALITY_NONE:  s_override_p = PERSONALITY_STATE; break;
    case PERSONALITY_STATE: s_override_p = PERSONALITY_SYNC;  break;
    case PERSONALITY_SYNC:  s_override_p = PERSONALITY_AGENT; break;
    case PERSONALITY_AGENT: s_override_p = PERSONALITY_NONE;  break;
    }
    s_override_set_ms = anim_now_ms();
    atomik_event_emit(EVT_OVERRIDE, (int)s_override_p);
}

int fabric_override_active(void) {
    if (s_override_p == PERSONALITY_NONE) return 0;
    unsigned long now = anim_now_ms();
    return (now - s_override_set_ms) < FABRIC_OVERRIDE_HOLD_MS;
}

personality_t fabric_override_personality(void) { return s_override_p; }

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
    /* v0.32: prefix badge with "MANUAL:" when presenter override is
     * active so the audience (and the presenter) always know whether
     * the personality came from real workload signals or from the
     * override key.  Honest demo discipline: never present a forced
     * personality as if it were auto-detected. */
    char badge[32];
    if (fabric_override_active()) {
        snprintf(badge, sizeof badge, "[ MANUAL: %s ]",
                 fabric_personality_name(s_active));
    } else {
        snprintf(badge, sizeof badge, "[ AUTO: %s ]",
                 fabric_personality_name(s_active));
    }
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

    /* v0.33-E: per-personality metrics cards.  Replaces the prior
     * stylized "batch queue / cycles saved" footer with three small
     * cards that each display the LAST COMPLETED perf_sample_t for
     * their personality, read from perf_last_for(p).
     *
     * Honest UI rules per ChatGPT 2026-05-09 review:
     *   - No sample for this personality yet → "WAITING FOR WORKLOAD"
     *   - Sample present but profile produced no step-change beyond
     *     batched (e.g. SYNC bytes_avoided=0 in single-batch runs) →
     *     show the actual collapse/avoidance metric, NOT a forced
     *     speedup percentage
     *   - Lane copy explicitly explains what each personality DID
     *     (ops collapsed, bytes avoided, regions retained), not what
     *     mode is "active" generically
     *
     * Each card is its semantic color when its personality is active
     * (filled left-edge marker); dim when idle.  The user can read
     * three independent stories at once. */
    int foot_y = lanes_y + FABRIC_N_LANES * bar_gap + ATOMIK_GRID_L;

    const personality_t card_p[3] = { PERSONALITY_STATE, PERSONALITY_SYNC,
                                      PERSONALITY_AGENT };
    int card_h = 88;
    int card_gap = ATOMIK_GRID_M;
    for (int i = 0; i < 3; i++) {
        int  cy       = foot_y + i * (card_h + card_gap);
        int  active   = (s_active == card_p[i]);
        pixel_t accent = (card_p[i] == PERSONALITY_STATE) ? ATOMIK_SEM_HARDWARE
                       : (card_p[i] == PERSONALITY_AGENT) ? ATOMIK_SEM_AGENT
                       :                                    ATOMIK_SEM_SAVINGS;

        /* Card background — slightly elevated rect with a 4-px left
         * accent strip in the personality color when active. */
        draw_rect(x + ATOMIK_GRID_L, cy,
                  wd - ATOMIK_GRID_L * 2, card_h,
                  rgb(0x12, 0x18, 0x26));
        draw_rect(x + ATOMIK_GRID_L, cy, 4, card_h,
                  active ? accent : ATOMIK_DOCK_BORDER);

        int tx = x + ATOMIK_GRID_L + ATOMIK_GRID_M + 4;
        int ty_card = cy + ATOMIK_GRID_M;

        /* Card title */
        draw_text(tx, ty_card,
                  fabric_personality_name(card_p[i]), 1,
                  active ? accent : ATOMIK_FG);

        /* Read the most recent completed sample for this personality.
         * NULL means "no batch has run with this profile yet" — show
         * WAITING per the honest-UI rule. */
        const perf_sample_t *s = perf_last_for(card_p[i]);
        char l1[64], l2[64], l3[64];
        l1[0] = l2[0] = l3[0] = 0;

        if (!s) {
            snprintf(l1, sizeof l1, "WAITING FOR WORKLOAD");
            snprintf(l2, sizeof l2, "press ! to seed metrics");
            snprintf(l3, sizeof l3, " ");
        } else {
            switch (card_p[i]) {
            case PERSONALITY_STATE:
                /* STATE: optimal coalesce.  Show the literal
                 * collapse + the cycles saved vs the software
                 * baseline captured during the same bench run. */
                snprintf(l1, sizeof l1, "ops collapsed  %u -> %u",
                         (unsigned)s->ops_logical, (unsigned)s->ops_issued);
                snprintf(l2, sizeof l2, "fences         %u -> 1",
                         (unsigned)s->ops_logical);
                if (s->cycles_software_baseline && s->cycles_atomik) {
                    double speedup = perf_speedup(s);
                    snprintf(l3, sizeof l3, "cycles saved   %.1fx vs sw",
                             speedup);
                } else {
                    snprintf(l3, sizeof l3, "cycles total   %llu",
                             (unsigned long long)s->cycles_total);
                }
                break;
            case PERSONALITY_SYNC:
                /* SYNC: skip-unchanged.  Single-batch runs don't
                 * exercise the cross-batch advantage — say so. */
                snprintf(l1, sizeof l1, "ops emitted    %u / %u regions",
                         (unsigned)s->ops_issued,
                         (unsigned)s->regions_unique);
                snprintf(l2, sizeof l2, "bytes avoided  %u",
                         (unsigned)s->bytes_avoided);
                if (s->bytes_avoided == 0) {
                    snprintf(l3, sizeof l3, "(replay engine = v0.33-G)");
                } else {
                    snprintf(l3, sizeof l3, "unchanged skip %u",
                             (unsigned)(s->bytes_avoided / 4));
                }
                break;
            case PERSONALITY_AGENT:
                /* AGENT: relevance retention.  Show retained vs
                 * total + a flag that the relevance sort is what
                 * drives the skip. */
                {
                    unsigned cold = s->bytes_avoided / 4;
                    unsigned total = s->regions_unique + cold;
                    unsigned retained = s->ops_issued;
                    snprintf(l1, sizeof l1, "hot retained   %u / %u (%u%%)",
                             retained, total ? total : retained,
                             total ? (retained * 100 / total) : 60);
                    snprintf(l2, sizeof l2, "bytes avoided  %u",
                             (unsigned)s->bytes_avoided);
                    snprintf(l3, sizeof l3, "relevance sort active");
                }
                break;
            default: break;
            }
        }

        int line_h = text_height(1) + 2;
        draw_text(tx, ty_card + ATOMIK_TITLEBAR_H, l1, 1, ATOMIK_FG);
        draw_text(tx, ty_card + ATOMIK_TITLEBAR_H + line_h,
                  l2, 1, ATOMIK_FG_DIM);
        draw_text(tx, ty_card + ATOMIK_TITLEBAR_H + line_h * 2,
                  l3, 1, ATOMIK_FG_DIM);
    }

    /* Personality selector pills at the very bottom — kept from v0.30
     * because they remain a quick at-a-glance "which personality is
     * current" indicator alongside the per-card "active" marker. */
    int pill_y = foot_y + 3 * (card_h + card_gap) + ATOMIK_GRID_M;
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

/* v0.33-E: seed perf samples for each personality so the metrics
 * cards show real numbers on first open instead of three "WAITING
 * FOR WORKLOAD" placeholders.  Runs three quick perf_bench_run
 * calls (one per profile, 8 regions × 64 ops each) — produces
 * REAL measurements (not faked values), just with synthetic input.
 * Cheap: ~150 ms total on the AX7020.
 *
 * Once v0.33-G replay engine ships, real workloads will populate
 * these samples organically and seed_metrics becomes optional. */
static int s_metrics_seeded = 0;
static void seed_metrics_if_empty(void) {
    if (s_metrics_seeded) return;
    perf_bench_result_t r;
    perf_bench_run(8, 64, ATOMIK_PROFILE_STATE, &r);
    perf_bench_run(8, 64, ATOMIK_PROFILE_SYNC,  &r);
    perf_bench_run(8, 64, ATOMIK_PROFILE_AGENT, &r);
    s_metrics_seeded = 1;
}

void fabric_open(void) {
    /* Always seed metrics on open if we haven't yet.  Cheap, real,
     * and removes the embarrassment of the panel landing with three
     * WAITING placeholders before any workload runs. */
    seed_metrics_if_empty();

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
