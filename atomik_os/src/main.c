/* main.c — ATOMiK OS v0.1 entry point.
 *
 * Brings up the framebuffer, draws the desktop (wallpaper + dock), and adds
 * a window manager that hosts floating app windows over the desktop. */
#include "atomik_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int  s_running    = 1;
static int  s_dock_hover = -1;

void about_draw(window_t *w, int x, int y, int wd, int ht);    /* about.c */
void monitor_draw(window_t *w, int x, int y, int wd, int ht);  /* monitor.c */

static void redraw_frame(void) {
    /* Bottom-up: wallpaper, dock, then windows on top. */
    wallpaper_draw();
    dock_draw(s_dock_hover);

    char buf[64];
    snprintf(buf, sizeof buf, "ATOMiK OS v0.1  \xb7  HDMI 1920x1080  \xb7  RV64GC");
    int sw = text_width(buf, 1);
    draw_text(FB_W - sw - 20, 18, buf, 1, ATOMIK_FG_DIM);

    /* Hint footer about keys */
    const char *hint = "[A] About   [M] Monitor   [Tab] cycle   [Esc] close   [Q] quit";
    draw_text(20, 18, hint, 1, ATOMIK_FG_DIM);

    /* Agent prediction surface — top-center. */
    action_t pred = agent_predict();
    if (pred != ACT_NONE) {
        char buf[80];
        snprintf(buf, sizeof buf, "next likely: %s", agent_action_name(pred));
        int tw = text_width(buf, 1);
        draw_text((FB_W - tw) / 2, 18, buf, 1, ATOMIK_ACCENT);
    }

    wm_draw_all();
    fb_present();
}

static void open_about(void) {
    int ww = 720, wh = 540;
    int wx = (FB_W - ww) / 2;
    int wy = (FB_H - wh) / 2 - 80;
    wm_open("About ATOMiK OS", wx, wy, ww, wh, about_draw, NULL);
}

static void open_monitor(void) {
    int ww = 980, wh = 660;
    int wx = (FB_W - ww) / 2 + 40;     /* offset so it doesn't fully cover About */
    int wy = (FB_H - wh) / 2 - 40;
    wm_open("ATOMiK Monitor", wx, wy, ww, wh, monitor_draw, NULL);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    if (fb_open() < 0) { fprintf(stderr, "fb_open failed\n"); return 1; }
    fb_clear(0);
    fb_present();
    fb_enable_scanout(1);

    font_init();
    input_open();
    wm_init();
    agent_init();
    atomik_open();   /* /dev/mem map for the live monitor; non-fatal if it fails */

    /* Open the About window automatically so first-launch shows the WM
     * working without requiring a key press. */
    open_about();
    redraw_frame();

    while (s_running) {
        event_t ev = input_poll(50);
        if (ev.kind == EV_QUIT) { s_running = 0; break; }
        if (ev.kind == EV_KEY) {
            int dirty = 0;

            /* Window manager gets first crack at the key (Tab, Esc, etc). */
            if (wm_handle_key(ev.key)) {
                /* Agent log: WM-consumed keys count as cycle/close. */
                if (ev.key == '\t')      agent_log(ACT_CYCLE_FOCUS);
                else if (ev.key == 0x1B) agent_log(ACT_CLOSE_WINDOW);
                dirty = 1;
            } else if (ev.key == 'a' || ev.key == 'A') {
                open_about();
                agent_log(ACT_OPEN_ABOUT);
                dirty = 1;
            } else if (ev.key == 'm' || ev.key == 'M') {
                open_monitor();
                agent_log(ACT_OPEN_MONITOR);
                dirty = 1;
            } else if (ev.key >= '1' && ev.key < '1' + dock_count()) {
                s_dock_hover = ev.key - '1';
                agent_log(ACT_DOCK_HOVER);
                dirty = 1;
            } else if (ev.key == ' ') {
                int n = dock_count();
                s_dock_hover = (s_dock_hover + 1) % n;
                agent_log(ACT_DOCK_HOVER);
                dirty = 1;
            }

            if (dirty) redraw_frame();
        }
    }

    fb_clear(ATOMIK_BG_BOT);
    fb_present();
    atomik_close();
    input_close();
    fb_close();
    return 0;
}
