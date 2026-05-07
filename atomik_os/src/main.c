/* main.c — ATOMiK OS v0.1 entry point.
 *
 * Brings up the framebuffer, draws the desktop (wallpaper + dock), and adds
 * a window manager that hosts floating app windows over the desktop. */
#include "atomik_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

static int  s_running    = 1;
static int  s_dock_hover = -1;

void about_draw(window_t *w, int x, int y, int wd, int ht);    /* about.c */
void monitor_draw(window_t *w, int x, int y, int wd, int ht);  /* monitor.c */
/* terminal_draw, terminal_send_key, terminal_start declared in atomik_os.h */

static void redraw_frame(void) {
    /* v0.31 patch 3 (ChatGPT review 2026-05-07): status_draw() now fires
     * AFTER wm_draw_all() so the status bar is global chrome layered on
     * top of every window, not painted underneath them.  Previously, any
     * window straying into y=0..31 (or any future bug that did) would
     * silently overdraw the bar.  Defensive z-layer discipline: chrome
     * always wins. */
    wallpaper_draw();
    dock_draw(s_dock_hover);
    wm_draw_all();
    status_draw();      /* global chrome — drawn after windows */
    notify_draw();
    fb_present();
}

/* v0.31: workspace_w returns the left-of-shelf width — the column
 * where normal app windows live.  Resource Fabric occupies the right
 * shelf as a pinned system panel; other apps center themselves
 * horizontally INSIDE the workspace so they don't overlap the shelf
 * even when Fabric isn't open yet.  This encodes the layout
 * convention: left = workspace, right = system shelf. */
static int workspace_w(void) { return fabric_shelf_x() - ATOMIK_GRID_L; }
static int workspace_cx(int ww) { return (workspace_w() - ww) / 2; }

static int s_about_id = 0;
static void open_about(void) {
    if (s_about_id) { wm_focus(s_about_id); return; }
    int ww = 720, wh = 540;
    int wx = workspace_cx(ww);
    int wy = (FB_H - wh) / 2 - 80;
    window_t *w = wm_open("About ATOMiK OS", wx, wy, ww, wh, about_draw, NULL);
    if (w) s_about_id = w->id;
}

static int s_monitor_id = 0;
static void open_monitor(void) {
    if (s_monitor_id) { wm_focus(s_monitor_id); return; }
    int ww = 980, wh = 660;            /* restored: fits in 1424-px workspace */
    int wx = workspace_cx(ww) + 40;    /* offset so it doesn't fully cover About */
    int wy = (FB_H - wh) / 2 - 40;
    window_t *w = wm_open("ATOMiK Monitor", wx, wy, ww, wh, monitor_draw, NULL);
    if (w) s_monitor_id = w->id;
}

static int s_terminal_id = 0;
static void open_terminal(void) {
    if (s_terminal_id) { wm_focus(s_terminal_id); return; }
    if (terminal_start() < 0) return;
    int ww = 880, wh = 560;
    int wx = workspace_cx(ww) - 40;
    int wy = (FB_H - wh) / 2 + 20;
    window_t *w = wm_open("Terminal", wx, wy, ww, wh, terminal_draw, NULL);
    if (w) s_terminal_id = w->id;
}

static int s_files_id = 0;
static void open_files(void) {
    if (s_files_id) { wm_focus(s_files_id); return; }
    files_open();
    int ww = 760, wh = 520;
    int wx = workspace_cx(ww) + 60;
    int wy = (FB_H - wh) / 2 - 60;
    window_t *w = wm_open("Files", wx, wy, ww, wh, files_draw, NULL);
    if (w) s_files_id = w->id;
    notify_post("Files opened");
}

static int s_notes_id = 0;
static void open_notes(void) {
    if (s_notes_id) { wm_focus(s_notes_id); return; }
    notes_open();
    int ww = 700, wh = 540;
    int wx = workspace_cx(ww) - 80;
    int wy = (FB_H - wh) / 2 + 40;
    window_t *w = wm_open("Notes", wx, wy, ww, wh, notes_draw, NULL);
    if (w) s_notes_id = w->id;
    notify_post("Notes opened");
}

