/* system_surface.c — the "SYSTEM" capability surface (the 2026 task-manager).
 *
 * Per the agentic-rail vision, SYSTEM is about COMPUTE & RESOURCE ALLOCATION:
 * how the hardware's parallel banks are allocated and what that buys.  Its
 * centerpiece is the REAL, hardware-validated PARALLEL BANKS bench (moved here
 * from the Resource Fabric so the Fabric can be the concept's 5 lanes) — the
 * 8.00x / VERIFIED measurement from the engine @0xF0021000.  Future work: live
 * per-bank reallocation + virtual CPU/GPU/NPU tuning.  Numbers are real
 * (bench_*) or clearly-labeled; no fabricated telemetry. */
#include "atomik_os.h"
#include <stdio.h>

static int s_window_id = -1;

/* PARALLEL BANKS bench viz (moved verbatim from fabric.c). */
static void banks_panel(int x, int y, int wd, int h) {
    pixel_t accent = ATOMIK_SEM_HARDWARE;
    draw_rect_rounded(x, y, wd, h, 10, wm_card_bg());
    draw_rect(x, y, wd, 1, wm_card_border());
    draw_rect(x, y + h - 1, wd, 1, wm_card_border());
    draw_rect(x, y, 1, h, wm_card_border());
    draw_rect(x + wd - 1, y, 1, h, wm_card_border());
    for (int sy = 0; sy < 10; sy++) {
        uint8_t a = (uint8_t)(55 - sy * 5);
        for (int sx = 0; sx < wd; sx++) draw_blend_pixel(x + sx, y + sy, accent, a);
    }
    int pad = ATOMIK_GRID_L, tx = x + pad, ty = y + ATOMIK_GRID_M;
    int title_h;
    if (font_aa_loaded(FONT_AA_UI)) {
        draw_text_aa(FONT_AA_UI, tx, ty, "PARALLEL BANKS", accent);
        title_h = text_height_aa(FONT_AA_UI);
    } else { draw_text(tx, ty, "PARALLEL BANKS", 2, accent); title_h = text_height(2); }

    int src = bench_source();
    const char *chip; pixel_t chipc;
    if      (src == METRIC_LIVE)  { chip = "VERIFIED";  chipc = ATOMIK_SEM_SAVINGS; }
    else if (src == METRIC_STALE) { chip = "MEASURING"; chipc = ATOMIK_SEM_WASTE;  }
    else                          { chip = "WAITING";   chipc = ATOMIK_FG_DIM;     }
    int cu = font_aa_loaded(FONT_AA_LABEL);
    int cw = cu ? text_width_aa(FONT_AA_LABEL, chip) : text_width(chip, 1);
    int chh = (cu ? text_height_aa(FONT_AA_LABEL) : text_height(1)) + 4;
    int cwp = cw + ATOMIK_GRID_M, cxp = x + wd - cwp - pad, cyp = ty + (title_h - chh) / 2;
    draw_rect_rounded(cxp, cyp, cwp, chh, chh / 2, wm_card_bg() & 0x0F0F0F);
    draw_rect(cxp + chh / 2, cyp, cwp - chh, 1, chipc);
    draw_rect(cxp + chh / 2, cyp + chh - 1, cwp - chh, 1, chipc);
    if (cu) draw_text_aa(FONT_AA_LABEL, cxp + ATOMIK_GRID_S, cyp + 2, chip, chipc);
    else    draw_text(cxp + ATOMIK_GRID_S, cyp + 2, chip, 1, chipc);

    int npts = 0; const bench_point_t *pts = bench_sweep_points(&npts);
    int chart_x = tx, chart_w = (wd - pad * 2) * 58 / 100;
    int bars_top = ty + title_h + ATOMIK_GRID_M;
    int label_h = cu ? text_height_aa(FONT_AA_LABEL) : text_height(1);
    int bars_h = (y + h - ATOMIK_GRID_M) - bars_top - label_h - 2; if (bars_h < 16) bars_h = 16;
    double vmax = 0.0;
    for (int i = 0; i < npts; i++) if (pts[i].mdeltas_s > vmax) vmax = pts[i].mdeltas_s;
    int slots = (npts > 0) ? npts : BENCH_N_POINTS;
    int active = (npts > 0) ? (int)pts[npts - 1].banks : 8;
    int bw = chart_w / (slots * 2); if (bw < 6) bw = 6;
    for (int i = 0; i < slots; i++) {
        int bx = chart_x + (chart_w * (2 * i + 1)) / (slots * 2) - bw / 2;
        int banks = (npts > 0) ? (int)pts[i].banks : (1 << i);
        int bh; pixel_t fill;
        if (npts > 0 && vmax > 0.0) {
            bh = (int)(pts[i].mdeltas_s / vmax * bars_h);
            int is_active = (pts[i].banks == (uint32_t)active);
            fill = is_active ? accent : ATOMIK_ACCENT_DIM; if (bh < 2) bh = 2;
            int by = bars_top + bars_h - bh;
            draw_rect_rounded(bx, by, bw, bh, 3, fill);
            if (is_active) for (int g = 1; g <= 4; g++) {
                uint8_t a = (uint8_t)(40 - (g - 1) * 9);
                for (int sx = -g; sx < bw + g; sx++) draw_blend_pixel(bx + sx, by - g, accent, a);
                for (int sy = -g; sy < bh + g; sy++) {
                    draw_blend_pixel(bx - g, by + sy, accent, a);
                    draw_blend_pixel(bx + bw - 1 + g, by + sy, accent, a);
                }
            }
        } else { bh = bars_h / 6 + i * 3; int by = bars_top + bars_h - bh;
                 draw_rect_rounded(bx, by, bw, bh, 3, rgb(0x2A, 0x34, 0x48)); }
        char bl[8]; snprintf(bl, sizeof bl, "%d", banks);
        int blw = cu ? text_width_aa(FONT_AA_LABEL, bl) : text_width(bl, 1);
        int blx = bx + bw / 2 - blw / 2, bly = bars_top + bars_h + 1;
        pixel_t lcol = (npts > 0 && pts[i].banks == (uint32_t)active) ? accent : ATOMIK_FG_DIM;
        if (cu) draw_text_aa(FONT_AA_LABEL, blx, bly, bl, lcol);
        else    draw_text(blx, bly, bl, 1, lcol);
    }
    int rx = chart_x + chart_w + ATOMIK_GRID_L, ry = bars_top;
    if (npts > 0) {
        char big[16]; snprintf(big, sizeof big, "%.2f", pts[npts - 1].speedup);
        if (font_aa_loaded(FONT_AA_DISPLAY)) {
            draw_text_aa(FONT_AA_DISPLAY, rx, ry, big, accent);
            int nbw = text_width_aa(FONT_AA_DISPLAY, big);
            draw_text_aa(FONT_AA_LABEL, rx + nbw + 2,
                         ry + text_height_aa(FONT_AA_DISPLAY) - text_height_aa(FONT_AA_LABEL),
                         "x", ATOMIK_FG_DIM);
            ry += text_height_aa(FONT_AA_DISPLAY) + 2;
        } else { draw_text(rx, ry, big, 3, accent); ry += text_height(3) + 2; }
        char sub[48]; snprintf(sub, sizeof sub, "up to %.0f Md/s @ %d banks",
                               pts[npts - 1].mdeltas_s, (int)pts[npts - 1].banks);
        if (cu) draw_text_aa(FONT_AA_LABEL, rx, ry, sub, ATOMIK_FG_DIM);
        else    draw_text(rx, ry, sub, 1, ATOMIK_FG_DIM);
    } else {
        const char *msg = "engine @0xF0021000 - waiting";
        if (cu) draw_text_aa(FONT_AA_LABEL, rx, ry, msg, ATOMIK_FG_DIM);
        else    draw_text(rx, ry, msg, 1, ATOMIK_FG_DIM);
    }
}

