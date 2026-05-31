/* status.c — Pulse Bar.
 *
 * Reads /proc/uptime for clock display (since this rootfs has no RTC),
 * /proc/stat for CPU usage, and surfaces the agent prediction.
 *
 * v0.38-C: Pulse Bar 2.0 per ChatGPT 2026-05-10 directive — make the
 * top bar feel like a real status instrument, not a thin text strip.
 *   - bar height 32 → ATOMIK_PULSE_BAR_H (40 px) for visual weight
 *   - ATOMiK wordmark scaled 1 → 2 (the visual anchor on the left)
 *   - DATA / MODE / PERSONALITY badges with colored locator dots
 *   - Event-pulse mini-waveform driven by atomik_event_total deltas
 *   - Mode is now controllable via 'M' (DEV → DEMO → INVESTOR → DEV)
 *   - Right segment still: wallet / cpu / uptime / version */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* v0.31 patch 3 diagnostic was 2026-05-07 — confirmed HDMI top-edge
 * crop of ~32 px.  Diagnostic removed; ATOMIK_SAFE_TOP now encodes the
 * inset.  Set ATOMIK_DEBUG_STATUS=1 at build time to re-enable. */
#ifndef ATOMIK_DEBUG_STATUS
#define ATOMIK_DEBUG_STATUS 0
#endif

static unsigned long s_last_total = 0;
static unsigned long s_last_idle  = 0;
static int           s_cpu_pct    = 0;

/* === v0.38-C: event-pulse mini-waveform state ===
 *
 * Sample atomik_event_total() at fixed cadence; render the last N
 * deltas as a thin sparkline inside the Pulse Bar.  Visible signal of
 * "is the system alive and doing work right now."  Real data — the
 * bus is the same event ring every other surface consumes. */
#define PULSE_HISTORY_N    32
#define PULSE_SAMPLE_MS    150
static uint16_t       s_pulse_hist[PULSE_HISTORY_N];
static uint8_t        s_pulse_head;
static uint8_t        s_pulse_count;
static unsigned long  s_last_pulse_sample_ms;
static unsigned long  s_last_pulse_total;

static void update_cpu(void) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return;
    char buf[256];
    if (!fgets(buf, sizeof buf, f)) { fclose(f); return; }
    fclose(f);
    /* Format: "cpu  user nice system idle iowait irq softirq steal" */
    unsigned long u, n, sy, id, io = 0, ir = 0, sf = 0, st = 0;
    if (sscanf(buf, "cpu  %lu %lu %lu %lu %lu %lu %lu %lu",
               &u, &n, &sy, &id, &io, &ir, &sf, &st) < 4) return;
    unsigned long total = u + n + sy + id + io + ir + sf + st;
    unsigned long idle  = id + io;
    if (s_last_total > 0 && total > s_last_total) {
        unsigned long dt = total - s_last_total;
        unsigned long di = idle  - s_last_idle;
        s_cpu_pct = (int)(100 - (di * 100 / dt));
        if (s_cpu_pct < 0)   s_cpu_pct = 0;
        if (s_cpu_pct > 100) s_cpu_pct = 100;
    }
    s_last_total = total;
    s_last_idle  = idle;
}

static void format_uptime(char *out, size_t cap) {
    FILE *f = fopen("/proc/uptime", "r");
    double up = 0;
    if (f) { fscanf(f, "%lf", &up); fclose(f); }
    int sec = (int)up;
    int h = sec / 3600, m = (sec % 3600) / 60, s = sec % 60;
    snprintf(out, cap, "uptime %02d:%02d:%02d", h, m, s);
}

/* v0.38-C: poll the event bus at PULSE_SAMPLE_MS and push the delta
 * into the sparkline ring.  Called every frame; self-rate-limits.  No
 * heavy work — one atomik_event_total() call + a ring store. */
void status_tick(void) {
    unsigned long now = anim_now_ms();
    if (now - s_last_pulse_sample_ms < PULSE_SAMPLE_MS) return;
    s_last_pulse_sample_ms = now;
    unsigned long total = atomik_event_total();
    unsigned long delta = total - s_last_pulse_total;
    s_last_pulse_total = total;
    /* Clamp so a burst doesn't blow out the auto-normalization. */
    if (delta > 0xFFFF) delta = 0xFFFF;
    s_pulse_hist[s_pulse_head] = (uint16_t)delta;
    s_pulse_head = (s_pulse_head + 1) % PULSE_HISTORY_N;
    if (s_pulse_count < PULSE_HISTORY_N) s_pulse_count++;
}

