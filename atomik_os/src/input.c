/* input.c — keyboard input from BOTH stdin (UART relay) AND USB HID.
 *
 * v0.24: ATOMiK OS finally listens to the USB keyboard plugged into
 * the Zynq directly. Until v0.24 the OS only read stdin, which works
 * for the laptop-side UART relay (printf to /tmp/aos_keys) but
 * leaves users staring at a desktop with a connected keyboard that
 * does nothing. Now both paths feed the same event stream.
 *
 * Implementation: scan /dev/input/event*, open each, poll() them all
 * alongside stdin. When a key-press input_event lands, translate the
 * Linux keycode (KEY_A=30, KEY_S=31, ...) to ASCII via a lookup table.
 */
#include "atomik_os.h"
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_KBDS 4
#define MAX_MICE 4

static struct termios s_old_term;
static int            s_have_old   = 0;
static int            s_kbd_fd[MAX_KBDS];
static int            s_n_kbd      = 0;
static int            s_shift_held = 0;
static int            s_ctrl_held  = 0;    /* v0.31 patch 8: Ctrl-W close support */

/* v0.41 mouse: relative-pointer devices (EV_REL). We accumulate REL_X/REL_Y
 * into an absolute cursor clamped to the framebuffer, and surface BTN_LEFT as
 * EV_MOUSE_DOWN/UP.  Cursor starts at screen center. */
static int s_mouse_fd[MAX_MICE];
static int s_n_mouse    = 0;
static int s_mx         = FB_W / 2;
static int s_my         = FB_H / 2;

int input_mouse_x(void)       { return s_mx; }
int input_mouse_y(void)       { return s_my; }
int input_mouse_present(void) { return s_n_mouse > 0; }

/* Linux keycode → ASCII. Sparse table; unmapped codes return 0.
 * Numbers, letters, and the most common punctuation/control. */
static char keycode_to_ascii(int kc, int shift) {
    static const char ROW_NUM[]   = "1234567890-=";  /* KEY_1..KEY_EQUAL = 2..13 */
    static const char ROW_NUM_S[] = "!@#$%^&*()_+";
    static const char ROW_QWE[]   = "qwertyuiop[]";  /* KEY_Q..KEY_RIGHTBRACE = 16..27 */
    static const char ROW_QWE_S[] = "QWERTYUIOP{}";
    static const char ROW_ASD[]   = "asdfghjkl;'";   /* KEY_A..KEY_APOSTROPHE = 30..40 */
    static const char ROW_ASD_S[] = "ASDFGHJKL:\"";
    static const char ROW_ZXC[]   = "zxcvbnm,./";    /* KEY_Z..KEY_SLASH = 44..53 */
    static const char ROW_ZXC_S[] = "ZXCVBNM<>?";

    if (kc >= 2  && kc <= 13) return shift ? ROW_NUM_S[kc - 2]  : ROW_NUM[kc - 2];
    if (kc >= 16 && kc <= 27) return shift ? ROW_QWE_S[kc - 16] : ROW_QWE[kc - 16];
    if (kc >= 30 && kc <= 40) return shift ? ROW_ASD_S[kc - 30] : ROW_ASD[kc - 30];
    if (kc >= 44 && kc <= 53) return shift ? ROW_ZXC_S[kc - 44] : ROW_ZXC[kc - 44];
    if (kc == KEY_SPACE)      return ' ';
    if (kc == KEY_ENTER)      return '\n';
    if (kc == KEY_BACKSPACE)  return 0x08;
    if (kc == KEY_TAB)        return '\t';
    if (kc == KEY_ESC)        return 0x1B;
    if (kc == KEY_GRAVE)      return shift ? '~' : '`';
    if (kc == KEY_BACKSLASH)  return shift ? '|' : '\\';
    return 0;
}

/* Try to open a /dev/input/eventN as a keyboard. Returns the fd on
 * success or -1. We accept any device that reports EV_KEY support;
 * mice and joysticks declare EV_REL/EV_ABS instead and we ignore
 * spurious key events from them by checking keycode range. */
static int try_open_kbd(const char *path) {
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return -1;
    unsigned long evbits = 0;
    if (ioctl(fd, EVIOCGBIT(0, sizeof evbits), &evbits) < 0 ||
        !(evbits & (1UL << EV_KEY))) {
        close(fd);
        return -1;
    }
    return fd;
}

static void scan_keyboards(void) {
    DIR *d = opendir("/dev/input");
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) && s_n_kbd < MAX_KBDS) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof path, "/dev/input/%s", e->d_name);
        int fd = try_open_kbd(path);
        if (fd >= 0) s_kbd_fd[s_n_kbd++] = fd;
    }
    closedir(d);
}

/* Open a /dev/input/eventN as a mouse: it must report EV_REL (relative
 * pointer).  Keyboards declare EV_KEY without EV_REL, so this cleanly
 * separates the two even for combo devices. */
static int try_open_mouse(const char *path) {
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return -1;
    unsigned long evbits = 0;
    if (ioctl(fd, EVIOCGBIT(0, sizeof evbits), &evbits) < 0 ||
        !(evbits & (1UL << EV_REL))) {
        close(fd);
        return -1;
    }
    return fd;
}

static void scan_mice(void) {
    DIR *d = opendir("/dev/input");
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) && s_n_mouse < MAX_MICE) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof path, "/dev/input/%s", e->d_name);
        int fd = try_open_mouse(path);
        if (fd >= 0) s_mouse_fd[s_n_mouse++] = fd;
    }
    closedir(d);
}

