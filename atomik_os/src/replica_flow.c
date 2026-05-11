/* replica_flow.c — v0.37 Replica Flow surface.
 *
 * The SYNC personality's product story made visible.  Per ChatGPT
 * 2026-05-09 sequence: after State Watch and the asset pipeline, the
 * next surface answers "what does the SYNC personality DO?".  Three
 * panels — LOCAL node, DELTA PROPAGATION stream, REMOTE replica —
 * plus a footer of the most recent SYNC_REPLICA events from the bus.
 *
 * Producers consumed (all real, all existing):
 *   fabric_active()                    — current personality (header badge)
 *   perf_last_for(PERSONALITY_SYNC)    — last SYNC batch metrics
 *   atomik_event_count(EVT_SYNC_REPLICA) — cumulative replica event count
 *   atomik_event_iter(idx, …)          — chronological event ring
 *
 * Honest UI rule (from feedback_no_class_c_metrics): there is NO real
 * remote endpoint connected.  The right panel is labeled
 * "(local simulation)" so the audience never confuses the visual
 * with a wired-up network sync.  Real SYNC perf metrics still
 * display — they describe what ATOMiK *would* ship if a remote
 * were attached.  bytes_avoided=0 single-batch case surfaces the
 * "(replay engine = v0.39)" placeholder rather than fabricating. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

static int  s_window_id = -1;
static unsigned long s_last_replica_count = 0;

/* In-flight delta pulses — each one is a phase 0..1 across the stream.
 * Spawned when atomik_event_count(EVT_SYNC_REPLICA) advances; decay
 * over PULSE_LIFE_MS as they "arrive" at the remote endpoint. */
typedef struct {
    unsigned long spawn_ms;     /* anim_now_ms() at spawn; 0 = unused */
} pulse_t;

static pulse_t s_pulses[REPLICA_DELTA_RING_N];
#define PULSE_LIFE_MS  1800    /* time to traverse local→remote */

static void spawn_pulse(unsigned long now) {
    /* Reuse oldest slot — stable in-flight count, no allocation. */
    int slot = -1;
    unsigned long oldest = 0;
    for (int i = 0; i < REPLICA_DELTA_RING_N; i++) {
        if (s_pulses[i].spawn_ms == 0) { slot = i; break; }
        if (s_pulses[i].spawn_ms < oldest || oldest == 0) {
            oldest = s_pulses[i].spawn_ms; slot = i;
        }
    }
    if (slot >= 0) s_pulses[slot].spawn_ms = now;
}

void replica_flow_tick(void) {
    unsigned long now = anim_now_ms();
    unsigned long count = atomik_event_count(EVT_SYNC_REPLICA);
    /* Spawn one pulse per replica event since we last looked.  Cap at
     * the ring size to avoid spamming when the workload bursts — the
     * ring is for visualization, not auditing. */
    unsigned long delta = count - s_last_replica_count;
    if (delta > (unsigned long)REPLICA_DELTA_RING_N) {
        delta = REPLICA_DELTA_RING_N;
    }
    for (unsigned long i = 0; i < delta; i++) spawn_pulse(now);
    s_last_replica_count = count;

    /* Reap aged pulses (lifetime expired). */
    for (int i = 0; i < REPLICA_DELTA_RING_N; i++) {
        if (s_pulses[i].spawn_ms == 0) continue;
        if (now - s_pulses[i].spawn_ms > PULSE_LIFE_MS) {
            s_pulses[i].spawn_ms = 0;
        }
    }
}

/* === geometry helpers === */

#define HEADER_H       40
#define PANEL_GAP      ATOMIK_GRID_M
#define BOTTOM_STRIP_H 110
#define ICON_SIZE      120         /* hexagonal node glyph */
#define LANE_BG        rgb(0x10, 0x16, 0x22)

/* === icon: stylized hexagonal node ===
 *
 * Drawn entirely with primitives — no PNG.  Three concentric
 * outlines + radial spokes.  Color depends on which "side" we're
 * drawing (cyan for LOCAL, green for REMOTE) so the eye distinguishes
 * even at a glance.  Closer to the concept image's geometric icons
 * than a flat circle, cheaper to draw than the asset path.  */