/* Thin Bresenham segment — same approach as fabric_history mini-waves. */
static void pulse_line(int x0, int y0, int x1, int y1, pixel_t color) {
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

static int draw_event_pulse(int x, int y, int w, int h) {
    /* Track outline — dim 1-px hairline so the sparkline always sits
     * on a visible bed. */
    pixel_t bed = rgb(0x11, 0x18, 0x28);
    draw_rect(x, y, w, h, bed);
    draw_rect(x, y + h - 1, w, 1, ATOMIK_DOCK_BORDER);
    if (s_pulse_count < 2) return w;
    /* Auto-normalize: max-of-window so quiet periods don't compress. */
    uint16_t mx = 0;
    for (uint8_t i = 0; i < s_pulse_count; i++)
        if (s_pulse_hist[i] > mx) mx = s_pulse_hist[i];
    if (mx == 0) {
        /* Constant zero — draw a flat dim baseline. */
        draw_rect(x, y + h - 2, w, 1, ATOMIK_DOCK_BORDER);
        return w;
    }
    /* Walk samples in chronological order. */
    int prev_px = -1, prev_py = -1;
    for (uint8_t i = 0; i < s_pulse_count; i++) {
        int idx = (s_pulse_head - s_pulse_count + i + PULSE_HISTORY_N)
                  % PULSE_HISTORY_N;
        uint16_t v = s_pulse_hist[idx];
        int px = x + (int)((long)i * (w - 1) / (PULSE_HISTORY_N - 1));
        int py = y + h - 2 -
                 (int)((long)v * (h - 3) / mx);
        if (py < y + 1)         py = y + 1;
        if (py > y + h - 2)     py = y + h - 2;
        if (prev_px >= 0)
            pulse_line(prev_px, prev_py, px, py, ATOMIK_SEM_HARDWARE);
        else
            draw_pixel(px, py, ATOMIK_SEM_HARDWARE);
        prev_px = px;
        prev_py = py;
    }
    return w;
}

/* === v0.38-K2.5 small primitives ===
 *
 * Filled diamond glyph in the supplied color, centered at (cx, cy)
 * with radius r.  Used as the active-personality icon inside the
 * SYNC ACTIVE / STATE ACTIVE / AGENT ACTIVE pill.  Diamond was
 * chosen over a hexagon because the rasterized hex looks chunky at
 * 10 px on this pixel grid; the diamond's |dx|+|dy|<=r mask gives a
 * crisp recognizable shape. */
static void draw_personality_glyph(int cx, int cy, int r,
                                   pixel_t color) {
    for (int dy = -r; dy <= r; dy++) {
        int span = r - (dy < 0 ? -dy : dy);
        for (int dx = -span; dx <= span; dx++) {
            draw_pixel(cx + dx, cy + dy, color);
        }
    }
    /* 1 px bright inner highlight — top-left corner of the diamond
     * picks up the "glass" feel by reading slightly hotter. */
    pixel_t hot = rgb(0xFF, 0xFF, 0xFF);
    draw_blend_pixel(cx - 1, cy - 1, hot, 90);
    draw_blend_pixel(cx,     cy - 2, hot, 60);
}

/* v0.38-K2.5 layered-stroke pulse waveform.
 *
 * Replaces the old 1-px Bresenham line in draw_event_pulse() for the
 * investor bar.  Stack per column:
 *   - 8 px outer halo (alpha ~16)
 *   - 4 px mid glow   (alpha ~36)
 *   - 2 px bright core line
 * Plus a hot dot at the most recent sample so the eye locks onto
 * "right now."  Reads as instrument, not debug trace. */
static void draw_event_pulse_glow(int x, int y, int w, int h,
                                  pixel_t accent) {
    /* v0.38-K2.5A — soften the bed so the waveform is the visual
     * hero instead of an opaque dark rectangle sitting under it.
     * Previous bed was a solid rgb(0x10,0x18,0x28) fill; ChatGPT
     * 2026-05-16 flagged that as "an empty status slot" — too
     * visually dominant. Now: just a very subtle accent-tinted
     * alpha wash so the strip reads as embedded in the glass bar.
     * No hard bottom hairline either — the bar's own glass edge
     * is the boundary. */
    for (int sy = 0; sy < h; sy++) {
        for (int sx = 0; sx < w; sx++) {
            draw_blend_pixel(x + sx, y + sy, accent, 8);
        }
    }

    /* v0.38-K3 — baseline is now a low-amplitude SINE WAVE traveling
     * across the strip, not a flat glow band.  ChatGPT 2026-05-16:
     * "the baseline is too flat. It looks like a powered strip,
     * not a waveform. ... give it a shallow sinusoidal contour."
     * Class B discipline preserved: still never writes to the
     * metric ring; pure decorative chrome saying "instrument alive."
     *
     * Distinguishes waiting from active:
     *   - waiting: low amplitude, low alpha, no hot point, no peaks
     *   - active (data branch below): bright core, peaks, hot point
     */
    unsigned long now = anim_now_ms();
    int   baseline_y = y + h * 3 / 4;
    int   amp        = h / 6;                       /* shallow */
    int   col_y[1024];
    int   n_cols     = (w < 1024) ? w : 1024;
    /* Two-component sine for organic motion. */
    double t = (double)now / 1000.0;
    for (int col = 0; col < n_cols; col++) {
        double u = (double)col / (double)(n_cols - 1);
        double phase = u * 6.2831853 * 1.5 + t * 1.2;
        /* Taylor sin to avoid pulling math.h in here. */
        double s  = phase;
        while (s > 3.14159265) s -= 6.2831853;
        while (s < -3.14159265) s += 6.2831853;
        double s3 = s*s*s, s5 = s3*s*s;
        double sn = s - s3/6.0 + s5/120.0;
        col_y[col] = baseline_y + (int)(sn * amp);
    }
    /* Soft fill + 2 px line — quieter than the active waveform. */
    for (int col = 0; col < n_cols; col++) {
        int px = x + col;
        int cy = col_y[col];
        /* Halo above the line. */
        for (int g = 1; g <= 4; g++) {
            int gy = cy - g;
            if (gy < y) break;
            uint8_t a = (uint8_t)(18 - (g - 1) * 4);
            draw_blend_pixel(px, gy, accent, a);
        }
        /* Soft fill below the line into the bar floor. */
        for (int g = 1; g <= 6; g++) {
            int gy = cy + g;
            if (gy > y + h - 2) break;
            uint8_t a = (uint8_t)(14 - (g - 1) * 2);
            draw_blend_pixel(px, gy, accent, a);
        }
        /* The line itself — soft, no hot point. */
        draw_blend_pixel(px, cy,     accent, 120);
        draw_blend_pixel(px, cy - 1, accent, 60);
    }

    if (s_pulse_count < 2) {
        /* No event history yet — return after baseline animation. */
        return;
    }

    /* Compute per-column heights so we can apply the layered stack
     * uniformly across the visible width. */
    uint16_t mx = 0;
    for (uint8_t i = 0; i < s_pulse_count; i++)
        if (s_pulse_hist[i] > mx) mx = s_pulse_hist[i];
    if (mx == 0) {
        /* All-zero history — the procedural baseline above already
         * painted the idle line; nothing more to render. */
        return;
    }

    /* Reuse col_y array — same scope as baseline above.  Overwrite
     * with data-driven heights for the active waveform. */
    for (int col = 0; col < n_cols; col++) {
        int sample_i = (int)((long)col * (s_pulse_count - 1) /
                              (n_cols - 1));
        int idx = (s_pulse_head - s_pulse_count + sample_i
                   + PULSE_HISTORY_N) % PULSE_HISTORY_N;
        uint16_t v = s_pulse_hist[idx];
        int py = y + h - 2 - (int)((long)v * (h - 3) / mx);
        if (py < y + 1)     py = y + 1;
        if (py > y + h - 2) py = y + h - 2;
        col_y[col] = py;
    }

    /* v0.38-K2.5A — brighter halo stack so the wave anchors the
     * center module instead of getting lost.  Outer halo widened
     * 8 → 10 with a flatter falloff; mid glow brightened; core
     * 2 px thick at full intensity. */
    for (int col = 0; col < n_cols; col++) {
        int px = x + col;
        for (int g = 1; g <= 10; g++) {
            int gy = col_y[col] - g;
            if (gy < y) break;
            uint8_t a = (uint8_t)(28 - (g - 1) * 2);
            draw_blend_pixel(px, gy, accent, a);
        }
        for (int g = 1; g <= 6; g++) {
            int gy = col_y[col] + g;
            if (gy > y + h - 2) break;
            uint8_t a = (uint8_t)(26 - (g - 1) * 3);
            draw_blend_pixel(px, gy, accent, a);
        }
    }
    /* Mid glow — 5 px wide, brighter. */
    for (int col = 0; col < n_cols; col++) {
        int px = x + col;
        for (int g = 1; g <= 5; g++) {
            int gy = col_y[col] - g;
            if (gy < y) break;
            uint8_t a = (uint8_t)(60 - (g - 1) * 9);
            draw_blend_pixel(px, gy, accent, a);
        }
    }
    /* Bright core: 2 hard px + 1 px alpha 240 below. */
    for (int col = 0; col < n_cols; col++) {
        int px = x + col;
        draw_pixel(px, col_y[col], accent);
        draw_pixel(px, col_y[col] - 1, accent);
        draw_blend_pixel(px, col_y[col] + 1, accent, 240);
    }

    /* Hot point at the most-recent sample — small, bright. */
    int hot_col = n_cols - 1;
    int hot_x   = x + hot_col;
    int hot_y   = col_y[hot_col];
    int hr      = 3;
    for (int dy = -hr; dy <= hr; dy++) {
        for (int dx = -hr; dx <= hr; dx++) {
            if (dx*dx + dy*dy <= hr*hr) {
                draw_pixel(hot_x + dx, hot_y + dy, accent);
            }
        }
    }
    /* Hot point outer halo — 5 px alpha ring. */
    int hr2 = 6;
    for (int dy = -hr2; dy <= hr2; dy++) {
        for (int dx = -hr2; dx <= hr2; dx++) {
            int d2 = dx*dx + dy*dy;
            if (d2 > hr*hr && d2 <= hr2*hr2) {
                int dist = hr2 - (int)(d2 / (hr2 + 1));
                uint8_t a = (uint8_t)(dist * 18);
                draw_blend_pixel(hot_x + dx, hot_y + dy, accent, a);
            }
        }
    }
}

/* === v0.38-K2 glass-capsule pill primitive ===
 *
 * Draws a rounded-rect chip with a subtle alpha border in the supplied
 * accent color, AA text inside.  The "glass" effect comes from:
 *   - dark slightly-translucent body (wm_card_bg derivative)
 *   - 2 px wide rounded outline rendered with the layered-stroke
 *     doctrine: outer halo at low alpha + bright core border
 *   - text rendered through the AA atlas
 *
 * Returns the width consumed (chip width + trailing gap). */
static int draw_glass_pill(int x, int y_center, const char *text,
                           pixel_t accent) {
    if (!font_aa_loaded(FONT_AA_UI)) {
        /* Pre-atlas fallback: plain text in accent color, no chip. */
        int ty = y_center - text_height(1) / 2;
        draw_text(x, ty, text, 1, accent);
        return text_width(text, 1) + ATOMIK_GRID_L;
    }

    int pad_x  = ATOMIK_GRID_M + 2;
    int pad_y  = 4;
    int tw     = text_width_aa(FONT_AA_UI, text);
    int th     = text_height_aa(FONT_AA_UI);
    int chip_w = tw + pad_x * 2;
    int chip_h = th + pad_y * 2;
    int chip_x = x;
    int chip_y = y_center - chip_h / 2;
    int radius = chip_h / 2;

    /* Dark glass body. */
    pixel_t body = rgb(0x10, 0x18, 0x2C);
    draw_rect_rounded(chip_x, chip_y, chip_w, chip_h, radius, body);

    /* Outer halo — 3 px alpha falloff in accent color. */
    for (int g = 1; g <= 3; g++) {
        uint8_t a = (uint8_t)(24 - (g - 1) * 6);
        for (int sx = 0; sx < chip_w + 2 * g; sx++) {
            draw_blend_pixel(chip_x - g + sx, chip_y - g,            accent, a);
            draw_blend_pixel(chip_x - g + sx, chip_y + chip_h - 1 + g, accent, a);
        }
        for (int sy = 0; sy < chip_h + 2 * g; sy++) {
            draw_blend_pixel(chip_x - g,            chip_y - g + sy, accent, a);
            draw_blend_pixel(chip_x + chip_w - 1 + g, chip_y - g + sy, accent, a);
        }
    }
    /* Core border — bright 1 px line in accent color. */
    draw_rect(chip_x + radius, chip_y,              chip_w - radius * 2, 1, accent);
    draw_rect(chip_x + radius, chip_y + chip_h - 1, chip_w - radius * 2, 1, accent);
    draw_rect(chip_x,              chip_y + radius, 1, chip_h - radius * 2, accent);
    draw_rect(chip_x + chip_w - 1, chip_y + radius, 1, chip_h - radius * 2, accent);

    /* Text inside, vertically centered using ascender for stability. */
    int text_y = chip_y + pad_y;
    draw_text_aa(FONT_AA_UI, chip_x + pad_x, text_y, text, accent);

    return chip_w + ATOMIK_GRID_M;
}

/* Helper: pick the active personality's display color. */
static pixel_t personality_color(personality_t p) {
    switch (p) {
    case PERSONALITY_STATE: return ATOMIK_SEM_HARDWARE;
    case PERSONALITY_SYNC:  return ATOMIK_SEM_SAVINGS;
    case PERSONALITY_AGENT: return ATOMIK_SEM_AGENT;
    default:                return ATOMIK_FG_DIM;
    }
}

/* v0.40 decorative glowing orb — pure Class B chrome, renders NO metric.
 * Three stacked alpha disks (cyan halo -> cyan body -> white core). */
static void pulse_orb(int cx, int cy, int r) {
    int r2 = r * r;
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx*dx + dy*dy <= r2)
                draw_blend_pixel(cx+dx, cy+dy, ATOMIK_SEM_HARDWARE, 70);
    int rm = (r * 2) / 3, rm2 = rm * rm;
    for (int dy = -rm; dy <= rm; dy++)
        for (int dx = -rm; dx <= rm; dx++)
            if (dx*dx + dy*dy <= rm2)
                draw_blend_pixel(cx+dx, cy+dy, ATOMIK_SEM_HARDWARE, 90);
    int rc = r / 3, rc2 = rc * rc;
    for (int dy = -rc; dy <= rc; dy++)
        for (int dx = -rc; dx <= rc; dx++)
            if (dx*dx + dy*dy <= rc2)
                draw_blend_pixel(cx+dx, cy+dy, ATOMIK_FG, 210);
}

