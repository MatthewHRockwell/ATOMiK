/* workloads_surface.c — the WORKLOADS capability: a customer-facing, board-
 * MEASURED demo of ATOMiK running a real lock-free aggregation workload on the
 * parallel-bank engine @0xF0021000.
 *
 * Framing (edge/IoT + observability): "ingest a stream of telemetry deltas,
 * aggregate them lock-free into XOR banks, scale throughput linearly with the
 * bank count, and get a BYTE-IDENTICAL result every run regardless of order or
 * bank count."  The last property — order-independence — is the architectural
 * headline no Von-Neumann counter array can match, shown here on real silicon.
 *
 * Every number is bench_* (real, self-verified by the standalone sweep tool) or
 * a clearly-labeled WAITING state.  No fabricated telemetry. */
#include "atomik_os.h"
#include <stdio.h>

static int s_window_id = -1;

/* throughput scaling chart: one bar per bank count (1/2/4/8), height ∝ events/s,
 * active config highlighted, speedup printed at the full-bank bar. */
static void scaling_chart(int x, int y, int wd, int h,
                          const bench_point_t *pts, int npts, pixel_t accent) {
    int cu = font_aa_loaded(FONT_AA_LABEL);
    int lh = cu ? text_height_aa(FONT_AA_LABEL) : text_height(1);
    int bars_top = y, bars_h = h - lh - 4; if (bars_h < 20) bars_h = 20;
    int slots = (npts > 0) ? npts : BENCH_N_POINTS;
    int active = (npts > 0) ? (int)pts[npts - 1].banks : 8;
    double vmax = 0.0;
    for (int i = 0; i < npts; i++) if (pts[i].mdeltas_s > vmax) vmax = pts[i].mdeltas_s;
    int bw = (wd / slots) * 6 / 10; if (bw < 10) bw = 10;
    for (int i = 0; i < slots; i++) {
        int cxi = x + (wd * (2 * i + 1)) / (slots * 2);
        int bx  = cxi - bw / 2;
        int banks = (npts > 0) ? (int)pts[i].banks : (1 << i);
        int bh; pixel_t fill;
        int is_active = (npts > 0) && (pts[i].banks == (uint32_t)active);
        if (npts > 0 && vmax > 0.0) {
            bh = (int)(pts[i].mdeltas_s / vmax * bars_h); if (bh < 3) bh = 3;
            fill = is_active ? accent : ATOMIK_ACCENT_DIM;
        } else { bh = bars_h / 6 + i * bars_h / 8; fill = rgb(0x2A, 0x34, 0x48); }
        int by = bars_top + bars_h - bh;
        draw_rect_rounded(bx, by, bw, bh, 3, fill);
        if (is_active) for (int g = 1; g <= 5; g++) {
            uint8_t a = (uint8_t)(44 - (g - 1) * 8);
            for (int sx = -g; sx < bw + g; sx++) draw_blend_pixel(bx + sx, by - g, accent, a);
        }
        /* per-bar speedup (real) above active-ish bars; bank label below */
        if (npts > 0) {
            char sp[12]; snprintf(sp, sizeof sp, "%.0fx", pts[i].speedup);
            int spw = cu ? text_width_aa(FONT_AA_LABEL, sp) : text_width(sp, 1);
            pixel_t spc = is_active ? accent : ATOMIK_FG_DIM;
            if (cu) draw_text_aa(FONT_AA_LABEL, cxi - spw / 2, by - lh - 6, sp, spc);
            else    draw_text(cxi - spw / 2, by - lh - 6, sp, 1, spc);
        }
        char bl[12]; snprintf(bl, sizeof bl, "%d", banks);
        int blw = cu ? text_width_aa(FONT_AA_LABEL, bl) : text_width(bl, 1);
        pixel_t lc = is_active ? accent : ATOMIK_FG_DIM;
        if (cu) draw_text_aa(FONT_AA_LABEL, cxi - blw / 2, bars_top + bars_h + 2, bl, lc);
        else    draw_text(cxi - blw / 2, bars_top + bars_h + 2, bl, 1, lc);
    }
    /* axis caption */
    const char *cap = "BANKS ALLOCATED";
    if (cu) draw_text_aa(FONT_AA_LABEL, x, bars_top + bars_h + lh + 6, cap, rgb(0x5E,0x6A,0x80));
}