static void hex_outline(int cx, int cy, int radius, pixel_t color) {
    /* 6 vertices: angle 0, 60, 120, …, 300.  Plot lines between them. */
    int xs[7], ys[7];
    for (int i = 0; i < 6; i++) {
        /* sin/cos via lookup since we don't link <math.h> in some builds. */
        static const double cos60[6] = { 1.0, 0.5, -0.5, -1.0, -0.5,  0.5 };
        static const double sin60[6] = { 0.0, 0.866, 0.866, 0.0, -0.866, -0.866 };
        xs[i] = cx + (int)(radius * cos60[i]);
        ys[i] = cy + (int)(radius * sin60[i]);
    }
    xs[6] = xs[0]; ys[6] = ys[0];
    for (int i = 0; i < 6; i++) {
        /* Bresenham segment between consecutive vertices. */
        int x0 = xs[i],   y0 = ys[i];
        int x1 = xs[i+1], y1 = ys[i+1];
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
}

static void draw_node_glyph(int cx, int cy, pixel_t accent) {
    /* Three concentric hexagons, brightest in the middle, with radial
     * spokes — visually echoes the concept-image edge-node icons
     * without needing a pre-rendered asset. */
    hex_outline(cx, cy, ICON_SIZE / 2 - 2, accent);
    hex_outline(cx, cy, ICON_SIZE / 3, accent);
    hex_outline(cx, cy, ICON_SIZE / 5, accent);

    /* Center dot — a stronger anchor. */
    for (int dy = -3; dy <= 3; dy++)
        for (int dx = -3; dx <= 3; dx++)
            if (dx*dx + dy*dy <= 9) draw_pixel(cx + dx, cy + dy, accent);

    /* Radial spokes (6 directions, same as hex vertices). */
    static const double cos60[6] = { 1.0, 0.5, -0.5, -1.0, -0.5,  0.5 };
    static const double sin60[6] = { 0.0, 0.866, 0.866, 0.0, -0.866, -0.866 };
    int outer = ICON_SIZE / 2 - 2;
    int inner = ICON_SIZE / 5;
    for (int i = 0; i < 6; i++) {
        int x0 = cx + (int)(inner * cos60[i]);
        int y0 = cy + (int)(inner * sin60[i]);
        int x1 = cx + (int)(outer * cos60[i]);
        int y1 = cy + (int)(outer * sin60[i]);
        int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
        int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;
        while (1) {
            draw_pixel(x0, y0, accent);
            if (x0 == x1 && y0 == y1) break;
            int e2 = err * 2;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 <  dx) { err += dx; y0 += sy; }
        }
    }
}

/* === panel renderers === */

static void draw_local_panel(int x, int y, int w, int h) {
    draw_rect(x, y, w, h, wm_card_bg());
    draw_rect(x, y, w, 1, wm_card_border());
    draw_rect(x, y + h - 1, w, 1, wm_card_border());
    draw_rect(x, y, 1, h, wm_card_border());
    draw_rect(x + w - 1, y, 1, h, wm_card_border());
    /* Cyan accent strip (active when SYNC personality is current). */
    if (fabric_active() == PERSONALITY_SYNC) {
        draw_rect(x, y, 3, h, ATOMIK_SEM_HARDWARE);
    }

    int tx = x + ATOMIK_GRID_L;
    draw_text(tx, y + ATOMIK_GRID_M, "LOCAL EDGE NODE", 1, ATOMIK_FG);
    draw_text(tx, y + ATOMIK_GRID_M + text_height(1) + 4,
              "where deltas originate", 1, ATOMIK_FG_DIM);

    /* Node glyph centered horizontally, upper half. */
    int cx = x + w / 2;
    int cy = y + h / 2 - 8;
    draw_node_glyph(cx, cy, ATOMIK_SEM_HARDWARE);

    /* Real SYNC perf metrics in a 4-line stack at the bottom. */
    const perf_sample_t *s = perf_last_for(PERSONALITY_SYNC);
    int my = y + h - text_height(1) * 5 - 12;
    if (!s) {
        draw_text(tx, my, "no SYNC batch yet", 1, ATOMIK_FG);
        draw_text(tx, my + text_height(1) + 4,
                  "press ! to seed perf bench", 1, ATOMIK_FG_DIM);
    } else {
        char l1[64], l2[64], l3[64], l4[64];
        snprintf(l1, sizeof l1, "regions touched   %u",
                 (unsigned)s->regions_touched);
        snprintf(l2, sizeof l2, "ops emitted       %u / %u",
                 (unsigned)s->ops_issued, (unsigned)s->regions_unique);
        snprintf(l3, sizeof l3, "bytes processed   %u",
                 (unsigned)s->bytes_processed);
        if (s->bytes_avoided == 0) {
            snprintf(l4, sizeof l4, "(replay engine = v0.39 unlocks bytes-avoided)");
        } else {
            snprintf(l4, sizeof l4, "bytes avoided     %u",
                     (unsigned)s->bytes_avoided);
        }
        int line_h = text_height(1) + 4;
        draw_text(tx, my,                   l1, 1, ATOMIK_FG);
        draw_text(tx, my + line_h,          l2, 1, ATOMIK_FG);
        draw_text(tx, my + line_h * 2,      l3, 1, ATOMIK_FG);
        draw_text(tx, my + line_h * 3,      l4, 1, ATOMIK_FG_DIM);
    }
}