/* Edge-app windows. NO per-app open code beyond a wm_open() — all three
 * share eapp_draw(). The rendering, layout, theming come from the
 * invariant frame. */
static int s_cal_id = 0, s_task_id = 0, s_code_id = 0;
static void open_calendar(void) {
    if (s_cal_id) { wm_focus(s_cal_id); return; }
    int ww = 1080, wh = 620;           /* restored: fits in 1424-px workspace */
    int wx = workspace_cx(ww);
    int wy = (FB_H - wh) / 2 - 60;
    window_t *w = wm_open("Calendar (edge app)", wx, wy, ww, wh,
                          edge_calendar_draw, NULL);
    if (w) s_cal_id = w->id;
    notify_post("Calendar streamed");
}
static void open_tasks(void) {
    if (s_task_id) { wm_focus(s_task_id); return; }
    int ww = 720, wh = 600;
    int wx = workspace_cx(ww) - 100;
    int wy = (FB_H - wh) / 2 - 30;
    window_t *w = wm_open("Tasks (edge app)", wx, wy, ww, wh,
                          edge_tasks_draw, NULL);
    if (w) s_task_id = w->id;
    notify_post("Tasks streamed");
}
static void open_code(void) {
    if (s_code_id) { wm_focus(s_code_id); return; }
    int ww = 880, wh = 580;
    int wx = workspace_cx(ww) + 60;
    int wy = (FB_H - wh) / 2 + 20;
    window_t *w = wm_open("Code (edge app)", wx, wy, ww, wh,
                          edge_code_draw, NULL);
    if (w) s_code_id = w->id;
    notify_post("Code streamed");
}

static int s_brief_id = 0, s_chat_id = 0;
static void open_brief(void) {
    if (s_brief_id) { wm_focus(s_brief_id); return; }
    int ww = 820, wh = 500;
    int wx = workspace_cx(ww);
    int wy = (FB_H - wh) / 2 - 40;
    window_t *w = wm_open("Brief (edge app)", wx, wy, ww, wh,
                          edge_brief_draw, NULL);
    if (w) s_brief_id = w->id;
    notify_post("Brief streamed");
}

static void open_chat(void) {
    if (s_chat_id) { wm_focus(s_chat_id); return; }
    int ww = 760, wh = 540;
    int wx = workspace_cx(ww) + 100;
    int wy = (FB_H - wh) / 2;
    window_t *w = wm_open("Chat (edge app)", wx, wy, ww, wh,
                          edge_chat_draw, NULL);
    if (w) s_chat_id = w->id;
    notify_post("Chat streamed");
}

/* The Document app — chat-driven UI morphing. v0.13: every press of D
 * opens an INDEPENDENT Document instance. Each has its own state file,
 * its own chat history, its own edge_app_t. The WM tiles them with a
 * cascading offset so they don't fully overlap. */
#define MAX_DOCS 6
static int  s_n_docs = 0;
static int  s_doc_win_ids[MAX_DOCS];
static void open_document(void) {
    if (s_n_docs >= MAX_DOCS) {
        notify_post("max documents reached");
        return;
    }
    doc_state_t *d = document_open_new();
    if (!d) { notify_post("doc alloc failed"); return; }
    int ww = 1100, wh = 660;           /* restored: fits in 1424-px workspace */
    int offset = s_n_docs * 40;
    int wx = workspace_cx(ww) - 80 + offset;
    int wy = (FB_H - wh) / 2 - 40 + offset;
    char title[40];
    snprintf(title, sizeof title, "Document #%d", s_n_docs + 1);
    window_t *w = wm_open(title, wx, wy, ww, wh, document_draw_for, d);
    if (w) {
        s_doc_win_ids[s_n_docs++] = w->id;
        notify_post("Document opened — type to morph");
    } else {
        document_close(d);
    }
}

