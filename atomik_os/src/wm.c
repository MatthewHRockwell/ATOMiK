/* wm.c — window manager.
 *
 * v0.1 capability set:
 *   - up to WM_MAX_WINDOWS floating windows
 *   - explicit z-order, top-of-stack drawn last
 *   - per-window title bar with title text + close button
 *   - keyboard navigation: Tab cycles focus, Esc closes focused window
 *   - the WM owns the rect math; content rendering is delegated via
 *     a callback (`draw_content`) so apps stay decoupled from the WM
 *
 * No mouse yet (USB host is paused). Focus moves with Tab. Drag/resize will
 * land in v0.1.1 once we add a mouse-event source (currently keyboard-only). */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

static window_t s_windows[WM_MAX_WINDOWS];
static int      s_count   = 0;
static int      s_next_id = 1;

void wm_init(void) {
    memset(s_windows, 0, sizeof s_windows);
    s_count   = 0;
    s_next_id = 1;
}

static window_t *find_by_id(int id) {
    for (int i = 0; i < s_count; i++) {
        if (s_windows[i].id == id) return &s_windows[i];
    }
    return NULL;
}

static int max_z(void) {
    int z = 0;
    for (int i = 0; i < s_count; i++) {
        if (s_windows[i].z > z) z = s_windows[i].z;
    }
    return z;
}

window_t *wm_open(const char *title, int x, int y, int w, int h,
                  win_draw_fn draw_content, void *user) {
    if (s_count >= WM_MAX_WINDOWS) return NULL;
    window_t *win = &s_windows[s_count++];
    win->id      = s_next_id++;
    win->x       = x;
    win->y       = y;
    win->w       = w;
    win->h       = h;
    win->visible = 1;
    win->z       = max_z() + 1;
    win->opened_at_ms = anim_now_ms();
    win->draw_content = draw_content;
    win->user    = user;
    snprintf(win->title, sizeof win->title, "%s", title ? title : "Window");
    anim_tick();
    return win;
}

void wm_close(int id) {
    for (int i = 0; i < s_count; i++) {
        if (s_windows[i].id == id) {
            for (int j = i; j < s_count - 1; j++) s_windows[j] = s_windows[j+1];
            s_count--;
            return;
        }
    }
}

window_t *wm_focus(int id) {
    window_t *win = find_by_id(id);
    if (!win) return NULL;
    win->z = max_z() + 1;
    return win;
}

window_t *wm_topmost(void) {
    if (s_count == 0) return NULL;
    window_t *top = &s_windows[0];
    for (int i = 1; i < s_count; i++) {
        if (s_windows[i].z > top->z) top = &s_windows[i];
    }
    return top;
}

/* v0.34-C unified card chrome.
 *
 * Every ATOMiK Desk window reads as a coherent card: same body tone,
 * same title-bar treatment, same border, same focus indicator
 * placement.  Apps that previously fought the WM-level background
 * with their own draw_rect on first paint can now trust the WM
 * to provide a clean canvas at WM_CARD_BG.
 *
 * The v0.30 chrome had a drop-shadow rect that read as "Web 2.0"
 * (offset black box behind the card).  v0.34-C replaces it with a
 * thin 1-px border outline in DOCK_BORDER tone — reads as "premium
 * lab instrument" rather than "browser pop-up", matching the
 * visual north star.
 *
 * Close button: red square → small dim dot.  Per ChatGPT's design
 * discipline rule, decorative chrome that doesn't communicate state
 * (and the close button has no state — it's always close) should
 * recede.  The button still hit-tests; just less visually loud.
 *
 * Tones are exported so app draw_content callbacks can match the
 * WM-level card body without hard-coding hex literals everywhere. */

/* Canonical card background — slightly lighter than the wallpaper so
 * windows visually rise off the desktop without needing a shadow. */
#define WM_CARD_BG       rgb(0x12, 0x18, 0x26)
/* Title bar tones — focused gets a subtle indigo wash; idle gets a
 * flatter dim band. */
#define WM_TITLE_BG_FOC  rgb(0x1A, 0x24, 0x3A)
#define WM_TITLE_BG_IDLE rgb(0x16, 0x1C, 0x2A)
#define WM_BORDER_COL    rgb(0x2E, 0x36, 0x48)