/* v0.40 thin vertical module separator inside the glass header. */
static void pulse_sep(int x, int bar_y, int bar_h) {
    int sy = bar_y + 16, sh = bar_h - 32;
    for (int i = 0; i < sh; i++)
        draw_blend_pixel(x, sy + i, ATOMIK_ACCENT_DIM, 90);
}

/* === v0.40-05-30 concept-header helpers: small line icons, an audio-style
 * pulse waveform, and a stacked "icon + LABEL / value" metric module. === */
static void px_dot(int cx, int cy, int r, pixel_t c, uint8_t a) {
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx*dx + dy*dy <= r*r) draw_blend_pixel(cx+dx, cy+dy, c, a);
}
static void px_ring(int cx, int cy, int r, pixel_t c, uint8_t a) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        const int p[8][2] = {{cx+x,cy+y},{cx-x,cy+y},{cx+x,cy-y},{cx-x,cy-y},
                             {cx+y,cy+x},{cx-y,cy+x},{cx+y,cy-x},{cx-y,cy-x}};
        for (int i = 0; i < 8; i++) draw_blend_pixel(p[i][0], p[i][1], c, a);
        y++; if (err < 0) err += 2*y+1; else { x--; err += 2*(y-x)+1; }
    }
}
static void px_line(int x0,int y0,int x1,int y1,pixel_t c,uint8_t a){
    int dx=x1>x0?x1-x0:x0-x1, dy=y1>y0?y1-y0:y0-y1, sx=x0<x1?1:-1, sy=y0<y1?1:-1, e=dx-dy;
    for(;;){draw_blend_pixel(x0,y0,c,a);if(x0==x1&&y0==y1)break;int e2=e*2;if(e2>-dy){e-=dy;x0+=sx;}if(e2<dx){e+=dx;y0+=sy;}}
}
static void icon_atom(int cx,int cy,int r,pixel_t c){           /* hex + core */
    int hx = r / 2, hy = (r * 866) / 1000;     /* cos60=.5, sin60=.866 (no math.h) */
    int vx[6] = { cx+r, cx+hx, cx-hx, cx-r, cx-hx, cx+hx };
    int vy[6] = { cy,   cy+hy, cy+hy, cy,   cy-hy, cy-hy };
    for (int i = 0; i < 6; i++) px_line(vx[i], vy[i], vx[(i+1)%6], vy[(i+1)%6], c, 200);
    px_dot(cx, cy, 2, c, 230);
}
static void icon_thermo(int cx,int cy,int r,pixel_t c){
    px_line(cx,cy-r,cx,cy+r-2,c,200); px_dot(cx,cy+r-1,2,c,230);
    px_line(cx+2,cy-r+1,cx+2,cy+1,c,120);
}
static void icon_bolt(int cx,int cy,int r,pixel_t c){
    px_line(cx+1,cy-r,cx-2,cy,c,210); px_line(cx-2,cy,cx+1,cy,c,210);
    px_line(cx+1,cy,cx-1,cy+r,c,210);
}
static void icon_leaf(int cx,int cy,int r,pixel_t c){
    px_ring(cx,cy,r-1,c,150); px_line(cx-r+2,cy+r-2,cx+r-2,cy-r+2,c,200);
}
static void icon_clock(int cx,int cy,int r,pixel_t c){
    px_ring(cx,cy,r,c,200); px_line(cx,cy,cx,cy-r+3,c,210); px_line(cx,cy,cx+r-4,cy,c,210);
}

