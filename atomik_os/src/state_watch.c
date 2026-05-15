/* state_watch.c — v0.35 State Watch surface (Class A).
 *
 * Per ChatGPT 2026-05-09 sequence: after the v0.34-D 5-lane Resource
 * Fabric, the next surface is State Watch — a horizontal time-ribbon
 * answering "what has the system been doing for the last 25 seconds?"
 * Every plotted value comes from a real producer; nothing is fabricated.
 *
 * Producers consumed:
 *   fabric_active()           — current personality, sampled here every tick
 *   fabric_history(lane)      — last 64 lane samples, replayed wider
 *   perf_last_for(STATE)      — most recent STATE batch; speedup pushed
 *                               into our own sparkline ring on change
 *   atomik_event_iter(idx,…)  — chronological walk of the 64-record bus ring
 *   atomik_event_count(kind)  — cumulative per-kind counts (footer summary)
 *
 * Producers we add (kept private to this file):
 *   s_pers_history[N]         — personality_t per tick (color ribbon)
 *   s_speedup_history[N]      — perf_speedup × 100 per fresh STATE sample
 *
 * State Watch owns NO MMIO; it is a passive observer.  No /dev/mem, no
 * heavy work in tick() — safe to call every frame.  Honest UI rule:
 * if a track has no data yet (e.g. no STATE bench has run), it stays
 * empty rather than drawing a phantom flat line. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

/* Sample cadence — matches the fabric internal sampler so the
 * personality ribbon and the lane mini-strips have aligned time bases. */
#define SW_SAMPLE_MS  200

static personality_t  s_pers_history[SW_HISTORY_N];
static uint8_t        s_pers_head;
static uint8_t        s_pers_count;

/* Speedup history: perf_speedup × 100 (fixed-point so we don't pull in
 * float printf in this hot path).  Only pushed when STATE
 * cycles_total changes — i.e. a fresh batch landed. */
static uint16_t       s_speedup_history[SW_HISTORY_N];
static uint8_t        s_speedup_head;
static uint8_t        s_speedup_count;
static uint64_t       s_last_state_cycles;

static unsigned long  s_last_sample_ms;
static int            s_window_id = -1;

static void pers_push(personality_t p) {
    s_pers_history[s_pers_head] = p;
    s_pers_head = (s_pers_head + 1) % SW_HISTORY_N;
    if (s_pers_count < SW_HISTORY_N) s_pers_count++;
}

static void speedup_push(uint16_t v_x100) {
    s_speedup_history[s_speedup_head] = v_x100;
    s_speedup_head = (s_speedup_head + 1) % SW_HISTORY_N;
    if (s_speedup_count < SW_HISTORY_N) s_speedup_count++;
}

void state_watch_tick(void) {
    unsigned long now = anim_now_ms();
    if (now - s_last_sample_ms < SW_SAMPLE_MS) return;
    s_last_sample_ms = now;

    pers_push(fabric_active());

    const perf_sample_t *st = perf_last_for(PERSONALITY_STATE);
    if (st && st->cycles_total != s_last_state_cycles) {
        s_last_state_cycles = st->cycles_total;
        if (st->cycles_software_baseline && st->cycles_atomik) {
            double sp = perf_speedup(st);
            if (sp < 0.0) sp = 0.0;
            uint32_t v = (uint32_t)(sp * 100.0 + 0.5);
            if (v > 0xFFFF) v = 0xFFFF;
            speedup_push((uint16_t)v);
        }
    }
}

/* === colour helpers === */

static pixel_t pers_color(personality_t p) {
    switch (p) {
    case PERSONALITY_STATE: return ATOMIK_SEM_HARDWARE;   /* cyan   */
    case PERSONALITY_SYNC:  return ATOMIK_SEM_SAVINGS;    /* green  */
    case PERSONALITY_AGENT: return ATOMIK_SEM_AGENT;      /* violet */
    default:                return rgb(0x1F, 0x27, 0x38); /* idle bed */
    }
}

static pixel_t event_color(atomik_event_kind_t k) {
    switch (k) {
    case EVT_STATE_DELTA:   return ATOMIK_SEM_HARDWARE;
    case EVT_SYNC_REPLICA:  return ATOMIK_SEM_SAVINGS;
    case EVT_AGENT_CONTEXT: return ATOMIK_SEM_AGENT;
    case EVT_VIS_RENDER:    return rgb(0xE5, 0x6E, 0xC0);
    case EVT_BUILD_RUN:     return rgb(0xF0, 0x9C, 0x55);
    case EVT_OVERRIDE:      return ATOMIK_SEM_WASTE;
    default:                return ATOMIK_FG_DIM;
    }
}

