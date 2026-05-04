/* about.c — first ATOMiK OS app: an "About" window with system info. */
#include "atomik_os.h"

void about_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    /* Body fill with a slightly lighter tone than the WM body */
    draw_rect(x, y, wd, ht, rgb(0x1A, 0x22, 0x32));

    int line_y = y + 24;
    int line_h = text_height(1) + 8;

    draw_text(x + 24, line_y, "ATOMiK OS", 3, ATOMIK_FG);            line_y += 48;
    draw_text(x + 24, line_y, AOS_VERSION " -- invariant frame + Document", 1, ATOMIK_ACCENT);
    line_y += line_h * 2;

    const char *lines[] = {
        "Hardware:",
        "  AX7020 (Xilinx Zynq-7020, XC7Z020-2CLG484-2)",
        "  NaxRiscv RV64GC @ 100 MHz, 32 KB L2",
        "  ATOMiK delta-state adapter @ 0xF0020000",
        "  HDMI 1920x1080@30 via AXI HP0 framebuffer",
        "",
        "Software:",
        "  Linux 6.9.0 + Buildroot RV64 rootfs",
        "  atomik_os user-space compositor",
        "  18.5 KB stripped riscv64 ELF",
        "",
        "Why this exists:",
        "  delta-state algebra makes adaptive UIs cheap.",
        "  every click is a delta. every habit is an accumulator.",
        "  the OS learns YOU.",
        "",
        "Keys:",
        "  A    open this window",
        "  Tab  cycle window focus",
        "  Esc  close focused window",
        "  Q    quit ATOMiK OS",
    };
    for (size_t i = 0; i < sizeof lines / sizeof lines[0]; i++) {
        draw_text(x + 24, line_y, lines[i], 1, ATOMIK_FG_DIM);
        line_y += line_h;
        if (line_y > y + ht - line_h) break;
    }
}