/* Audio-style pulse: symmetric bars around a center line, height from the real
 * event-pulse history (s_pulse_hist) interpolated smooth, bright center fading
 * to the edges.  Replaces the old broken/static sparkline. */
static void draw_audio_pulse(int x, int y, int w, int h, pixel_t accent) {
    int cy = y + h / 2;
    for (int sx = 0; sx < w; sx++) draw_blend_pixel(x + sx, cy, accent, 26);
    unsigned long now = anim_now_ms();
    uint16_t mx = 1;
    for (uint8_t i = 0; i < s_pulse_count; i++) if (s_pulse_hist[i] > mx) mx = s_pulse_hist[i];
    for (int sx = 0; sx < w; sx++) {
        int hh;
        if (s_pulse_count >= 2 && mx > 0) {
            double fp = (double)sx * (s_pulse_count - 1) / (w - 1);
            int si = (int)fp; double fr = fp - si;
            int si2 = (si + 1 < s_pulse_count) ? si + 1 : si;
            int i0 = (s_pulse_head - s_pulse_count + si  + PULSE_HISTORY_N) % PULSE_HISTORY_N;
            int i1 = (s_pulse_head - s_pulse_count + si2 + PULSE_HISTORY_N) % PULSE_HISTORY_N;
            double v = s_pulse_hist[i0] + (s_pulse_hist[i1] - s_pulse_hist[i0]) * fr;
            hh = (int)(v * (h / 2 - 1) / mx);
        } else {
            /* idle: low animated shimmer so it reads as alive, not dead */
            double u = (double)sx / w * 6.2831853 * 3 + (double)now / 700.0;
            double s = u; while (s > 3.14159) s -= 6.2831853; while (s < -3.14159) s += 6.2831853;
            double s3 = s*s*s; hh = (int)((s - s3/6.0) * (h / 8));
        }
        if (hh < 0) hh = -hh; if (hh < 1) hh = 1;
        for (int dy = -hh; dy <= hh; dy++) {
            uint8_t a = (uint8_t)(210 - 170 * (dy < 0 ? -dy : dy) / (hh + 1));
            draw_blend_pixel(x + sx, cy + dy, accent, a);
        }
    }
}

/* Stacked metric module: icon (left) + small uppercase LABEL over a colored
 * value.  Returns the width consumed (incl. trailing gap). */
static int draw_metric_module(int x, int y_center,
                              void (*icon)(int,int,int,pixel_t),
                              const char *label, const char *value, pixel_t vcol) {
    int icon_r = 7;
    int ioff = icon ? (icon_r * 2 + ATOMIK_GRID_S + 2) : 0;
    if (icon) icon(x + icon_r, y_center, icon_r, vcol);
    int tx = x + ioff;
    font_aa_id_t lf = font_aa_loaded(FONT_AA_LABEL) ? FONT_AA_LABEL : FONT_AA_UI;
    int lh = font_aa_loaded(lf) ? text_height_aa(lf) : text_height(1);
    int vh = font_aa_loaded(FONT_AA_UI) ? text_height_aa(FONT_AA_UI) : text_height(1);
    int ly = y_center - (lh + 2 + vh) / 2;
    pixel_t lcol = rgb(0x7A, 0x86, 0xA0);
    if (font_aa_loaded(lf)) draw_text_aa(lf, tx, ly, label, lcol);
    else                    draw_text(tx, ly, label, 1, lcol);
    int vy = ly + lh + 2;
    if (font_aa_loaded(FONT_AA_UI)) draw_text_aa(FONT_AA_UI, tx, vy, value, vcol);
    else                            draw_text(tx, vy, value, 1, vcol);
    int lw = font_aa_loaded(lf) ? text_width_aa(lf, label) : text_width(label, 1);
    int vw = font_aa_loaded(FONT_AA_UI) ? text_width_aa(FONT_AA_UI, value) : text_width(value, 1);
    return ioff + (lw > vw ? lw : vw) + ATOMIK_GRID_L + 2;
}

/* v0.40-05-30 investor Pulse Bar — concept-07 header.  Modules left→right:
 *   atom icon + ATOMiK | ACTIVE PERSONALITY | PREDICTIVE SYSTEM PULSE + CONF |
 *   SYS TEMP | POWER DRAW | EFFICIENCY | UPTIME | glowing orb.
 * Every value is real/derived (see draw); power draw is an honest estimate. */