/* === thin line draw (Bresenham-ish) === */

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

/* === track renderers ===
 *
 * All tracks share one X-axis convention: oldest (left) → newest
 * (right).  Track renderers receive (x, y, w, h) of the rect they own
 * and never overflow it. */

static void draw_track_bg(int x, int y, int w, int h, pixel_t bed) {
    draw_rect(x, y, w, h, bed);
    /* 1-px top + bottom hairline for definition. */
    draw_rect(x, y, w, 1, wm_card_border());
    draw_rect(x, y + h - 1, w, 1, wm_card_border());
}

static void draw_personality_ribbon(int x, int y, int w, int h) {
    draw_track_bg(x, y, w, h, rgb(0x10, 0x16, 0x22));
    if (s_pers_count == 0) return;
    /* Each sample paints `slot_w` pixels of color.  We walk samples in
     * chronological order and paint a vertical strip the height of the
     * track for each one, so transitions read as crisp colour changes. */
    int slot_w = w / SW_HISTORY_N;
    if (slot_w < 1) slot_w = 1;
    int draw_w = slot_w * s_pers_count;
    /* Right-anchor the ribbon so the most-recent sample sits at the
     * right edge — natural "now" position. */
    int x0 = x + (w - draw_w);
    if (x0 < x) x0 = x;
    for (uint8_t i = 0; i < s_pers_count; i++) {
        int idx = (s_pers_head - s_pers_count + i + SW_HISTORY_N) % SW_HISTORY_N;
        personality_t p = s_pers_history[idx];
        int px = x0 + i * slot_w;
        draw_rect(px, y + 1, slot_w, h - 2, pers_color(p));
    }
}

