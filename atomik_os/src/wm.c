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
    win->draw_content = draw_content;
    win->user    = user;
    snprintf(win->title, sizeof win->title, "%s", title ? title : "Window");
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

static void draw_one(const window_t *win, int focused) {
    /* Outer shadow (1px offset) */
    draw_rect_rounded(win->x + 4, win->y + 6, win->w, win->h, 12,
                      rgb(0x00, 0x00, 0x00));
    /* Body background */
    draw_rect_rounded(win->x, win->y, win->w, win->h, 12,
                      rgb(0x16, 0x1C, 0x2A));
    /* Title bar */
    pixel_t title_bg = focused ? ATOMIK_ACCENT_DIM : rgb(0x22, 0x2A, 0x3A);
    draw_rect_rounded(win->x, win->y, win->w, WM_TITLE_H, 12, title_bg);
    /* Title text */
    int ty = win->y + (WM_TITLE_H - text_height(1)) / 2;
    draw_text(win->x + 12, ty, win->title, 1,
              focused ? ATOMIK_FG : ATOMIK_FG_DIM);
    /* Close button (right side of title bar) */
    int cb_size = 16;
    int cb_x = win->x + win->w - cb_size - 8;
    int cb_y = win->y + (WM_TITLE_H - cb_size) / 2;
    draw_rect_rounded(cb_x, cb_y, cb_size, cb_size, 4,
                      rgb(0xFF, 0x6F, 0x91));
    /* X glyph drawn as two small rects */
    draw_rect(cb_x + 4, cb_y + 7, 8, 2, ATOMIK_FG);
    draw_rect(cb_x + 7, cb_y + 4, 2, 8, ATOMIK_FG);

    /* Content area */
    int cx = win->x + WM_BORDER;
    int cy = win->y + WM_TITLE_H;
    int cw = win->w - 2 * WM_BORDER;
    int ch = win->h - WM_TITLE_H - WM_BORDER;
    if (win->draw_content) win->draw_content((window_t *)win, cx, cy, cw, ch);
}

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

int wm_handle_key(int key) {
    if (s_count == 0) return 0;
    if (key == 0x1B /* Esc */) {
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
