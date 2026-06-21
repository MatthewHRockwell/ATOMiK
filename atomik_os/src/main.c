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

/* Representative recent-usage history so PREDICTIVE INSIGHTS shows real,
 * differentiated predictions.  The predictor is real (agent_score); this is a
 * demo warm-start, analogous to the Fabric's perf-sample seed. */
static void seed_demo_usage(void) {
    static const action_t seq[] = {
        ACT_OPEN_CHAT, ACT_OPEN_CODE, ACT_OPEN_TERMINAL, ACT_OPEN_DOCUMENT,
        ACT_OPEN_DOCUMENT, ACT_OPEN_CODE, ACT_OPEN_DOCUMENT, ACT_OPEN_DOCUMENT,
        ACT_OPEN_CODE, ACT_OPEN_DOCUMENT, ACT_OPEN_DOCUMENT, ACT_OPEN_DOCUMENT,
    };
    for (unsigned i = 0; i < sizeof(seq) / sizeof(seq[0]); i++) agent_log(seq[i]);
}

/* v0.42 pointer cursor — drawn via SAVE-UNDER so a mouse move is a true DELTA,
 * not a full-frame recomposite (the whole ATOMiK point: push what changed).
 * The background under the cursor is saved before the arrow is painted; moving
 * the cursor restores the old region and paints the new one, presenting only
 * those two ~10x18 rects.  Classic tilted arrow, white fill + dark edge.
 * Hotspot at the top-left tip.  No-op until a mouse device is present. */
#define CUR_W 10
#define CUR_H 18
static const char *CUR[CUR_H] = {
    "E         ", "EE        ", "EFE       ", "EFFE      ", "EFFFE     ",
    "EFFFFE    ", "EFFFFFE   ", "EFFFFFFE  ", "EFFFFFFFE ", "EFFFFFFFFE",
    "EFFFFEEEEE", "EFFEFFE   ", "EFE EFFE  ", "EE  EFFE  ", "E    EFFE ",
    "     EFFE ", "      EFFE", "      EEEE",
};
static pixel_t s_cur_save[CUR_W * CUR_H]; /* background under the composited cursor */
static int s_cur_sx = -1, s_cur_sy = -1;  /* where the cursor is currently composited */
static int s_cur_has_save = 0;
static unsigned long s_last_mouse_ms = 0; /* last pointer event — gates Atom's float */

static void cursor_paint(int x0, int y0) {
    pixel_t fillc = rgb(0xF0, 0xF6, 0xFF);
    pixel_t edge  = rgb(0x06, 0x0A, 0x14);
    for (int r = 0; r < CUR_H; r++)
        for (int c = 0; c < CUR_W; c++) {
            char ch = CUR[r][c];
            if (ch == ' ') continue;
            int px = x0 + c, py = y0 + r;
            if (px < 0 || px >= FB_W || py < 0 || py >= FB_H) continue;
            draw_pixel(px, py, ch == 'F' ? fillc : edge);
        }
}
static void cursor_save_under(int x0, int y0) {
    pixel_t *bb = fb_back();
    for (int r = 0; r < CUR_H; r++)
        for (int c = 0; c < CUR_W; c++) {
            int px = x0 + c, py = y0 + r;
            s_cur_save[r * CUR_W + c] =
                (px >= 0 && px < FB_W && py >= 0 && py < FB_H) ? bb[py * FB_W + px] : 0;
        }
    s_cur_has_save = 1;
}
static void cursor_restore_under(int x0, int y0) {
    if (!s_cur_has_save) return;
    pixel_t *bb = fb_back();
    for (int r = 0; r < CUR_H; r++)
        for (int c = 0; c < CUR_W; c++) {
            int px = x0 + c, py = y0 + r;
            if (px >= 0 && px < FB_W && py >= 0 && py < FB_H)
                bb[py * FB_W + px] = s_cur_save[r * CUR_W + c];
        }
}

/* Called at the end of a FULL redraw_frame: the back buffer is freshly
 * composited (no cursor), so save the background under the cursor and paint it
 * on top.  Records position so a later delta move knows what to restore. */
static void cursor_draw(void) {
    int cx_ovr = -1, cy_ovr = -1;
    const char *pc = getenv("ATOMIK_PREVIEW_CURSOR");  /* "x,y" for host shots */
    if (pc && *pc) { int a = -1, b = -1; if (sscanf(pc, "%d,%d", &a, &b) == 2) { cx_ovr = a; cy_ovr = b; } }
    if (!input_mouse_present() && cx_ovr < 0) { s_cur_sx = s_cur_sy = -1; s_cur_has_save = 0; return; }
    int x0 = cx_ovr >= 0 ? cx_ovr : input_mouse_x();
    int y0 = cy_ovr >= 0 ? cy_ovr : input_mouse_y();
    cursor_save_under(x0, y0);
    cursor_paint(x0, y0);
    s_cur_sx = x0; s_cur_sy = y0;
    dirty_rect(x0, y0, CUR_W, CUR_H);
}