int input_open(void) {
    /* stdin path — still used for UART relay (atomik_ai_daemon writes
     * to /tmp/aos_keys, the launching shell or fifo feeds stdin). */
    if (tcgetattr(0, &s_old_term) == 0) s_have_old = 1;
    struct termios t = s_old_term;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN]  = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
    int fl = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, fl | O_NONBLOCK);

    /* USB HID path — find the keyboard(s) and mouse/mice. */
    scan_keyboards();
    scan_mice();
    return 0;
}

void input_close(void) {
    if (s_have_old) tcsetattr(0, TCSANOW, &s_old_term);
    for (int i = 0; i < s_n_kbd; i++) close(s_kbd_fd[i]);
    s_n_kbd = 0;
    for (int i = 0; i < s_n_mouse; i++) close(s_mouse_fd[i]);
    s_n_mouse = 0;
}

event_t input_poll(int timeout_ms) {
    event_t ev = { EV_NONE, 0, 0, 0 };
    struct pollfd pfd[1 + MAX_KBDS + MAX_MICE];
    int nfd = 0;
    pfd[nfd].fd = 0; pfd[nfd].events = POLLIN; nfd++;
    for (int i = 0; i < s_n_kbd; i++) {
        pfd[nfd].fd = s_kbd_fd[i]; pfd[nfd].events = POLLIN; nfd++;
    }
    int mouse_base = nfd;
    for (int i = 0; i < s_n_mouse; i++) {
        pfd[nfd].fd = s_mouse_fd[i]; pfd[nfd].events = POLLIN; nfd++;
    }
    int n = poll(pfd, nfd, timeout_ms);
    if (n <= 0) return ev;

    /* stdin first — preserves UART-relay semantics. */
    if (pfd[0].revents & POLLIN) {
        char c = 0;
        if (read(0, &c, 1) == 1) {
            if (c == 'q' || c == 'Q' || c == 0x03) ev.kind = EV_QUIT;
            else { ev.kind = EV_KEY; ev.key = (int)(unsigned char)c; }
            return ev;
        }
    }

    /* USB keyboard event. Read one input_event per poll round so the
     * caller's per-key dirty/redraw logic stays simple. */
    for (int i = 0; i < s_n_kbd; i++) {
        if (!(pfd[1 + i].revents & POLLIN)) continue;
        struct input_event ie;
        ssize_t r = read(s_kbd_fd[i], &ie, sizeof ie);
        if (r != (ssize_t)sizeof ie) continue;
        if (ie.type != EV_KEY) continue;
        if (ie.code == KEY_LEFTSHIFT || ie.code == KEY_RIGHTSHIFT) {
            s_shift_held = (ie.value != 0);
            continue;
        }
        /* v0.31 patch 8: track Ctrl so we can emit 0x17 (Ctrl-W) when
         * the user presses Ctrl+W to close the focused window — Esc
         * backup per ChatGPT's plan.  Convention: Ctrl+letter → ASCII
         * 0x01..0x1A (control codes), matching what a UART terminal
         * would emit.  wm_handle_key() reads 0x17 and treats it like
         * Esc on the focused window. */
        if (ie.code == KEY_LEFTCTRL || ie.code == KEY_RIGHTCTRL) {
            s_ctrl_held = (ie.value != 0);
            continue;
        }
        if (ie.value == 0) continue;          /* release: ignore */
        char c = keycode_to_ascii(ie.code, s_shift_held);
        if (!c) continue;                     /* unmapped key */
        /* Ctrl + letter → control code (Ctrl+W = 0x17, Ctrl+C = 0x03,
         * etc.).  Standard terminal convention; lets the rest of
         * atomik_os stay agnostic to whether the source is UART or
         * USB-HID. */
        if (s_ctrl_held && c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 1);
        } else if (s_ctrl_held && c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 1);
        }
        if (c == 'q' || c == 'Q' || c == 0x03) {
            ev.kind = EV_QUIT;
        } else {
            ev.kind = EV_KEY;
            ev.key  = (int)(unsigned char)c;
        }
        return ev;
    }

    /* USB mouse: drain each ready fd, accumulating relative motion into the
     * absolute cursor and capturing a left-button transition.  A button event
     * takes priority over motion (the caller dispatches a click at the current
     * cursor); otherwise motion emits EV_MOUSE_MOVE.  Cursor is clamped to the
     * framebuffer. */
    for (int i = 0; i < s_n_mouse; i++) {
        if (!(pfd[mouse_base + i].revents & POLLIN)) continue;
        struct input_event ie;
        int moved = 0, btn_evt = 0, btn_val = 0;
        while (read(s_mouse_fd[i], &ie, sizeof ie) == (ssize_t)sizeof ie) {
            if (ie.type == EV_REL) {
                if (ie.code == REL_X) { s_mx += ie.value; moved = 1; }
                else if (ie.code == REL_Y) { s_my += ie.value; moved = 1; }
            } else if (ie.type == EV_KEY &&
                       (ie.code == BTN_LEFT || ie.code == BTN_TOUCH)) {
                btn_evt = 1; btn_val = ie.value;
            }
        }
        if (s_mx < 0) s_mx = 0; if (s_mx >= FB_W) s_mx = FB_W - 1;
        if (s_my < 0) s_my = 0; if (s_my >= FB_H) s_my = FB_H - 1;
        ev.mx = s_mx; ev.my = s_my;
        if (btn_evt) { ev.kind = btn_val ? EV_MOUSE_DOWN : EV_MOUSE_UP; return ev; }
        if (moved)   { ev.kind = EV_MOUSE_MOVE; return ev; }
    }
    return ev;
}
