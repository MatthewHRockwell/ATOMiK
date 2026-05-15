/* monitor.c — System Monitor app.
 *
 * Renders the 8 ATOMiK delta-slot accumulators as colored bars + their
 * current 32-bit hex value. Subsequent versions will overlay rate-of-change,
 * historical timeline, and per-slot virtual-processor labels (see the
 * dynamic-vproc roadmap parked in project_atomik_os_design.md). */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

static const pixel_t SLOT_COLORS[ATOMIK_N_SLOTS] = {
    rgb(0x4F, 0xC3, 0xFF), /* cyan       */
    rgb(0x6E, 0xC4, 0x6E), /* green      */
    rgb(0xFF, 0xCB, 0x4A), /* amber      */
    rgb(0xFF, 0x6F, 0x91), /* pink       */
    rgb(0xB388, 0xFF, 0xFF) & 0xFFFFFF, /* lavender (mask) */
    rgb(0x80, 0xDE, 0xEA), /* aqua       */
    rgb(0xFF, 0x90, 0x4A), /* orange     */
    rgb(0xC8, 0xC8, 0xC8), /* gray       */
};

static uint32_t s_last[ATOMIK_N_SLOTS];

void monitor_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    /* Background */
    draw_rect(x, y, wd, ht, rgb(0x16, 0x1C, 0x2A));

    /* Header */
    draw_text(x + 24, y + 16, "ATOMiK Adapter -- Live Slot State",
              2, ATOMIK_FG);
    draw_text(x + 24, y + 50,
              "Reading /dev/mem @ 0xF0020000  (8 delta-state accumulators)",
              1, ATOMIK_FG_DIM);

    /* Read live values */
    uint32_t v[ATOMIK_N_SLOTS] = {0};
    int ok = atomik_read_slots(v);

    int row_h     = 56;
    int row_y     = y + 96;
    int label_x   = x + 24;
    int bar_x     = x + 120;
    int bar_w     = wd - 220;
    int hex_x     = x + wd - 96 - 8;

    for (int i = 0; i < ATOMIK_N_SLOTS; i++) {
        int yy = row_y + i * row_h;
        char label[16];
        snprintf(label, sizeof label, "slot %d", i);
        draw_text(label_x, yy + 16, label, 2, ATOMIK_FG_DIM);

        /* Bar background */
        draw_rect_rounded(bar_x, yy + 12, bar_w, 24, 6,
                          rgb(0x22, 0x2A, 0x3A));

        /* Bar fill — width proportional to popcount of the 32-bit value
         * (a quick visual hash of activity in the slot, capped at 32). */
        int pop = __builtin_popcount(v[i]);
        int fill = (bar_w - 4) * pop / 32;
        if (fill < 0) fill = 0;
        if (fill > bar_w - 4) fill = bar_w - 4;
        draw_rect_rounded(bar_x + 2, yy + 14, fill, 20, 5, SLOT_COLORS[i]);

        /* Change indicator: a small filled circle if the slot value changed
         * since last draw. */
        if (s_last[i] != v[i]) {
            for (int dy = -3; dy <= 3; dy++)
                for (int dx = -3; dx <= 3; dx++)
                    if (dx*dx + dy*dy <= 9)
                        draw_pixel(bar_x + bar_w + 18 + dx, yy + 24 + dy,
                                   ATOMIK_ACCENT);
        }
        s_last[i] = v[i];

        /* Hex value text */
        char hex[16];
        snprintf(hex, sizeof hex, "0x%08x", v[i]);
        draw_text(hex_x, yy + 16, hex, 2, ATOMIK_FG);
    }

    /* Footer */
    const char *footer = ok == 0
        ? "delta accumulators sampled OK  /  XOR-commutative, lock-free"
        : "delta accumulators NOT mapped (need root for /dev/mem)";
    draw_text(x + 24, y + ht - 28, footer, 1,
              ok == 0 ? ATOMIK_ACCENT : rgb(0xFF, 0x6F, 0x91));
}
