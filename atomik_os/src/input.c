/* input.c — stdin (UART) → key events.
 *
 * Until USB HID is fixed, the user types into the host laptop's bridge.py
 * which forwards bytes to the board over UART. This module reads stdin
 * non-blocking and maps to event_t values. */

#include "atomik_os.h"
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

static struct termios s_old_term;
static int            s_have_old = 0;

int input_open(void) {
    /* Put stdin into raw, non-blocking mode so single keystrokes arrive
     * without line buffering. */
    if (tcgetattr(0, &s_old_term) == 0) s_have_old = 1;
    struct termios t = s_old_term;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN]  = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);

    int fl = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, fl | O_NONBLOCK);
    return 0;
}

void input_close(void) {
    if (s_have_old) tcsetattr(0, TCSANOW, &s_old_term);
}

event_t input_poll(int timeout_ms) {
    event_t ev = { EV_NONE, 0 };
    struct pollfd pfd = { .fd = 0, .events = POLLIN };
    int n = poll(&pfd, 1, timeout_ms);
    if (n <= 0) return ev;
    char c = 0;
    if (read(0, &c, 1) != 1) return ev;
    if (c == 'q' || c == 'Q' || c == 0x03 /* Ctrl-C */) {
        ev.kind = EV_QUIT;
    } else {
        ev.kind = EV_KEY;
        ev.key  = (int)c;
    }
    return ev;
}
