/* notify.c — toast notifications.
 *
 * Single visible toast at a time. Auto-dismisses after NOTIFY_LIFETIME_MS.
 * Used by the OS to surface short status messages (file saved, app
 * launched, etc.). Renders on top of windows but under the dock. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

#define NOTIFY_LIFETIME_MS  3000
#define NOTIFY_FADE_MS      300

static char          s_text[160] = {0};
static unsigned long s_posted_ms = 0;

void notify_post(const char *text) {
    if (!text) return;
    snprintf(s_text, sizeof s_text, "%s", text);
    s_posted_ms = anim_now_ms();
    anim_tick();
}

void notify_draw(void) {
    if (s_text[0] == 0) return;
    unsigned long age = anim_now_ms() - s_posted_ms;
    if (age >= NOTIFY_LIFETIME_MS) { s_text[0] = 0; return; }

    /* Slide-in from the right edge with a small ease-out. */
    int    target_w = 360;
    int    box_h    = 56;
    int    box_y    = 80;
    double in_t     = age < NOTIFY_FADE_MS ? (double)age / NOTIFY_FADE_MS : 1.0;
    double out_t    = 1.0;
    if (age > NOTIFY_LIFETIME_MS - NOTIFY_FADE_MS) {
        out_t = (double)(NOTIFY_LIFETIME_MS - age) / NOTIFY_FADE_MS;
        if (out_t < 0.0) out_t = 0.0;
    }
    int slide_x = FB_W - target_w - 24 -
                  (int)((1.0 - anim_ease_out(in_t)) * (target_w + 32));

    /* Body */
    draw_rect_rounded(slide_x, box_y, target_w, box_h, 12,
                      rgb(0x1A, 0x22, 0x32));
    /* Cyan accent stripe */
    draw_rect_rounded(slide_x, box_y, 4, box_h, 2, ATOMIK_ACCENT);
    /* Text — alpha approximated by reducing brightness during out_t */
    pixel_t fg = ATOMIK_FG;
    if (out_t < 1.0) {
        uint8_t r = (uint8_t)(((fg >> 16) & 0xff) * out_t);
        uint8_t g = (uint8_t)(((fg >>  8) & 0xff) * out_t);
        uint8_t b = (uint8_t)(((fg >>  0) & 0xff) * out_t);
        fg = rgb(r, g, b);
    }
    draw_text(slide_x + 18, box_y + (box_h - text_height(1)) / 2,
              s_text, 1, fg);

    anim_tick();
}