static void draw_remote_panel(int x, int y, int w, int h) {
    draw_rect(x, y, w, h, wm_card_bg());
    draw_rect(x, y, w, 1, wm_card_border());
    draw_rect(x, y + h - 1, w, 1, wm_card_border());
    draw_rect(x, y, 1, h, wm_card_border());
    draw_rect(x + w - 1, y, 1, h, wm_card_border());

    int tx = x + ATOMIK_GRID_L;
    draw_text(tx, y + ATOMIK_GRID_M, "REMOTE REPLICA", 1, ATOMIK_FG);
    draw_text(tx, y + ATOMIK_GRID_M + text_height(1) + 4,
              "(local simulation — no wire)", 1, ATOMIK_SEM_WASTE);

    /* Same glyph, green tone — stays distinct from the cyan local. */
    int cx = x + w / 2;
    int cy = y + h / 2 - 8;
    draw_node_glyph(cx, cy, ATOMIK_SEM_SAVINGS);

    /* "Replica state" footer.  We can honestly report:
     *   - cumulative SYNC_REPLICA event count → "deltas received"
     *   - latency: number of in-flight pulses → backlog
     *   - last replica timestamp → "last update"
     *
     * No simulated CPU/RAM/temp/etc — those would be Class C. */
    unsigned long total = atomik_event_count(EVT_SYNC_REPLICA);
    unsigned long last_ts = atomik_event_last_ts(EVT_SYNC_REPLICA);
    unsigned long now = anim_now_ms();
    int in_flight = 0;
    for (int i = 0; i < REPLICA_DELTA_RING_N; i++)
        if (s_pulses[i].spawn_ms != 0) in_flight++;

    int my = y + h - text_height(1) * 5 - 12;
    char l1[64], l2[64], l3[64], l4[64];
    snprintf(l1, sizeof l1, "deltas received   %lu", total);
    snprintf(l2, sizeof l2, "in-flight         %d", in_flight);
    if (last_ts == 0) {
        snprintf(l3, sizeof l3, "last update       n/a");
    } else {
        unsigned long age_ms = now - last_ts;
        if      (age_ms < 1000)  snprintf(l3, sizeof l3, "last update       %lu ms ago", age_ms);
        else if (age_ms < 60000) snprintf(l3, sizeof l3, "last update       %lu s ago",  age_ms / 1000);
        else                     snprintf(l3, sizeof l3, "last update       %lu m ago",  age_ms / 60000);
    }
    snprintf(l4, sizeof l4, "(persistent storage = v0.40+)");

    int line_h = text_height(1) + 4;
    draw_text(tx, my,                   l1, 1, ATOMIK_FG);
    draw_text(tx, my + line_h,          l2, 1, ATOMIK_FG);
    draw_text(tx, my + line_h * 2,      l3, 1, ATOMIK_FG);
    draw_text(tx, my + line_h * 3,      l4, 1, ATOMIK_FG_DIM);
}

/* === middle panel: delta propagation stream === */

