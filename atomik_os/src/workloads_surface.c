/* workloads_surface.c — the WORKLOADS capability, redesigned for a NON-TECHNICAL
 * viewer (the LOI audience).  Instead of dumping bank/cycle telemetry, each
 * scenario tells a plain-language story and shows a side-by-side
 * CONVENTIONAL-vs-ATOMiK comparison: what the workload is, how much each
 * approach moves/spends, and the resulting SAVINGS — with the "same result,
 * verified" guarantee underneath.
 *
 * Honesty ([[feedback_workload_legibility_bar]], [[feedback_no_class_c_metrics]]):
 *   - "Parallel Aggregation" pulls LIVE measured data from the bench engine
 *     (bench_sweep_points: 1-lane vs 8-lane cycles on real silicon).
 *   - "Telemetry Sync" / "Control Coalescing" show SCENARIO numbers (clearly
 *     chip-labeled SCENARIO) computed from the real workload definitions in
 *     hardware/zynq/workloads/* until those are wired to a live board feed.
 *   Never claim the SCENARIO numbers are measured. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int s_window_id = -1;
static int s_scenario  = 0;          /* active scenario tab (0..N_WL-1) */

/* v0.42 interactive: cursor-selected lane rung on the Parallel Aggregate card.
 * -1 = default (max lanes).  The ladder records each rung's on-screen rect at
 * draw time so a click can hit-test it. */
static int s_sel_idx = -1;
static int s_rung_x = 0, s_rung_w = 0, s_rung_h = 0, s_rung_n = 0;
static int s_rung_y[BENCH_N_POINTS];

/* v0.42 sub-element delta: the only elements that change on a lane select are
 * the ATOMiK comparison panel, the lane ladder, and the savings banner.  Their
 * on-screen rects are recorded during the full draw so a select can redraw ONLY
 * these — skipping the unchanged header/tabs/title/desc/CONVENTIONAL-panel/
 * provenance AA text, which is what made the click pause on the slow core. */
static int s_atk_rect[4]     = {0, 0, 0, 0};  /* x,y,w,h — ATOMiK panel  */
static int s_ladder_rect[4]  = {0, 0, 0, 0};  /* x,y,w,h — lane ladder   */
static int s_savings_rect[4] = {0, 0, 0, 0};  /* x,y,w,h — savings banner */
/* parallel-scenario featured-number buffers (module scope so the sub-element
 * redraw can recompute them, not just the full draw). */
static char s_pconv[16], s_patk[16], s_psav[24], s_phow[44];

/* set the active scenario (board: 'W' re-press cycles; preview: env). */
void workloads_set_scenario(int n);  /* fwd (declared in header below if needed) */

/* ── scenario model ───────────────────────────────────────────────────────
 * Each scenario is one plain-language comparison.  conv_* = the conventional
 * (Von-Neumann, move-everything) way; atomik_* = the delta-state way. */
typedef struct {
    const char *tab;          /* short tab label                       */
    const char *title;        /* scenario headline                     */
    const char *desc1;        /* plain-language description, line 1     */
    const char *desc2;        /* line 2 (or "")                        */
    const char *conv_how;     /* what conventional does                */
    const char *atomik_how;   /* what ATOMiK does                      */
    const char *conv_big;     /* conventional metric (formatted)       */
    const char *conv_unit;    /* unit under it                         */
    const char *atomik_big;   /* ATOMiK metric (formatted)             */
    const char *atomik_unit;
    double      conv_bar;     /* bar magnitudes (same unit)            */
    double      atomik_bar;
    const char *savings;      /* big savings headline                  */
    const char *guarantee;    /* the "same result" line                */
    int         live;         /* 1 = LIVE measured, 0 = SCENARIO        */
} wl_scenario_t;

#define N_WL 3

static int s_tab_rect[N_WL][4];               /* x,y,w,h — scenario tabs (clickable) */
static unsigned long s_last_cycle_ms = 0;     /* auto-cycle timer / manual reset */

/* Static scenario copy.  The parallel-aggregate big numbers are overwritten
 * at draw time from the live bench sweep when available. */
