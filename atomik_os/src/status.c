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
    draw_text(cur_x, ty, brand, 1, ATOMIK_SEM_HARDWARE);
    cur_x += text_width(brand, 1) + ATOMIK_GRID_M;

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
        snprintf(data_badge, sizeof data_badge, "DATA: %s",
                 metric_source_label(worst));
        pixel_t dcol = metric_source_color(worst);
        /* Filled dot for visual lock-on. */
        int dot_y3 = bar_y + (bar_h - ATOMIK_GRID_M) / 2;
        draw_rect(cur_x, dot_y3, ATOMIK_GRID_M, ATOMIK_GRID_M, dcol);
        cur_x += ATOMIK_GRID_M + ATOMIK_GRID_S;
        draw_text(cur_x, ty, data_badge, 1, dcol);
        cur_x += text_width(data_badge, 1) + ATOMIK_GRID_M;
        draw_rect(cur_x, bar_y + ATOMIK_GRID_S,
                  1, bar_h - ATOMIK_GRID_S * 2, ATOMIK_DOCK_BORDER);
        cur_x += ATOMIK_GRID_M + 1;
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
        int dot_y2 = bar_y + (bar_h - ATOMIK_GRID_M) / 2;
        draw_rect(cur_x, dot_y2, ATOMIK_GRID_M, ATOMIK_GRID_M, pcol);
        cur_x += ATOMIK_GRID_M + ATOMIK_GRID_S;
        draw_text(cur_x, ty, pbadge, 1, pcol);
        cur_x += text_width(pbadge, 1) + ATOMIK_GRID_M;
    }

    /* === Last-batch headline (mini-readout) ===
     *
     * v0.34-B: pulls the most recent perf_sample_t and shows the
     * personality's headline number.  STATE shows ops collapsed,
     * SYNC shows bytes avoided, AGENT shows hot/cold split.  When no
     * sample exists (process just started, no batch run) shows nothing
     * — the personality badge alone is enough chrome.
     *
     * Honest UI rule (ChatGPT): never fake a number.  When perf_last is
     * NULL or zero, drop the readout entirely.
     */
    {
        const perf_sample_t *s = perf_last_sample();
        if (s && s->ops_logical > 0) {
            char mini[32];
            mini[0] = 0;
            switch (s->active_personality) {
            case PERSONALITY_STATE:
                snprintf(mini, sizeof mini, "%u-%u ops",
                         (unsigned)s->ops_logical,
                         (unsigned)s->ops_issued);
                break;
            case PERSONALITY_SYNC:
                if (s->bytes_avoided > 0) {
                    snprintf(mini, sizeof mini, "%uB avoided",
                             (unsigned)s->bytes_avoided);
                } else {
                    snprintf(mini, sizeof mini, "%u/%u emitted",
                             (unsigned)s->ops_issued,
                             (unsigned)s->regions_unique);
                }
                break;
            case PERSONALITY_AGENT:
                snprintf(mini, sizeof mini, "%u hot",
                         (unsigned)s->ops_issued);
                break;
            default: break;
            }
            if (mini[0]) {
                draw_text(cur_x, ty, mini, 1, ATOMIK_FG_DIM);
                cur_x += text_width(mini, 1) + ATOMIK_GRID_M;
            }
        }
    }

    /* Separator before window strip + hints. */
    draw_rect(cur_x, bar_y + ATOMIK_GRID_S,
              1, bar_h - ATOMIK_GRID_S * 2, ATOMIK_DOCK_BORDER);
    cur_x += ATOMIK_GRID_M + 1;

    /* === Window strip (v0.31): one dot per open window ===
     * Focused = filled cyan; buried = hollow outline.  Lets the user
     * see buried windows even when fully covered. */
    int dot_size  = ATOMIK_GRID_M;
    int dot_gap   = ATOMIK_GRID_S;
    int dot_y     = bar_y + (bar_h - dot_size) / 2;
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

    /* Key hints — dim, last in the left segment because they're
     * informational (which keys exist) not state (which one is
     * currently happening). */
    const char *hint =
        "[R]es [P]ers   [Tab]/[Esc]/[^W]   "
        "[D] [W] [S] | [A] [M] [T] [F] [N]";
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