static void draw_one(const window_t *win, int focused) {
    int rx = win->x, ry = win->y, rw = win->w, rh = win->h;
#if ATOMIK_ANIM_WINDOW_OPEN
    double age = anim_window_age(win->id, win->opened_at_ms);
    double sc  = anim_ease_out(age);
    int    inset_w = (int)((1.0 - sc) * (double)win->w * 0.04);
    int    inset_h = (int)((1.0 - sc) * (double)win->h * 0.04);
    rx = win->x + inset_w;
    ry = win->y + inset_h;
    rw = win->w - 2 * inset_w;
    rh = win->h - 2 * inset_h;
    if (age < 1.0) anim_tick();
#else
    (void)win->id; (void)win->opened_at_ms;
#endif

    /* Card body — single rounded fill at the canonical card tone. */
    draw_rect_rounded(rx, ry, rw, rh, 12, WM_CARD_BG);

    /* 1-px border outline — replaces the drop-shadow.  Drawn as four
     * thin rects rather than a rounded outline so the corners look
     * crisp without anti-aliasing. */
    draw_rect(rx + 12,           ry,                rw - 24, 1,       WM_BORDER_COL);
    draw_rect(rx + 12,           ry + rh - 1,       rw - 24, 1,       WM_BORDER_COL);
    draw_rect(rx,                ry + 12,           1,       rh - 24, WM_BORDER_COL);
    draw_rect(rx + rw - 1,       ry + 12,           1,       rh - 24, WM_BORDER_COL);

    /* v0.40: CHROMELESS convention — a window opened with an empty title
     * ("") is a pinned system panel (e.g. the Resource Fabric shelf): keep
     * the glass body + border, but skip the title bar, focus underline, and
     * close dot, and hand the FULL inner rect to the content callback so the
     * panel renders its own header.  Removes the "window-in-a-window" look
     * the concept doesn't have. */
    int chromeless = (win->title[0] == '\0');
    int cx, cy, cw, ch;
    if (!chromeless) {
        /* Title bar — flat tone differentiated by focus.  Same rounded
         * radius as the card body so the top corners read continuously. */
        pixel_t title_bg = focused ? WM_TITLE_BG_FOC : WM_TITLE_BG_IDLE;
        draw_rect_rounded(rx, ry, rw, WM_TITLE_H, 12, title_bg);

        /* Title text — full FG when focused, dim when idle. */
        int ty = ry + (WM_TITLE_H - text_height(1)) / 2;
        draw_text(rx + 16, ty, win->title, 1,
                  focused ? ATOMIK_FG : ATOMIK_FG_DIM);

        /* Focus indicator: 1-row cyan underline beneath the title bar. */
        if (focused) {
            draw_rect(rx + WM_BORDER, ry + WM_TITLE_H,
                      rw - 2 * WM_BORDER, 1, ATOMIK_SEM_HARDWARE);
        }

        /* Close affordance — small overload-red dot at the top-right. */
        int cb_size = 10;
        int cb_x = rx + rw - cb_size - 14;
        int cb_y = ry + (WM_TITLE_H - cb_size) / 2;
        draw_rect_rounded(cb_x, cb_y, cb_size, cb_size, 5,
                          focused ? ATOMIK_SEM_OVERLOAD
                                  : rgb(0x55, 0x35, 0x42));

        cx = rx + WM_BORDER;
        cy = ry + WM_TITLE_H;
        cw = rw - 2 * WM_BORDER;
        ch = rh - WM_TITLE_H - WM_BORDER;
    } else {
        cx = rx + WM_BORDER;
        cy = ry + WM_BORDER;
        cw = rw - 2 * WM_BORDER;
        ch = rh - 2 * WM_BORDER;
    }

    /* Content area — call the app's draw callback with a clean rectangle. */
    if (win->draw_content) win->draw_content((window_t *)win, cx, cy, cw, ch);
}

/* Public accessor: apps that need to fill their content background
 * should match the WM-level card tone for visual continuity. */
pixel_t wm_card_bg(void) { return WM_CARD_BG; }
pixel_t wm_card_border(void) { return WM_BORDER_COL; }

void wm_draw_all(void) {
    /* Painter's algorithm: sort by z then draw bottom-to-top. */
    int order[WM_MAX_WINDOWS];
    for (int i = 0; i < s_count; i++) order[i] = i;
    for (int i = 0; i < s_count - 1; i++) {
        for (int j = i + 1; j < s_count; j++) {
            if (s_windows[order[i]].z > s_windows[order[j]].z) {
                int t = order[i]; order[i] = order[j]; order[j] = t;
            }
        }
    }
    window_t *top = wm_topmost();
    for (int i = 0; i < s_count; i++) {
        const window_t *w = &s_windows[order[i]];
        if (!w->visible) continue;
        draw_one(w, w == top);
    }
}

/* v0.31: enumeration getters for the status-bar window-strip. */
int wm_count(void) { return s_count; }
const window_t *wm_get(int idx) {
    if (idx < 0 || idx >= s_count) return NULL;
    return &s_windows[idx];
}

int wm_handle_key(int key) {
    if (s_count == 0) return 0;
    /* Esc OR Ctrl-W (= 0x17) — close the focused window.  Ctrl-W is
     * added in v0.31 patch 8 because Esc gets trapped by some apps
     * for "cancel input" semantics; Ctrl-W is the universal "close
     * focused window" shortcut every desktop OS recognizes. */
    if (key == 0x1B || key == 0x17) {
        window_t *t = wm_topmost();
        if (t) wm_close(t->id);
        return 1;
    }
    if (key == '\t') {
        /* Cycle focus to the next window in the stack. */
        window_t *t = wm_topmost();
        if (!t) return 0;
        /* Find a window whose z is just below the topmost, raise it. */
        int    cur_z   = t->z;
        int    best    = -1;
        int    best_z  = -1;
        for (int i = 0; i < s_count; i++) {
            if (s_windows[i].z < cur_z && s_windows[i].z > best_z) {
                best   = i;
                best_z = s_windows[i].z;
            }
        }
        if (best < 0) return 1;        /* only one window — stay focused */
        s_windows[best].z = max_z() + 1;
        return 1;
    }
    return 0;
}