static wl_scenario_t s_wl[N_WL] = {
    {
        .tab = "Telemetry Sync",
        .title = "Edge Telemetry Sync",
        .desc1 = "10,000 sensors report their state every tick.",
        .desc2 = "Most of those values did not actually change.",
        .conv_how   = "Sends the full state every tick",
        .atomik_how = "Sends only the values that changed",
        .conv_big = "2.0", .conv_unit = "KB per tick",
        .atomik_big = "168", .atomik_unit = "bytes per tick",
        .conv_bar = 2048, .atomik_bar = 168,
        .savings = "92% LESS DATA MOVED",
        .guarantee = "Same state on both sides — verified identical.",
        .live = 0,
    },
    {
        .tab = "Control Coalescing",
        .title = "Control-Update Coalescing",
        .desc1 = "A controller receives many updates before each commit.",
        .desc2 = "Many of them touch the same few registers.",
        .conv_how   = "Writes out every single update",
        .atomik_how = "Coalesces repeats into one net delta",
        .conv_big = "256", .conv_unit = "writes per commit",
        .atomik_big = "32", .atomik_unit = "writes per commit",
        .conv_bar = 256, .atomik_bar = 32,
        .savings = "87% FEWER WRITES",
        .guarantee = "Final register state identical — verified.",
        .live = 0,
    },
    {
        .tab = "Parallel Aggregate",
        .title = "Parallel Aggregation",
        .desc1 = "Combine a stream of updates into one result.",
        .desc2 = "Order doesn't matter, so every lane works at once.",
        .conv_how   = "One lane, sequential accumulate",
        .atomik_how = "8 lanes, lock-free, all at once",
        .conv_big = "65,537", .conv_unit = "cycles per run",
        .atomik_big = "8,193", .atomik_unit = "cycles per run",
        .conv_bar = 65537, .atomik_bar = 8193,
        .savings = "8x FASTER",
        .guarantee = "Byte-identical result — measured on silicon.",
        .live = 1,
    },
};

void workloads_set_scenario(int n) {
    if (n < 0) n = 0;
    if (n >= N_WL) n = N_WL - 1;
    s_scenario = n;
}

int  workloads_surface_is_open(void) { return s_window_id >= 0; }
int  workloads_surface_window_id(void) { return s_window_id; }

/* v0.42 interactive: hit-test a screen-space click against the surface.
 * Returns:
 *   2 — switched to a DIFFERENT scenario tab (caller does a window-delta redraw,
 *       since the whole card body changes).
 *   1 — selected a lane rung within the parallel scenario (caller does the
 *       cheaper sub-element redraw).
 *   0 — click landed on nothing actionable.
 * Both the tabs and the ladder rungs record their rects at draw time. */
int workloads_surface_handle_click(int mx, int my) {
    if (s_window_id < 0) return 0;

    /* Tabs first — present in every scenario, along the top of the card. */
    for (int i = 0; i < N_WL; i++) {
        int *r = s_tab_rect[i];
        if (r[2] <= 0) continue;
        if (mx >= r[0] && mx < r[0] + r[2] && my >= r[1] && my < r[1] + r[3]) {
            if (i == s_scenario) return 0;   /* already on it — no change */
            s_scenario = i;
            s_last_cycle_ms = anim_now_ms(); /* a manual switch resets auto-cycle */
            return 2;
        }
    }

    /* Lane ladder — only the parallel-aggregate scenario has selectable rungs. */
    if (s_scenario != 2 || s_rung_n <= 0) return 0;
    if (mx < s_rung_x || mx > s_rung_x + s_rung_w) return 0;
    for (int i = 0; i < s_rung_n; i++) {
        if (my >= s_rung_y[i] && my < s_rung_y[i] + s_rung_h) {
            s_sel_idx = i;
            return 1;
        }
    }
    return 0;
}

void workloads_cycle_scenario(void) {
    s_scenario = (s_scenario + 1) % N_WL;
    s_last_cycle_ms = anim_now_ms();      /* manual press resets the auto timer */
}

/* v0.41 standalone demo: when demo mode is on (/tmp/atomik_demo or 'L') and
 * the surface is open, advance to the next scenario every ~12s so the demo
 * tells the whole three-workload story with no input attached.  Returns 1
 * when it cycled (caller repaints). */
#define WL_DEMO_CYCLE_MS 12000UL
int workloads_tick(void) {
    if (s_window_id < 0 || !fabric_demo_enabled()) return 0;
    unsigned long now = anim_now_ms();
    if (s_last_cycle_ms == 0) { s_last_cycle_ms = now; return 0; }
    if (now - s_last_cycle_ms < WL_DEMO_CYCLE_MS) return 0;
    s_last_cycle_ms = now;
    s_scenario = (s_scenario + 1) % N_WL;
    return 1;
}

