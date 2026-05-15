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
 *   │  ↳ change detection · dirty regions ·    │
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

static void sample_perf_lane(fabric_lane_t lane, personality_t p,
                             unsigned long now) {
    const perf_sample_t *s = perf_last_for(p);
    if (!s) return;
    /* Only push when a NEW sample has landed since we last looked. */
    if (s->cycles_total == s_last_cycles_total[p]) return;
    s_last_cycles_total[p] = s->cycles_total;

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

void fabric_tick(void) {
    unsigned long now = anim_now_ms();
    s_active = detect(now);

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
    case FABRIC_LANE_STATE:  return ATOMIK_SEM_HARDWARE;     /* cyan   */
    case FABRIC_LANE_SYNC:   return ATOMIK_SEM_SAVINGS;      /* green  */
    case FABRIC_LANE_AGENT:  return ATOMIK_SEM_AGENT;        /* violet */
    case FABRIC_LANE_EVENT:  return rgb(0xF0, 0x9C, 0x55);   /* amber-warm: cross-cutting bus */
    case FABRIC_LANE_VISUAL: return rgb(0xE5, 0x6E, 0xC0);   /* magenta: pixel/render */
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
                snprintf(secondary, slen, "%.2fx vs sw  ·  fences 1",
                         sp);
            } else {
                snprintf(secondary, slen, "fences 1  ·  %u cycles",
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
            snprintf(primary, plen, "%lu total · %u in last %dms",
                     (unsigned long)total, last_window, FABRIC_SAMPLE_MS);
            snprintf(secondary, slen, "STATE %lu · SYNC %lu · AGENT %lu",
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

    if (h->count < 2) return;

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
        int   sample_i = (int)((long)col * (h->count - 1) / (n_cols - 1));
        int   idx      = (h->head - h->count + sample_i + FABRIC_HISTORY_N)
                          % FABRIC_HISTORY_N;
        uint16_t v     = h->values[idx];
        int      hh    = (int)((long)(v - mn) * (hgt - 2) / span);
        if (hh < 1) hh = 1;
        if (hh > hgt - 2) hh = hgt - 2;
        col_h[col] = hh;
    }

    /* Pass 1: dim body fill from baseline up to the wave height.
     * Uses `fill` (lane color at ~1/3 intensity) for low-glow body. */
    for (int col = 0; col < n_cols; col++) {
        int px    = x + col;
        int top_y = y + hgt - 1 - col_h[col];
        for (int yy = top_y; yy < y + hgt - 1; yy++) {
            draw_pixel(px, yy, fill);
        }
    }

    /* Pass 2: outer glow halo — 8 px wide above the wave top edge.
     * Per-column alpha-blended strokes; falls off with distance. */
    for (int col = 0; col < n_cols; col++) {
        int px    = x + col;
        int top_y = y + hgt - 1 - col_h[col];
        for (int g = 1; g <= 8; g++) {
            int gy = top_y - g;
            if (gy < y) break;
            uint8_t a = (uint8_t)(18 - (g - 1) * 2);
            draw_blend_pixel(px, gy, line, a);
        }
    }

    /* Pass 3: mid glow — 4 px wide above the wave top edge, brighter. */
    for (int col = 0; col < n_cols; col++) {
        int px    = x + col;
        int top_y = y + hgt - 1 - col_h[col];
        for (int g = 1; g <= 4; g++) {
            int gy = top_y - g;
            if (gy < y) break;
            uint8_t a = (uint8_t)(38 - (g - 1) * 6);
            draw_blend_pixel(px, gy, line, a);
        }
    }

    /* Pass 4: core line — 2 px thick at the wave top.  Bright. */
    for (int col = 0; col < n_cols; col++) {
        int px    = x + col;
        int top_y = y + hgt - 1 - col_h[col];
        draw_pixel(px, top_y, line);
        if (top_y - 1 >= y) {
            draw_blend_pixel(px, top_y - 1, line, 220);
        }
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
            snprintf(unit, unit_cap, "%%");
        } else {
            snprintf(buf, cap, "--");
            snprintf(unit, unit_cap, "%%");
        }
        break;
    case FABRIC_LANE_SYNC:
        if (s) {
            snprintf(buf, cap, "%u", (unsigned)s->ops_issued);
            snprintf(unit, unit_cap, "deltas");
        } else {
            snprintf(buf, cap, "--");
            snprintf(unit, unit_cap, "deltas");
        }
        break;
    case FABRIC_LANE_AGENT:
        if (s) {
            unsigned cold = s->bytes_avoided / 4;
            unsigned total = s->regions_unique + cold;
            unsigned pct = total ? (s->ops_issued * 100u / total) : 0;
            snprintf(buf, cap, "%u", pct);
            snprintf(unit, unit_cap, "%% hot");
        } else {
            snprintf(buf, cap, "--");
            snprintf(unit, unit_cap, "%% hot");
        }
        break;
    case FABRIC_LANE_EVENT: {
        unsigned long total = atomik_event_total();
        if (total > 0) {
            snprintf(buf, cap, "%lu", total);
            snprintf(unit, unit_cap, "events");
        } else {
            snprintf(buf, cap, "--");
            snprintf(unit, unit_cap, "events");
        }
        break;
    }
    case FABRIC_LANE_VISUAL: {
        const atomik_metric_t *avd = metric_get("visual.frame_pct_avoided");
        if (avd && avd->source != METRIC_WAITING) {
            snprintf(buf, cap, "%.0f", avd->value);
            snprintf(unit, unit_cap, "%% avoided");
        } else {
            snprintf(buf, cap, "--");
            snprintf(unit, unit_cap, "%% avoided");
        }
        (void)h;
        break;
    }
    default: break;
    }
}

/* === main draw === */

#define LANE_ROW_H        158    /* v0.38-G: was 76; taller panels = instrument feel */
#define LANE_GAP          ATOMIK_GRID_M
#define LANE_ACCENT_W     3      /* v0.38-J: thicker active rim (was 2) */
#define LANE_ACTIVE_HALO  6      /* v0.38-J: outer alpha halo px */

void fabric_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w; (void)ht;

    /* v0.38-J header — drop the verbose 2-line subtitle.  Just the
     * "RESOURCE FABRIC" title + the AUTO/MANUAL personality capsule.
     * Concept images keep the panel header tight; the active
     * personality is communicated by lane glow, not header prose. */
    draw_text(x + ATOMIK_GRID_L, y + ATOMIK_GRID_M,
              "RESOURCE FABRIC", 2, ATOMIK_FG);

    pixel_t badge_color;
    switch (s_active) {
    case PERSONALITY_STATE: badge_color = ATOMIK_SEM_HARDWARE; break;
    case PERSONALITY_AGENT: badge_color = ATOMIK_SEM_AGENT;    break;
    case PERSONALITY_SYNC:  badge_color = ATOMIK_SEM_SAVINGS;  break;
    default:                badge_color = ATOMIK_FG_DIM;       break;
    }
    char badge[32];
    if (fabric_override_active()) {
        snprintf(badge, sizeof badge, "MANUAL · %s",
                 fabric_personality_name(s_active));
    } else {
        snprintf(badge, sizeof badge, "AUTO · %s",
                 fabric_personality_name(s_active));
    }
    /* Badge as a rounded capsule with brighter border in active color. */
    int bw   = text_width(badge, 1);
    int bh   = text_height(1) + 8;
    int bx   = x + wd - bw - ATOMIK_GRID_L * 2 - ATOMIK_GRID_M;
    int by   = y + ATOMIK_GRID_M;
    int bcap = bw + ATOMIK_GRID_L;
    draw_rect_rounded(bx, by, bcap, bh, 6, wm_card_bg());
    for (int t = 0; t < 1; t++) {
        draw_rect(bx + 2, by + t,        bcap - 4, 1, badge_color);
        draw_rect(bx + 2, by + bh - 1 - t, bcap - 4, 1, badge_color);
    }
    draw_text(bx + ATOMIK_GRID_S + 2, by + 4, badge, 1, badge_color);

    /* === lanes === */
    int lanes_y = y + ATOMIK_GRID_M + text_height(2) + ATOMIK_GRID_L;
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
        draw_rect(lane_x, ly, lane_w, LANE_ROW_H, wm_card_bg());

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

        /* Row 1: scale-2 lane NAME in saturated lane color +
         * freshness chip right-aligned. */
        int ty1 = ly + ATOMIK_GRID_M;
        const char *name = fabric_lane_name(lane);
        draw_text(tx, ty1, name, 2, lc);

        const char *fl = fresh_label(h->fresh);
        int fw = text_width(fl, 1);
        /* Small filled freshness chip with color background. */
        pixel_t fcol = fresh_color(h->fresh);
        int chip_w = fw + ATOMIK_GRID_M;
        int chip_h = text_height(1) + 4;
        int chip_x = lane_x + lane_w - chip_w - pad_x;
        int chip_y = ty1 + (text_height(2) - chip_h) / 2;
        draw_rect(chip_x, chip_y, chip_w, chip_h,
                  wm_card_bg() & 0x0F0F0F);   /* darker chip body */
        draw_rect(chip_x, chip_y, chip_w, 1, fcol);
        draw_rect(chip_x, chip_y + chip_h - 1, chip_w, 1, fcol);
        draw_text(chip_x + ATOMIK_GRID_S, chip_y + 2, fl, 1, fcol);

        /* Row 2: scale-3 BIG METRIC number in lane color +
         * unit label scale-1 dim below. */
        int ty2 = ty1 + text_height(2) + ATOMIK_GRID_M;
        char big[16], unit[24];
        lane_big_metric(lane, big, sizeof big, unit, sizeof unit);
        draw_text(tx, ty2, big, 3, lc);
        int big_w = text_width(big, 3);
        draw_text(tx + big_w + ATOMIK_GRID_M, ty2 + text_height(3) - text_height(1),
                  unit, 1, ATOMIK_FG_DIM);

        /* Row 3: FILLED area waveform across inner width. */
        int wf_y = ty2 + text_height(3) + ATOMIK_GRID_S;
        int wf_x = tx;
        int wf_w = inner_w;
        int wf_h = LANE_ROW_H - (wf_y - ly) - ATOMIK_GRID_M;
        if (wf_h < 12) wf_h = 12;
        /* Build a darker fill color (lane color at half intensity) so the
         * filled band reads as glow body, with brighter line on top. */
        uint8_t fr = (lc >> 16) & 0xFF;
        uint8_t fg_ = (lc >>  8) & 0xFF;
        uint8_t fb = (lc      ) & 0xFF;
        pixel_t fill_dim = rgb(fr / 3, fg_ / 3, fb / 3);
        draw_filled_waveform(h, wf_x, wf_y, wf_w, wf_h, fill_dim, lc);
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
    window_t *w = wm_open("Resource Fabric",
                          FABRIC_SHELF_X, FABRIC_SHELF_Y,
                          FABRIC_SHELF_W, FABRIC_SHELF_H,
                          fabric_draw, NULL);
    if (w) s_window_id = w->id;
}