static void draw_propagation(int x, int y, int w, int h) {
    /* Background */
    draw_rect(x, y, w, h, LANE_BG);
    draw_rect(x, y, w, 1, wm_card_border());
    draw_rect(x, y + h - 1, w, 1, wm_card_border());

    /* Header text */
    draw_text(x + ATOMIK_GRID_M, y + ATOMIK_GRID_M,
              "DELTA PROPAGATION", 1, ATOMIK_FG);
    char sub[64];
    unsigned long total = atomik_event_count(EVT_SYNC_REPLICA);
    snprintf(sub, sizeof sub,
             "%lu total replicas shipped (real, from event bus)", total);
    draw_text(x + ATOMIK_GRID_M,
              y + ATOMIK_GRID_M + text_height(1) + 2,
              sub, 1, ATOMIK_FG_DIM);

    /* Two horizontal "rails" along the centerline, with pulses
     * interpolating between LOCAL and REMOTE. */
    int rail_y = y + h / 2 + 8;
    int margin = ATOMIK_GRID_L;
    int rail_left  = x + margin;
    int rail_right = x + w - margin;
    int rail_w     = rail_right - rail_left;

    /* Static rail line — dim, suggests the path. */
    for (int i = 0; i < rail_w; i += 6) {
        draw_pixel(rail_left + i,     rail_y, ATOMIK_DOCK_BORDER);
        draw_pixel(rail_left + i + 1, rail_y, ATOMIK_DOCK_BORDER);
    }
    /* Arrow head at the right. */
    for (int i = 0; i < 8; i++) {
        draw_pixel(rail_right - i, rail_y - i, ATOMIK_FG_DIM);
        draw_pixel(rail_right - i, rail_y + i, ATOMIK_FG_DIM);
    }

    /* Animated pulses — render each in-flight delta as a small dot
     * along the rail at position phase × rail_w.  Trailing fade by
     * stamping 3 dots per pulse with decreasing brightness. */
    unsigned long now = anim_now_ms();
    for (int i = 0; i < REPLICA_DELTA_RING_N; i++) {
        unsigned long sp = s_pulses[i].spawn_ms;
        if (sp == 0) continue;
        unsigned long age = now - sp;
        if (age >= PULSE_LIFE_MS) continue;
        double phase = (double)age / (double)PULSE_LIFE_MS;
        int px = rail_left + (int)(phase * (rail_w - 8));

        /* Head: bright cyan dot. */
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++)
                if (dx*dx + dy*dy <= 4)
                    draw_pixel(px + dx, rail_y + dy, ATOMIK_SEM_HARDWARE);
        /* Tail: a small dimmer trail behind the head. */
        for (int t = 1; t <= 4; t++) {
            int tx_p = px - t * 4;
            if (tx_p <= rail_left) break;
            uint8_t r = 0x4F * (5 - t) / 8;
            uint8_t g = 0xC3 * (5 - t) / 8;
            uint8_t b = 0xFF * (5 - t) / 8;
            draw_pixel(tx_p,     rail_y, rgb(r, g, b));
            draw_pixel(tx_p + 1, rail_y, rgb(r, g, b));
        }
    }

    /* Side annotations — the personality + total count. */
    pixel_t badge_color =
        (fabric_active() == PERSONALITY_SYNC)  ? ATOMIK_SEM_SAVINGS  :
        (fabric_active() == PERSONALITY_STATE) ? ATOMIK_SEM_HARDWARE :
        (fabric_active() == PERSONALITY_AGENT) ? ATOMIK_SEM_AGENT    :
                                                  ATOMIK_FG_DIM;
    char badge[32];
    snprintf(badge, sizeof badge, "[ AUTO: %s ]",
             fabric_personality_name(fabric_active()));
    int bw = text_width(badge, 1);
    draw_text(x + w - bw - ATOMIK_GRID_M, y + ATOMIK_GRID_M,
              badge, 1, badge_color);
}

/* === bottom strip: recent delta shares === */