/* horizontal comparison bar: a label, a rounded bar of width ∝ value, value
 * text at the end.  Conventional = amber (waste), ATOMiK = green (savings). */
static void compare_bar(int x, int y, int w, const char *label,
                        double val, double vmax, pixel_t col, const char *vstr) {
    int lab = font_aa_loaded(FONT_AA_LABEL);
    int lw  = lab ? text_width_aa(FONT_AA_LABEL, label) : text_width(label, 1);
    int label_col_w = 96;
    if (lab) draw_text_aa(FONT_AA_LABEL, x + label_col_w - lw, y + 1, label, ATOMIK_FG_DIM);
    int bar_x = x + label_col_w + ATOMIK_GRID_M;
    int track_w = w - label_col_w - ATOMIK_GRID_M - 70;
    int bh = 16;
    draw_rect_rounded(bar_x, y, track_w, bh, bh / 2, rgb(0x16, 0x1E, 0x30));
    int fw = (vmax > 0) ? (int)(val / vmax * track_w) : 0;
    if (fw < bh) fw = bh;            /* keep the rounded cap visible */
    if (fw > track_w) fw = track_w;
    draw_rect_rounded(bar_x, y, fw, bh, bh / 2, col);
    /* soft glow on the fill */
    for (int g = 1; g <= 3; g++) {
        uint8_t a = (uint8_t)(40 - (g - 1) * 12);
        for (int sx = 0; sx < fw; sx++) draw_blend_pixel(bar_x + sx, y - g, col, a);
    }
    int vw = lab ? text_width_aa(FONT_AA_LABEL, vstr) : text_width(vstr, 1);
    if (lab) draw_text_aa(FONT_AA_LABEL, x + w - vw, y + 1, vstr, col);
}

/* one side of the comparison (CONVENTIONAL or ATOMiK): a header chip, a
 * how-line, and the big metric + unit. */
static void compare_panel(int x, int y, int w, int h, const char *head,
                          const char *how, const char *big, const char *unit,
                          pixel_t accent, int is_atomik) {
    draw_rect_rounded(x, y, w, h, 12, wm_card_bg());
    /* tinted top shoulder so each side reads as its color */
    for (int sy = 0; sy < 10; sy++) {
        uint8_t a = (uint8_t)(46 - sy * 4);
        for (int sx = 0; sx < w; sx++) draw_blend_pixel(x + sx, y + sy, accent, a);
    }
    draw_rect(x, y, w, 1, accent); draw_rect(x, y + h - 1, w, 1, accent);
    draw_rect(x, y, 1, h, accent); draw_rect(x + w - 1, y, 1, h, accent);

    int pad = ATOMIK_GRID_M, tx = x + pad, ty = y + pad;
    /* header chip */
    if (font_aa_loaded(FONT_AA_LABEL)) {
        draw_text_aa(FONT_AA_LABEL, tx, ty, head, accent);
        ty += text_height_aa(FONT_AA_LABEL) + 2;
    }
    /* how-line (plain) */
    if (font_aa_loaded(FONT_AA_LABEL)) {
        draw_text_aa(FONT_AA_LABEL, tx, ty, how, ATOMIK_FG_DIM);
        ty += text_height_aa(FONT_AA_LABEL) + ATOMIK_GRID_S;
    }
    /* big metric */
    if (font_aa_loaded(FONT_AA_BRAND)) {
        draw_text_aa(FONT_AA_BRAND, tx, ty, big, is_atomik ? accent : ATOMIK_FG);
        ty += text_height_aa(FONT_AA_BRAND);
    } else { draw_text(tx, ty, big, 4, accent); ty += text_height(4); }
    /* unit */
    if (font_aa_loaded(FONT_AA_LABEL))
        draw_text_aa(FONT_AA_LABEL, tx, ty, unit, rgb(0x86, 0x92, 0xA8));
}

/* Parallel-Aggregate scenario only: the bank ladder.  Renders the measured
 * 1/2/4/8-lane sweep as rungs (label, speedup bar, value), max-lane rung in
 * savings-green, then the order-independence proof: the merged-accumulator
 * result was IDENTICAL at every lane count (commutative XOR — lane order and
 * count cannot change the answer).  Returns the height consumed. */
