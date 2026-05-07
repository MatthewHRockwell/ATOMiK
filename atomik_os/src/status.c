/* status.c — top status bar.
 *
 * Reads /proc/uptime for clock display (since this rootfs has no RTC),
 * /proc/stat for CPU usage, and surfaces the agent prediction. Renders
 * along the very top edge of the screen above the desktop. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

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

    /* Subtle top bar — height 32 (= 8 * GRID_S, on grid).  Text
     * vertically centered: y = (32 - text_height(1)) / 2.  v0.25
     * collapses the prior 36-px bar to grid-compliant 32. */
    const int bar_h = 32;
    const int ty    = (bar_h - text_height(1)) / 2;
    draw_rect(0, 0, FB_W, bar_h, rgba(0x10, 0x14, 0x1E, 0xC0) & 0xFFFFFF);

    /* Left: ATOMiK identity wordmark (cyan = HARDWARE, the "this is
     * the OS chrome itself" signal in our semantic grammar) followed by
     * key hints in dim foreground.  Separating the wordmark with grid-S
     * gives the bar a visible "branded" anchor instead of hint-soup. */
    const char *brand = "ATOMiK";
    int brand_x = ATOMIK_GRID_L;
    draw_text(brand_x, ty, brand, 1, ATOMIK_SEM_HARDWARE);
    int hint_x = brand_x + text_width(brand, 1) + ATOMIK_GRID_M * 2;
    const char *hint =
        "[R]esource  [D]oc  [W]allet  -  sys [A][M][T][F][N]  "
        "edge [C][K][G][B][H]  -  [Tab] [Esc] [Q]";
    draw_text(hint_x, ty, hint, 1, ATOMIK_FG_DIM);

    /* Center: agent prediction (violet = AGENT in the semantic grammar).
     * The previous version used ATOMIK_ACCENT (cyan), which conflicted
     * with the hardware-meaning of cyan.  Switching to violet makes
     * "agent reasoning" instantly distinguishable from system-state
     * chrome at a glance. */
    action_t pred = agent_predict();
    if (pred != ACT_NONE) {
        char buf[80];
        snprintf(buf, sizeof buf, "▸ %s", agent_action_name(pred));
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