/* Cursor-only motion: a pure delta.  Restore the old region from save-under,
 * paint the cursor at the new spot, and present ONLY those two rects — no
 * surface recomposite, no full present.  This is the desktop running ATOMiK on
 * itself: a cursor move touches ~360 pixels, not 2,073,600. */
static void cursor_move_delta(void) {
    if (!input_mouse_present()) return;
    int nx = input_mouse_x(), ny = input_mouse_y();
    if (nx == s_cur_sx && ny == s_cur_sy) return;
    if (s_cur_sx >= 0 && s_cur_has_save) {        /* erase old cursor */
        cursor_restore_under(s_cur_sx, s_cur_sy);
        fb_present_rect(s_cur_sx, s_cur_sy, CUR_W, CUR_H);
    }
    cursor_save_under(nx, ny);                     /* paint new cursor */
    cursor_paint(nx, ny);
    fb_present_rect(nx, ny, CUR_W, CUR_H);
    s_cur_sx = nx; s_cur_sy = ny;
}

/* v0.42 Atom save-under.  The assistant bubble's footprint is stationary while
 * visible (only the avatar bobs INSIDE it), so we delta-animate Atom exactly
 * like the cursor: save the clean background under the footprint during a full
 * frame, then on each float frame restore it, repaint Atom, and present just
 * that rect.  Without this, Atom's 160 ms float forced a full redraw_frame()
 * that starved the input loop and made the cursor stutter whenever Atom was up. */
static pixel_t *s_atom_save     = NULL;
static int      s_atom_cap      = 0;      /* allocated pixel capacity        */
static int      s_atom_fx, s_atom_fy, s_atom_fw, s_atom_fh;  /* saved rect     */
static int      s_atom_has_save = 0;

static void atom_capture_under(int x, int y, int w, int h) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > FB_W) w = FB_W - x;
    if (y + h > FB_H) h = FB_H - y;
    if (w <= 0 || h <= 0) { s_atom_has_save = 0; return; }
    if (w * h > s_atom_cap) {
        pixel_t *n = realloc(s_atom_save, (size_t)w * h * sizeof(pixel_t));
        if (!n) { s_atom_has_save = 0; return; }
        s_atom_save = n; s_atom_cap = w * h;
    }
    pixel_t *bb = fb_back();
    for (int r = 0; r < h; r++)
        memcpy(&s_atom_save[r * w], &bb[(y + r) * FB_W + x], (size_t)w * sizeof(pixel_t));
    s_atom_fx = x; s_atom_fy = y; s_atom_fw = w; s_atom_fh = h;
    s_atom_has_save = 1;
}

static void atom_restore_under(void) {
    if (!s_atom_has_save) return;
    pixel_t *bb = fb_back();
    for (int r = 0; r < s_atom_fh; r++)
        memcpy(&bb[(s_atom_fy + r) * FB_W + s_atom_fx], &s_atom_save[r * s_atom_fw],
               (size_t)s_atom_fw * sizeof(pixel_t));
}

static void redraw_frame(void) {
    /* v0.38-A: tile-based dirty tracking.  Each visible surface
     * declares the rect it touches via dirty_rect() during its draw
     * call.  At frame end, dirty_finalize_frame() snapshots the per-
     * frame stats into the metric provider so the VISUAL Resource
     * Fabric lane reports a real measurement instead of an event-
     * count proxy. */
    dirty_clear();
    /* v0.31 patch 3 (ChatGPT review 2026-05-07): status_draw() now fires
     * AFTER wm_draw_all() so the status bar is global chrome layered on
     * top of every window, not painted underneath them.  Previously, any
     * window straying into y=0..31 (or any future bug that did) would
     * silently overdraw the bar.  Defensive z-layer discipline: chrome
     * always wins. */
    wallpaper_draw();
    hero_draw();        /* v0.38-E: procedural centerpiece (after wallpaper,
                         * before windows so windows can cover it cleanly) */
    dock_draw(s_dock_hover);
    wm_draw_all();
    status_draw();      /* global chrome — drawn after windows */
    assistant_tick();   /* v0.39-A: auto-dismiss timeout check */
    /* v0.42: snapshot the clean background under Atom's footprint BEFORE drawing
     * it, so later float frames can restore + repaint Atom as a regional delta
     * instead of forcing another full recomposite. */
    { int fx, fy, fw, fh;
      if (assistant_footprint(&fx, &fy, &fw, &fh)) atom_capture_under(fx, fy, fw, fh); }
    assistant_draw();   /* v0.39-A: summoned Atom overlay on top of chrome */
    notify_draw();
    cursor_draw();      /* v0.41: pointer cursor — topmost layer */
    fb_present();
    dirty_finalize_frame();
}

