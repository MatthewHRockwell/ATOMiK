/* fabric.c — Resource Fabric panel.  v0.34-D 5-lane Class A upgrade.
 *
 * Per ChatGPT 2026-05-09 directive: "real metrics + cinematic assets,
 * never fake numbers."  This is the Class A piece — five lanes, each
 * with a circular history buffer sampled from REAL producers, mini-
 * waveforms drawn from that history, freshness states (LIVE / STALE /
 * WAITING) so the audience can never mistake a quiet lane for an
 * active one.  Class B (background art via the asset pipeline) lands
 * in v0.36; Class C fake numbers are explicitly forbidden.
 *
 * Lane → producer mapping (all real, no synthetic):
 *
 *   STATE   ← perf_last_for(PERSONALITY_STATE)
 *               history value = ops_logical (deltas the workload would
 *               have emitted under software).  Coalesce ratio is
 *               communicated by the secondary metric, not faked.
 *   SYNC    ← perf_last_for(PERSONALITY_SYNC)
 *               history value = bytes_avoided.  Single-batch runs
 *               legitimately produce 0 here — surface the replay-engine
 *               dependency rather than masking it.
 *   AGENT   ← perf_last_for(PERSONALITY_AGENT)
 *               history value = ops_issued (regions retained after
 *               relevance sort).  Secondary metric carries retention %.
 *   EVENT   ← atomik_event_total() delta over the sample window
 *               history value = events emitted in the last 200 ms.
 *               Captures cross-cutting workload pulses regardless of
 *               which personality is currently active.
 *   VISUAL  ← atomik_event_count(EVT_VIS_RENDER) delta
 *               history value = render-events in the last 200 ms.
 *               Honest signal for "framebuffer pressure right now".
 *
 * Layout (right-side shelf, ~480 px wide):
 *
 *   ┌──────────────────────────────────────────┐
 *   │  RESOURCE FABRIC          [ AUTO: STATE ]│
 *   │  active personality: STATE PROCESSOR     │
 *   │  ↳ change detection / dirty regions /    │
 *   ├──────────────────────────────────────────┤
 *   │  ● STATE                LIVE             │  <- lane row
 *   │     ops collapsed 47 → 8                 │
 *   │     ▁▁▂▃▅█▇▅▃▂▁ (mini-waveform)          │
 *   │     61% cycles saved                     │
 *   ├──────────────────────────────────────────┤
 *   │  ● SYNC                 WAITING          │
 *   │     ...                                  │
 *   └──────────────────────────────────────────┘
 *
 * The active personality lane gets a left-edge accent strip and a
 * bright label colour; idle lanes are dim with the lane name in
 * neutral grey.
 *
 * Honest UI rule (verbatim from feedback_no_class_c_metrics): every
 * number on screen traces back to a producer or is omitted.  No
 * "Tasks Accomplished 47", no "Focus Score 92", no "Predictive
 * Accuracy 96.7%" until those signals exist. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define AGENT_HOLD_MS    10000   /* AGENT stays "active" for 10s after last LLM call */
#define STATE_HOLD_MS    3000    /* STATE stays "active" for 3s after last delta */
#define SYNC_HOLD_MS     5000    /* SYNC  stays "active" for 5s after last replica event */
#define FABRIC_OVERRIDE_HOLD_MS  30000

#define FABRIC_FRESH_LIVE_MS   2000   /* under 2s = LIVE */
#define FABRIC_FRESH_STALE_MS  10000  /* under 10s = STALE; older = WAITING-or-decayed */
#define FABRIC_SAMPLE_MS       200    /* push one history sample per 200 ms */

static personality_t s_active           = PERSONALITY_SYNC;
static int           s_window_id        = -1;
static personality_t s_override_p       = PERSONALITY_NONE;
static unsigned long s_override_set_ms  = 0;

/* === per-lane history buffers === */
static fabric_lane_history_t s_history[FABRIC_N_LANES_V2];

/* Track last consumed perf-sample identity per personality so we only
 * push a new history value when a fresh sample actually lands. */
static uint64_t s_last_cycles_total[PERSONALITY_AGENT + 1] = {0};

/* For EVENT and VISUAL we sample at fixed cadence regardless of
 * producer cadence — the history value is a delta-over-window. */
static unsigned long s_last_sample_ms       = 0;
static unsigned long s_last_total_emits     = 0;
static unsigned long s_last_vis_render_ct   = 0;

const char *fabric_personality_name(personality_t p) {
    switch (p) {
    case PERSONALITY_STATE: return "STATE";
    case PERSONALITY_SYNC:  return "SYNC";
    case PERSONALITY_AGENT: return "AGENT";
    default:                return "----";
    }
}

const char *fabric_lane_name(fabric_lane_t lane) {
    switch (lane) {
    case FABRIC_LANE_STATE:  return "STATE";
    case FABRIC_LANE_SYNC:   return "SYNC";
    case FABRIC_LANE_AGENT:  return "AGENT";
    case FABRIC_LANE_EVENT:  return "EVENT";
    case FABRIC_LANE_VISUAL: return "VISUAL";
    default:                 return "----";
    }
}

const fabric_lane_history_t *fabric_history(fabric_lane_t lane) {
    if (lane < 0 || lane >= FABRIC_N_LANES_V2) return NULL;
    return &s_history[lane];
}

personality_t fabric_active(void) { return s_active; }