static void draw_speedup_sparkline(int x, int y, int w, int h) {
    draw_track_bg(x, y, w, h, rgb(0x10, 0x16, 0x22));
    if (s_speedup_count < 2) {
        /* Empty state — print "no STATE batches yet" centered. */
        const char *msg = "no STATE batches yet — press ! to seed";
        int tw = text_width(msg, 1);
        draw_text(x + (w - tw) / 2,
                  y + (h - text_height(1)) / 2,
                  msg, 1, ATOMIK_FG_DIM);
        return;
    }
    /* Normalize against the running min/max in the buffer. */
    uint16_t mn = 0xFFFF, mx = 0;
    for (uint8_t i = 0; i < s_speedup_count; i++) {
        uint16_t v = s_speedup_history[i];
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    /* If the speedup is constant, draw a flat line near the bottom and
     * still print the value — honest "constant" signal. */
    if (mx <= mn) {
        int py = y + h - 4;
        draw_rect(x + 4, py, w - 8, 1, ATOMIK_SEM_SAVINGS);
        char buf[32];
        snprintf(buf, sizeof buf, "%.2fx (constant)", mx / 100.0);
        draw_text(x + w - text_width(buf, 1) - 6, y + 4,
                  buf, 1, ATOMIK_FG);
        return;
    }
    uint16_t span = mx - mn;
    int prev_px = -1, prev_py = -1;
    for (uint8_t i = 0; i < s_speedup_count; i++) {
        int idx = (s_speedup_head - s_speedup_count + i + SW_HISTORY_N) % SW_HISTORY_N;
        uint16_t v = s_speedup_history[idx];
        int px = x + 1 +
                 (int)((long)i * (w - 2) / (SW_HISTORY_N - 1));
        int py = y + h - 2 -
                 (int)((long)(v - mn) * (h - 4) / span);
        if (py < y + 1) py = y + 1;
        if (py > y + h - 2) py = y + h - 2;
        if (prev_px >= 0) {
            draw_line_segment(prev_px, prev_py, px, py,
                              ATOMIK_SEM_SAVINGS);
        } else {
            draw_pixel(px, py, ATOMIK_SEM_SAVINGS);
        }
        prev_px = px;
        prev_py = py;
    }
    /* Latest value annotation on the right side. */
    int last_idx = (s_speedup_head + SW_HISTORY_N - 1) % SW_HISTORY_N;
    uint16_t latest = s_speedup_history[last_idx];
    char buf[32];
    snprintf(buf, sizeof buf, "%.2fx vs sw  (last STATE batch)",
             latest / 100.0);
    int tw = text_width(buf, 1);
    draw_text(x + w - tw - 6, y + 3, buf, 1, ATOMIK_FG);
}

static void draw_lane_strip(int x, int y, int w, int h,
                            fabric_lane_t lane) {
    pixel_t bed = rgb(0x10, 0x16, 0x22);
    draw_track_bg(x, y, w, h, bed);

    /* Left-side label (5-char wide name). */
    const char *name = fabric_lane_name(lane);
    int label_w = text_width("VISUAL", 1) + 6;
    draw_text(x + 4, y + (h - text_height(1)) / 2, name, 1,
              ATOMIK_FG_DIM);

    int wf_x = x + label_w;
    int wf_w = w - label_w - 4;
    int wf_y = y + 1;
    int wf_h = h - 2;

    const fabric_lane_history_t *hh = fabric_history(lane);
    if (!hh || hh->count < 2) return;

    uint16_t mn = hh->v_min, mx = hh->v_max;
    if (mx <= mn) {
        /* Same constant rule as the fabric mini-waveform: draw a flat
         * line so the lane's "I have data, just no variation" reads. */
        draw_rect(wf_x, wf_y + wf_h - 2, wf_w, 1,
                  pers_color((lane == FABRIC_LANE_STATE) ? PERSONALITY_STATE :
                             (lane == FABRIC_LANE_SYNC)  ? PERSONALITY_SYNC :
                             (lane == FABRIC_LANE_AGENT) ? PERSONALITY_AGENT :
                                                           PERSONALITY_NONE));
        return;
    }
    uint16_t span = mx - mn;

    pixel_t color;
    switch (lane) {
    case FABRIC_LANE_STATE:  color = ATOMIK_SEM_HARDWARE; break;
    case FABRIC_LANE_SYNC:   color = ATOMIK_SEM_SAVINGS;  break;
    case FABRIC_LANE_AGENT:  color = ATOMIK_SEM_AGENT;    break;
    case FABRIC_LANE_EVENT:  color = rgb(0xF0, 0x9C, 0x55); break;
    case FABRIC_LANE_VISUAL: color = rgb(0xE5, 0x6E, 0xC0); break;
    default:                 color = ATOMIK_FG_DIM;       break;
    }

    int prev_px = -1, prev_py = -1;
    for (uint8_t i = 0; i < hh->count; i++) {
        int idx = (hh->head - hh->count + i + FABRIC_HISTORY_N) % FABRIC_HISTORY_N;
        uint16_t v = hh->values[idx];
        int px = wf_x + (int)((long)i * (wf_w - 1) / (FABRIC_HISTORY_N - 1));
        int py = wf_y + wf_h - 1 -
                 (int)((long)(v - mn) * (wf_h - 2) / span);
        if (py < wf_y) py = wf_y;
        if (py > wf_y + wf_h - 1) py = wf_y + wf_h - 1;
        if (prev_px >= 0) draw_line_segment(prev_px, prev_py, px, py, color);
        else              draw_pixel(px, py, color);
        prev_px = px;
        prev_py = py;
    }
}

static void draw_event_spikes(int x, int y, int w, int h) {
    pixel_t bed = rgb(0x10, 0x16, 0x22);
    draw_track_bg(x, y, w, h, bed);

    /* Walk the event ring chronologically.  Map idx → x linearly so the
     * leftmost slot is the oldest event in the ring and the rightmost is
     * the newest.  This is index-based, not time-based — even bursts of
     * events show as evenly spaced spikes which reads more clearly than
     * collapsing them by absolute time. */
    int total = 0;
    for (int i = 0; i < 64; i++) {
        atomik_event_kind_t kind;
        if (!atomik_event_iter(i, &kind, NULL, NULL)) break;
        total++;
    }
    if (total == 0) {
        const char *msg = "event bus is idle";
        int tw = text_width(msg, 1);
        draw_text(x + (w - tw) / 2,
                  y + (h - text_height(1)) / 2,
                  msg, 1, ATOMIK_FG_DIM);
        return;
    }

    int span = total > 1 ? (w - 4) : 1;
    for (int i = 0; i < total; i++) {
        atomik_event_kind_t kind;
        unsigned long ts;
        int detail;
        if (!atomik_event_iter(i, &kind, &ts, &detail)) break;
        (void)ts; (void)detail;
        int px = x + 2 + (total > 1 ? (i * span / (total - 1)) : 0);
        pixel_t c = event_color(kind);
        /* 2-px wide spike, top-anchored — height proportional to a
         * "weight" that values rare kinds (override, build) over the
         * common ones so the eye doesn't wash out. */
        int spike_h = (h - 4);
        if (kind == EVT_VIS_RENDER) spike_h = (h - 4) / 2;   /* common, dampen */
        draw_rect(px, y + h - 2 - spike_h, 2, spike_h, c);
    }
}

static void draw_footer(int x, int y, int w, int h) {
    draw_track_bg(x, y, w, h, rgb(0x10, 0x16, 0x22));
    char buf[256];
    unsigned long total = atomik_event_total();
    snprintf(buf, sizeof buf,
             "TOTAL %lu  /  STATE %lu  /  SYNC %lu  /  AGENT %lu  /  VIS %lu  /  BUILD %lu  /  OVR %lu",
             total,
             atomik_event_count(EVT_STATE_DELTA),
             atomik_event_count(EVT_SYNC_REPLICA),
             atomik_event_count(EVT_AGENT_CONTEXT),
             atomik_event_count(EVT_VIS_RENDER),
             atomik_event_count(EVT_BUILD_RUN),
             atomik_event_count(EVT_OVERRIDE));
    draw_text(x + 6, y + (h - text_height(1)) / 2, buf, 1, ATOMIK_FG);
}

/* === main draw === */

void state_watch_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w; (void)ht;

    /* Header */
    draw_text(x + ATOMIK_GRID_L, y + ATOMIK_GRID_M,
              "STATE WATCH", 2, ATOMIK_FG);
    char sub[96];
    snprintf(sub, sizeof sub,
             "history ribbon  /  %d samples  /  %d ms cadence  /  ~%d s window",
             SW_HISTORY_N, SW_SAMPLE_MS, (SW_HISTORY_N * SW_SAMPLE_MS) / 1000);
    draw_text(x + ATOMIK_GRID_L,
              y + ATOMIK_GRID_M + ATOMIK_TITLEBAR_H + 4,
              sub, 1, ATOMIK_FG_DIM);

    int track_x = x + ATOMIK_GRID_L;
    int track_w = wd - ATOMIK_GRID_L * 2;
    int cur_y = y + ATOMIK_GRID_M + ATOMIK_TITLEBAR_H + text_height(1) + 16;
    int gap = 4;

    /* 1. personality ribbon */
    int ribbon_h = 22;
    draw_text(track_x, cur_y - text_height(1) - 2,
              "active personality", 1, ATOMIK_FG_DIM);
    draw_personality_ribbon(track_x, cur_y, track_w, ribbon_h);
    cur_y += ribbon_h + gap + text_height(1) + 4;

    /* 2. speedup sparkline */
    int sp_h = 50;
    draw_text(track_x, cur_y - text_height(1) - 2,
              "STATE batch speedup vs software (real)", 1, ATOMIK_FG_DIM);
    draw_speedup_sparkline(track_x, cur_y, track_w, sp_h);
    cur_y += sp_h + gap + text_height(1) + 4;

    /* 3. lane mini-strips × 5 */
    draw_text(track_x, cur_y - text_height(1) - 2,
              "lanes (replay of fabric_history)", 1, ATOMIK_FG_DIM);
    int lane_h = 18;
    for (int i = 0; i < FABRIC_N_LANES_V2; i++) {
        draw_lane_strip(track_x, cur_y, track_w, lane_h, (fabric_lane_t)i);
        cur_y += lane_h + 2;
    }
    cur_y += gap;

    /* 4. event spikes */
    int sp2_h = 30;
    draw_text(track_x, cur_y - text_height(1) - 2,
              "event spikes (atomik_event ring, oldest → newest)", 1, ATOMIK_FG_DIM);
    draw_event_spikes(track_x, cur_y, track_w, sp2_h);
    cur_y += sp2_h + gap + text_height(1) + 4;

    /* 5. footer */
    int ft_h = 22;
    draw_footer(track_x, cur_y, track_w, ft_h);
}

/* Geometry: bottom-anchored wide ribbon spanning the workspace
 * (between Capability Rail and the Resource Fabric shelf).  Tall
 * enough to comfortably fit 5 tracks plus the lane strip stack. */
void state_watch_open(void) {
    if (s_window_id >= 0) {
        wm_focus(s_window_id);
        return;
    }
    int x = dock_right_edge() + ATOMIK_GRID_L;
    int w = fabric_shelf_x() - x - ATOMIK_GRID_L;
    int h = 360;
    int y = FB_H - h - ATOMIK_GRID_L;
    if (w < 600) w = 600;            /* sanity-clamp on smaller fb */
    window_t *win = wm_open("State Watch",
                            x, y, w, h,
                            state_watch_draw, NULL);
    if (win) s_window_id = win->id;
}