/* v0.42: Atom float animation as a regional delta.  Restore the clean bg under
 * the (stationary) bubble footprint, repaint Atom at its new float offset, and
 * present only that rect + the cursor — no full recomposite, so the cursor
 * stays smooth the whole time Atom is on screen.  Falls back to a full frame if
 * no clean save-under has been captured yet. */
static void redraw_assistant_delta(void) {
    int fx, fy, fw, fh;
    if (!assistant_footprint(&fx, &fy, &fw, &fh) || !s_atom_has_save) {
        redraw_frame();
        return;
    }
    int had_cursor = (s_cur_sx >= 0 && s_cur_has_save);
    int ocx = s_cur_sx, ocy = s_cur_sy;
    if (had_cursor) cursor_restore_under(s_cur_sx, s_cur_sy);
    s_cur_has_save = 0; s_cur_sx = s_cur_sy = -1;

    atom_restore_under();   /* clean bg back under the bubble */
    assistant_tick();       /* may auto-dismiss — then draw no-ops, leaving the
                             * restored clean bg, which erases Atom cleanly. */
    assistant_draw();       /* Atom at its new float position */
    cursor_draw();
    fb_present_rect(s_atom_fx, s_atom_fy, s_atom_fw, s_atom_fh);
    if (had_cursor)    fb_present_rect(ocx, ocy, CUR_W, CUR_H);
    if (s_cur_sx >= 0) fb_present_rect(s_cur_sx, s_cur_sy, CUR_W, CUR_H);
}

/* v0.42 delta render for clicks: when a click mutates ONE window's content (e.g.
 * selecting a lane on the Parallel Aggregate card), recomposite only that window
 * + the cursor and present only those rects — instead of a full-desktop
 * redraw_frame() that brute-forces 2,073,600 pixels and freezes the cursor for a
 * frame.  This is the cursor's save-under generalized to the window manager. */
static void redraw_window_delta(int id) {
    int rx, ry, rw, rh;
    int had_cursor = (s_cur_sx >= 0 && s_cur_has_save);
    int ocx = s_cur_sx, ocy = s_cur_sy;
    /* lift the cursor off the back buffer so the window recomposite + cursor
     * repaint start from a clean background. */
    if (had_cursor) cursor_restore_under(s_cur_sx, s_cur_sy);
    s_cur_has_save = 0; s_cur_sx = s_cur_sy = -1;

    wm_redraw_window(id, &rx, &ry, &rw, &rh);
    if (rw == 0) { redraw_frame(); return; }   /* window vanished -> full redraw */

    /* If Atom is up and overlaps the recomposited window, the window draw just
     * wiped the overlapping aura pixels — recapture the (now-fresh) bg under the
     * footprint so the float save-under stays current, then repaint Atom on top
     * and present its footprint so it doesn't flicker out on a click. */
    int ax, ay, aw, ah, have_atom = 0;
    if (assistant_visible()) {
        if (assistant_footprint(&ax, &ay, &aw, &ah)) atom_capture_under(ax, ay, aw, ah);
        assistant_draw();
        have_atom = assistant_bounds(&ax, &ay, &aw, &ah);
    }

    cursor_draw();                             /* save-under + paint at cursor pos */
    fb_present_rect(rx, ry, rw, rh);           /* the changed window */
    if (have_atom) fb_present_rect(ax, ay, aw, ah);              /* Atom on top */
    if (had_cursor) fb_present_rect(ocx, ocy, CUR_W, CUR_H);      /* clear old cursor */
    if (s_cur_sx >= 0) fb_present_rect(s_cur_sx, s_cur_sy, CUR_W, CUR_H); /* new cursor */
}

/* v0.42 finest delta: a workloads lane-select changes only three elements
 * (ATOMiK panel, ladder, savings banner).  Redraw JUST those + the cursor and
 * present only their rects — skipping the window's unchanged header/tabs/title/
 * CONVENTIONAL-panel/provenance AA text.  A lane select touches a few thousand
 * pixels, not the window, not the desktop.  Falls back to a full redraw if the
 * surface can't do the sub-element path. */
static void redraw_selection_delta(void) {
    int rx, ry, rw, rh;
    int had_cursor = (s_cur_sx >= 0 && s_cur_has_save);
    int ocx = s_cur_sx, ocy = s_cur_sy;
    if (had_cursor) cursor_restore_under(s_cur_sx, s_cur_sy);
    s_cur_has_save = 0; s_cur_sx = s_cur_sy = -1;

    if (!workloads_surface_redraw_selection(&rx, &ry, &rw, &rh)) {
        redraw_window_delta(workloads_surface_window_id());  /* -> full if no window */
        return;
    }

    /* keep Atom on top if it overlaps the repainted sub-elements (see above). */
    int ax, ay, aw, ah, have_atom = 0;
    if (assistant_visible()) {
        if (assistant_footprint(&ax, &ay, &aw, &ah)) atom_capture_under(ax, ay, aw, ah);
        assistant_draw();
        have_atom = assistant_bounds(&ax, &ay, &aw, &ah);
    }

    cursor_draw();
    fb_present_rect(rx, ry, rw, rh);
    if (have_atom) fb_present_rect(ax, ay, aw, ah);
    if (had_cursor) fb_present_rect(ocx, ocy, CUR_W, CUR_H);
    if (s_cur_sx >= 0) fb_present_rect(s_cur_sx, s_cur_sy, CUR_W, CUR_H);
}

