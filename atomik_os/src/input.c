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

static struct termios s_old_term;
static int            s_have_old   = 0;
static int            s_kbd_fd[MAX_KBDS];
static int            s_n_kbd      = 0;
static int            s_shift_held = 0;

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

    /* USB HID path — find the keyboard. */
    scan_keyboards();
    return 0;
}

void input_close(void) {
    if (s_have_old) tcsetattr(0, TCSANOW, &s_old_term);
    for (int i = 0; i < s_n_kbd; i++) close(s_kbd_fd[i]);
    s_n_kbd = 0;
}

event_t input_poll(int timeout_ms) {
    event_t ev = { EV_NONE, 0 };
    struct pollfd pfd[1 + MAX_KBDS];
    int nfd = 0;
    pfd[nfd].fd = 0; pfd[nfd].events = POLLIN; nfd++;
    for (int i = 0; i < s_n_kbd; i++) {
        pfd[nfd].fd = s_kbd_fd[i]; pfd[nfd].events = POLLIN; nfd++;
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
        if (ie.value == 0) continue;          /* release: ignore */
        char c = keycode_to_ascii(ie.code, s_shift_held);
        if (!c) continue;                     /* unmapped key */
        if (c == 'q' || c == 'Q' || c == 0x03) {
            ev.kind = EV_QUIT;
        } else {
            ev.kind = EV_KEY;
            ev.key  = (int)(unsigned char)c;
        }
        return ev;
    }
    return ev;
}
