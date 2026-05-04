/* dock.c — bottom dock with placeholder app icons. */
#include "atomik_os.h"
#include <string.h>

#define DOCK_HEIGHT      88
#define DOCK_PADDING_X   24
#define DOCK_PADDING_Y   12
#define ICON_SIZE        56
#define ICON_GAP         16
#define DOCK_RADIUS      18

static const struct {
    const char *label;
    pixel_t     color;
} ICONS[] = {
    { "Terminal",  rgb(0x36, 0x44, 0x60) },
    { "Files",     rgb(0xC9, 0x8C, 0x3C) },
    { "Editor",    rgb(0x4F, 0xC3, 0xFF) },
    { "Monitor",   rgb(0x6E, 0xC4, 0x6E) },
    { "ATOMiK",    rgb(0xFF, 0x6F, 0x91) },
    { "About",     rgb(0xA8, 0xB2, 0xC4) },
};
#define N_ICONS ((int)(sizeof(ICONS) / sizeof(ICONS[0])))

int dock_count(void) { return N_ICONS; }

static int dock_x0(void) {
    int total_w = N_ICONS * ICON_SIZE + (N_ICONS - 1) * ICON_GAP +
                  2 * DOCK_PADDING_X;
    return (FB_W - total_w) / 2;
}

static int dock_y0(void) {
    return FB_H - DOCK_HEIGHT - 16;
}

void dock_draw(int hover_index) {
    int total_w = N_ICONS * ICON_SIZE + (N_ICONS - 1) * ICON_GAP +
                  2 * DOCK_PADDING_X;
    int dx = dock_x0();
    int dy = dock_y0();

    /* Dock background panel — translucent dark, rounded. */
    draw_rect_rounded(dx, dy, total_w, DOCK_HEIGHT, DOCK_RADIUS,
                      ATOMIK_DOCK_BG & 0xFFFFFF);

    /* 1px border highlight */
    draw_rect_rounded(dx, dy, total_w, 1,             DOCK_RADIUS,
                      ATOMIK_DOCK_BORDER);

    int ix = dx + DOCK_PADDING_X;
    int iy = dy + DOCK_PADDING_Y;
    for (int i = 0; i < N_ICONS; i++) {
        int hover = (i == hover_index);
        int size  = hover ? ICON_SIZE + 8 : ICON_SIZE;
        int off   = (ICON_SIZE - size) / 2;
        draw_rect_rounded(ix + off, iy + off, size, size, 12, ICONS[i].color);

        /* First-letter chip for now — real glyph icons later. */
        char ch[2] = { ICONS[i].label[0], 0 };
        int  scale = 3;
        int  tw    = text_width(ch, scale);
        int  th    = text_height(scale);
        draw_text(ix + off + (size - tw) / 2,
                  iy + off + (size - th) / 2,
                  ch, scale, ATOMIK_FG);

        ix += ICON_SIZE + ICON_GAP;
    }
}

int dock_hit_test(int mouse_x, int mouse_y) {
    int dy = dock_y0();
    if (mouse_y < dy || mouse_y > dy + DOCK_HEIGHT) return -1;
    int ix = dock_x0() + DOCK_PADDING_X;
    int iy = dy + DOCK_PADDING_Y;
    for (int i = 0; i < N_ICONS; i++) {
        if (mouse_x >= ix && mouse_x < ix + ICON_SIZE &&
            mouse_y >= iy && mouse_y < iy + ICON_SIZE) return i;
        ix += ICON_SIZE + ICON_GAP;
    }
    return -1;
}
