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
/* terminal_draw, terminal_send_key, terminal_start declared in atomik_os.h */

static void redraw_frame(void) {
    /* Bottom-up: wallpaper, dock, then windows on top. */
    wallpaper_draw();
    dock_draw(s_dock_hover);

    char buf[64];
    snprintf(buf, sizeof buf, "ATOMiK OS v0.1  \xb7  HDMI 1920x1080  \xb7  RV64GC");
    int sw = text_width(buf, 1);
    draw_text(FB_W - sw - 20, 18, buf, 1, ATOMIK_FG_DIM);

    /* Hint footer about keys */
    const char *hint = "[A] About   [M] Monitor   [T] Terminal   [F] Files   [Tab] cycle   [Esc] close   [Q] quit";
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

static int s_terminal_id = 0;
static void open_terminal(void) {
    if (s_terminal_id) { wm_focus(s_terminal_id); return; }
    if (terminal_start() < 0) return;
    int ww = 880, wh = 560;
    int wx = (FB_W - ww) / 2 - 40;
    int wy = (FB_H - wh) / 2 + 20;
    window_t *w = wm_open("Terminal", wx, wy, ww, wh, terminal_draw, NULL);
    if (w) s_terminal_id = w->id;
}

static int s_files_id = 0;
static void open_files(void) {
    if (s_files_id) { wm_focus(s_files_id); return; }
    files_open();
    int ww = 760, wh = 520;
    int wx = (FB_W - ww) / 2 + 60;
    int wy = (FB_H - wh) / 2 - 60;
    window_t *w = wm_open("Files", wx, wy, ww, wh, files_draw, NULL);
    if (w) s_files_id = w->id;
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

            /* Window manager gets first crack at WM keys (Tab, Esc). */
            if (wm_handle_key(ev.key)) {
                if (ev.key == '\t')      agent_log(ACT_CYCLE_FOCUS);
                else if (ev.key == 0x1B) agent_log(ACT_CLOSE_WINDOW);
                dirty = 1;
            }
            /* If terminal is focused, all other keys feed the pty. */
            else if (s_terminal_id) {
                window_t *top = wm_topmost();
                if (top && top->id == s_terminal_id) {
                    terminal_send_key(ev.key);
                    dirty = 1;
                }
            }
            /* If files is focused, route navigation keys there. */
            if (!dirty && s_files_id) {
                window_t *top = wm_topmost();
                if (top && top->id == s_files_id) {
                    files_handle_key(ev.key);
                    dirty = 1;
                }
            }
            /* Global app shortcuts (only reach here if WM didn't consume
             * AND terminal isn't focused). */
            if (!dirty && (ev.key == 'a' || ev.key == 'A')) {
                open_about();
                agent_log(ACT_OPEN_ABOUT);
                dirty = 1;
            } else if (!dirty && (ev.key == 'm' || ev.key == 'M')) {
                open_monitor();
                agent_log(ACT_OPEN_MONITOR);
                dirty = 1;
            } else if (!dirty && (ev.key == 't' || ev.key == 'T')) {
                open_terminal();
                agent_log(ACT_OPEN_TERMINAL);
                dirty = 1;
            } else if (!dirty && (ev.key == 'f' || ev.key == 'F')) {
                open_files();
                agent_log(ACT_OPEN_FILES);
                dirty = 1;
            } else if (!dirty && ev.key >= '1' && ev.key < '1' + dock_count()) {
                /* Number key launches whatever app is currently in that
                 * dock slot. The slot meaning shifts as the agent
                 * reorders icons by predicted relevance. */
                int slot = ev.key - '1';
                s_dock_hover = slot;
                action_t a = dock_action_for_slot(slot);
                if      (a == ACT_OPEN_ABOUT)    open_about();
                else if (a == ACT_OPEN_MONITOR)  open_monitor();
                else if (a == ACT_OPEN_TERMINAL) open_terminal();
                else if (a == ACT_OPEN_FILES)    open_files();
                if (a != ACT_NONE) agent_log(a);
                else               agent_log(ACT_DOCK_HOVER);
                dirty = 1;
            } else if (!dirty && ev.key == ' ') {
                int n = dock_count();
                s_dock_hover = (s_dock_hover + 1) % n;
                agent_log(ACT_DOCK_HOVER);
                dirty = 1;
            }

            /* While the terminal is focused, repaint at least every poll
             * cycle so newly-arrived pty output appears even without a
             * keypress. */
            if (s_terminal_id) {
                window_t *top = wm_topmost();
                if (top && top->id == s_terminal_id) dirty = 1;
            }

            if (dirty) redraw_frame();
        }
    }

    fb_clear(ATOMIK_BG_BOT);
    fb_present();
    agent_flush();      /* persist user-model on clean exit */
    atomik_close();
    input_close();
    fb_close();
    return 0;
}