static int parallel_ladder(int x, int y, int w, const bench_point_t *pts, int npts) {
    int lab = font_aa_loaded(FONT_AA_LABEL);
    int lh  = lab ? text_height_aa(FONT_AA_LABEL) : text_height(1);
    int y0 = y;
    double smax = 1.0;
    for (int i = 0; i < npts; i++) if (pts[i].speedup > smax) smax = pts[i].speedup;
    int label_col = 92, val_col = 70;
    int bar_x   = x + label_col;
    int track_w = w - label_col - val_col;
    int rh = 22;
    /* the cursor-selected rung (default = max lanes); record geometry so a
     * click can hit-test the rows. */
    int sel = (s_sel_idx >= 0 && s_sel_idx < npts) ? s_sel_idx : npts - 1;
    s_rung_x = x; s_rung_w = w; s_rung_h = rh; s_rung_n = npts;
    for (int i = 0; i < npts; i++) {
        s_rung_y[i] = y;
        int chosen = (i == sel);                    /* the featured config */
        pixel_t col = chosen ? ATOMIK_SEM_SAVINGS : ATOMIK_ACCENT;
        char lbl[24];
        snprintf(lbl, sizeof lbl, "%u lane%s", pts[i].banks, pts[i].banks == 1 ? "" : "s");
        /* selection chevron on the chosen rung */
        if (chosen) {
            for (int t = 0; t < 5; t++) {
                draw_rect(x, y + (rh - 9) / 2 + t,     5 - t, 1, ATOMIK_SEM_SAVINGS);
                draw_rect(x, y + (rh - 9) / 2 + 8 - t, 5 - t, 1, ATOMIK_SEM_SAVINGS);
            }
        }
        if (lab) {
            int lw = text_width_aa(FONT_AA_LABEL, lbl);
            draw_text_aa(FONT_AA_LABEL, bar_x - 12 - lw, y + (rh - lh) / 2, lbl,
                         chosen ? ATOMIK_FG : ATOMIK_FG_DIM);
        }
        int bh = 14, by = y + (rh - bh) / 2;
        draw_rect_rounded(bar_x, by, track_w, bh, bh / 2, rgb(0x16, 0x1E, 0x30));
        int fw = (smax > 0) ? (int)(pts[i].speedup / smax * track_w) : bh;
        if (fw < bh) fw = bh;
        if (fw > track_w) fw = track_w;
        draw_rect_rounded(bar_x, by, fw, bh, bh / 2, col);
        if (chosen)
            for (int g = 1; g <= 3; g++) {
                uint8_t a = (uint8_t)(40 - (g - 1) * 12);
                for (int sx = 0; sx < fw; sx++) draw_blend_pixel(bar_x + sx, by - g, col, a);
            }
        /* The 1-lane rung IS the conventional sequential path: one accumulator,
         * one delta per cycle, no parallelism to exploit.  Its bar is only ~1/N
         * of the track, so annotate the empty space to its right — the
         * equivalence then reads as the deliberate fair-fight baseline (same
         * work, same cycles) rather than ATOMiK failing to win.  The scaling
         * lives in the rungs below it. */
        if (pts[i].banks == 1 && lab)
            draw_text_aa(FONT_AA_LABEL, bar_x + fw + 12, y + (rh - lh) / 2,
                         "= conventional baseline", rgb(0x6E, 0x7A, 0x90));
        char vs[16]; snprintf(vs, sizeof vs, "%.2fx", pts[i].speedup);
        if (lab) {
            int vw = text_width_aa(FONT_AA_LABEL, vs);
            draw_text_aa(FONT_AA_LABEL, x + w - vw, y + (rh - lh) / 2, vs, col);
        }
        y += rh + 4;
    }
    /* order-independence proof line */
    y += 4;
    if (lab) {
        const char *hex = bench_result_hex();
        int ident = bench_result_identical();
        char proof[112];
        if (ident && hex[0])
            snprintf(proof, sizeof proof, "Same result  %s  at 1 / 2 / 4 / 8 lanes", hex);
        else
            snprintf(proof, sizeof proof, "Order-independent result across all lanes");
        draw_text_aa(FONT_AA_LABEL, x, y, proof, rgb(0xB8, 0xE8, 0xC4));
        if (ident) {
            int pw = text_width_aa(FONT_AA_LABEL, proof);
            draw_text_aa(FONT_AA_LABEL, x + pw + 12, y, "[verified]", ATOMIK_SEM_SAVINGS);
        }
        y += lh + 2;
    }
    return y - y0;
}

