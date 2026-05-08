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

static void draw_one(const window_t *win, int focused) {
    /* Open-fade tween: scale the window up subtly during the first ~220ms
     * so it pops in instead of snapping.
     *
     * v0.25: gated behind ATOMIK_ANIM_WINDOW_OPEN (default 0 — off).
     * Linear's motion-restraint rule: an interface that doesn't animate
     * feels faster than one that does. The fade was costing ~6M pixel
     * writes per window open (full bounding-rect redraw × 13 frames)
     * and earned no measurable user-experience win. Snap-open it is. */
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

    /* Outer shadow (1px offset) */
    draw_rect_rounded(rx + 4, ry + 6, rw, rh, 12,
                      rgb(0x00, 0x00, 0x00));
    /* Body background */
    draw_rect_rounded(rx, ry, rw, rh, 12,
                      rgb(0x16, 0x1C, 0x2A));
    /* Title bar */
    pixel_t title_bg = focused ? ATOMIK_ACCENT_DIM : rgb(0x22, 0x2A, 0x3A);
    draw_rect_rounded(rx, ry, rw, WM_TITLE_H, 12, title_bg);
    /* Title text */
    int ty = ry + (WM_TITLE_H - text_height(1)) / 2;
    draw_text(rx + 12, ty, win->title, 1,
              focused ? ATOMIK_FG : ATOMIK_FG_DIM);
    /* v0.25 focus indicator: 1-row cyan underline beneath the title bar
     * of the focused window. Cheap delta — 2 horizontal lines per focus
     * change. Linear's "don't compete for attention you haven't earned"
     * rule: blurred windows get nothing, only the focused one signals.
     * The indicator uses ATOMIK_SEM_HARDWARE because focus = active. */
    if (focused) {
        draw_rect(rx + WM_BORDER, ry + WM_TITLE_H,
                  rw - 2 * WM_BORDER, 1, ATOMIK_SEM_HARDWARE);
    }
    /* Close button (right side of title bar) */
    int cb_size = 16;
    int cb_x = rx + rw - cb_size - 8;
    int cb_y = ry + (WM_TITLE_H - cb_size) / 2;
    draw_rect_rounded(cb_x, cb_y, cb_size, cb_size, 4,
                      rgb(0xFF, 0x6F, 0x91));
    /* X glyph drawn as two small rects */
    draw_rect(cb_x + 4, cb_y + 7, 8, 2, ATOMIK_FG);
    draw_rect(cb_x + 7, cb_y + 4, 2, 8, ATOMIK_FG);

    /* Content area */
    int cx = rx + WM_BORDER;
    int cy = ry + WM_TITLE_H;
    int cw = rw - 2 * WM_BORDER;
    int ch = rh - WM_TITLE_H - WM_BORDER;
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