/* one allocation row: name + sublabel + a horizontal allocation bar + value. */
static void alloc_row(int x, int y, int w, const char *name, const char *sub,
                      int pct, pixel_t col, int placeholder) {
    int lab = font_aa_loaded(FONT_AA_LABEL);
    int lh  = lab ? text_height_aa(FONT_AA_LABEL) : text_height(1);
    if (lab) draw_text_aa(FONT_AA_LABEL, x, y, name, ATOMIK_FG);
    else     draw_text(x, y, name, 1, ATOMIK_FG);
    if (lab) draw_text_aa(FONT_AA_LABEL, x, y + lh + 1, sub, rgb(0x76, 0x82, 0x98));
    else     draw_text(x, y + lh + 1, sub, 1, rgb(0x76, 0x82, 0x98));
    /* bar */
    int bar_x = x + w * 42 / 100, bar_w = w * 46 / 100, bar_y = y + lh / 2, bar_h = 8;
    draw_rect_rounded(bar_x, bar_y, bar_w, bar_h, 4, rgb(0x1C, 0x24, 0x38));
    int fillw = bar_w * pct / 100; if (fillw < 2 && pct > 0) fillw = 2;
    pixel_t bc = placeholder ? rgb(0x3A, 0x46, 0x60) : col;
    if (fillw > 0) draw_rect_rounded(bar_x, bar_y, fillw, bar_h, 4, bc);
    /* value */
    char v[24];
    if (placeholder) snprintf(v, sizeof v, "%d%%  (planned)", pct);
    else             snprintf(v, sizeof v, "%d%%", pct);
    int vw = lab ? text_width_aa(FONT_AA_LABEL, v) : text_width(v, 1);
    pixel_t vc = placeholder ? ATOMIK_FG_DIM : col;
    if (lab) draw_text_aa(FONT_AA_LABEL, x + w - vw, y, v, vc);
    else     draw_text(x + w - vw, y, v, 1, vc);
}