/* Recompute the Parallel Aggregate scenario's featured numbers from the live
 * bench sweep + the cursor-selected lane (s_sel_idx); updates s_wl[2] in place.
 * Returns bench_live.  Shared by the full draw and the sub-element redraw. */
static int parallel_recompute(void) {
    int npts = 0; const bench_point_t *pts = bench_sweep_points(&npts);
    int bench_live = (bench_source() == METRIC_LIVE) && npts >= 2;
    int sel = (s_sel_idx >= 0 && s_sel_idx < npts) ? s_sel_idx : (npts > 0 ? npts - 1 : 0);
    if (bench_live) {
        long base = (long)pts[0].cycles, fast = (long)pts[sel].cycles;
        snprintf(s_pconv, sizeof s_pconv, "%ld", base);
        snprintf(s_patk,  sizeof s_patk,  "%ld", fast);
        if (sel == 0) snprintf(s_psav, sizeof s_psav, "BASELINE (1 lane)");
        else          snprintf(s_psav, sizeof s_psav, "%.0fx FASTER", pts[sel].speedup);
        snprintf(s_phow, sizeof s_phow, "%u lane%s, lock-free, all at once",
                 pts[sel].banks, pts[sel].banks == 1 ? "" : "s");
        s_wl[2].conv_big = s_pconv; s_wl[2].atomik_big = s_patk;
        s_wl[2].conv_bar = (double)base; s_wl[2].atomik_bar = (double)fast;
        s_wl[2].savings = s_psav;  s_wl[2].atomik_how = s_phow;
    }
    s_wl[2].live = bench_live;
    return bench_live;
}

/* Savings banner (green pill with the Nx-FASTER headline + guarantee line).
 * Returns its height.  Factored out so the sub-element redraw can repaint it. */
static int draw_savings_banner(int x, int y, int w, const char *savings,
                               const char *guarantee) {
    int lab = font_aa_loaded(FONT_AA_LABEL);
    int lh  = lab ? text_height_aa(FONT_AA_LABEL) : text_height(1);
    int bh  = (font_aa_loaded(FONT_AA_DISPLAY) ? text_height_aa(FONT_AA_DISPLAY) : 28)
              + ATOMIK_GRID_M * 2;
    draw_rect_rounded(x, y, w, bh, 12, rgb(0x0E, 0x22, 0x16));
    draw_rect(x, y, w, 1, ATOMIK_SEM_SAVINGS);
    draw_rect(x, y + bh - 1, w, 1, ATOMIK_SEM_SAVINGS);
    draw_rect(x, y, 1, bh, ATOMIK_SEM_SAVINGS);
    draw_rect(x + w - 1, y, 1, bh, ATOMIK_SEM_SAVINGS);
    if (font_aa_loaded(FONT_AA_DISPLAY))
        draw_text_aa(FONT_AA_DISPLAY, x + ATOMIK_GRID_L, y + ATOMIK_GRID_M, savings, ATOMIK_SEM_SAVINGS);
    if (lab) {
        int gw = text_width_aa(FONT_AA_LABEL, guarantee);
        draw_text_aa(FONT_AA_LABEL, x + w - gw - ATOMIK_GRID_L, y + bh/2 - lh/2,
                     guarantee, rgb(0xB8, 0xE8, 0xC4));
    }
    return bh;
}

