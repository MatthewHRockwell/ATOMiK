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

    /* Subtle top bar */
    draw_rect(0, 0, FB_W, 36, rgba(0x10, 0x14, 0x1E, 0xC0) & 0xFFFFFF);

    /* Left: app keys hint */
    const char *hint =
        "[D]ocument <- the pitch  -  sys [A][M][T][F][N]  "
        "edge [C][K][G][B][H]  -  [Tab] [Esc] [Q]";
    draw_text(20, 12, hint, 1, ATOMIK_FG_DIM);

    /* Center: agent prediction */
    action_t pred = agent_predict();
    if (pred != ACT_NONE) {
        char buf[80];
        snprintf(buf, sizeof buf, "next likely: %s", agent_action_name(pred));
        int tw = text_width(buf, 1);
        draw_text((FB_W - tw) / 2, 12, buf, 1, ATOMIK_ACCENT);
    }

    /* Right: cpu + uptime */
    char up[64];
    format_uptime(up, sizeof up);
    char right[128];
    snprintf(right, sizeof right, "cpu %3d%%   %s   ATOMiK OS v0.9", s_cpu_pct, up);
    int rw = text_width(right, 1);
    draw_text(FB_W - rw - 20, 12, right, 1, ATOMIK_FG);
}