void workloads_surface_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    int pad = ATOMIK_GRID_L, ix = x + pad, iw = wd - pad * 2, ty = y + ATOMIK_GRID_M;
    pixel_t accent = ATOMIK_SEM_HARDWARE;
    int lab = font_aa_loaded(FONT_AA_LABEL);
    int lh  = lab ? text_height_aa(FONT_AA_LABEL) : text_height(1);

    int npts = 0; const bench_point_t *pts = bench_sweep_points(&npts);
    int src  = bench_source();
    const bench_point_t *top = (npts > 0) ? &pts[npts - 1] : NULL;

    /* ── header ───────────────────────────────────────────────────────── */
    if (font_aa_loaded(FONT_AA_DISPLAY)) {
        draw_text_aa(FONT_AA_DISPLAY, ix, ty, "WORKLOADS", ATOMIK_FG);
        ty += text_height_aa(FONT_AA_DISPLAY);
    } else { draw_text(ix, ty, "WORKLOADS", 3, ATOMIK_FG); ty += text_height(3); }
    if (lab) {
        draw_text_aa(FONT_AA_LABEL, ix, ty,
                     "Real-Time Telemetry Aggregation  /  lock-free delta accumulation",
                     rgb(0x86, 0x92, 0xA8));
        ty += lh + ATOMIK_GRID_L;
    }

    /* ── HERO throughput — the standout number ────────────────────────── */
    {
        char big[16], unit[40];
        if (top) {
            snprintf(big, sizeof big, "%.0f", top->mdeltas_s);
            snprintf(unit, sizeof unit, "M events / sec  @ %d banks", (int)top->banks);
        } else { snprintf(big, sizeof big, "--"); snprintf(unit, sizeof unit, "M events / sec"); }
        int by = ty;
        if (font_aa_loaded(FONT_AA_BRAND)) {
            draw_text_aa(FONT_AA_BRAND, ix, by, big, accent);
            int bw2 = text_width_aa(FONT_AA_BRAND, big);
            int uy = by + text_height_aa(FONT_AA_BRAND) - (lab ? lh : text_height(1)) - 4;
            if (lab) draw_text_aa(FONT_AA_LABEL, ix + bw2 + ATOMIK_GRID_M, uy, unit, ATOMIK_FG_DIM);
            ty += text_height_aa(FONT_AA_BRAND) + 2;
        } else { draw_text(ix, by, big, 4, accent); ty += text_height(4) + 2; }
        /* source chip */
        const char *chip; pixel_t chipc;
        if      (src == METRIC_LIVE)  { chip = "LIVE / VERIFIED"; chipc = ATOMIK_SEM_SAVINGS; }
        else if (src == METRIC_STALE) { chip = "MEASURING";       chipc = ATOMIK_SEM_WASTE;  }
        else                          { chip = "WAITING - start aggregation engine"; chipc = ATOMIK_FG_DIM; }
        if (lab) draw_text_aa(FONT_AA_LABEL, ix, ty, chip, chipc);
        ty += lh + ATOMIK_GRID_L;
    }

    /* ── scaling chart ────────────────────────────────────────────────── */
    if (lab) { draw_text_aa(FONT_AA_LABEL, ix, ty, "THROUGHPUT SCALES WITH BANKS", rgb(0xC6,0xD2,0xE6)); ty += lh + ATOMIK_GRID_S; }
    int chart_h = 150; if (chart_h > (y + ht) - ty - 180) chart_h = (y + ht) - ty - 180; if (chart_h < 96) chart_h = 96;
    scaling_chart(ix, ty, iw, chart_h, pts, npts, accent);
    ty += chart_h + ATOMIK_GRID_L;

    /* ── PROOF callout — order-independence (the architectural headline) ── */
    {
        int ph = lh * 2 + ATOMIK_GRID_M * 2 + 6;
        int verified = (src == METRIC_LIVE);
        pixel_t edge = verified ? ATOMIK_SEM_SAVINGS : ATOMIK_FG_DIM;
        draw_rect_rounded(ix, ty, iw, ph, 10, wm_card_bg());
        draw_rect(ix, ty, iw, 1, edge); draw_rect(ix, ty + ph - 1, iw, 1, edge);
        draw_rect(ix, ty, 1, ph, edge); draw_rect(ix + iw - 1, ty, 1, ph, edge);
        int px = ix + ATOMIK_GRID_L, py = ty + ATOMIK_GRID_M;
        char l1[80];
        if (verified)
            snprintf(l1, sizeof l1, "%c  Byte-identical result across all %d bank configurations",
                     '+', npts > 0 ? npts : BENCH_N_POINTS);
        else
            snprintf(l1, sizeof l1, "Awaiting a verified run from the engine @0xF0021000");
        if (lab) draw_text_aa(FONT_AA_LABEL, px, py, l1, verified ? ATOMIK_FG : ATOMIK_FG_DIM);
        py += lh + 6;
        const char *l2 = "order-independent  -  no locks  -  no cache coherency  -  deterministic latency";
        if (lab) draw_text_aa(FONT_AA_LABEL, px, py, l2, rgb(0x86, 0x92, 0xA8));
        ty += ph + ATOMIK_GRID_M;
    }

    /* ── value rows (real) ────────────────────────────────────────────── */
    {
        char v[40];
        int col2 = ix + iw * 55 / 100;
        if (lab) {
            draw_text_aa(FONT_AA_LABEL, ix, ty, "Deltas aggregated / run", rgb(0x76,0x82,0x98));
            snprintf(v, sizeof v, "%u", 65537u);
            draw_text_aa(FONT_AA_LABEL, col2, ty, v, ATOMIK_FG);
            ty += lh + 6;
            draw_text_aa(FONT_AA_LABEL, ix, ty, "Peak speedup vs single bank", rgb(0x76,0x82,0x98));
            if (top) snprintf(v, sizeof v, "%.2fx  @ %d banks", top->speedup, (int)top->banks);
            else     snprintf(v, sizeof v, "--");
            draw_text_aa(FONT_AA_LABEL, col2, ty, v, top ? accent : ATOMIK_FG_DIM);
            ty += lh + 6;
            draw_text_aa(FONT_AA_LABEL, ix, ty, "Substrate", rgb(0x76,0x82,0x98));
            snprintf(v, sizeof v, "XC7Z020 FPGA  -  XOR delta-state banks");
            draw_text_aa(FONT_AA_LABEL, col2, ty, v, ATOMIK_FG);
        }
    }
}

void workloads_surface_open(void) {
    if (s_window_id >= 0) { wm_focus(s_window_id); return; }
    int ww = 760, wh = 600;
    int wx = (FB_W - ww) / 2, wy = (FB_H - wh) / 2 - 20;
    window_t *win = wm_open("WORKLOADS", wx, wy, ww, wh, workloads_surface_draw, NULL);
    if (win) s_window_id = win->id;
}
