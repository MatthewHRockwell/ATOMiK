/* terminal.c — minimal terminal app.
 *
 * Spawns /bin/sh on a pty (forkpty). The parent reads the pty master into a
 * scrollback ring and renders it in the window content area. Keystrokes
 * routed here when the terminal window is focused get written to the pty
 * master, driving the shell.
 *
 * No ANSI/VT100 parsing in v0.5 — we strip ESC sequences crudely so output
 * is readable. A real VTE port lands in v0.6.1.
 */
#include "atomik_os.h"
#include <errno.h>
#include <fcntl.h>
#include <pty.h>           /* forkpty()           */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define TERM_COLS    100
#define TERM_ROWS    32
#define TERM_BUF     (TERM_COLS * TERM_ROWS)

static char  s_buf[TERM_BUF];
static int   s_cur_row = 0;
static int   s_cur_col = 0;
static pid_t s_child   = -1;
static int   s_master  = -1;

static void term_clear(void) {
    memset(s_buf, ' ', sizeof s_buf);
    s_cur_row = 0;
    s_cur_col = 0;
}

static void scroll_up(void) {
    memmove(s_buf, s_buf + TERM_COLS, TERM_BUF - TERM_COLS);
    memset(s_buf + TERM_BUF - TERM_COLS, ' ', TERM_COLS);
    s_cur_row = TERM_ROWS - 1;
}

static void put_ch(char c) {
    if (c == '\n') { s_cur_col = 0; s_cur_row++; }
    else if (c == '\r') { s_cur_col = 0; }
    else if (c == '\b') { if (s_cur_col > 0) s_cur_col--; }
    else if (c == '\t') { s_cur_col = (s_cur_col + 8) & ~7; }
    else if (c >= 32 && c < 127) {
        if (s_cur_col >= TERM_COLS) { s_cur_col = 0; s_cur_row++; }
        s_buf[s_cur_row * TERM_COLS + s_cur_col] = c;
        s_cur_col++;
    }
    if (s_cur_row >= TERM_ROWS) scroll_up();
}

int terminal_start(void) {
    if (s_child > 0) return 0;            /* already started */
    term_clear();
    char buf[16];
    int  amaster;
    pid_t pid = forkpty(&amaster, NULL, NULL, NULL);
    if (pid < 0) { snprintf(buf, sizeof buf, "fork err"); return -1; }
    if (pid == 0) {
        /* child */
        const char *shell = "/bin/sh";
        execl(shell, shell, "-i", (char *)NULL);
        _exit(127);
    }
    /* parent */
    s_child  = pid;
    s_master = amaster;
    fcntl(s_master, F_SETFL, O_NONBLOCK);
    /* Set winsize so apps that ask for it (less, vim) get plausible dims. */
    struct winsize ws = { .ws_row = TERM_ROWS, .ws_col = TERM_COLS };
    ioctl(s_master, TIOCSWINSZ, &ws);
    return 0;
}

void terminal_stop(void) {
    if (s_child > 0) { kill(s_child, 9); waitpid(s_child, NULL, 0); }
    if (s_master > 0) close(s_master);
    s_child  = -1;
    s_master = -1;
}

/* Pump up to N bytes from the pty master into the scrollback. Strips
 * crude ESC[…m color codes (we don't render colors yet). */
void terminal_pump(void) {
    if (s_master < 0) return;
    char buf[1024];
    for (;;) {
        ssize_t n = read(s_master, buf, sizeof buf);
        if (n <= 0) break;
        int in_esc = 0;
        for (ssize_t i = 0; i < n; i++) {
            char c = buf[i];
            if (in_esc) {
                if ((c >= '@' && c <= '~')) in_esc = 0;
                continue;
            }
            if (c == 0x1B) { in_esc = 1; continue; }
            put_ch(c);
        }
    }
}

void terminal_send_key(int key) {
    if (s_master < 0) return;
    ssize_t r;
    if (key == '\n')  { r = write(s_master, "\r", 1); (void)r; return; }
    if (key == 0x7F)  { r = write(s_master, "\b", 1); (void)r; return; }
    char c = (char)key;
    r = write(s_master, &c, 1); (void)r;
}

void terminal_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    /* Pump pty before drawing so the latest output is on screen. */
    terminal_pump();

    draw_rect(x, y, wd, ht, rgb(0x0E, 0x12, 0x1C));
    int char_w = 8;
    int char_h = 16;
    int rows = ht / char_h; if (rows > TERM_ROWS) rows = TERM_ROWS;
    int cols = wd / char_w; if (cols > TERM_COLS) cols = TERM_COLS;
    /* Render bottom-aligned: show the most recent rows first. */
    int start_row = TERM_ROWS - rows;
    if (start_row < 0) start_row = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            char ch = s_buf[(start_row + r) * TERM_COLS + c];
            if (ch == ' ') continue;
            char tmp[2] = { ch, 0 };
            draw_text(x + 8 + c * char_w, y + 6 + r * char_h, tmp, 1,
                      ATOMIK_FG);
        }
    }
    /* Cursor block */
    int cur_y = y + 6 + (s_cur_row - start_row) * char_h;
    int cur_x = x + 8 + s_cur_col * char_w;
    if (s_cur_row >= start_row && s_cur_row < start_row + rows) {
        draw_rect(cur_x, cur_y, char_w, char_h, ATOMIK_ACCENT);
    }
}
