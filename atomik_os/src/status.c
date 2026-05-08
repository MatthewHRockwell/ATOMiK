/* status.c — top status bar.
 *
 * Reads /proc/uptime for clock display (since this rootfs has no RTC),
 * /proc/stat for CPU usage, and surfaces the agent prediction. Renders
 * along the very top edge of the screen above the desktop. */
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

void status_draw(void) {
    update_cpu();

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
    const int bar_h = 32;
    const int bar_y = ATOMIK_SAFE_TOP;
    const int ty    = bar_y + (bar_h - text_height(1)) / 2;

    /* Bar fills only the safe zone — wallpaper covers y=0..SAFE_TOP
     * (cropped/invisible on this monitor; visible as a thin strip on
     * monitors with less overscan, but that's fine — wallpaper looks
     * intentional even at edges). */
    draw_rect(0, bar_y,             FB_W, bar_h, rgb(0x1A, 0x22, 0x38));
    /* Hard cyan-dim separator at the bottom of the bar — sharp visual
     * end of the chrome, start of the desktop. */
    draw_rect(0, bar_y + bar_h - 1, FB_W, 1,     ATOMIK_ACCENT_DIM);

#if ATOMIK_DEBUG_STATUS
    /* If re-enabled, draws a magenta sentinel + PID-marker for diagnosis. */
    draw_rect(0, bar_y, FB_W, bar_h, rgb(0xFF, 0x00, 0xFF));
    int p = (int)getpid();
    pixel_t marker = rgb((uint8_t)((p >> 8) & 0xFF),
                         (uint8_t)(p & 0xFF), 0xFF);
    draw_rect(FB_W - ATOMIK_SAFE_RIGHT - 8, bar_y, 8, 8, marker);
#endif

    /* Left: ATOMiK identity wordmark (cyan = HARDWARE, the "this is
     * the OS chrome itself" signal in our semantic grammar) followed by
     * the v0.31 window-strip and then key hints in dim foreground. */
    const char *brand = "ATOMiK";
    int brand_x = ATOMIK_GRID_L;
    draw_text(brand_x, ty, brand, 1, ATOMIK_SEM_HARDWARE);
    int after_brand = brand_x + text_width(brand, 1) + ATOMIK_GRID_M * 2;

    /* v0.31 window-strip: one dot per open window, focused window
     * gets a bright cyan filled square, buried windows get a dim
     * outlined square.  Solves "I opened R but it's hidden under
     * Document" — even when a window is fully covered, its dot
     * stays visible on the status bar. */
    int dot_size  = ATOMIK_GRID_M;
    int dot_gap   = ATOMIK_GRID_S;
    int dot_y     = bar_y + (bar_h - dot_size) / 2;
    int dots_x    = after_brand;
    const window_t *top = wm_topmost();
    for (int i = 0; i < wm_count(); i++) {
        const window_t *w = wm_get(i);
        if (!w || !w->visible) continue;
        int focused = (top && w->id == top->id);
        if (focused) {
            draw_rect(dots_x, dot_y, dot_size, dot_size, ATOMIK_SEM_HARDWARE);
        } else {
            /* Hollow rect for buried windows — outline only. */
            draw_rect(dots_x,            dot_y,                  dot_size, 1,        ATOMIK_DOCK_BORDER);
            draw_rect(dots_x,            dot_y + dot_size - 1,   dot_size, 1,        ATOMIK_DOCK_BORDER);
            draw_rect(dots_x,            dot_y,                  1,        dot_size, ATOMIK_DOCK_BORDER);
            draw_rect(dots_x + dot_size - 1, dot_y,              1,        dot_size, ATOMIK_DOCK_BORDER);
        }
        dots_x += dot_size + dot_gap;
    }
    /* Spacer before hints so the strip + hints don't run together. */
    int hint_x = (wm_count() > 0) ? dots_x + ATOMIK_GRID_M * 2 : after_brand;

    /* v0.31 patch 8 + v0.32: hint reflects the global input router.
     * R = Resource Fabric (always-on system shelf).  P = personality
     * override (always-on demo control).  Tab/Esc/Ctrl-W = WM keys.
     * Letter launchers fire only on bare desktop. */
    const char *hint =
        "[R]es [P]ers   [Tab]/[Esc]/[^W]   "
        "[D] [W] [S] | [A] [M] [T] [F] [N] | [C] [K] [G] [B] [H]";
    draw_text(hint_x, ty, hint, 1, ATOMIK_FG_DIM);

    /* Center: agent prediction (violet = AGENT in the semantic grammar).
     * The previous version used ATOMIK_ACCENT (cyan), which conflicted
     * with the hardware-meaning of cyan.  Switching to violet makes
     * "agent reasoning" instantly distinguishable from system-state
     * chrome at a glance. */
    action_t pred = agent_predict();
    if (pred != ACT_NONE) {
        char buf[80];
        /* ASCII '>>' instead of '▸' (U+25B8): the bitmap font is 8-bit
         * single-byte and renders multibyte UTF-8 codepoints as '????'
         * placeholder boxes — user-confirmed 2026-05-07. */
        snprintf(buf, sizeof buf, ">> %s", agent_action_name(pred));
        int tw = text_width(buf, 1);
        draw_text((FB_W - tw) / 2, ty, buf, 1, ATOMIK_SEM_AGENT);
    }

    /* Right: wallet/spend (violet — agent activity) || cpu/uptime (cyan
     * — hardware) || version (dim).  Three-segment design lets you read
     * the bar at a glance: agent-cost stuff = violet, hardware-status
     * stuff = cyan, meta = dim.  v0.30 will turn the wallet/spend
     * segment into a chevron-pulse while an LLM dispatch is in-flight
     * (Humane-Pin failure-avoidance rule: never hide active state).
     * Today dispatch is synchronous-blocking so the pulse can't fire
     * — wait for async dispatch in v0.30. */
    char up[64];
    format_uptime(up, sizeof up);
    const wallet_state_t *w = wallet_get();
    int spend_uusd = llm_audit_total_uusd();

    char hw_seg[80];
    snprintf(hw_seg, sizeof hw_seg, "cpu %3d%%   %s", s_cpu_pct, up);
    char ai_seg[80];
    snprintf(ai_seg, sizeof ai_seg, "wallet $%d.%02d   spent $%d.%03d",
             w->balance_uusd / 1000000,
             (w->balance_uusd / 10000) % 100,
             spend_uusd / 1000000,
             (spend_uusd / 1000) % 1000);
    int hw_w = text_width(hw_seg, 1);
    int ai_w = text_width(ai_seg, 1);
    int ver_w = text_width(AOS_VERSION, 1);
    int sep   = ATOMIK_GRID_M * 2;
    int total = ai_w + sep + hw_w + sep + ver_w;
    int rx    = FB_W - ATOMIK_GRID_L - total;
    draw_text(rx,                              ty, ai_seg,      1, ATOMIK_SEM_AGENT);
    draw_text(rx + ai_w + sep,                 ty, hw_seg,      1, ATOMIK_SEM_HARDWARE);
    draw_text(rx + ai_w + sep + hw_w + sep,    ty, AOS_VERSION, 1, ATOMIK_FG_DIM);
}