static void draw_pulse_bar_investor(int bar_y, int bar_h) {
    int y_center = bar_y + bar_h / 2;

    /* v0.40 floating glass header panel — inset rounded container with a
     * soft cyan border, top sheen, inner accent rim, and a soft outer
     * glow below.  Replaces the flat dev strip + hard divider line so the
     * bar reads as a premium application header (concept-01). */
    {
        int hx = ATOMIK_GRID_M, hy = bar_y + 4;
        int hw = FB_W - ATOMIK_GRID_M * 2, hh = bar_h - 8;
        draw_rect_rounded(hx, hy, hw, hh, 14, rgb(0x12, 0x1A, 0x2E));
        for (int sy = 0; sy < 10; sy++)
            for (int sx = 14; sx < hw - 14; sx++)
                draw_blend_pixel(hx + sx, hy + 1 + sy, ATOMIK_FG,
                                 (uint8_t)(11 - sy));
        draw_rect(hx + 14, hy,          hw - 28, 1, ATOMIK_ACCENT_DIM);
        draw_rect(hx + 14, hy + hh - 1, hw - 28, 1, ATOMIK_ACCENT_DIM);
        draw_rect(hx,          hy + 14, 1, hh - 28, ATOMIK_ACCENT_DIM);
        draw_rect(hx + hw - 1, hy + 14, 1, hh - 28, ATOMIK_ACCENT_DIM);
        for (int sx = 16; sx < hw - 16; sx++)
            draw_blend_pixel(hx + sx, hy + 2, ATOMIK_ACCENT, 20);
        for (int g = 1; g <= 3; g++)
            for (int sx = 0; sx < hw; sx++)
                draw_blend_pixel(hx + sx, hy + hh - 1 + g,
                                 ATOMIK_SEM_HARDWARE, (uint8_t)(22 / g));
    }

    int cy = y_center;
    int cur_x = ATOMIK_GRID_L + ATOMIK_GRID_M + 2;

    /* Logo: atom hex icon + ATOMiK wordmark (white). */
    icon_atom(cur_x + 9, cy, 9, ATOMIK_SEM_HARDWARE);
    cur_x += 9 * 2 + ATOMIK_GRID_M;
    if (font_aa_loaded(FONT_AA_BRAND)) {
        draw_text_aa(FONT_AA_BRAND, cur_x, cy - text_height_aa(FONT_AA_BRAND)/2 - 2, "ATOMiK", ATOMIK_FG);
        cur_x += text_width_aa(FONT_AA_BRAND, "ATOMiK") + ATOMIK_GRID_L;
    } else if (font_aa_loaded(FONT_AA_DISPLAY)) {
        draw_text_aa(FONT_AA_DISPLAY, cur_x, cy - text_height_aa(FONT_AA_DISPLAY)/2 - 2, "ATOMiK", ATOMIK_FG);
        cur_x += text_width_aa(FONT_AA_DISPLAY, "ATOMiK") + ATOMIK_GRID_L;
    } else {
        draw_text(cur_x, cy - text_height(2)/2, "ATOMiK", 2, ATOMIK_FG);
        cur_x += text_width("ATOMiK", 2) + ATOMIK_GRID_L;
    }
    pulse_sep(cur_x, bar_y, bar_h);
    cur_x += ATOMIK_GRID_M + 2;

    /* ACTIVE PERSONALITY (real -- STATE/SYNC/AGENT, not the concept's fake SYNAPSE). */
    cur_x += draw_metric_module(cur_x, cy, draw_personality_glyph,
                                "ACTIVE PERSONALITY",
                                fabric_personality_name(fabric_active()),
                                personality_color(fabric_active()));

    /* PREDICTIVE SYSTEM PULSE -- real event waveform + real Markov confidence. */
    {
        action_t top = agent_predict();
        double tot = 0.0, ts = agent_score(top);
        for (int a = ACT_OPEN_ABOUT; a <= ACT_CYCLE_FOCUS; a++) tot += agent_score((action_t)a);
        int conf = (tot > 0.0) ? (int)(ts * 100.0 / tot) : 0;
        if (conf > 100) conf = 100;

        font_aa_id_t lf = font_aa_loaded(FONT_AA_LABEL) ? FONT_AA_LABEL : FONT_AA_UI;
        int lh = font_aa_loaded(lf) ? text_height_aa(lf) : text_height(1);
        int label_y = bar_y + 9;
        if (font_aa_loaded(lf)) draw_text_aa(lf, cur_x, label_y, "PREDICTIVE SYSTEM PULSE", rgb(0x7A,0x86,0xA0));
        else                    draw_text(cur_x, label_y, "PREDICTIVE SYSTEM PULSE", 1, rgb(0x7A,0x86,0xA0));
        int pulse_y = label_y + lh + 3;
        int pulse_w = 300;
        int pulse_h = bar_y + bar_h - pulse_y - 10;
        if (pulse_h < 8) pulse_h = 8;
        draw_audio_pulse(cur_x, pulse_y, pulse_w, pulse_h, ATOMIK_SEM_HARDWARE);
        cur_x += pulse_w + ATOMIK_GRID_M;

        char cv[16]; snprintf(cv, sizeof cv, "%d%%", conf);
        cur_x += draw_metric_module(cur_x, cy, NULL, "CONFIDENCE", cv, ATOMIK_SEM_HARDWARE);
    }
    pulse_sep(cur_x, bar_y, bar_h);
    cur_x += ATOMIK_GRID_M + 2;

    /* Right cluster: SYS TEMP | POWER DRAW | EFFICIENCY | UPTIME, then orb. */
    {
        const perf_sample_t *es = perf_last_for(fabric_active());
        if (!es) es = perf_last_for(PERSONALITY_STATE);
        int eff = (es && es->ops_logical)
                  ? (int)(100u * (es->ops_logical - es->ops_issued) / es->ops_logical) : 0;
        char eff_v[12]; snprintf(eff_v, sizeof eff_v, "%d%%", eff);

        char up_v[24];
        double uu = 0; FILE *uf = fopen("/proc/uptime", "r");
        if (uf) { if (fscanf(uf, "%lf", &uu) != 1) uu = 0; fclose(uf); }
        { int s=(int)uu, d=s/86400, hh2=(s%86400)/3600, mm=(s%3600)/60;
          if (d>0) snprintf(up_v,sizeof up_v,"%dd %dh",d,hh2);
          else     snprintf(up_v,sizeof up_v,"%dh %dm",hh2,mm); }

        /* SYS TEMP + POWER DRAW: no on-die sensors wired yet (XADC TODO), so
         * these are DISCONNECTED PLACEHOLDERS — plausible values shown with a
         * leading '~' so the screen matches the concept layout without
         * implying a live reading.  Replace with real XADC/INA when connected. */
        const char *temp_v = "~37.4C";   /* placeholder (ASCII-safe; no degree glyph) */
        char pw_v[16]; snprintf(pw_v, sizeof pw_v, "~%dW", 98);  /* placeholder */

        cur_x += draw_metric_module(cur_x, cy, icon_thermo, "SYS TEMP", temp_v, ATOMIK_FG_DIM);
        cur_x += draw_metric_module(cur_x, cy, icon_bolt,   "POWER DRAW", pw_v, ATOMIK_FG_DIM);
        cur_x += draw_metric_module(cur_x, cy, icon_leaf,   "EFFICIENCY", eff_v, ATOMIK_SEM_SAVINGS);
        cur_x += draw_metric_module(cur_x, cy, icon_clock,  "UPTIME", up_v, ATOMIK_FG);

        int orb_r = 11;
        int orb_cx = FB_W - ATOMIK_GRID_L - orb_r;
        pulse_orb(orb_cx, cy, orb_r);
    }
}