static int s_stocks_id = 0;
static void open_stocks(void) {
    if (s_stocks_id) { wm_focus(s_stocks_id); return; }
    int ww = 760, wh = 540;
    int wx = workspace_cx(ww) - 60;
    int wy = (FB_H - wh) / 2 + 40;
    window_t *w = wm_open("Stocks (live deltas)", wx, wy, ww, wh,
                          edge_stocks_draw, NULL);
    if (w) s_stocks_id = w->id;
    notify_post("Stocks streaming - same chrome, fields move");
}

static int s_wallet_id = 0;
static void open_wallet(void) {
    if (s_wallet_id) { wm_focus(s_wallet_id); return; }
    int ww = 720, wh = 560;
    int wx = workspace_cx(ww);
    int wy = (FB_H - wh) / 2;
    window_t *w = wm_open("Wallet", wx, wy, ww, wh, wallet_draw, NULL);
    if (w) s_wallet_id = w->id;
    notify_post("Wallet — token-pay ledger");
}

/* Returns the doc_state_t for the currently focused Document window, or
 * NULL if no Document is focused. */
static doc_state_t *focused_doc(void) {
    window_t *top = wm_topmost();
    if (!top) return NULL;
    for (int i = 0; i < s_n_docs; i++) {
        if (top->id == s_doc_win_ids[i]) return (doc_state_t *)top->user;
    }
    return NULL;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    /* v0.31 patch 3 (ChatGPT review 2026-05-07): single-instance lock.
     *
     * Multiple atomik_os processes drawing to /dev/fb0 simultaneously
     * has been a recurring debug nightmare this session — every
     * "killall -9 atomik_os" iteration that left an orphan running
     * produced overlay garbage on screen.  Fixing it at the source:
     * acquire an exclusive flock on /tmp/atomik_os.lock at startup,
     * exit if a sibling already holds it.  Two atomik_os processes
     * fighting over the framebuffer is now architecturally impossible. */
    int lock_fd = open("/tmp/atomik_os.lock", O_CREAT | O_RDWR, 0644);
    if (lock_fd < 0) {
        fprintf(stderr, "atomik_os: cannot open lockfile: %s\n",
                strerror(errno));
        /* Continue anyway — better to draw than to fail. */
    } else if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        fprintf(stderr, "atomik_os: another instance is running, exiting\n");
        return 2;
    } else {
        /* Hold the lock for the lifetime of this process by leaking the
         * fd.  Will be released when the kernel reaps us. */
        char pidbuf[32];
        int n = snprintf(pidbuf, sizeof pidbuf, "%d\n", (int)getpid());
        if (write(lock_fd, pidbuf, (size_t)n) < 0) { /* ignore */ }
    }

    /* Stamp the running version + PID to known paths so the host can
     * verify the binary it just shipped is the one actually executing.
     * Without this, a stale process pinning /dev/fb0 would silently keep
     * showing an old build (the bug that motivated these stamps in the
     * first place). */
    FILE *vf = fopen("/tmp/atomik_os_version", "w");
    if (vf) { fputs(AOS_VERSION "\n", vf); fclose(vf); }
    FILE *pf = fopen("/tmp/atomik_os_pid", "w");
    if (pf) { fprintf(pf, "%d\n", (int)getpid()); fclose(pf); }

    if (fb_open() < 0) { fprintf(stderr, "fb_open failed\n"); return 1; }
    fb_clear(0);
    fb_present();
    fb_enable_scanout(1);

    font_init();
    input_open();
    wm_init();
    agent_init();
    wallet_init();
    atomik_open();   /* /dev/mem map for the live monitor; non-fatal if it fails */

    /* Open the About window automatically so first-launch shows the WM
     * working without requiring a key press. */
    open_about();
    redraw_frame();

    while (s_running) {
        /* Frame-loop: when an animation is active or the terminal is
         * focused (async pty output), poll fast (16ms = ~60Hz). Otherwise
         * idle at 100ms to save CPU. */
        int poll_ms = anim_active() ? 16 : 100;
        if (s_terminal_id) {
            window_t *top = wm_topmost();
            if (top && top->id == s_terminal_id) poll_ms = 16;
        }
        event_t ev = input_poll(poll_ms);
        /* v0.23: stocks ticker mutates one row's price every ~1 s.
         * Cheap call (no allocation, no I/O) — safe to invoke every
         * frame; it self-rate-limits via an internal counter. v0.23.1:
         * returns 1 when it actually changed something so we can
         * force a repaint. */
        int stocks_changed = stocks_tick();
        /* v0.30: tick the Resource Fabric once per frame so its
         * personality auto-detection re-classifies based on recent
         * LLM/state activity.  Cheap (no I/O, no allocation). */
        fabric_tick();
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
            /* If notes is focused, all printable keys edit the buffer. */
            if (!dirty && s_notes_id) {
                window_t *top = wm_topmost();
                if (top && top->id == s_notes_id) {
                    notes_handle_key(ev.key);
                    dirty = 1;
                }
            }
            /* Document app — chat panel takes typed commands.
             * v0.13: route to whichever Document window is focused. */
            if (!dirty) {
                doc_state_t *d = focused_doc();
                if (d) {
                    document_handle_key_for(d, ev.key);
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
            } else if (!dirty && (ev.key == 'n' || ev.key == 'N')) {
                open_notes();
                agent_log(ACT_OPEN_NOTES);
                dirty = 1;
            } else if (!dirty && (ev.key == 'c' || ev.key == 'C')) {
                open_calendar();
                agent_log(ACT_OPEN_CALENDAR);
                dirty = 1;
            } else if (!dirty && (ev.key == 'k' || ev.key == 'K')) {
                open_tasks();
                agent_log(ACT_OPEN_TASKS);
                dirty = 1;
            } else if (!dirty && (ev.key == 'g' || ev.key == 'G')) {
                open_code();
                agent_log(ACT_OPEN_CODE);
                dirty = 1;
            } else if (!dirty && (ev.key == 'b' || ev.key == 'B')) {
                open_brief();
                agent_log(ACT_OPEN_BRIEF);
                dirty = 1;
            } else if (!dirty && (ev.key == 'h' || ev.key == 'H')) {
                open_chat();
                agent_log(ACT_OPEN_CHAT);
                dirty = 1;
            } else if (!dirty && (ev.key == 'd' || ev.key == 'D')) {
                open_document();
                agent_log(ACT_OPEN_DOCUMENT);
                dirty = 1;
            } else if (!dirty && (ev.key == 'w' || ev.key == 'W')) {
                open_wallet();
                dirty = 1;
            } else if (!dirty && (ev.key == 's' || ev.key == 'S')) {
                open_stocks();
                dirty = 1;
            } else if (!dirty && (ev.key == 'r' || ev.key == 'R')) {
                /* v0.30: Resource Fabric — see fabric.c.  No agent_log
                 * action yet; will add ACT_OPEN_FABRIC in v0.31 once the
                 * agent layer learns to predict R alongside D/S/etc. */
                fabric_open();
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
                else if (a == ACT_OPEN_NOTES)    open_notes();
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
        } else {
            /* No event arrived. Redraw a frame if an animation is in
             * progress (window fade-in, predicted-icon pulse, etc.) OR if
             * the terminal is focused and might have new async pty output. */
            int need_frame = anim_active();
            if (!need_frame && s_terminal_id) {
                window_t *top = wm_topmost();
                if (top && top->id == s_terminal_id) need_frame = 1;
            }
            if (!need_frame && stocks_changed && s_stocks_id) need_frame = 1;
            if (need_frame) redraw_frame();
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