void system_surface_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    int pad = ATOMIK_GRID_L, ix = x + pad, iw = wd - pad * 2, ty = y + ATOMIK_GRID_M;

    /* header */
    if (font_aa_loaded(FONT_AA_DISPLAY)) {
        draw_text_aa(FONT_AA_DISPLAY, ix, ty, "SYSTEM", ATOMIK_FG);
        ty += text_height_aa(FONT_AA_DISPLAY);
    } else { draw_text(ix, ty, "SYSTEM", 3, ATOMIK_FG); ty += text_height(3); }
    if (font_aa_loaded(FONT_AA_LABEL)) {
        draw_text_aa(FONT_AA_LABEL, ix, ty, "Compute & Resource Allocation",
                     rgb(0x86, 0x92, 0xA8));
        ty += text_height_aa(FONT_AA_LABEL) + ATOMIK_GRID_L;
    } else ty += text_height(1) + ATOMIK_GRID_L;

    /* PARALLEL BANKS — the real, hardware-validated centerpiece */
    int bp_h = 150; if (bp_h > (y + ht) - ty - 200) bp_h = (y + ht) - ty - 200;
    if (bp_h < 110) bp_h = 110;
    banks_panel(ix, ty, iw, bp_h);
    ty += bp_h + ATOMIK_GRID_L;

    /* RESOURCE ALLOCATION (virtual processors) — bank throughput is REAL; the
     * vCPU/GPU/NPU rows are the planned dynamic-allocation vision, labeled. */
    if (font_aa_loaded(FONT_AA_LABEL)) {
        draw_text_aa(FONT_AA_LABEL, ix, ty, "VIRTUAL PROCESSORS", rgb(0xC6, 0xD2, 0xE6));
        ty += text_height_aa(FONT_AA_LABEL) + ATOMIK_GRID_M;
    }
    int rowh = (font_aa_loaded(FONT_AA_LABEL) ? text_height_aa(FONT_AA_LABEL) : text_height(1)) + 20;
    int nb = bench_nbanks();
    char bsub[40]; snprintf(bsub, sizeof bsub, "%d XOR banks - delta-state", nb > 0 ? nb : 8);
    alloc_row(ix, ty, iw, "vCPU  /  Banks", bsub, 100, ATOMIK_SEM_HARDWARE, 0); ty += rowh;
    alloc_row(ix, ty, iw, "vGPU  /  Render", "tile + glyph pipeline", 0, ATOMIK_SEM_SAVINGS, 1); ty += rowh;
    alloc_row(ix, ty, iw, "vNPU  /  Agents", "prediction + routing", 0, ATOMIK_SEM_AGENT, 1); ty += rowh;
}

void system_surface_open(void) {
    if (s_window_id >= 0) { wm_focus(s_window_id); return; }
    int ww = 760, wh = 560;
    int wx = (FB_W - ww) / 2, wy = (FB_H - wh) / 2;
    window_t *win = wm_open("SYSTEM", wx, wy, ww, wh, system_surface_draw, NULL);
    if (win) s_window_id = win->id;
}
