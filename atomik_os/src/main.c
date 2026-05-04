/* main.c — ATOMiK OS v0 entry point.
 *
 * Bring up the framebuffer, draw an empty desktop (wallpaper + dock), and
 * pump input. Window manager + apps are next iterations. */
#include "atomik_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int  s_running    = 1;
static int  s_dock_hover = -1;

static void redraw_frame(void) {
    wallpaper_draw();
    dock_draw(s_dock_hover);

    /* Top-right system tray placeholder — clock string is good enough for v0
     * and proves the small text path renders. We use uptime since we don't
     * have a real RTC binding here yet. */
    char buf[64];
    snprintf(buf, sizeof buf, "ATOMiK OS v0  ·  HDMI 1920x1080  ·  RV64GC");
    int sw = text_width(buf, 1);
    draw_text(FB_W - sw - 20, 18, buf, 1, ATOMIK_FG_DIM);

    fb_present();
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    if (fb_open() < 0) { fprintf(stderr, "fb_open failed\n"); return 1; }
    fb_clear(0);
    fb_present();           /* avoid showing previous screen contents */
    fb_enable_scanout(1);   /* turn on VTG + DMA */

    font_init();
    input_open();

    /* Initial render. */
    redraw_frame();

    while (s_running) {
        event_t ev = input_poll(50);
        if (ev.kind == EV_QUIT) { s_running = 0; break; }
        if (ev.kind == EV_KEY) {
            /* Demo key handling: 1..6 highlights a dock icon. */
            if (ev.key >= '1' && ev.key < '1' + dock_count()) {
                s_dock_hover = ev.key - '1';
                redraw_frame();
            } else if (ev.key == ' ') {
                /* Cycle hover */
                int n = dock_count();
                s_dock_hover = (s_dock_hover + 1) % n;
                redraw_frame();
            }
        }
    }

    /* Clean exit: blank framebuffer, leave scanout on so caller still sees
     * something coherent. */
    fb_clear(ATOMIK_BG_BOT);
    fb_present();
    input_close();
    fb_close();
    return 0;
}