/* v0.31: workspace_w returns the width of the central column where
 * normal app windows live.  Resource Fabric occupies the right shelf
 * (pinned system panel); the Capability Rail (v0.34) occupies the
 * left.  Apps center themselves INSIDE the workspace so they don't
 * overlap either side.  This encodes the layout convention:
 *
 *   [Rail] [ Workspace ] [Fabric]
 *
 * v0.34 update: subtracts dock_right_edge() (Capability Rail's right
 * edge) so apps no longer center over the Rail when small. */
static int workspace_left(void)  { return dock_right_edge() + ATOMIK_GRID_L; }
static int workspace_right(void) { return fabric_shelf_x() - ATOMIK_GRID_L; }
static int workspace_w(void)     { return workspace_right() - workspace_left(); }
static int workspace_cx(int ww)  {
    /* Center inside the workspace, then offset back to absolute X. */
    return workspace_left() + (workspace_w() - ww) / 2;
}

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

    /* v0.38-J++ startup mode read.  Deploy / operator writes one of
     * "DEV" / "DEMO" / "INVESTOR" to /tmp/atomik_mode BEFORE launching;
     * we honor it instead of the DEV default so the screenshot can
     * capture investor-mode chrome without keystroke injection.
     * 'O' still cycles at runtime. */
    {
        FILE *mf = fopen("/tmp/atomik_mode", "r");
        if (mf) {
            char buf[16] = {0};
            if (fgets(buf, sizeof buf, mf)) {
                for (size_t i = 0; buf[i]; i++) {
                    if (buf[i] == '\n' || buf[i] == '\r') { buf[i] = 0; break; }
                }
                if      (strcmp(buf, "INVESTOR") == 0) metric_set_mode(METRIC_MODE_INVESTOR);
                else if (strcmp(buf, "DEMO")     == 0) metric_set_mode(METRIC_MODE_DEMO);
                else if (strcmp(buf, "DEV")      == 0) metric_set_mode(METRIC_MODE_DEV);
            }
            fclose(mf);
        }
    }

    /* v0.40: /tmp/atomik_demo present => start the self-driving demo workload
     * so a deploy capture shows live Fabric activity (lanes cycling ACTIVE with
     * real on-board perf-bench data) without fragile UART keystroke injection.
     * 'L' toggles it at runtime. */
    {
        FILE *df = fopen("/tmp/atomik_demo", "r");
        if (df) { fabric_demo_enable(1); seed_demo_usage(); fabric_seed_waveforms(); fclose(df); }
    }

    /* v0.40 in-OS AUTO-CAPTURE: /tmp/atomik_autoshot => after AUTOSHOT_MS of
     * runtime, atomik_os snapshots its OWN screen to /tmp/atomik_autoshot.png
     * and writes /tmp/atomik_autoshot.done — no host/UART command touches the
     * OS during render, so the capture is clean (no console-input bleed, no
     * window clutter). The deploy host just waits silently then pulls the PNG. */
    int autoshot = 0, autoshot_done = 0;
    {
        FILE *as = fopen("/tmp/atomik_autoshot", "r");
        if (as) { autoshot = 1; fclose(as); }
    }

    if (fb_open() < 0) { fprintf(stderr, "fb_open failed\n"); return 1; }
    fb_clear(0);
    fb_present();
    fb_enable_scanout(1);

    font_init();
    /* v0.38-K: load AA bitmap atlases.  Silently no-ops if the .atomik_font
     * files haven't been shipped to /tmp/atomik_fonts/ yet — draw_text_aa
     * falls back to the pixel font, so the board is still usable. */
    font_aa_init();
    /* v0.39-A: load the Atom assistant asset.  Silently no-ops if the
     * .atomik_asset hasn't been shipped yet; the summon overlay falls
     * back to a placeholder rectangle. */
    assistant_init();
    input_open();
    wm_init();
    agent_init();
    wallet_init();
    atomik_open();   /* /dev/mem map for the live monitor; non-fatal if it fails */
    /* v0.38: register every built-in metric with the truth-aware
     * provider.  Surfaces can now consume by ID through metric_get(),
     * and the Pulse Bar's "DATA:" badge reads metric_worst_source(). */
    metric_init();
    /* v0.40: live parallel-bank throughput producer.  Maps the bench engine
     * @0xF0021000 (O_RDWR) and registers bench.* LIVE metrics.  Non-fatal:
     * if the engine/mem is absent it stays WAITING and the surface dims. */
    bench_open();
    bench_register_metrics();
    /* v0.38-A: tile-based dirty-region tracker.  Components declare
     * their dirty rects each frame; redraw_frame() finalizes per-
     * frame stats so the VISUAL Resource Fabric lane reflects real
     * OS redraw waste. */
    dirty_init();

    /* v0.38-E: About no longer auto-opens.  The hero centerpiece
     * (hero_draw, called from redraw_frame between wallpaper and
     * windows) is the OS's identity statement.  About remains
     * available via 'A' for users who want the system facts.
     * Resource Fabric still auto-opens on the right shelf — it's
     * the differentiator and belongs in the default chrome. */
    redraw_frame();
    fabric_open();
    redraw_frame();    /* second paint so Fabric is visible immediately */

    /* ── Host live-loop repro harness (ATOMIK_PREVIEW_LIVELOOP) ───────────────
     * Runs the REAL per-frame tick + redraw cycle over a fast-forwarded sim
     * clock, with Atom summoned, so time-triggered runtime crashes (e.g. the
     * ~12 s Atom auto-dismiss segfault) reproduce on the host under a
     * sanitizer — something the single-shot preview can't do because it never
     * runs the live event loop.  Build with -fsanitize=address for a trace.
     *   ATOMIK_LIVELOOP_MS=N    total sim ms (default 16000, past the 12 s dismiss)
     *   ATOMIK_LIVELOOP_SUMMON=N  ms at which Atom is summoned (default 2000) */
    if (getenv("ATOMIK_PREVIEW_LIVELOOP")) {
        seed_demo_usage();
        fabric_seed_waveforms();
        fabric_demo_enable(1);          /* match the board's self-driving demo  */
        workloads_surface_open();       /* Workloads open, as on the board      */
        const char *fm = getenv("ATOMIK_LIVELOOP_MODE");
        assistant_mode_t lm = ASSIST_EXPLAIN;
        if (fm) {
            if      (!strcmp(fm, "thinking")) lm = ASSIST_THINKING;
            else if (!strcmp(fm, "success"))  lm = ASSIST_SUCCESS;
            else if (!strcmp(fm, "warning"))  lm = ASSIST_WARNING;
        }
        long total  = getenv("ATOMIK_LIVELOOP_MS") ? atol(getenv("ATOMIK_LIVELOOP_MS")) : 40000;
        long summon = getenv("ATOMIK_LIVELOOP_SUMMON") ? atol(getenv("ATOMIK_LIVELOOP_SUMMON")) : 2000;
        long next_summon = summon;
        for (long t = 0; t <= total; t += 16) {
            anim_set_sim_time((unsigned long)t);
            /* re-summon after each auto-dismiss so multiple summon/dismiss
             * cycles run (the board's demo re-summons on each event). */
            if (t >= next_summon && !assistant_visible()) {
                assistant_summon_mode(lm);
                next_summon = t + 13000;   /* dismiss is at +12s; re-arm after */
            }
            stocks_tick();
            workloads_tick();
            fabric_tick();
            bench_tick();
            wlmeasure_tick();
            state_watch_tick();
            replica_flow_tick();
            status_tick();
            redraw_frame();   /* drives assistant_tick (auto-dismiss) + draw */
            if ((t % 1000) == 0) {
                long rss_kb = 0;
                FILE *sf = fopen("/proc/self/statm", "r");
                if (sf) { long pages = 0, res = 0;
                    if (fscanf(sf, "%ld %ld", &pages, &res) == 2) rss_kb = res * (sysconf(_SC_PAGESIZE) / 1024);
                    fclose(sf); }
                fprintf(stderr, "[liveloop] sim t=%ldms  RSS=%ldKB\n", t, rss_kb);
            }
        }
        fprintf(stderr, "[liveloop] survived %ldms — no crash\n", total);
        fb_close();
        return 0;
    }

    /* Host preview build: redraw_frame() above already wrote the PNG via the
     * host fb backend.  Exit instead of entering the interactive input loop
     * so `make host-shot` dumps a faithful frame of the REAL renderer in
     * seconds.  Gated on ATOMIK_PREVIEW so the binary could still loop. */
    if (getenv("ATOMIK_PREVIEW")) {
        seed_demo_usage();   /* representative usage so PREDICTIVE INSIGHTS populates */
        fabric_seed_waveforms();   /* lively Resource Fabric waves for the capture */
        /* v0.41: open a named surface for the host shot (layout verify only). */
        if (getenv("ATOMIK_PREVIEW_WORKLOAD")) workloads_surface_open();
        /* v0.42: summon Atom in a chosen mood for animation host-shots:
         * ATOMIK_PREVIEW_ASSIST=success|thinking|warning|explain */
        {
            const char *am = getenv("ATOMIK_PREVIEW_ASSIST");
            if (am) {
                assistant_mode_t m = ASSIST_EXPLAIN;
                if      (!strcmp(am, "success"))  m = ASSIST_SUCCESS;
                else if (!strcmp(am, "thinking")) m = ASSIST_THINKING;
                else if (!strcmp(am, "warning"))  m = ASSIST_WARNING;
                assistant_summon_mode(m);
            }
        }
        /* Optional warm-up: run N frames (ticking fabric/status each) before
         * the final dump so time-evolving surfaces — the self-driving demo
         * workload, building waveforms — are visible in the capture.  Default
         * 1 keeps the plain preview unchanged. */
        const char *nf = getenv("ATOMIK_PREVIEW_FRAMES");
        int frames = nf ? atoi(nf) : 1;
        if (frames < 1) frames = 1;
        for (int i = 0; i < frames; i++) {
            status_tick();
            fabric_tick();
            bench_tick();                   /* parse the live sweep file too   */
            wlmeasure_tick();               /* + memory-workload measurements  */
            redraw_frame();                 /* host fb_present() dumps the PNG */
            if (i + 1 < frames) usleep(120000);  /* real-time advance for timers */
        }
        /* clean idle home for concept comparison — no transient assistant */
        if (getenv("ATOMIK_PREVIEW_CLEAN")) { assistant_dismiss(); redraw_frame(); }
        fb_close();
        return 0;
    }

    /* Demo/capture startup hook: open the Workloads surface on launch if the
     * flag file exists — robust where the stdin 'w' keystroke is timing-flaky
     * (the launcher just `touch`es it). */
    if (access("/tmp/atomik_open_workloads", F_OK) == 0) {
        workloads_surface_open();
        redraw_frame();
    }

    unsigned long autoshot_start = anim_now_ms();
    while (s_running) {
        /* v0.40 in-OS auto-capture: once the demo workload has had time to
         * cycle the lanes + build waveforms, snapshot our own screen.  Force a
         * fresh frame so the back buffer is current, write the PNG + done flag,
         * and only do it once. */
        if (autoshot && !autoshot_done &&
            anim_now_ms() - autoshot_start > 18000UL) {
            redraw_frame();
            fb_write_png("/tmp/atomik_autoshot.png");
            FILE *adf = fopen("/tmp/atomik_autoshot.done", "w");
            if (adf) { fputs("ok\n", adf); fclose(adf); }
            autoshot_done = 1;
        }

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
        int wl_changed = workloads_tick();   /* v0.41 demo auto-cycle */
        /* v0.30: tick the Resource Fabric once per frame so its
         * personality auto-detection re-classifies based on recent
         * LLM/state activity.  Cheap (no I/O, no allocation). */
        fabric_tick();
        /* v0.40: re-measure the parallel-bank sweep on the engine.
         * Self-rate-limits to BENCH_REFRESH_MS (4 sub-ms HW runs), so
         * calling it every frame is fine; advances the demo's bank
         * allocation cursor so the Fabric surface visibly reallocates. */
        bench_tick();
        wlmeasure_tick();
        /* v0.35: tick the State Watch sampler.  Self-rate-limits to
         * 200 ms per sample, so calling it every frame is fine.
         * Passive observer — reads existing producers + maintains
         * its own personality timeline + speedup sparkline rings. */
        state_watch_tick();
        /* v0.37: tick the Replica Flow surface.  Spawns/expires the
         * in-flight delta pulses based on EVT_SYNC_REPLICA event-bus
         * deltas.  Passive observer; cheap. */
        replica_flow_tick();
        /* v0.38-C: tick the Pulse Bar's event-pulse sparkline sampler.
         * Self-rate-limits to 150 ms.  Cheap. */
        status_tick();
        if (ev.kind == EV_QUIT) { s_running = 0; break; }
        if (ev.kind == EV_KEY) {
            int dirty = 0;

            /* v0.31 patch 8 — Global Input Router.  Order matters and
             * is now explicit (per ChatGPT review 2026-05-06):
             *
             *   1. SYSTEM HOTKEYS  (Tab, Esc, Ctrl-W, future Shift-Tab)
             *      — always win, regardless of focused app.  Implemented
             *      by wm_handle_key().
             *   2. SYSTEM-SHELF KEYS  (R for Resource Fabric)
             *      — always-on launchers for the system-shelf surfaces.
             *      Press R from anywhere → Resource Fabric raises.
             *   3. FOCUSED-APP KEYS  (terminal, files, notes, document)
             *      — when one of these owns the focused window, all
             *      remaining keys feed it as text input.  This is why
             *      A/M/T/F/etc letter launchers DON'T fire while you're
             *      typing in Document — same convention as every desktop
             *      OS where letter launchers require a modifier.
             *   4. APP LAUNCHERS  (A, M, T, F, N, C, K, G, B, H, D, W,
             *      S, digits, space) — only reach here if no app holds
             *      focus.  Letter pressed on bare desktop → app opens.
             */

            /* v0.39-A: Esc dismisses the Atom assistant overlay
             * BEFORE the WM gets it.  Otherwise wm_handle_key would
             * eat Esc to close the topmost window — Esc-to-dismiss
             * an overlay is the universal expectation. */
            if (ev.key == 0x1B && assistant_visible()) {
                assistant_dismiss();
                dirty = 1;
                continue;
            }

            /* (1) SYSTEM HOTKEYS */
            if (wm_handle_key(ev.key)) {
                if (ev.key == '\t')      agent_log(ACT_CYCLE_FOCUS);
                else if (ev.key == 0x1B) agent_log(ACT_CLOSE_WINDOW);
                dirty = 1;
            }
            /* (2) SYSTEM-SHELF KEYS — always-on launchers.  R = Resource
             * Fabric, which is the system-shelf differentiator.  Catches
             * R even while a text-input app is focused.  P = Resource
             * Fabric personality cycle override (v0.32 demo control —
             * always-on for presenter convenience).  Both bypass focused
             * apps because they ARE chrome. */
            else if (ev.key == 'r' || ev.key == 'R') {
                fabric_open();
                dirty = 1;
            }
            else if (ev.key == 'w' || ev.key == 'W') {
                /* v0.41: open the Workloads surface; re-pressing W while it's
                 * already up cycles through the scenarios (Telemetry Sync ->
                 * Control Coalescing -> Parallel Aggregate). */
                if (workloads_surface_is_open()) workloads_cycle_scenario();
                workloads_surface_open();
                agent_log(ACT_OPEN_WORKLOAD);
                dirty = 1;
            }
            else if (ev.key == 'p' || ev.key == 'P') {
                fabric_cycle_override();
                fabric_open();        /* surface the panel so the change is visible */
                dirty = 1;
            }
            /* v0.33-D: press '!' to dump the four-way perf matrix to
             * stdout (UART console).  System-shelf class because it's
             * an instrumentation / debug command, not an app launcher.
             * Uses '!' (Shift+1) to avoid conflicting with any app
             * letter shortcut. */
            else if (ev.key == '!') {
                perf_bench_matrix();
                dirty = 1;
            }
            /* v0.40: 'L' toggles the self-driving demo workload (lanes cycle
             * ACTIVE with real perf-bench data + building waveforms). */
            else if (ev.key == 'l' || ev.key == 'L') {
                fabric_demo_enable(!fabric_demo_enabled());
                dirty = 1;
            }
            /* v0.35: 'V' opens the State Watch surface — the time-
             * ribbon view of system history.  System-shelf class
             * (always wins over focused apps) because it's chrome,
             * not an app. */
            else if (ev.key == 'v' || ev.key == 'V') {
                state_watch_open();
                dirty = 1;
            }
            /* v0.37: 'X' opens the Replica Flow surface — the SYNC
             * personality's product story (LOCAL → DELTA → REMOTE).
             * System-shelf class. */
            else if (ev.key == 'x' || ev.key == 'X') {
                replica_flow_open();
                dirty = 1;
            }
            /* v0.38-C: 'O' cycles the metric_mode (DEV → DEMO →
             * INVESTOR → DEV).  System-shelf class — the Pulse Bar's
             * MODE badge updates immediately to show the new gating. */
            else if (ev.key == 'o' || ev.key == 'O') {
                metric_mode_t m = metric_mode();
                m = (metric_mode_t)((m + 1) % 3);
                metric_set_mode(m);
                dirty = 1;
            }
            /* v0.39-E capture hotkeys — always-global control codes,
             * fire BEFORE focused-app dispatch so review captures can
             * summon Atom even with the Code/Document panel focused.
             * These are not part of the user-facing UX (printf '\x05'
             * / printf '\x07' from a deploy script).
             *   Ctrl+E (0x05) → manual EXPLAIN  (cyan halo)
             *   Ctrl+G (0x07) → forced SUCCESS (cyan core + emerald rim)
             */
            else if (ev.key == 0x05) {
                assistant_summon();
                dirty = 1;
            }
            else if (ev.key == 0x07) {
                assistant_summon_capture_success();
                dirty = 1;
            }
            /* (3) FOCUSED-APP KEYS — terminal/files/notes/document */
            else if (s_terminal_id) {
                window_t *top = wm_topmost();
                if (top && top->id == s_terminal_id) {
                    terminal_send_key(ev.key);
                    dirty = 1;
                }
            }
            if (!dirty && s_files_id) {
                window_t *top = wm_topmost();
                if (top && top->id == s_files_id) {
                    files_handle_key(ev.key);
                    dirty = 1;
                }
            }
            if (!dirty && s_notes_id) {
                window_t *top = wm_topmost();
                if (top && top->id == s_notes_id) {
                    notes_handle_key(ev.key);
                    dirty = 1;
                }
            }
            if (!dirty) {
                doc_state_t *d = focused_doc();
                if (d) {
                    document_handle_key_for(d, ev.key);
                    dirty = 1;
                }
            }
            /* (4) APP LAUNCHERS — only when no focused app claimed the key. */
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
            } else if (!dirty && (ev.key == 'i' || ev.key == 'I')) {
                /* v0.39-A — 'I' (info) toggles the Atom assistant.
                 * Note: only fires when no focused app claimed the
                 * key.  For capture flows that need Atom even with
                 * a focused window, the Ctrl+E / Ctrl+G hotkeys in
                 * the always-global section run regardless. */
                assistant_summon();
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
            } else if (!dirty && ev.key >= '1' && ev.key < '1' + dock_count()) {
                /* Number key launches whatever app is currently in that
                 * dock slot. The slot meaning shifts as the agent
                 * reorders icons by predicted relevance. */
                int slot = ev.key - '1';
                s_dock_hover = slot;
                action_t a = dock_action_for_slot(slot);
                if      (a == ACT_OPEN_ABOUT)     open_about();
                else if (a == ACT_OPEN_MONITOR)   system_surface_open();  /* SYSTEM rail */
                else if (a == ACT_OPEN_TERMINAL)  open_terminal();
                else if (a == ACT_OPEN_FILES)     open_files();
                else if (a == ACT_OPEN_NOTES)     open_notes();
                else if (a == ACT_OPEN_ASSISTANT) assistant_summon();
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
        } else if (ev.kind == EV_MOUSE_MOVE || ev.kind == EV_MOUSE_DOWN ||
                   ev.kind == EV_MOUSE_UP) {
            /* v0.41 pointer: motion moves the cursor + updates rail hover;
             * a left-click on a rail cell activates that capability (same
             * dispatch as the number-key launcher). */
            s_last_mouse_ms = anim_now_ms();   /* freezes Atom's float while moving */
            int slot = dock_hit_test(ev.mx, ev.my);
            int s_wl_click = 0;
            if (ev.kind == EV_MOUSE_DOWN && slot >= 0) {
                s_dock_hover = slot;
                action_t a = dock_action_for_slot(slot);
                if      (a == ACT_OPEN_ABOUT)     open_about();
                else if (a == ACT_OPEN_MONITOR)   system_surface_open();
                else if (a == ACT_OPEN_TERMINAL)  open_terminal();
                else if (a == ACT_OPEN_FILES)     open_files();
                else if (a == ACT_OPEN_NOTES)     open_notes();
                else if (a == ACT_OPEN_ASSISTANT) assistant_summon();
                else if (a == ACT_OPEN_WORKLOAD)  workloads_surface_open();
                if (a != ACT_NONE) agent_log(a); else agent_log(ACT_DOCK_HOVER);
                redraw_frame();
            } else if (ev.kind == EV_MOUSE_DOWN &&
                       (s_wl_click = workloads_surface_handle_click(ev.mx, ev.my))) {
                if (s_wl_click == 2) {
                    /* v0.42: switched scenario tab — the whole card body changes,
                     * but the wallpaper/hero/dock/status don't, so redraw JUST the
                     * workloads window (+ cursor), not the 1080p desktop. */
                    redraw_window_delta(workloads_surface_window_id());
                } else {
                    /* clicked a lane rung — only the ATOMiK panel, ladder, and
                     * savings banner change, so redraw JUST those three (+ the
                     * cursor), not the window, not the desktop. */
                    redraw_selection_delta();
                }
            } else if (ev.kind == EV_MOUSE_MOVE && slot != s_dock_hover) {
                /* moved across a rail-slot boundary: the hover highlight changed,
                 * so repaint (full — the dock spans the rail). */
                s_dock_hover = slot;
                redraw_frame();
            } else {
                /* pure cursor motion (same hover), button RELEASE, or a press on
                 * dead space — nothing composited changes, so a DELTA cursor
                 * update only.  No full recomposite.  (A mouse-up landing here
                 * used to trigger redraw_frame() — that was the ~1s post-click
                 * freeze.)  This is ATOMiK on its own desktop. */
                s_dock_hover = slot;
                cursor_move_delta();
            }
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
            if (!need_frame && wl_changed) need_frame = 1;
            /* live parallel-bank sweep refreshed -> repaint the open Workloads
             * surface (it launches with no input; nothing else would). */
            { int bd = bench_take_dirty();
              if (!need_frame && bd && workloads_surface_is_open()) need_frame = 1; }
            if (need_frame) {
                redraw_frame();
            } else if (assistant_animating() &&
                       anim_now_ms() - s_last_mouse_ms > 220) {
                /* nothing else changed — Atom's float is the only thing moving.
                 * Animate it as a regional delta so the cursor stays smooth
                 * instead of a full recomposite every 160 ms.  AND only when the
                 * cursor has been idle >220 ms: a float frame costs ~10-25 ms, so
                 * letting it fire mid-motion makes the cursor hitch + jump to
                 * catch up.  Atom bobs when you're not moving; freezes when you
                 * are.  Cursor smoothness wins. */
                redraw_assistant_delta();
            }
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
