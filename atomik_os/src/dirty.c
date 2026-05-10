/* dirty.c — v0.38-A tile-based dirty-region tracker.
 *
 * Per ChatGPT 2026-05-09 perf directive: the OS itself uses ATOMiK's
 * change-awareness on its own framebuffer.  This module is the
 * measurement layer: every UI component declares the rectangle it
 * touches each frame, and we turn that into a dirty-tile bitmap +
 * per-frame stats published to the metric provider.
 *
 * v0.38-A ships measurement only.  Even un-optimized, the VISUAL
 * Resource Fabric lane becomes truthful (was EVT_VIS_RENDER count
 * proxy).  v0.38-B will use the same bitmap to actually skip clean
 * tiles in the wallpaper memcpy and window blits, turning the
 * measurement into a real speedup.
 *
 * Tile geometry: 32×32 px.  At 1920×1080 that's a 60×34 grid =
 * 2040 tiles.  One byte per tile = 2 KB bitmap, fits trivially in
 * cache.  `dirty[ty][tx]` is 0 (clean) or 1 (dirty).  We avoid
 * bit-packing because byte stores are cheap on RV64 and the array
 * size is tiny. */
#include "atomik_os.h"
#include <string.h>

static uint8_t        s_dirty[DIRTY_TILES_Y][DIRTY_TILES_X];
static int            s_count_now      = 0;
static int            s_count_last     = 0;
static unsigned long  s_frame_count    = 0;

/* Running averages over a sliding window so the metric isn't pinned
 * to whatever the most recent frame happened to do.  Cheap EMA with
 * alpha = 1/8 — converges in ~8 frames at typical 16 ms cadence. */
static double         s_avg_dirty       = 0.0;
static double         s_avg_pct_avoided = 0.0;

void dirty_init(void) {
    memset(s_dirty, 0, sizeof s_dirty);
    s_count_now      = 0;
    s_count_last     = 0;
    s_frame_count    = 0;
    s_avg_dirty       = 0.0;
    s_avg_pct_avoided = 0.0;
}

void dirty_clear(void) {
    if (s_count_now == 0) return;
    memset(s_dirty, 0, sizeof s_dirty);
    s_count_now = 0;
}

void dirty_all(void) {
    memset(s_dirty, 1, sizeof s_dirty);
    s_count_now = DIRTY_TILES_TOTAL;
}

void dirty_rect(int x, int y, int w, int h) {
    /* Clip to framebuffer bounds. */
    if (w <= 0 || h <= 0) return;
    if (x < 0)         { w += x; x = 0; }
    if (y < 0)         { h += y; y = 0; }
    if (x + w > FB_W)  { w  = FB_W - x; }
    if (y + h > FB_H)  { h  = FB_H - y; }
    if (w <= 0 || h <= 0) return;

    /* Convert to tile coords (inclusive both ends). */
    int tx0 = x / DIRTY_TILE_W;
    int ty0 = y / DIRTY_TILE_H;
    int tx1 = (x + w - 1) / DIRTY_TILE_W;
    int ty1 = (y + h - 1) / DIRTY_TILE_H;
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    if (tx1 >= DIRTY_TILES_X) tx1 = DIRTY_TILES_X - 1;
    if (ty1 >= DIRTY_TILES_Y) ty1 = DIRTY_TILES_Y - 1;

    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            if (!s_dirty[ty][tx]) {
                s_dirty[ty][tx] = 1;
                s_count_now++;
            }
        }
    }
}

void dirty_finalize_frame(void) {
    s_count_last = s_count_now;
    s_frame_count++;

    double dirty_d = (double)s_count_last;
    double pct_avoided =
        100.0 * (DIRTY_TILES_TOTAL - s_count_last) /
                 (double)DIRTY_TILES_TOTAL;

    /* EMA(alpha=1/8) on the per-frame stats so the published metric
     * reflects steady-state behavior, not just the most recent paint. */
    if (s_frame_count == 1) {
        s_avg_dirty       = dirty_d;
        s_avg_pct_avoided = pct_avoided;
    } else {
        s_avg_dirty       = (s_avg_dirty       * 7 + dirty_d)     / 8;
        s_avg_pct_avoided = (s_avg_pct_avoided * 7 + pct_avoided) / 8;
    }
}

int dirty_count(void)            { return s_count_now;  }
int dirty_count_last_frame(void) { return s_count_last; }
int dirty_total(void)            { return DIRTY_TILES_TOTAL; }
unsigned long dirty_frame_count(void) { return s_frame_count; }

/* === metric-provider getters ===
 *
 * Exposed via internal accessors so the registry can pull them in
 * metric_init().  Each returns LIVE once at least one frame has been
 * finalized; WAITING before the first frame. */
double dirty_metric_tiles_dirty(void) {
    return s_avg_dirty;
}
double dirty_metric_tiles_avoided(void) {
    return (double)DIRTY_TILES_TOTAL - s_avg_dirty;
}
double dirty_metric_pct_avoided(void) {
    return s_avg_pct_avoided;
}
double dirty_metric_frame_count(void) {
    return (double)s_frame_count;
}