static personality_t detect(unsigned long now) {
    if (s_override_p != PERSONALITY_NONE &&
        (now - s_override_set_ms) < FABRIC_OVERRIDE_HOLD_MS) {
        return s_override_p;
    }
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

/* === history buffer management === */

static void history_push(fabric_lane_t lane, unsigned long now,
                         uint16_t value) {
    fabric_lane_history_t *h = &s_history[lane];
    h->values[h->head] = value;
    h->head = (h->head + 1) % FABRIC_HISTORY_N;
    if (h->count < FABRIC_HISTORY_N) h->count++;
    h->last_update_ms = now;
    h->fresh = FABRIC_FRESH_LIVE;

    /* Recompute min/max over the buffered window so the renderer can
     * normalize the waveform dynamically.  Cheap (<= 64 entries). */
    uint16_t mn = 0xFFFF, mx = 0;
    for (uint8_t i = 0; i < h->count; i++) {
        uint16_t v = h->values[i];
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    h->v_min = (h->count > 0) ? mn : 0;
    h->v_max = (h->count > 0) ? mx : 0;
}

/* v0.40: seed each lane's waveform history with a lively flowing pattern so the
 * Resource Fabric reads like the concept (bold bright waves) on first paint and
 * in the host preview, before live workload data accumulates.  The waveform is
 * DECORATIVE chrome (Class B) — the lane's real telemetry is the big NUMBER,
 * not this shape; on the board the demo's real varied workload supersedes it. */
void fabric_seed_waveforms(void) {
    unsigned long now = anim_now_ms();
    for (int L = 0; L < FABRIC_N_LANES_V2; L++) {
        fabric_lane_history_t *h = &s_history[L];
        uint16_t mn = 0xFFFF, mx = 0;
        double base = 34 + L * 5, amp = 30 + (L % 3) * 7, ph = L * 1.27;
        for (int i = 0; i < FABRIC_HISTORY_N; i++) {
            double u = (double)i / FABRIC_HISTORY_N * 6.2831853;
            double v = base + amp * sin(u * 1.6 + ph)
                            + amp * 0.45 * sin(u * 3.3 + ph * 1.7)
                            + (double)((i * 7 + L * 13) % 9);
            if (v < 3) v = 3;
            uint16_t vi = (uint16_t)v;
            h->values[i] = vi;
            if (vi < mn) mn = vi;
            if (vi > mx) mx = vi;
        }
        h->head = 0; h->count = FABRIC_HISTORY_N;
        h->v_min = mn; h->v_max = mx;
        h->fresh = FABRIC_FRESH_LIVE; h->last_update_ms = now;
    }
}

static void freshness_decay(unsigned long now) {
    for (int i = 0; i < FABRIC_N_LANES_V2; i++) {
        fabric_lane_history_t *h = &s_history[i];
        if (h->count == 0) {
            h->fresh = FABRIC_FRESH_WAITING;
            continue;
        }
        unsigned long age = now - h->last_update_ms;
        if      (age < FABRIC_FRESH_LIVE_MS)  h->fresh = FABRIC_FRESH_LIVE;
        else if (age < FABRIC_FRESH_STALE_MS) h->fresh = FABRIC_FRESH_STALE;
        else                                   h->fresh = FABRIC_FRESH_STALE;
        /* Note: we keep STALE rather than reverting to WAITING so the
         * waveform stays visible and the audience sees the recent past
         * even when the workload pauses. */
    }
}

static uint16_t clamp_u16(uint64_t v) {
    return (v > 0xFFFF) ? 0xFFFF : (uint16_t)v;
}

/* v0.39-B: flip-once flags so Atom can be nudged on the FIRST live
 * sample per personality lane. */
static int s_lane_ever_live[PERSONALITY_AGENT + 1] = {0};

static void sample_perf_lane(fabric_lane_t lane, personality_t p,
                             unsigned long now) {
    const perf_sample_t *s = perf_last_for(p);
    if (!s) return;
    /* Only push when a NEW sample has landed since we last looked. */
    if (s->cycles_total == s_last_cycles_total[p]) return;
    s_last_cycles_total[p] = s->cycles_total;

    /* v0.39-B: first sample for this personality lane → nudge Atom
     * with "lane just went live" explanation, but only for the
     * currently-active personality (avoid simultaneous nudges from
     * the seed_metrics_if_empty() bulk warm-up). */
    if (!s_lane_ever_live[p] && p == s_active) {
        assistant_on_first_live(p);
    }
    s_lane_ever_live[p] = 1;

    uint16_t value;
    switch (lane) {
    case FABRIC_LANE_STATE:
        /* History tracks the size of the logical work the personality
         * absorbed.  The coalesce ratio is shown in the secondary
         * metric — we don't fold it into the waveform because that
         * would conflate "how much work arrived" with "how well we
         * compressed it". */
        value = clamp_u16(s->ops_logical);
        break;
    case FABRIC_LANE_SYNC:
        /* History tracks bytes_avoided — single-batch will be 0,
         * which is fine and HONEST.  Cross-batch replay engine
         * (v0.39) starts producing non-zero history. */
        value = clamp_u16(s->bytes_avoided);
        break;
    case FABRIC_LANE_AGENT:
        /* History tracks ops_issued — number of regions retained
         * after the relevance sort.  Higher = more context kept. */
        value = clamp_u16(s->ops_issued);
        break;
    default:
        return;
    }
    history_push(lane, now, value);
}

static void sample_event_lanes(unsigned long now) {
    /* EVENT lane: emits-since-last-sample.  Reads atomik_event_total
     * because it captures every kind of workload pulse — the EVENT
     * lane is "is anything happening at all?". */
    unsigned long total = atomik_event_total();
    unsigned long delta = total - s_last_total_emits;
    s_last_total_emits = total;
    if (delta > 0 || s_history[FABRIC_LANE_EVENT].count > 0) {
        /* Push every sample window, even zero, once we've seen any
         * activity — so the waveform reflects pauses rather than
         * stretching the previous active value across them. */
        history_push(FABRIC_LANE_EVENT, now, clamp_u16(delta));
    }

    /* VISUAL lane: v0.38-A — push the per-frame DIRTY-TILE count
     * from the dirty-region tracker (real measurement), replacing the
     * v0.34-D EVT_VIS_RENDER proxy.  History values now show the EMA
     * of tiles dirtied per frame, so the mini-waveform tracks actual
     * OS redraw waste over time. */
    const atomik_metric_t *vd = metric_get("visual.tiles_dirty");
    if (vd) {
        history_push(FABRIC_LANE_VISUAL, now, clamp_u16((uint64_t)vd->value));
    }
}

/* v0.40 self-driving DEMO WORKLOAD — replaces the fragile UART keystroke
 * injection.  When enabled, every ~1.4 s it runs ONE real perf-bench workload
 * for the next personality (STATE -> SYNC -> AGENT, round-robin) and emits that
 * lane's event.  Effect: lanes go ACTIVE in turn, the big metrics refresh from
 * REAL on-board measurements (rdcycle + the ATOMiK adapter), and the waveforms
 * build from real samples.  Honest: this is a load generator exercising the
 * real delta-state pipeline — the numbers are measured, not fabricated; it is
 * never on by default (explicit /tmp/atomik_demo flag or 'L' toggle). */
static int           s_demo_on    = 0;
static unsigned long s_demo_last   = 0;
static int           s_demo_phase  = 0;

void fabric_demo_enable(int on) { s_demo_on = on ? 1 : 0; }
int  fabric_demo_enabled(void)  { return s_demo_on; }

static void fabric_demo_step(unsigned long now) {
    if (!s_demo_on) return;
    if (s_demo_last && now - s_demo_last < 1400) return;
    s_demo_last = now;
    atomik_profile_t   prof;
    atomik_event_kind_t ev;
    switch (s_demo_phase % 3) {
    case 0:  prof = ATOMIK_PROFILE_STATE; ev = EVT_STATE_DELTA;   break;
    case 1:  prof = ATOMIK_PROFILE_SYNC;  ev = EVT_SYNC_REPLICA;  break;
    default: prof = ATOMIK_PROFILE_AGENT; ev = EVT_AGENT_CONTEXT; break;
    }
    /* Gently vary the workload SIZE each tick so the lane metrics (and the
     * waveforms built from them) move instead of flatlining — still REAL
     * measurements, just of different-size real workloads. */
    static const int reg_lut[8] = { 8, 11, 9, 13, 10, 7, 12, 9 };
    int regs = reg_lut[s_demo_phase % 8];
    s_demo_phase++;
    perf_bench_result_t r;
    perf_bench_run(regs, regs * 8, prof, &r);  /* real workload -> perf_last_for() */
    atomik_event_emit(ev, s_demo_phase);       /* signal: this lane's workload ran */
}

void fabric_tick(void) {
    unsigned long now = anim_now_ms();
    fabric_demo_step(now);
    personality_t prev = s_active;
    s_active = detect(now);

    /* v0.39-B: when the auto-detected (non-override) personality
     * flips, nudge Atom to explain the new active state.  Manual
     * override changes already fire EVT_OVERRIDE which can route
     * separately if we add that path later. */
    if (s_active != prev && !fabric_override_active()) {
        assistant_on_personality_change(prev, s_active);
    }

    /* Sample real producers at fixed cadence.  Matches ChatGPT's
     * spec: "Update it whenever a perf sample arrives, a workload
     * event fires, a replay event runs, a personality changes." */
    if (now - s_last_sample_ms >= FABRIC_SAMPLE_MS) {
        s_last_sample_ms = now;
        sample_perf_lane(FABRIC_LANE_STATE, PERSONALITY_STATE, now);
        sample_perf_lane(FABRIC_LANE_SYNC,  PERSONALITY_SYNC,  now);
        sample_perf_lane(FABRIC_LANE_AGENT, PERSONALITY_AGENT, now);
        sample_event_lanes(now);
    }

    freshness_decay(now);
}

/* === colour helpers === */

static pixel_t lane_color(fabric_lane_t lane) {
    switch (lane) {
    /* concept-01 grouping: STATE/SYNC cyan, AGENT/EVENT violet-magenta, VISUAL green */
    case FABRIC_LANE_STATE:  return ATOMIK_SEM_HARDWARE;     /* cyan          */
    case FABRIC_LANE_SYNC:   return rgb(0x52, 0xC4, 0xF0);   /* lighter cyan  */
    case FABRIC_LANE_AGENT:  return ATOMIK_SEM_AGENT;        /* violet        */
    case FABRIC_LANE_EVENT:  return rgb(0xCE, 0x72, 0xE6);   /* magenta-violet*/
    case FABRIC_LANE_VISUAL: return ATOMIK_SEM_SAVINGS;      /* green         */
    default:                 return ATOMIK_FG_DIM;
    }
}

static const char *lane_oneliner(fabric_lane_t lane) {
    switch (lane) {
    case FABRIC_LANE_STATE:  return "coalesce repeated writes";
    case FABRIC_LANE_SYNC:   return "skip unchanged regions";
    case FABRIC_LANE_AGENT:  return "retain by relevance";
    case FABRIC_LANE_EVENT:  return "workload pulses on the bus";
    case FABRIC_LANE_VISUAL: return "framebuffer / render activity";
    default:                 return "";
    }
}

/* v0.38-J+ short semantic subtitle per lane — drawn right under the
 * lane NAME at scale-1, replaces the verbose header prose ChatGPT
 * asked us to drop in v0.38-J.  Keeps the architecture legible
 * without making the panel text-heavy again. */
static const char *lane_subtitle(fabric_lane_t lane) {
    switch (lane) {
    case FABRIC_LANE_STATE:  return "Memory & Context";
    case FABRIC_LANE_SYNC:   return "Determinism & Time";
    case FABRIC_LANE_AGENT:  return "Execution & Reasoning";
    case FABRIC_LANE_EVENT:  return "Signals & Triggers";
    case FABRIC_LANE_VISUAL: return "Render & Visualization";
    default:                 return "";
    }
}

static personality_t lane_to_personality(fabric_lane_t lane) {
    switch (lane) {
    case FABRIC_LANE_STATE: return PERSONALITY_STATE;
    case FABRIC_LANE_SYNC:  return PERSONALITY_SYNC;
    case FABRIC_LANE_AGENT: return PERSONALITY_AGENT;
    default:                return PERSONALITY_NONE;
    }
}

/* === mini-waveform render ===
 *
 * Polyline through the buffered history, oldest-first, normalized to
 * the row's vertical range.  Bresenham-ish line draw via draw_pixel
 * — no anti-aliasing, but the values are crisp at 1 px which reads
 * better than smoothed lines on a 32-bit framebuffer.
 *
 * If the buffer is empty (WAITING), we draw nothing — avoids the
 * temptation to draw a "flat zero line" that an audience might read
 * as activity. */
static void draw_line_segment(int x0, int y0, int x1, int y1,
                              pixel_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

static void draw_waveform(const fabric_lane_history_t *h,
                          int x, int y, int w, int hgt, pixel_t color) {
    /* Track outline — single dim hairline so an empty lane still has
     * a visible bed.  Always 1 px tall along the bottom. */
    pixel_t bed = rgb(0x1F, 0x27, 0x38);
    draw_rect(x, y + hgt - 1, w, 1, bed);

    if (h->count < 2) return;     /* not enough samples for a line */

    uint16_t mn = h->v_min;
    uint16_t mx = h->v_max;
    if (mx <= mn) {
        /* Constant-value series — draw a flat line near the bottom so
         * the audience can see the lane is producing samples (just
         * with no variation).  This is honest: we DO have samples. */
        draw_rect(x, y + hgt - 2, w, 1, color);
        return;
    }
    uint16_t span = mx - mn;

    /* Map each sample to (px, py).  Walk the buffer in chronological
     * order so the rightmost point is the most recent — natural
     * "time flowing left to right". */
    int prev_px = -1, prev_py = -1;
    for (uint8_t i = 0; i < h->count; i++) {
        /* Oldest sample first.  In the ring, oldest = (head - count) mod N. */
        int idx = (h->head - h->count + i + FABRIC_HISTORY_N) % FABRIC_HISTORY_N;
        uint16_t v = h->values[idx];

        int px = x + (int)((long)i * (w - 1) / (FABRIC_HISTORY_N - 1));
        /* Normalize: bottom of band = v_min, top = v_max. */
        int py = y + hgt - 1 -
                 (int)((long)(v - mn) * (hgt - 2) / span);
        if (py < y) py = y;
        if (py > y + hgt - 1) py = y + hgt - 1;

        if (prev_px >= 0) {
            draw_line_segment(prev_px, prev_py, px, py, color);
        } else {
            draw_pixel(px, py, color);
        }
        prev_px = px;
        prev_py = py;
    }
}

/* === per-lane primary/secondary metric strings ===
 *
 * Honest framing: each string's content is derived from real producer
 * data.  When the producer hasn't fired, we say so — no placeholder
 * numbers. */
static void lane_metrics(fabric_lane_t lane, char *primary, size_t plen,
                         char *secondary, size_t slen) {
    primary[0] = secondary[0] = 0;
    personality_t p = lane_to_personality(lane);
    const perf_sample_t *s = (p != PERSONALITY_NONE) ? perf_last_for(p) : NULL;
    const fabric_lane_history_t *h = &s_history[lane];

    switch (lane) {
    case FABRIC_LANE_STATE:
        if (!s) {
            snprintf(primary, plen, "no STATE workload yet");
            snprintf(secondary, slen, "press ! to seed perf bench");
        } else {
            snprintf(primary, plen, "ops collapsed  %u → %u",
                     (unsigned)s->ops_logical, (unsigned)s->ops_issued);
            if (s->cycles_software_baseline && s->cycles_atomik) {
                double sp = perf_speedup(s);
                snprintf(secondary, slen, "%.2fx vs sw  /  fences 1",
                         sp);
            } else {
                snprintf(secondary, slen, "fences 1  /  %u cycles",
                         (unsigned)s->cycles_total);
            }
        }
        break;
    case FABRIC_LANE_SYNC:
        if (!s) {
            snprintf(primary, plen, "no SYNC workload yet");
            snprintf(secondary, slen, "press ! to seed perf bench");
        } else {
            snprintf(primary, plen, "regions emitted %u / %u",
                     (unsigned)s->ops_issued,
                     (unsigned)s->regions_unique);
            if (s->bytes_avoided == 0) {
                snprintf(secondary, slen, "(replay engine = v0.39)");
            } else {
                snprintf(secondary, slen, "%u bytes avoided",
                         (unsigned)s->bytes_avoided);
            }
        }
        break;
    case FABRIC_LANE_AGENT:
        if (!s) {
            snprintf(primary, plen, "no AGENT workload yet");
            snprintf(secondary, slen, "press ! to seed perf bench");
        } else {
            unsigned cold = s->bytes_avoided / 4;
            unsigned total = s->regions_unique + cold;
            unsigned retained = s->ops_issued;
            unsigned pct = total ? (retained * 100u / total)
                                  : (retained ? 100u : 0u);
            snprintf(primary, plen, "hot retained  %u / %u",
                     retained, total ? total : retained);
            snprintf(secondary, slen, "%u%% kept by relevance", pct);
        }
        break;
    case FABRIC_LANE_EVENT: {
        unsigned long total = atomik_event_total();
        if (total == 0) {
            snprintf(primary, plen, "no events yet");
            snprintf(secondary, slen, "bus is idle");
        } else {
            /* Most recent sample window = h->values[(head-1) mod N]. */
            uint16_t last_window = 0;
            if (h->count > 0) {
                int idx = (h->head + FABRIC_HISTORY_N - 1) % FABRIC_HISTORY_N;
                last_window = h->values[idx];
            }
            snprintf(primary, plen, "%lu total / %u in last %dms",
                     (unsigned long)total, last_window, FABRIC_SAMPLE_MS);
            snprintf(secondary, slen, "STATE %lu / SYNC %lu / AGENT %lu",
                     atomik_event_count(EVT_STATE_DELTA),
                     atomik_event_count(EVT_SYNC_REPLICA),
                     atomik_event_count(EVT_AGENT_CONTEXT));
        }
        break;
    }
    case FABRIC_LANE_VISUAL: {
        /* v0.38-A: real tile-based redraw measurement.  Was an
         * EVT_VIS_RENDER count proxy in v0.34-D; now a LIVE
         * measurement from the dirty-region tracker. */
        const atomik_metric_t *frames = metric_get("visual.frames");
        if (!frames || frames->source == METRIC_WAITING) {
            snprintf(primary, plen, "no frames painted yet");
            snprintf(secondary, slen, "(awaiting first redraw)");
        } else {
            const atomik_metric_t *dirty   = metric_get("visual.tiles_dirty");
            const atomik_metric_t *avoided = metric_get("visual.frame_pct_avoided");
            int dirty_n   = dirty   ? (int)dirty->value   : 0;
            int total     = dirty_total();
            double pct    = avoided ? avoided->value : 0.0;
            snprintf(primary, plen, "tiles dirty %d / %d",
                     dirty_n, total);
            snprintf(secondary, slen, "%.1f%% framebuffer avoided (EMA)",
                     pct);
        }
        break;
    }
    default: break;
    }
}

static const char *fresh_label(fabric_fresh_t f) {
    switch (f) {
    case FABRIC_FRESH_LIVE:    return "LIVE";
    case FABRIC_FRESH_STALE:   return "STALE";
    case FABRIC_FRESH_WAITING:
    default:                   return "WAITING";
    }
}

static pixel_t fresh_color(fabric_fresh_t f) {
    switch (f) {
    case FABRIC_FRESH_LIVE:    return ATOMIK_SEM_SAVINGS;    /* green */
    case FABRIC_FRESH_STALE:   return ATOMIK_SEM_WASTE;      /* amber */
    case FABRIC_FRESH_WAITING:
    default:                   return ATOMIK_FG_DIM;
    }
}

/* === filled-area waveform with layered glow (v0.38-J) ===
 *
 * Per ChatGPT 2026-05-15 audit + feedback_layered_stroke_rendering:
 * each lane reads as a luminous instrument, not a wireframe.  Stack:
 *   1. Dim base fill   — area under the wave at low alpha (body)
 *   2. Outer glow halo — 8 px wide above the wave edge, alpha ~16
 *   3. Mid glow        — 4 px wide above the wave edge, alpha ~32
 *   4. Core line       — 2 px thick at the wave's top edge, alpha ~210
 *   5. Hot points      — tiny brighter dots at the highest peaks
 *
 * Result: from 6-8 ft the lane reads as a glowing band with a sharp
 * top edge.  Mirrors concept-image instrument panels. */
static void draw_filled_waveform(const fabric_lane_history_t *h,
                                 int x, int y, int w, int hgt,
                                 pixel_t fill, pixel_t line) {
    pixel_t bed = rgb(0x1F, 0x27, 0x38);
    draw_rect(x, y + hgt - 1, w, 1, bed);

    /* v0.38-J+ WAITING baseline glow — when the lane has no data, paint
     * a calm low-amplitude glow band in lane color instead of nothing.
     * Per ChatGPT 2026-05-15: WAITING = calm instrument glow, never a
     * dead-flat empty lane.  Stays honest (no fake telemetry) — this is
     * pure visual chrome, no number is rendered from it. */
    if (h->count < 2) {
        /* v0.40 — idle/WAITING lanes render a calm card-spanning ambient
         * sine trace instead of a flat bottom band, matching the concept-
         * image instrument look.  PURE DECORATIVE CHROME (Class B): it
         * writes NO number and is never read as telemetry — the same
         * discipline as the Pulse Bar idle baseline (status.c
         * draw_event_pulse_glow).  Phase derives from the band's y so each
         * lane rests at a different point in the wave; STATIC (no time
         * term) to stay inside the dirty-render motion budget across 5
         * simultaneous lanes.  When >=2 real samples land, the live
         * waveform below replaces this. */
        int baseline = y + hgt / 2;
        int amp      = hgt / 5;
        double pha   = (double)(y & 0xFF) * 0.05;
        int prev_cy = -1, prev_px = -1;
        for (int sx = 0; sx < w; sx++) {
            double u  = (double)sx / (double)(w > 1 ? w - 1 : 1);
            int cy = baseline + (int)(amp * sin(u * 6.2831853 * 1.6 + pha));
            if (cy < y)             cy = y;
            if (cy > y + hgt - 1)   cy = y + hgt - 1;
            int px = x + sx;
            for (int g = 1; g <= 5; g++) {          /* soft fill below line */
                int gy = cy + g;
                if (gy > y + hgt - 1) break;
                draw_blend_pixel(px, gy, fill, (uint8_t)(40 - g * 6));
            }
            for (int g = 1; g <= 4; g++) {          /* glow above line */
                int gy = cy - g;
                if (gy < y) break;
                draw_blend_pixel(px, gy, line, (uint8_t)(16 - (g - 1) * 4));
            }
            draw_blend_pixel(px, cy, line, 110);    /* resting trace */
            if (prev_px >= 0 && prev_cy != cy) {    /* connect segments */
                int lo = prev_cy < cy ? prev_cy : cy;
                int hi = prev_cy < cy ? cy : prev_cy;
                for (int yy = lo; yy <= hi; yy++)
                    draw_blend_pixel(px, yy, line, 90);
            }
            prev_px = px; prev_cy = cy;
        }
        return;
    }

    uint16_t mn = h->v_min, mx = h->v_max;
    if (mx <= mn) {
        /* Constant series — flat glow band near the bottom. */
        for (int gy = 0; gy < 4; gy++) {
            uint8_t a = (uint8_t)(40 + gy * 25);
            for (int sx = 0; sx < w; sx++) {
                draw_blend_pixel(x + sx, y + hgt - 4 + gy, line, a);
            }
        }
        return;
    }
    uint16_t span = mx - mn;

    int col_h[256];
    int n_cols = (w < 256) ? w : 256;
    for (int col = 0; col < n_cols; col++) {
        /* v0.40-05-30: LINEAR-interpolate between adjacent history samples so
         * the waveform is a smooth flowing curve (concept Fabric), not the
         * blocky step function nearest-sampling produced. */
        long num    = (long)col * (h->count - 1);
        int  si     = (int)(num / (n_cols - 1));
        long frac_n = num - (long)si * (n_cols - 1);     /* 0 .. (n_cols-1) */
        int  si2    = (si + 1 < h->count) ? si + 1 : si;
        int  idx0   = (h->head - h->count + si  + FABRIC_HISTORY_N) % FABRIC_HISTORY_N;
        int  idx1   = (h->head - h->count + si2 + FABRIC_HISTORY_N) % FABRIC_HISTORY_N;
        long v0     = h->values[idx0], v1 = h->values[idx1];
        long v      = v0 + (v1 - v0) * frac_n / (n_cols - 1);
        int  hh     = (int)((v - mn) * (hgt - 2) / span);
        if (hh < 1) hh = 1;
        if (hh > hgt - 2) hh = hgt - 2;
        col_h[col] = hh;
    }

    /* Pass 1: TRANSLUCENT gradient fill — bright at the wave edge, fading to
     * the card floor.  v0.40-05-30: reads as a glowing band (concept Fabric),
     * never a solid block, even when the curve is near full height.  `fill`
     * (dim lane color) tints the deep floor so it doesn't read pure black. */
    for (int col = 0; col < n_cols; col++) {
        int px    = x + col;
        int top_y = y + hgt - 1 - col_h[col];
        int floor = y + hgt - 1;
        int span  = floor - top_y; if (span < 1) span = 1;
        for (int yy = top_y; yy < floor; yy++) {
            int d = yy - top_y;                          /* 0 at the edge */
            uint8_t a = (uint8_t)(84 - 70 * d / span);   /* brighter body fill */
            draw_blend_pixel(px, yy, line, a);
        }
        (void)fill;
    }

    /* Pass 2: outer glow halo above the edge — soft, 11 px, brighter. */
    for (int col = 0; col < n_cols; col++) {
        int px    = x + col;
        int top_y = y + hgt - 1 - col_h[col];
        for (int g = 1; g <= 11; g++) {
            int gy = top_y - g;
            if (gy < y) break;
            uint8_t a = (uint8_t)(38 - (g - 1) * 3);
            draw_blend_pixel(px, gy, line, a);
        }
    }

    /* Pass 3: mid glow above the edge — 5 px, brighter. */
    for (int col = 0; col < n_cols; col++) {
        int px    = x + col;
        int top_y = y + hgt - 1 - col_h[col];
        for (int g = 1; g <= 5; g++) {
            int gy = top_y - g;
            if (gy < y) break;
            uint8_t a = (uint8_t)(72 - (g - 1) * 12);
            draw_blend_pixel(px, gy, line, a);
        }
    }

    /* Pass 4: core line — bright, 2 px thick at the wave top + soft underline. */
    for (int col = 0; col < n_cols; col++) {
        int px    = x + col;
        int top_y = y + hgt - 1 - col_h[col];
        draw_pixel(px, top_y, line);
        if (top_y - 1 >= y)        draw_blend_pixel(px, top_y - 1, line, 255);
        if (top_y - 2 >= y)        draw_blend_pixel(px, top_y - 2, line, 150);
        if (top_y + 1 < y + hgt)   draw_blend_pixel(px, top_y + 1, line, 130);
    }

    /* Pass 5: hot points — 3 px square highlights at local maxima.
     * Local max = column whose col_h exceeds both neighbors by 2+. */
    for (int col = 1; col < n_cols - 1; col++) {
        if (col_h[col] >= col_h[col - 1] + 2 &&
            col_h[col] >= col_h[col + 1] + 2) {
            int px    = x + col;
            int top_y = y + hgt - 1 - col_h[col];
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    draw_pixel(px + dx, top_y + dy, line);
                }
            }
        }
    }
}

/* === big-metric string per lane (v0.38-G focal number) === */
static void lane_big_metric(fabric_lane_t lane,
                            char *buf, size_t cap,
                            char *unit, size_t unit_cap) {
    buf[0] = unit[0] = 0;
    personality_t p = lane_to_personality(lane);
    const perf_sample_t *s = (p != PERSONALITY_NONE) ? perf_last_for(p) : NULL;
    const fabric_lane_history_t *h = &s_history[lane];

    switch (lane) {
    case FABRIC_LANE_STATE:
        if (s && s->ops_logical > 0) {
            unsigned pct = (unsigned)
                (100u * (s->ops_logical - s->ops_issued) / s->ops_logical);
            snprintf(buf, cap, "%u", pct);
            snprintf(unit, unit_cap, "%% coalesced");
        } else {
            snprintf(buf, cap, "--");
            snprintf(unit, unit_cap, "%% coalesced");
        }
        break;
    case FABRIC_LANE_SYNC:
        if (s) {
            snprintf(buf, cap, "%u", (unsigned)s->ops_issued);
            snprintf(unit, unit_cap, "deltas emitted");
        } else {
            snprintf(buf, cap, "--");
            snprintf(unit, unit_cap, "deltas emitted");
        }
        break;
    case FABRIC_LANE_AGENT:
        if (s) {
            unsigned cold = s->bytes_avoided / 4;
            unsigned total = s->regions_unique + cold;
            unsigned pct = total ? (s->ops_issued * 100u / total) : 0;
            snprintf(buf, cap, "%u", pct);
            snprintf(unit, unit_cap, "%% retained");
        } else {
            snprintf(buf, cap, "--");
            snprintf(unit, unit_cap, "%% retained");
        }
        break;
    case FABRIC_LANE_EVENT: {
        unsigned long total = atomik_event_total();
        if (total > 0) {
            snprintf(buf, cap, "%lu", total);
            snprintf(unit, unit_cap, "events on bus");
        } else {
            /* v0.38-K3 — investor idle phrasing matches VISUAL. */
            snprintf(buf, cap, "0");
            if (metric_mode() == METRIC_MODE_INVESTOR) {
                snprintf(unit, unit_cap, "waiting for workload");
            } else {
                snprintf(unit, unit_cap, "events on bus");
            }
        }
        break;
    }
    case FABRIC_LANE_VISUAL: {
        const atomik_metric_t *avd = metric_get("visual.frame_pct_avoided");
        if (avd && avd->source != METRIC_WAITING) {
            snprintf(buf, cap, "%.0f", avd->value);
            snprintf(unit, unit_cap, "%% pixels avoided");
        } else {
            /* v0.38-K3 — investor mode shows a WAITING phrase instead
             * of "0 %% pixels avoided" which read as unfinished. */
            snprintf(buf, cap, "0");
            if (metric_mode() == METRIC_MODE_INVESTOR) {
                snprintf(unit, unit_cap, "waiting for render workload");
            } else {
                snprintf(unit, unit_cap, "render deltas observed");
            }
        }
        (void)h;
        break;
    }
    default: break;
    }
}

/* === main draw === */

#define LANE_ROW_H        156    /* v0.40: 5 tall lanes + header subtitle + bottom
                                  * capacity line fit the shelf (concept-01) */
#define LANE_GAP          ATOMIK_GRID_M
#define LANE_ACCENT_W     3      /* v0.38-J: thicker active rim (was 2) */
#define LANE_ACTIVE_HALO  6      /* v0.38-J: outer alpha halo px */

/* v0.40: PARALLEL BANKS panel — the live, self-verifying parallel-bank
 * throughput measured on the engine @0xF0021000 (bench.c).  Drawn below the
 * 5 lanes.  Bars are throughput per active-bank-count (1/2/4/8); the bar for
 * the currently-allocated count glows; the chip reports VERIFIED only when
 * the hardware XOR result matched the software recompute this measurement.
 * No fabricated values: if the engine is absent the panel dims to WAITING. */
static void draw_banks_panel(int x, int y, int wd, int h) {
    pixel_t accent = ATOMIK_SEM_HARDWARE;           /* cyan: silicon */
    /* card */
    draw_rect_rounded(x, y, wd, h, 10, wm_card_bg());
    draw_rect(x, y, wd, 1, wm_card_border());
    draw_rect(x, y + h - 1, wd, 1, wm_card_border());
    draw_rect(x, y, 1, h, wm_card_border());
    draw_rect(x + wd - 1, y, 1, h, wm_card_border());
    /* tinted top shoulder */
    for (int sy = 0; sy < 10; sy++) {
        uint8_t a = (uint8_t)(55 - sy * 5);
        for (int sx = 0; sx < wd; sx++) draw_blend_pixel(x + sx, y + sy, accent, a);
    }

    int pad = ATOMIK_GRID_L;
    int tx  = x + pad;
    int ty  = y + ATOMIK_GRID_M;

    /* title */
    int title_h;
    if (font_aa_loaded(FONT_AA_UI)) {
        draw_text_aa(FONT_AA_UI, tx, ty, "PARALLEL BANKS", accent);
        title_h = text_height_aa(FONT_AA_UI);
    } else {
        draw_text(tx, ty, "PARALLEL BANKS", 2, accent);
        title_h = text_height(2);
    }

    int src = bench_source();
    const char *chip; pixel_t chipc;
    if      (src == METRIC_LIVE)  { chip = "VERIFIED";  chipc = ATOMIK_SEM_SAVINGS; }
    else if (src == METRIC_STALE) { chip = "MEASURING"; chipc = ATOMIK_SEM_WASTE;  }
    else                          { chip = "WAITING";   chipc = ATOMIK_FG_DIM;     }
    int cu = font_aa_loaded(FONT_AA_LABEL);
    int cw = cu ? text_width_aa(FONT_AA_LABEL, chip) : text_width(chip, 1);
    int chh = (cu ? text_height_aa(FONT_AA_LABEL) : text_height(1)) + 4;
    int cwp = cw + ATOMIK_GRID_M;
    int cxp = x + wd - cwp - pad;
    int cyp = ty + (title_h - chh) / 2;
    draw_rect_rounded(cxp, cyp, cwp, chh, chh / 2, wm_card_bg() & 0x0F0F0F);
    draw_rect(cxp + chh / 2, cyp, cwp - chh, 1, chipc);
    draw_rect(cxp + chh / 2, cyp + chh - 1, cwp - chh, 1, chipc);
    if (cu) draw_text_aa(FONT_AA_LABEL, cxp + ATOMIK_GRID_S, cyp + 2, chip, chipc);
    else    draw_text(cxp + ATOMIK_GRID_S, cyp + 2, chip, 1, chipc);

    /* bar chart region (left ~58%), readout (right) */
    int npts = 0;
    const bench_point_t *pts = bench_sweep_points(&npts);
    int chart_x = tx;
    int chart_w = (wd - pad * 2) * 58 / 100;
    int bars_top = ty + title_h + ATOMIK_GRID_M;
    int label_h  = cu ? text_height_aa(FONT_AA_LABEL) : text_height(1);
    int bars_h   = (y + h - ATOMIK_GRID_M) - bars_top - label_h - 2;
    if (bars_h < 16) bars_h = 16;

    double vmax = 0.0;
    for (int i = 0; i < npts; i++) if (pts[i].mdeltas_s > vmax) vmax = pts[i].mdeltas_s;

    int slots = (npts > 0) ? npts : BENCH_N_POINTS;
    /* Highlight the MAX-bank bar (the headline result).  We deliberately do
     * NOT animate a "currently allocated" cursor here: atomik_os doesn't yet
     * control the allocation (the tool measures all bank counts each cycle),
     * so a moving cursor would imply a live reallocation that isn't happening.
     * The bars + live re-measurement carry the dynamism honestly. */
    int active = (npts > 0) ? (int)pts[npts - 1].banks : 8;
    int bw = chart_w / (slots * 2);            /* bar width; gaps between */
    if (bw < 6) bw = 6;
    for (int i = 0; i < slots; i++) {
        int bx = chart_x + (chart_w * (2 * i + 1)) / (slots * 2) - bw / 2;
        int banks = (npts > 0) ? (int)pts[i].banks : (1 << i);
        int bh; pixel_t fill;
        if (npts > 0 && vmax > 0.0) {
            bh = (int)(pts[i].mdeltas_s / vmax * bars_h);
            int is_active = (pts[i].banks == (uint32_t)active);
            fill = is_active ? accent : ATOMIK_ACCENT_DIM;
            if (bh < 2) bh = 2;
            int by = bars_top + bars_h - bh;
            draw_rect_rounded(bx, by, bw, bh, 3, fill);
            if (is_active) {                    /* glow rim on the live allocation */
                for (int g = 1; g <= 4; g++) {
                    uint8_t a = (uint8_t)(40 - (g - 1) * 9);
                    for (int sx = -g; sx < bw + g; sx++) {
                        draw_blend_pixel(bx + sx, by - g, accent, a);
                    }
                    for (int sy = -g; sy < bh + g; sy++) {
                        draw_blend_pixel(bx - g, by + sy, accent, a);
                        draw_blend_pixel(bx + bw - 1 + g, by + sy, accent, a);
                    }
                }
            }
        } else {
            /* WAITING: ambient dim stub bars, no data */
            bh = bars_h / 6 + i * 3;
            int by = bars_top + bars_h - bh;
            draw_rect_rounded(bx, by, bw, bh, 3, rgb(0x2A, 0x34, 0x48));
        }
        /* bank-count label under each bar */
        char bl[8]; snprintf(bl, sizeof bl, "%d", banks);
        int blw = cu ? text_width_aa(FONT_AA_LABEL, bl) : text_width(bl, 1);
        int blx = bx + bw / 2 - blw / 2;
        int bly = bars_top + bars_h + 1;
        pixel_t lcol = (npts > 0 && pts[i].banks == (uint32_t)active) ? accent
                                                                      : ATOMIK_FG_DIM;
        if (cu) draw_text_aa(FONT_AA_LABEL, blx, bly, bl, lcol);
        else    draw_text(blx, bly, bl, 1, lcol);
    }

    /* right-side readout: big speedup + throughput-at-current-allocation */
    int rx = chart_x + chart_w + ATOMIK_GRID_L;
    int ry = bars_top;
    if (npts > 0) {
        double max_spd = pts[npts - 1].speedup;
        char big[16]; snprintf(big, sizeof big, "%.2f", max_spd);
        /* draw the "x" suffix in dim after the big number */
        if (font_aa_loaded(FONT_AA_DISPLAY)) {
            draw_text_aa(FONT_AA_DISPLAY, rx, ry, big, accent);
            int nbw = text_width_aa(FONT_AA_DISPLAY, big);
            draw_text_aa(FONT_AA_LABEL, rx + nbw + 2,
                         ry + text_height_aa(FONT_AA_DISPLAY)
                            - text_height_aa(FONT_AA_LABEL),
                         "x", ATOMIK_FG_DIM);
            ry += text_height_aa(FONT_AA_DISPLAY) + 2;
        } else {
            draw_text(rx, ry, big, 3, accent);
            ry += text_height(3) + 2;
        }
        /* headline throughput: the measured MAX (matches the speedup above) */
        int mi = npts - 1;
        char sub[48];
        snprintf(sub, sizeof sub, "up to %.0f Md/s @ %d banks",
                 pts[mi].mdeltas_s, (int)pts[mi].banks);
        if (cu) draw_text_aa(FONT_AA_LABEL, rx, ry, sub, ATOMIK_FG_DIM);
        else    draw_text(rx, ry, sub, 1, ATOMIK_FG_DIM);
    } else {
        const char *msg = "engine @0xF0021000 — waiting";
        if (cu) draw_text_aa(FONT_AA_LABEL, rx, ry, msg, ATOMIK_FG_DIM);
        else    draw_text(rx, ry, msg, 1, ATOMIK_FG_DIM);
    }
}

void fabric_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w; (void)ht;

    /* v0.38-J header — drop the verbose 2-line subtitle.  Just the
     * "RESOURCE FABRIC" title + the AUTO/MANUAL personality capsule.
     * v0.38-K renders the title in the AA UI atlas when available
     * for a noticeably premium look. */
    if (font_aa_loaded(FONT_AA_UI)) {
        draw_text_aa(FONT_AA_UI, x + ATOMIK_GRID_L, y + ATOMIK_GRID_M,
                     "RESOURCE FABRIC", ATOMIK_FG);
    } else {
        draw_text(x + ATOMIK_GRID_L, y + ATOMIK_GRID_M,
                  "RESOURCE FABRIC", 2, ATOMIK_FG);
    }
    /* concept subtitle */
    if (font_aa_loaded(FONT_AA_LABEL)) {
        int sy = y + ATOMIK_GRID_M + (font_aa_loaded(FONT_AA_UI)
                 ? text_height_aa(FONT_AA_UI) : text_height(2)) + 2;
        draw_text_aa(FONT_AA_LABEL, x + ATOMIK_GRID_L, sy,
                     "ATOMIK COMPUTE BANKS", rgb(0x76, 0x82, 0x98));
    }

    pixel_t badge_color;
    switch (s_active) {
    case PERSONALITY_STATE: badge_color = ATOMIK_SEM_HARDWARE; break;
    case PERSONALITY_AGENT: badge_color = ATOMIK_SEM_AGENT;    break;
    case PERSONALITY_SYNC:  badge_color = ATOMIK_SEM_SAVINGS;  break;
    default:                badge_color = ATOMIK_FG_DIM;       break;
    }
    char badge[32];
    if (fabric_override_active()) {
        snprintf(badge, sizeof badge, "MANUAL / %s",
                 fabric_personality_name(s_active));
    } else {
        snprintf(badge, sizeof badge, "AUTO / %s",
                 fabric_personality_name(s_active));
    }
    /* v0.38-K3: Fabric panel capsule now uses the AA UI atlas when
     * available so it matches the premium top-bar pills.  Falls back
     * to pixel font 1 px otherwise. */
    int use_aa = font_aa_loaded(FONT_AA_LABEL);
    int bw = use_aa ? text_width_aa(FONT_AA_LABEL, badge)
                    : text_width(badge, 1);
    int bh = use_aa ? text_height_aa(FONT_AA_LABEL) + 8
                    : text_height(1) + 8;
    int bx   = x + wd - bw - ATOMIK_GRID_L * 2 - ATOMIK_GRID_M;
    int by   = y + ATOMIK_GRID_M;
    int bcap = bw + ATOMIK_GRID_L;
    draw_rect_rounded(bx, by, bcap, bh, 6, wm_card_bg());
    for (int t = 0; t < 1; t++) {
        draw_rect(bx + 2, by + t,        bcap - 4, 1, badge_color);
        draw_rect(bx + 2, by + bh - 1 - t, bcap - 4, 1, badge_color);
    }
    if (use_aa) {
        draw_text_aa(FONT_AA_LABEL, bx + ATOMIK_GRID_S + 2, by + 4,
                     badge, badge_color);
    } else {
        draw_text(bx + ATOMIK_GRID_S + 2, by + 4, badge, 1, badge_color);
    }

    /* === lanes === */
    int header_h = font_aa_loaded(FONT_AA_UI)
                   ? text_height_aa(FONT_AA_UI)
                   : text_height(2);
    int sub_hh   = font_aa_loaded(FONT_AA_LABEL) ? text_height_aa(FONT_AA_LABEL) : 0;
    int lanes_y = y + ATOMIK_GRID_M + header_h + sub_hh + 2 + ATOMIK_GRID_M;
    int lane_x  = x + ATOMIK_GRID_L;
    int lane_w  = wd - ATOMIK_GRID_L * 2;

    for (int i = 0; i < FABRIC_N_LANES_V2; i++) {
        fabric_lane_t lane  = (fabric_lane_t)i;
        int           ly    = lanes_y + i * (LANE_ROW_H + LANE_GAP);
        const fabric_lane_history_t *h = &s_history[i];
        personality_t lp     = lane_to_personality(lane);
        int           active = (lp != PERSONALITY_NONE && lp == s_active);
        pixel_t       lc     = lane_color(lane);

        /* === v0.38-G card: instrument, not wireframe ===
         *
         * Layered composition:
         *   1. Base card body (slightly elevated dark navy).
         *   2. Lane-color gradient stripe at the top (12 px) reading
         *      as a tinted shoulder so each lane has its identity
         *      color in chrome, not just text.
         *   3. 1-px hairline border.
         *   4. ACTIVE-rim: 2-px saturated outer border in lane color
         *      drawn AROUND the card (one px outside each edge) so the
         *      currently-active personality lane glows. */
        draw_rect_rounded(lane_x, ly, lane_w, LANE_ROW_H, 10, wm_card_bg());

        /* Tinted shoulder — top band gets the lane's accent color at
         * low alpha so the lane reads as colored even without active. */
        int shoulder_h = 12;
        for (int sy = 0; sy < shoulder_h; sy++) {
            uint8_t alpha = (uint8_t)(60 - (sy * 60 / shoulder_h));
            for (int sx = 0; sx < lane_w; sx++) {
                draw_blend_pixel(lane_x + sx, ly + sy, lc, alpha);
            }
        }

        /* Hairline border. */
        draw_rect(lane_x, ly, lane_w, 1, wm_card_border());
        draw_rect(lane_x, ly + LANE_ROW_H - 1, lane_w, 1, wm_card_border());
        draw_rect(lane_x, ly, 1, LANE_ROW_H, wm_card_border());
        draw_rect(lane_x + lane_w - 1, ly, 1, LANE_ROW_H, wm_card_border());

        /* v0.38-J active rim: 3-px saturated inner ring + soft outer
         * alpha halo so the active lane glows from the body outward.
         * Total visual rim ~ 9 px (3 hard + 6 halo). */
        if (active) {
            for (int t = 0; t < LANE_ACCENT_W; t++) {
                draw_rect(lane_x + t,     ly + t,     lane_w - 2 * t, 1, lc);
                draw_rect(lane_x + t,     ly + LANE_ROW_H - 1 - t,
                          lane_w - 2 * t, 1, lc);
                draw_rect(lane_x + t,     ly + t, 1, LANE_ROW_H - 2 * t, lc);
                draw_rect(lane_x + lane_w - 1 - t, ly + t,
                          1, LANE_ROW_H - 2 * t, lc);
            }
            /* Outer alpha halo: rings just outside the card edge, alpha
             * falling off with distance.  Reads as luminous spill from
             * the lane border, mirrors concept-image instrument glow. */
            for (int g = 1; g <= LANE_ACTIVE_HALO; g++) {
                uint8_t a = (uint8_t)(48 - (g - 1) * 7);
                int hx = lane_x - g;
                int hy = ly - g;
                int hw = lane_w + 2 * g;
                int hh = LANE_ROW_H + 2 * g;
                for (int sx = 0; sx < hw; sx++) {
                    draw_blend_pixel(hx + sx, hy,          lc, a);
                    draw_blend_pixel(hx + sx, hy + hh - 1, lc, a);
                }
                for (int sy = 0; sy < hh; sy++) {
                    draw_blend_pixel(hx,          hy + sy, lc, a);
                    draw_blend_pixel(hx + hw - 1, hy + sy, lc, a);
                }
            }
        }

        /* === content === */
        int pad_x = ATOMIK_GRID_L;
        int tx    = lane_x + pad_x;
        int inner_w = lane_w - pad_x * 2;

        /* v0.40: waveform is now a card-spanning BACKGROUND behind the
         * metric (concept-01 instrument look), not a thin bottom strip.
         * Drawn FIRST so the lane name, big number, and unit render on
         * top of it.  Spans from the big-number row down to the card
         * floor.  Honest: live data → real curve; idle → ambient sine
         * chrome (see draw_filled_waveform WAITING branch). */
        {
            int name_h0 = font_aa_loaded(FONT_AA_UI)
                          ? text_height_aa(FONT_AA_UI) : text_height(2);
            int sub_h0  = font_aa_loaded(FONT_AA_LABEL)
                          ? text_height_aa(FONT_AA_LABEL) : text_height(1);
            int wf_y = ly + ATOMIK_GRID_M + name_h0 + sub_h0 + ATOMIK_GRID_S;
            int wf_h = LANE_ROW_H - (wf_y - ly) - ATOMIK_GRID_M;
            if (wf_h < 12) wf_h = 12;
            uint8_t fr = (lc >> 16) & 0xFF, fgc = (lc >> 8) & 0xFF, fbc = lc & 0xFF;
            pixel_t fill_dim = rgb(fr / 3, fgc / 3, fbc / 3);
            draw_filled_waveform(h, tx, wf_y, inner_w, wf_h, fill_dim, lc);
        }

        /* Row 1: lane NAME in saturated lane color + dim semantic
         * subtitle below + freshness chip right-aligned.
         * v0.38-K: lane name uses AA UI atlas; subtitle uses AA LABEL
         * atlas.  Both fall back to pixel font if missing. */
        int ty1 = ly + ATOMIK_GRID_M;
        const char *name = fabric_lane_name(lane);
        const char *sub  = lane_subtitle(lane);
        int name_h, sub_h;
        if (font_aa_loaded(FONT_AA_UI)) {
            draw_text_aa(FONT_AA_UI, tx, ty1, name, lc);
            name_h = text_height_aa(FONT_AA_UI);
        } else {
            draw_text(tx, ty1, name, 2, lc);
            name_h = text_height(2);
        }
        if (font_aa_loaded(FONT_AA_LABEL)) {
            draw_text_aa(FONT_AA_LABEL, tx, ty1 + name_h + 2, sub,
                         ATOMIK_FG_DIM);
            sub_h = text_height_aa(FONT_AA_LABEL);
        } else {
            draw_text(tx, ty1 + name_h + 2, sub, 1, ATOMIK_FG_DIM);
            sub_h = text_height(1);
        }
        (void)sub_h;

        /* v0.40: LIVE/ACTIVE chip removed — the concept puts the big metric
         * figure in this top-right space, not a freshness badge.  (void) the
         * now-unused per-lane personality so -Wunused stays quiet.) */
        (void)lp;

        /* Big metric (real telemetry) — concept-01 layout: top-RIGHT, right-
         * aligned in lane color, with the dim unit below it. */
        char big[16], unit[24];
        lane_big_metric(lane, big, sizeof big, unit, sizeof unit);
        int disp  = font_aa_loaded(FONT_AA_DISPLAY);
        int big_h = disp ? text_height_aa(FONT_AA_DISPLAY) : text_height(3);
        int nw    = disp ? text_width_aa(FONT_AA_DISPLAY, big) : text_width(big, 3);
        if (disp) draw_text_aa(FONT_AA_DISPLAY, lane_x + lane_w - pad_x - nw, ty1, big, lc);
        else      draw_text(lane_x + lane_w - pad_x - nw, ty1, big, 3, lc);
        {
            int ulab = font_aa_loaded(FONT_AA_LABEL);
            int uw   = ulab ? text_width_aa(FONT_AA_LABEL, unit) : text_width(unit, 1);
            int uy   = ty1 + big_h + 1;
            if (ulab) draw_text_aa(FONT_AA_LABEL, lane_x + lane_w - pad_x - uw, uy, unit, rgb(0x86,0x92,0xA8));
            else      draw_text(lane_x + lane_w - pad_x - uw, uy, unit, 1, rgb(0x86,0x92,0xA8));
        }

        /* (Waveform is drawn earlier as a card-spanning background.)
         * v0.40 (2026-05-30): progress bars REMOVED per design direction —
         * the Resource Fabric is a glowing-waveform instrument, not a bar
         * chart.  The waveform carries every lane. */
    }
    /* v0.40: PARALLEL BANKS moved to the SYSTEM surface (system_surface.c) so
     * the Fabric is the concept's 5 tall lanes. */

    /* bottom: TOTAL FABRIC CAPACITY — concept's summary line.  Per-bank GB is
     * not wired yet, so this is a clearly-labeled PLACEHOLDER (dim, no live
     * implication) until real memory accounting exists. */
    if (font_aa_loaded(FONT_AA_LABEL)) {
        int cap_y = lanes_y + FABRIC_N_LANES_V2 * (LANE_ROW_H + LANE_GAP) + 2;
        if (cap_y + text_height_aa(FONT_AA_LABEL) < y + ht) {
            draw_rect(lane_x, cap_y - 4, lane_w, 1, wm_card_border());
            const char *cl = "TOTAL FABRIC CAPACITY";
            draw_text_aa(FONT_AA_LABEL, lane_x, cap_y, cl, rgb(0x76, 0x82, 0x98));
            const char *cv = "-- / -- GB  (placeholder)";
            int cvw = text_width_aa(FONT_AA_LABEL, cv);
            draw_text_aa(FONT_AA_LABEL, lane_x + lane_w - cvw, cap_y, cv, ATOMIK_FG_DIM);
        }
    }
}