static void draw_recent_strip(int x, int y, int w, int h) {
    draw_rect(x, y, w, h, wm_card_bg());
    draw_rect(x, y, w, 1, wm_card_border());
    draw_rect(x, y + h - 1, w, 1, wm_card_border());

    draw_text(x + ATOMIK_GRID_M, y + ATOMIK_GRID_M,
              "RECENT DELTA SHARES", 1, ATOMIK_FG);
    draw_text(x + ATOMIK_GRID_M,
              y + ATOMIK_GRID_M + text_height(1) + 2,
              "(walked from atomik_event ring; SYNC_REPLICA only)",
              1, ATOMIK_FG_DIM);

    /* Walk the event bus chronologically and pick out the most-recent
     * SYNC_REPLICA records.  We collect into a small array first so
     * we can render newest-on-top. */
    typedef struct { unsigned long ts; int detail; } rec_t;
    rec_t recs[8];
    int n_rec = 0;
    /* Iterate from oldest to newest; later overwrite gets newest in
     * recs[0..n_rec-1] when we shift. */
    for (int i = 0; i < 64 && n_rec < 8; i++) {
        atomik_event_kind_t kind;
        unsigned long ts;
        int detail;
        if (!atomik_event_iter(i, &kind, &ts, &detail)) break;
        if (kind != EVT_SYNC_REPLICA) continue;
        /* Append; we'll reverse for display. */
        recs[n_rec].ts = ts;
        recs[n_rec].detail = detail;
        n_rec++;
    }

    int row_y = y + ATOMIK_GRID_M + text_height(1) * 2 + 6;
    int row_h = text_height(1) + 3;
    if (n_rec == 0) {
        draw_text(x + ATOMIK_GRID_M, row_y,
                  "no SYNC_REPLICA events on the bus yet", 1, ATOMIK_FG_DIM);
        return;
    }
    /* Render newest first (reverse order). */
    unsigned long now = anim_now_ms();
    int row_count = 0;
    for (int i = n_rec - 1; i >= 0 && row_count < 4; i--) {
        unsigned long age = (recs[i].ts <= now) ? (now - recs[i].ts) : 0;
        char age_buf[32];
        if      (age < 1000)  snprintf(age_buf, sizeof age_buf, "%lu ms",  age);
        else if (age < 60000) snprintf(age_buf, sizeof age_buf, "%lu s",   age / 1000);
        else                  snprintf(age_buf, sizeof age_buf, "%lu min", age / 60000);
        char line[96];
        snprintf(line, sizeof line, "%-10s ago    detail=%-4d    region delta shipped",
                 age_buf, recs[i].detail);
        draw_text(x + ATOMIK_GRID_M, row_y + row_count * row_h,
                  line, 1, ATOMIK_FG);
        row_count++;
    }
}

/* === main draw === */

void replica_flow_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w; (void)ht;

    /* Header */
    draw_text(x + ATOMIK_GRID_L, y + ATOMIK_GRID_M,
              "REPLICA FLOW", 2, ATOMIK_FG);
    draw_text(x + ATOMIK_GRID_L,
              y + ATOMIK_GRID_M + ATOMIK_TITLEBAR_H + 4,
              "SYNC personality · LOCAL → DELTA STREAM → REMOTE",
              1, ATOMIK_FG_DIM);

    /* Three-column body: LOCAL | PROPAGATION | REMOTE.
     * Equal split, with gaps between. */
    int body_y = y + ATOMIK_GRID_M + ATOMIK_TITLEBAR_H + text_height(1) + 12;
    int body_h = ht - (body_y - y) - BOTTOM_STRIP_H - PANEL_GAP - 4;
    int cell_w = (wd - ATOMIK_GRID_L * 2 - PANEL_GAP * 2) / 3;
    int col1_x = x + ATOMIK_GRID_L;
    int col2_x = col1_x + cell_w + PANEL_GAP;
    int col3_x = col2_x + cell_w + PANEL_GAP;

    draw_local_panel(col1_x, body_y, cell_w, body_h);
    draw_propagation(col2_x, body_y, cell_w, body_h);
    draw_remote_panel(col3_x, body_y, cell_w, body_h);

    /* Bottom strip */
    int strip_y = body_y + body_h + PANEL_GAP;
    int strip_w = wd - ATOMIK_GRID_L * 2;
    draw_recent_strip(col1_x, strip_y, strip_w, BOTTOM_STRIP_H);
}

/* Geometry: wide, fills most of the workspace.  Sits at the top of
 * the workspace area so it doesn't overlap with State Watch (which is
 * bottom-anchored). */
void replica_flow_open(void) {
    if (s_window_id >= 0) {
        wm_focus(s_window_id);
        return;
    }
    int x  = dock_right_edge() + ATOMIK_GRID_L;
    int w  = fabric_shelf_x() - x - ATOMIK_GRID_L;
    int h  = 480;
    int y  = ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + ATOMIK_GRID_L;
    if (w < 700) w = 700;     /* clamp on narrower boards */
    window_t *win = wm_open("Replica Flow",
                            x, y, w, h,
                            replica_flow_draw, NULL);
    if (win) s_window_id = win->id;
}