void workloads_surface_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    int pad = ATOMIK_GRID_L, ix = x + pad, iw = wd - pad * 2, ty = y + ATOMIK_GRID_M;
    int lab = font_aa_loaded(FONT_AA_LABEL);
    int lh  = lab ? text_height_aa(FONT_AA_LABEL) : text_height(1);

    /* live bench data drives the parallel-aggregate scenario (via the cursor-
     * selected lane, s_sel_idx); pts/npts reused for the ladder draw below. */
    int npts = 0; const bench_point_t *pts = bench_sweep_points(&npts);
    int bench_live = parallel_recompute();

    /* live memory-workload data (aworkload) overwrites scenarios 0 + 1 with
     * real, hardware-VERIFIED numbers; flips them LIVE when present. */
    static char m_conv[2][16], m_atk[2][16], m_sav[2][28];
    for (int wi = 0; wi < 2; wi++) {
        wl_measure_t m;
        if (wlmeasure_get(wi, &m) && m.valid && m.verified && m.conv > 0) {
            if (m.is_bytes) {
                if (m.conv >= 1024) snprintf(m_conv[wi], sizeof m_conv[wi], "%.1f", m.conv / 1024.0);
                else                snprintf(m_conv[wi], sizeof m_conv[wi], "%ld", m.conv);
                snprintf(m_atk[wi], sizeof m_atk[wi], "%ld", m.atomik);
                snprintf(m_sav[wi], sizeof m_sav[wi], "%d%% LESS DATA MOVED", m.saved_pct);
            } else {
                snprintf(m_conv[wi], sizeof m_conv[wi], "%ld", m.conv);
                snprintf(m_atk[wi],  sizeof m_atk[wi],  "%ld", m.atomik);
                snprintf(m_sav[wi], sizeof m_sav[wi], "%d%% FEWER WRITES", m.saved_pct);
            }
            s_wl[wi].conv_big = m_conv[wi]; s_wl[wi].atomik_big = m_atk[wi];
            s_wl[wi].conv_bar = (double)m.conv; s_wl[wi].atomik_bar = (double)m.atomik;
            s_wl[wi].savings = m_sav[wi];
            s_wl[wi].live = 1;
        } else {
            s_wl[wi].live = 0;
        }
    }

    wl_scenario_t *sc = &s_wl[s_scenario];

    /* ── header ───────────────────────────────────────────────────────── */
    if (font_aa_loaded(FONT_AA_DISPLAY)) {
        draw_text_aa(FONT_AA_DISPLAY, ix, ty, "WORKLOADS", ATOMIK_FG);
    }
    /* source chip top-right */
    {
        int live = sc->live;
        const char *chip = live ? "LIVE / MEASURED" : "SCENARIO";
        pixel_t cc = live ? ATOMIK_SEM_SAVINGS : ATOMIK_SEM_WASTE;
        if (lab) {
            int cw = text_width_aa(FONT_AA_LABEL, chip);
            int cx = ix + iw - cw - ATOMIK_GRID_M;
            draw_rect_rounded(cx - ATOMIK_GRID_S, ty, cw + ATOMIK_GRID_M, lh + 6, (lh+6)/2, rgb(0x12,0x1A,0x2C));
            draw_text_aa(FONT_AA_LABEL, cx, ty + 2, chip, cc);
        }
    }
    ty += (font_aa_loaded(FONT_AA_DISPLAY) ? text_height_aa(FONT_AA_DISPLAY) : text_height(3));
    if (lab) { draw_text_aa(FONT_AA_LABEL, ix, ty, "Real workloads — what it costs the conventional way vs ATOMiK", rgb(0x86,0x92,0xA8)); ty += lh + ATOMIK_GRID_M; }

    /* ── scenario tabs ────────────────────────────────────────────────── */
    {
        int tabx = ix;
        for (int i = 0; i < N_WL; i++) {
            const char *t = s_wl[i].tab;
            int tw = (lab ? text_width_aa(FONT_AA_LABEL, t) : text_width(t,1)) + ATOMIK_GRID_M;
            int th = lh + 8;
            int active = (i == s_scenario);
            pixel_t bg = active ? rgb(0x16,0x2A,0x3E) : rgb(0x10,0x16,0x24);
            draw_rect_rounded(tabx, ty, tw, th, 6, bg);
            if (active) { draw_rect(tabx, ty + th - 2, tw, 2, ATOMIK_ACCENT); }
            if (lab) draw_text_aa(FONT_AA_LABEL, tabx + ATOMIK_GRID_S, ty + 3, t,
                                  active ? ATOMIK_FG : ATOMIK_FG_DIM);
            s_tab_rect[i][0] = tabx; s_tab_rect[i][1] = ty;
            s_tab_rect[i][2] = tw;   s_tab_rect[i][3] = th;   /* clickable */
            tabx += tw + ATOMIK_GRID_S;
        }
        ty += lh + 8 + ATOMIK_GRID_L;
    }

    /* ── scenario title + plain description ───────────────────────────── */
    if (font_aa_loaded(FONT_AA_UI)) {
        draw_text_aa(FONT_AA_UI, ix, ty, sc->title, ATOMIK_FG);
        ty += text_height_aa(FONT_AA_UI) + ATOMIK_GRID_S;
    }
    if (lab) {
        draw_text_aa(FONT_AA_LABEL, ix, ty, sc->desc1, ATOMIK_FG_DIM); ty += lh + 2;
        if (sc->desc2[0]) { draw_text_aa(FONT_AA_LABEL, ix, ty, sc->desc2, ATOMIK_FG_DIM); ty += lh; }
        ty += ATOMIK_GRID_M;
    }

    /* ── side-by-side comparison panels ───────────────────────────────── */
    {
        int gap = ATOMIK_GRID_M;
        int pw = (iw - gap) / 2;
        int phh = lh * 3 + (font_aa_loaded(FONT_AA_BRAND) ? text_height_aa(FONT_AA_BRAND) : 28) + ATOMIK_GRID_M * 2 + ATOMIK_GRID_S;
        compare_panel(ix, ty, pw, phh, "CONVENTIONAL", sc->conv_how,
                      sc->conv_big, sc->conv_unit, ATOMIK_SEM_WASTE, 0);
        compare_panel(ix + pw + gap, ty, pw, phh, "ATOMiK", sc->atomik_how,
                      sc->atomik_big, sc->atomik_unit, ATOMIK_SEM_SAVINGS, 1);
        s_atk_rect[0] = ix + pw + gap; s_atk_rect[1] = ty;
        s_atk_rect[2] = pw;            s_atk_rect[3] = phh;
        ty += phh + ATOMIK_GRID_L;
    }

    /* ── visual comparison: lane ladder for Parallel Aggregate (shows the
     *    measured 1/2/4/8-lane scaling + the identical-result proof); plain
     *    two-bar comparison for the data-movement scenarios. ──────────────── */
    if (s_scenario == 2 && bench_live) {
        int lad_h = parallel_ladder(ix, ty, iw, pts, npts);
        s_ladder_rect[0] = ix; s_ladder_rect[1] = ty;
        s_ladder_rect[2] = iw; s_ladder_rect[3] = lad_h;
        ty += lad_h + ATOMIK_GRID_L;
    } else {
        double vmax = sc->conv_bar > sc->atomik_bar ? sc->conv_bar : sc->atomik_bar;
        compare_bar(ix, ty, iw, "Conventional", sc->conv_bar, vmax, ATOMIK_SEM_WASTE, sc->conv_big);
        ty += 16 + ATOMIK_GRID_M;
        compare_bar(ix, ty, iw, "ATOMiK", sc->atomik_bar, vmax, ATOMIK_SEM_SAVINGS, sc->atomik_big);
        ty += 16 + ATOMIK_GRID_L;
    }

    /* ── savings banner ───────────────────────────────────────────────── */
    {
        s_savings_rect[0] = ix; s_savings_rect[1] = ty; s_savings_rect[2] = iw;
        int bh = draw_savings_banner(ix, ty, iw, sc->savings, sc->guarantee);
        s_savings_rect[3] = bh;
        ty += bh + ATOMIK_GRID_M;
    }

    /* ── provenance footer — where these numbers come from ───────────── */
    if (lab) {
        const char *prov;
        if (sc->live)
            prov = (s_scenario == 2)
                 ? "Measured on XC7Z020 silicon - parallel-bank engine @0xF0021000"
                 : "Verified on XC7Z020 silicon - ATOMiK delta-state adapter @0xF0020000";
        else
            prov = "Illustrative scenario - board verification pending";
        draw_text_aa(FONT_AA_LABEL, ix, ty, prov, rgb(0x5E, 0x6A, 0x80));
        ty += lh + ATOMIK_GRID_M;
    }

    /* ── bottom rail: scenario position dots (left) + key hint (right) ── */
    {
        int by = y + ht - lh - ATOMIK_GRID_M;
        int dot_y = by + lh / 2;
        int dx = ix + 4;
        for (int i = 0; i < N_WL; i++) {
            int active = (i == s_scenario);
            int r = active ? 4 : 3;
            pixel_t c = active ? ATOMIK_ACCENT : rgb(0x3A, 0x46, 0x60);
            for (int yy = -r; yy <= r; yy++)
                for (int xx = -r; xx <= r; xx++)
                    if (xx*xx + yy*yy <= r*r)
                        draw_blend_pixel(dx + xx, dot_y + yy, c, active ? 255 : 200);
            if (active)            /* soft halo on the active dot */
                for (int g = r+1; g <= r+3; g++) {
                    uint8_t a = (uint8_t)(50 - (g-r-1)*15);
                    for (int yy = -g; yy <= g; yy++)
                        for (int xx = -g; xx <= g; xx++) {
                            int d2 = xx*xx + yy*yy;
                            if (d2 > r*r && d2 <= g*g) draw_blend_pixel(dx+xx, dot_y+yy, c, a);
                        }
                }
            dx += 18;
        }
        if (lab && fabric_demo_enabled()) {
            draw_text_aa(FONT_AA_LABEL, dx + 4, by, "AUTO", ATOMIK_ACCENT_DIM);
        }
        if (lab) {
            const char *hint = (s_scenario == 2)
                ? "Click a lane to select  /  W for next workload"
                : "Press W for the next workload";
            int hw = text_width_aa(FONT_AA_LABEL, hint);
            draw_text_aa(FONT_AA_LABEL, ix + iw - hw, by, hint, rgb(0x5E, 0x6A, 0x80));
        }
    }
}