/* === shelf geometry — unchanged from v0.34-C === */
#define FABRIC_SHELF_W   480
#define FABRIC_SHELF_X   (FB_W - FABRIC_SHELF_W - ATOMIK_GRID_L)
#define FABRIC_SHELF_Y   (ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + ATOMIK_GRID_M)
/* v0.38-G: 5 lanes × (158 + 8) + header (~84) ≈ 920; bump shelf to 920
 * so the bigger instrument cards fit.  Workspace below the shelf is
 * the bottom 84 px (cushion), Memory Weave / State Watch surfaces
 * still open with their own y-positions and aren't affected. */
#define FABRIC_SHELF_H   920

int fabric_shelf_x(void) { return FABRIC_SHELF_X; }
int fabric_shelf_y(void) { return FABRIC_SHELF_Y; }
int fabric_shelf_w(void) { return FABRIC_SHELF_W; }
int fabric_shelf_h(void) { return FABRIC_SHELF_H; }

/* Seed perf samples so STATE/SYNC/AGENT lanes show real numbers on
 * first open instead of three "no workload yet" placeholders.  The
 * EVENT/VISUAL lanes don't need seeding — they pick up real activity
 * from the running system within one sample window. */
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
    seed_metrics_if_empty();
    if (s_window_id >= 0) {
        wm_focus(s_window_id);
        return;
    }
    /* Empty title => chromeless pinned panel (no WM titlebar/close dot).
     * The Fabric renders its own "RESOURCE FABRIC" header. */
    window_t *w = wm_open("",
                          FABRIC_SHELF_X, FABRIC_SHELF_Y,
                          FABRIC_SHELF_W, FABRIC_SHELF_H,
                          fabric_draw, NULL);
    if (w) s_window_id = w->id;
}