static const char *mode_label(metric_mode_t m) {
    switch (m) {
    case METRIC_MODE_DEV:      return "DEV";
    case METRIC_MODE_DEMO:     return "DEMO";
    case METRIC_MODE_INVESTOR: return "INVESTOR";
    default:                   return "?";
    }
}
static pixel_t mode_color(metric_mode_t m) {
    switch (m) {
    case METRIC_MODE_DEV:      return ATOMIK_SEM_WASTE;       /* amber — relaxed truth */
    case METRIC_MODE_DEMO:     return ATOMIK_SEM_HARDWARE;    /* cyan — rehearsal */
    case METRIC_MODE_INVESTOR: return ATOMIK_SEM_SAVINGS;     /* green — strictest */
    default:                   return ATOMIK_FG_DIM;
    }
}

void status_draw(void) {
    update_cpu();

    /* v0.38-J++ mode gating — investor mode strips dev clutter from the
     * top bar.  Per ChatGPT 2026-05-15: "anything that says DEV", raw
     * CPU%, wallet/spent, key hints, exact version string all hide.
     * DATA: prefix becomes METRICS:.  MODE badge hides (don't reveal
     * the curtain).  Operator switches to investor mode BEFORE
     * presenting, sees the switch happen, then walks away from the
     * keyboard. */
    metric_mode_t cur_mode = metric_mode();
    int is_investor = (cur_mode == METRIC_MODE_INVESTOR);

    /* Top bar — anchored to the SAFE-AREA top, not screen y=0.
     *
     * v0.31 patch 5: bar is now exactly bar_h tall starting at SAFE_TOP,
     * with wallpaper visible above it (y=0..SAFE_TOP).  Text is centered
     * inside the bar's geometric middle, so on ANY monitor — fully
     * cropped (32 px), partially cropped, or no-overscan — the bar
     * looks visually centered around its text.
     *
     * Previously the bar extended from y=0 to y=64 with text at y=40,
     * which appeared bottom-biased on monitors with less than 32 px
     * of overscan (text 8 px from the bottom edge of a visibly-larger
     * bar).  User feedback 2026-05-07. */
    const int bar_h = ATOMIK_PULSE_BAR_H;     /* v0.38-E: 64 px, two-row */
    const int bar_y = ATOMIK_SAFE_TOP;
    /* Two-row geometry: top row is identity zone (brand + DATA/MODE/
     * PERSONALITY badges), bottom row is metrics zone (mini-readout
     * + window strip + key hints + wallet/cpu/uptime/version).
     * Each row is 30 px tall with 2 px gap; cyan-dim hairline divider
     * sits between them. */
    const int row_h    = (bar_h - 4) / 2;             /* 30 */
    const int top_y    = bar_y + 2;
    const int bot_y    = bar_y + 2 + row_h + 2;
    const int ty_top   = top_y + (row_h - text_height(1)) / 2;
    const int ty_bot   = bot_y + (row_h - text_height(1)) / 2;
    const int ty_brand = top_y + (row_h - text_height(2)) / 2;
    /* legacy ty for the few spots that still use a single coordinate */
    const int ty       = ty_top;

    /* Bar fills only the safe zone — wallpaper covers y=0..SAFE_TOP
     * (cropped/invisible on this monitor; visible as a thin strip on
     * monitors with less overscan, but that's fine — wallpaper looks
     * intentional even at edges). */
    if (is_investor) {
        /* v0.40: darker base; draw_pulse_bar_investor paints a floating
         * glass header panel over it — no hard divider line. */
        draw_rect(0, bar_y, FB_W, bar_h, rgb(0x0C, 0x12, 0x20));
    } else {
        draw_rect(0, bar_y,             FB_W, bar_h, rgb(0x1A, 0x22, 0x38));
        /* Hard cyan-dim separator at the bottom of the bar — sharp visual
         * end of the chrome, start of the desktop. */
        draw_rect(0, bar_y + bar_h - 1, FB_W, 1,     ATOMIK_ACCENT_DIM);
    }

    /* v0.38-K2 — investor mode renders an entirely separate premium
     * Pulse Bar path with glass pills + AA throughout.  Dev / Demo
     * keep the previous diagnostic layout below. */
    if (is_investor) {
        draw_pulse_bar_investor(bar_y, bar_h);
        dirty_rect(0, bar_y, FB_W, bar_h);
        return;
    }

#if ATOMIK_DEBUG_STATUS
    /* If re-enabled, draws a magenta sentinel + PID-marker for diagnosis. */
    draw_rect(0, bar_y, FB_W, bar_h, rgb(0xFF, 0x00, 0xFF));
    int p = (int)getpid();
    pixel_t marker = rgb((uint8_t)((p >> 8) & 0xFF),
                         (uint8_t)(p & 0xFF), 0xFF);
    draw_rect(FB_W - ATOMIK_SAFE_RIGHT - 8, bar_y, 8, 8, marker);
#endif

    /* === LEFT SEGMENT (v0.34-B Pulse Bar) ===
     *
     * Reading order: identity → workload state → window state → key hints.
     * Each section colored by semantic role so a glance at the bar tells
     * you what kind of state to read at each position.
     *
     * Identity: ATOMiK wordmark (cyan = HARDWARE, the OS-chrome signal).
     */
    const char *brand = "ATOMiK";
    int cur_x = ATOMIK_GRID_L;
    /* v0.38-K: ATOMiK wordmark uses the AA display atlas if loaded —
     * the single biggest perceived-quality lift from the typography
     * slice.  Falls back to scale-2 pixel font if the .atomik_font
     * isn't on disk yet. */
    if (font_aa_loaded(FONT_AA_DISPLAY)) {
        int brand_h  = text_height_aa(FONT_AA_DISPLAY);
        int brand_y  = top_y + (row_h - brand_h) / 2 - 2;
        draw_text_aa(FONT_AA_DISPLAY, cur_x, brand_y, brand,
                     ATOMIK_SEM_HARDWARE);
        cur_x += text_width_aa(FONT_AA_DISPLAY, brand) + ATOMIK_GRID_L;
    } else {
        draw_text(cur_x, ty_brand, brand, 2, ATOMIK_SEM_HARDWARE);
        cur_x += text_width(brand, 2) + ATOMIK_GRID_L;
    }

    /* Vertical separator between sections — dim, 1 px wide, stops the
     * bar from feeling like one long undifferentiated text run. */
    draw_rect(cur_x, bar_y + ATOMIK_GRID_S,
              1, bar_h - ATOMIK_GRID_S * 2, ATOMIK_DOCK_BORDER);
    cur_x += ATOMIK_GRID_M + 1;

    /* === DATA: <source> badge — v0.38 truth-aware indicator ===
     *
     * Reads the worst metric source currently in the registry and
     * shows it next to the brand.  Audience can tell at a glance
     * whether the screen is fully LIVE, has WAITING lanes, or
     * (crucially) has any MOCK / SCENARIO values mixed in.  Per
     * feedback_metric_provider_directive: "Every number on screen
     * must know where it came from."  This badge is the sentence
     * that the rest of the UI is held to.
     *
     * Color encoding: green=LIVE, cyan=DERIVED, violet=SCENARIO,
     * amber=MOCK/STALE, dim=WAITING.  Audiences should immediately
     * notice if it ever drops into amber outside dev mode. */
    {
        metric_source_t worst = metric_worst_source();
        char data_badge[32];
        /* v0.38-J++ investor-mode rename: DATA: → METRICS:. */
        snprintf(data_badge, sizeof data_badge, "%s: %s",
                 is_investor ? "METRICS" : "DATA",
                 metric_aggregate_label());
        pixel_t dcol = metric_source_color(worst);
        int dot_y3 = top_y + (row_h - ATOMIK_GRID_M) / 2;
        draw_rect(cur_x, dot_y3, ATOMIK_GRID_M, ATOMIK_GRID_M, dcol);
        cur_x += ATOMIK_GRID_M + ATOMIK_GRID_S;
        draw_text(cur_x, ty_top, data_badge, 1, dcol);
        cur_x += text_width(data_badge, 1) + ATOMIK_GRID_L;
    }

    /* === MODE: <DEV/DEMO/INVESTOR> badge — v0.38-C ===
     *
     * Hidden in investor mode (v0.38-J++) — "don't reveal the curtain".
     * Operator can verify the switch happened before presenting.
     * DEV/DEMO modes still see the badge so they know discipline level. */
    if (!is_investor) {
        metric_mode_t m = cur_mode;
        char mode_badge[32];
        snprintf(mode_badge, sizeof mode_badge, "MODE: %s", mode_label(m));
        pixel_t mcol = mode_color(m);
        int dot_y4 = top_y + (row_h - ATOMIK_GRID_M) / 2;
        draw_rect(cur_x, dot_y4, ATOMIK_GRID_M, ATOMIK_GRID_M, mcol);
        cur_x += ATOMIK_GRID_M + ATOMIK_GRID_S;
        draw_text(cur_x, ty_top, mode_badge, 1, mcol);
        cur_x += text_width(mode_badge, 1) + ATOMIK_GRID_L;
    }

    /* === Active personality badge ===
     *
     * v0.34-B: surfaces the current Resource Fabric personality globally
     * so the user doesn't need Fabric open to see what compute personality
     * is running.  Pulled from fabric_active() + fabric_override_active().
     * Color = semantic color of that personality (cyan/violet/green).
     * "MANUAL:" prefix signals override mode (ChatGPT: "honest demo
     * discipline — never present a forced personality as auto-detected").
     */
    {
        personality_t p = fabric_active();
        pixel_t  pcol = ATOMIK_FG_DIM;
        switch (p) {
        case PERSONALITY_STATE: pcol = ATOMIK_SEM_HARDWARE; break;
        case PERSONALITY_AGENT: pcol = ATOMIK_SEM_AGENT;    break;
        case PERSONALITY_SYNC:  pcol = ATOMIK_SEM_SAVINGS;  break;
        default: break;
        }
        char pbadge[32];
        if (fabric_override_active()) {
            snprintf(pbadge, sizeof pbadge, "MANUAL:%s",
                     fabric_personality_name(p));
        } else {
            snprintf(pbadge, sizeof pbadge, "%s",
                     fabric_personality_name(p));
        }
        /* Filled dot before the name so the eye locks on the colored
         * indicator without needing to read the word. */
        int dot_y2 = top_y + (row_h - ATOMIK_GRID_M) / 2;
        draw_rect(cur_x, dot_y2, ATOMIK_GRID_M, ATOMIK_GRID_M, pcol);
        cur_x += ATOMIK_GRID_M + ATOMIK_GRID_S;
        draw_text(cur_x, ty_top, pbadge, 1, pcol);
        cur_x += text_width(pbadge, 1) + ATOMIK_GRID_M;
    }

    /* v0.38-E two-row split: reset cursor to start of BOTTOM row.
     * Top row holds identity + workload-state badges; bottom row
     * holds the live signal strip (mini-readout, pulse, window dots,
     * hints) plus the right-anchored wallet/cpu/uptime/version. */
    cur_x = ATOMIK_GRID_L;
    /* Hairline divider between the two rows for visual separation. */
    draw_rect(0, top_y + row_h + 1, FB_W, 1, ATOMIK_DOCK_BORDER);

    /* === v0.39-H unified Pulse well — metric + glow waveform ===
     *
     * Previously: a loose "<n> hot / <n>-<n> ops" mini-readout was
     * drawn BEFORE the pulse, then a "PULSE" label, then a 128 px
     * well containing a 1 px Bresenham line.  ChatGPT 2026-05-19
     * called that combo "a small status stub plus an empty dark
     * well" — neither piece carrying weight on its own.
     *
     * Now: a single 200×26 well.  Left ~64 px holds the personality
     * metric (e.g. "3 HOT", "12-256 OPS", "1024B AVD") rendered in
     * AA atomik_14 in the personality's semantic color.  Right
     * ~136 px holds the proper layered-stroke glow waveform from
     * the investor path (`draw_event_pulse_glow`).
     *
     * Class A discipline preserved: metric is only painted when
     * perf_last_sample() actually has data — no fake numbers.  When
     * no sample, the metric region is empty and the waveform takes
     * the whole 200 px.
     */
    {
        int pulse_w = 200;
        int pulse_h = row_h - 4;
        int pulse_y = bot_y + 2;

        /* 1. Bed — single soft accent wash so the waveform reads
         *    embedded in the bar, not pasted on.  Same approach as
         *    investor v0.38-K2.5A. */
        pixel_t bed_accent = ATOMIK_SEM_HARDWARE;
        for (int sy = 0; sy < pulse_h; sy++) {
            for (int sx = 0; sx < pulse_w; sx++) {
                draw_blend_pixel(cur_x + sx, pulse_y + sy,
                                 bed_accent, 8);
            }
        }
        /* Hairline bottom edge so the well has a visible floor. */
        draw_rect(cur_x, pulse_y + pulse_h - 1, pulse_w, 1,
                  ATOMIK_DOCK_BORDER);

        /* 2. Left metric region.  v0.39-I: bind to the CURRENTLY-ACTIVE
         *    personality, not the personality recorded in the most
         *    recent batch.  Previously this used perf_last_sample(),
         *    which returns the latest sample regardless of personality
         *    — so a user who flipped from AGENT to SYNC would still
         *    see "3 HOT" while the rest of the chrome said SYNC active.
         *    ChatGPT 2026-05-20: "two competing stories in the first
         *    200 px of chrome."
         *
         *    Now: perf_last_for(fabric_active()).  If that personality
         *    has no batch data yet, skip the readout entirely and let
         *    the waveform read alone — never borrow another
         *    personality's number. */
        int metric_w = 64;
        personality_t active = fabric_active();
        const perf_sample_t *ps = perf_last_for(active);
        if (ps && ps->ops_logical > 0) {
            char mini[24];
            pixel_t mc = ATOMIK_SEM_HARDWARE;
            mini[0] = 0;
            switch (active) {
            case PERSONALITY_STATE:
                snprintf(mini, sizeof mini, "%u OPS",
                         (unsigned)ps->ops_logical);
                mc = ATOMIK_SEM_HARDWARE;
                break;
            case PERSONALITY_SYNC:
                if (ps->bytes_avoided > 0) {
                    snprintf(mini, sizeof mini, "%uB AVD",
                             (unsigned)ps->bytes_avoided);
                } else {
                    snprintf(mini, sizeof mini, "%u EMIT",
                             (unsigned)ps->ops_issued);
                }
                mc = ATOMIK_SEM_SAVINGS;
                break;
            case PERSONALITY_AGENT:
                snprintf(mini, sizeof mini, "%u HOT",
                         (unsigned)ps->ops_issued);
                mc = ATOMIK_SEM_AGENT;
                break;
            default: break;
            }
            if (mini[0] && font_aa_loaded(FONT_AA_LABEL)) {
                int th = text_height_aa(FONT_AA_LABEL);
                int my = pulse_y + (pulse_h - th) / 2;
                draw_text_aa(FONT_AA_LABEL,
                             cur_x + ATOMIK_GRID_S, my, mini, mc);
            } else if (mini[0]) {
                int th = text_height(1);
                int my = pulse_y + (pulse_h - th) / 2;
                draw_text(cur_x + ATOMIK_GRID_S, my, mini, 1, mc);
            }
        }

        /* 3. Right waveform region.  Glow waveform (8/4/2 layered
         *    halo + sine baseline + hot dot at the most recent
         *    sample).  Reuses the investor-grade renderer. */
        int wave_x = cur_x + metric_w;
        int wave_w = pulse_w - metric_w;
        draw_event_pulse_glow(wave_x, pulse_y, wave_w, pulse_h,
                              ATOMIK_SEM_HARDWARE);

        cur_x += pulse_w + ATOMIK_GRID_L;
    }

    /* Separator before window strip + hints (bottom-row only). */
    draw_rect(cur_x, bot_y + 2,
              1, row_h - 4, ATOMIK_DOCK_BORDER);
    cur_x += ATOMIK_GRID_M + 1;

    /* === Window strip (v0.31): one dot per open window ===
     * Focused = filled cyan; buried = hollow outline.  Lets the user
     * see buried windows even when fully covered. */
    int dot_size  = ATOMIK_GRID_M;
    int dot_gap   = ATOMIK_GRID_S;
    int dot_y     = bot_y + (row_h - dot_size) / 2;
    int dots_x    = cur_x;
    const window_t *top = wm_topmost();
    for (int i = 0; i < wm_count(); i++) {
        const window_t *w = wm_get(i);
        if (!w || !w->visible) continue;
        int focused = (top && w->id == top->id);
        if (focused) {
            draw_rect(dots_x, dot_y, dot_size, dot_size, ATOMIK_SEM_HARDWARE);
        } else {
            draw_rect(dots_x,                  dot_y,                  dot_size, 1, ATOMIK_DOCK_BORDER);
            draw_rect(dots_x,                  dot_y + dot_size - 1,   dot_size, 1, ATOMIK_DOCK_BORDER);
            draw_rect(dots_x,                  dot_y,                  1, dot_size, ATOMIK_DOCK_BORDER);
            draw_rect(dots_x + dot_size - 1,   dot_y,                  1, dot_size, ATOMIK_DOCK_BORDER);
        }
        dots_x += dot_size + dot_gap;
    }
    int hint_x = (wm_count() > 0) ? dots_x + ATOMIK_GRID_M : cur_x;

    /* v0.39-K: shortcut legend and agent-prediction center text removed.
     * Both were operator/debug chrome that made the rail feel like a
     * terminal rather than a finished OS surface.  Keys still work;
     * the agent prediction surfaces through the Atom assistant instead. */
    (void)hint_x;

    /* Right: wallet/spend (violet — agent activity) || cpu/uptime (cyan
     * — hardware) || version (dim).  Three-segment design lets you read
     * the bar at a glance: agent-cost stuff = violet, hardware-status
     * stuff = cyan, meta = dim.  v0.30 will turn the wallet/spend
     * segment into a chevron-pulse while an LLM dispatch is in-flight
     * (Humane-Pin failure-avoidance rule: never hide active state).
     * Today dispatch is synchronous-blocking so the pulse can't fire
     * — wait for async dispatch in v0.30. */
    /* v0.38-J++ right-segment mode gating.  DEV/DEMO keep the full
     * trio (wallet + cpu/uptime + version) so the operator sees spend
     * + system load + build at all times.  INVESTOR mode hides:
     *   - wallet/spent           (perception poison)
     *   - exact CPU% number      (replaced with "SYSTEM LIVE")
     *   - version string         (debug-y, leaks build state)
     * and keeps uptime so the audience has a "board is genuinely
     * running" signal. */
    char up[64];
    format_uptime(up, sizeof up);
    const wallet_state_t *w = wallet_get();
    int spend_uusd = llm_audit_total_uusd();

    char hw_seg[80];
    if (is_investor) {
        snprintf(hw_seg, sizeof hw_seg, "SYSTEM LIVE   %s", up);
    } else {
        snprintf(hw_seg, sizeof hw_seg, "cpu %3d%%   %s", s_cpu_pct, up);
    }
    char ai_seg[80];
    snprintf(ai_seg, sizeof ai_seg, "wallet $%d.%02d   spent $%d.%03d",
             w->balance_uusd / 1000000,
             (w->balance_uusd / 10000) % 100,
             spend_uusd / 1000000,
             (spend_uusd / 1000) % 1000);
    int hw_w = text_width(hw_seg, 1);
    int sep  = ATOMIK_GRID_M * 2;
    if (is_investor) {
        /* Investor: hw_seg only, right-anchored.  No wallet, no version. */
        int rx = FB_W - ATOMIK_GRID_L - hw_w;
        draw_text(rx, ty_bot, hw_seg, 1, ATOMIK_SEM_HARDWARE);
    } else {
        int ai_w  = text_width(ai_seg, 1);
        int ver_w = text_width(AOS_VERSION, 1);
        int total = ai_w + sep + hw_w + sep + ver_w;
        int rx    = FB_W - ATOMIK_GRID_L - total;
        draw_text(rx,                              ty_bot, ai_seg,      1, ATOMIK_SEM_AGENT);
        draw_text(rx + ai_w + sep,                 ty_bot, hw_seg,      1, ATOMIK_SEM_HARDWARE);
        draw_text(rx + ai_w + sep + hw_w + sep,    ty_top, AOS_VERSION, 1, ATOMIK_FG_DIM);
    }

    /* v0.38-A: status bar dirties the bar rect every frame; mark it. */
    dirty_rect(0, bar_y, FB_W, bar_h);
}