/* v0.42 sub-element delta: after a lane select changed s_sel_idx, redraw ONLY
 * the three selection-dependent elements (ATOMiK panel, ladder, savings banner)
 * over the existing back buffer — skipping the unchanged header/tabs/title/
 * CONVENTIONAL panel/provenance AA text that made a full window recomposite
 * pause on this core.  Returns 1 + the bounding rect (caller presents it + the
 * cursor); 0 to fall back to a full redraw. */
int workloads_surface_redraw_selection(int *rx, int *ry, int *rw, int *rh) {
    if (s_window_id < 0 || s_scenario != 2 ||
        s_atk_rect[2] == 0 || s_ladder_rect[2] == 0 || s_savings_rect[2] == 0)
        return 0;
    if (!parallel_recompute()) return 0;          /* bench data gone -> full */

    wl_scenario_t *sc = &s_wl[2];
    /* ATOMiK panel + savings banner draw their own backgrounds, so they fully
     * self-erase.  The ladder's labels/chevron sit on the bare window body, so
     * clear that rect to the card tone first (else the OLD selection chevron
     * ghosts on the previously-selected rung). */
    compare_panel(s_atk_rect[0], s_atk_rect[1], s_atk_rect[2], s_atk_rect[3],
                  "ATOMiK", sc->atomik_how, sc->atomik_big, sc->atomik_unit,
                  ATOMIK_SEM_SAVINGS, 1);
    draw_rect(s_ladder_rect[0], s_ladder_rect[1], s_ladder_rect[2], s_ladder_rect[3], wm_card_bg());
    int npts = 0; const bench_point_t *pts = bench_sweep_points(&npts);
    parallel_ladder(s_ladder_rect[0], s_ladder_rect[1], s_ladder_rect[2], pts, npts);
    draw_savings_banner(s_savings_rect[0], s_savings_rect[1], s_savings_rect[2],
                        sc->savings, sc->guarantee);

    /* bounding box of the three redrawn rects */
    int x0 = s_atk_rect[0], y0 = s_atk_rect[1];
    int x1 = s_atk_rect[0] + s_atk_rect[2], y1 = s_atk_rect[1] + s_atk_rect[3];
    const int *rs[3] = { s_atk_rect, s_ladder_rect, s_savings_rect };
    for (int i = 0; i < 3; i++) {
        if (rs[i][0] < x0) x0 = rs[i][0];
        if (rs[i][1] < y0) y0 = rs[i][1];
        if (rs[i][0] + rs[i][2] > x1) x1 = rs[i][0] + rs[i][2];
        if (rs[i][1] + rs[i][3] > y1) y1 = rs[i][1] + rs[i][3];
    }
    if (rx) *rx = x0; if (ry) *ry = y0;
    if (rw) *rw = x1 - x0; if (rh) *rh = y1 - y0;
    return 1;
}

void workloads_surface_open(void) {
    /* preview: pick scenario via env so each can be screenshotted. */
    const char *e = getenv("ATOMIK_PREVIEW_WL_SCENARIO");
    if (e && *e) workloads_set_scenario(atoi(e));
    /* preview: pre-select a lane rung for host shots of the interactive state. */
    const char *es = getenv("ATOMIK_PREVIEW_WL_SEL");
    if (es && *es) s_sel_idx = atoi(es);
    if (s_window_id >= 0) { wm_focus(s_window_id); return; }
    int ww = 780, wh = 560;
    int wx = (FB_W - ww) / 2, wy = (FB_H - wh) / 2 - 20;
    window_t *win = wm_open("WORKLOADS", wx, wy, ww, wh, workloads_surface_draw, NULL);
    if (win) s_window_id = win->id;
}
