/* draw.c — primitives. All operate on the back buffer. */
#include "atomik_os.h"

extern pixel_t *fb_back(void);

static inline pixel_t *bb(void) { return fb_back(); }

static inline int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void draw_pixel(int x, int y, pixel_t color) {
    if (x < 0 || y < 0 || x >= FB_W || y >= FB_H) return;
    bb()[y * FB_W + x] = color;
}

void draw_blend_pixel(int x, int y, pixel_t color, uint8_t alpha) {
    if (x < 0 || y < 0 || x >= FB_W || y >= FB_H) return;
    pixel_t *p = &bb()[y * FB_W + x];
    if (alpha == 0xFF) { *p = color; return; }
    if (alpha == 0)    return;
    uint32_t dst = *p;
    uint32_t inv = 255 - alpha;
    uint32_t r = (((color >> 16) & 0xff) * alpha + ((dst >> 16) & 0xff) * inv) / 255;
    uint32_t g = (((color >>  8) & 0xff) * alpha + ((dst >>  8) & 0xff) * inv) / 255;
    uint32_t b = (((color >>  0) & 0xff) * alpha + ((dst >>  0) & 0xff) * inv) / 255;
    *p = (r << 16) | (g << 8) | b;
}

void draw_rect(int x, int y, int w, int h, pixel_t color) {
    int x0 = clamp_int(x,         0, FB_W);
    int y0 = clamp_int(y,         0, FB_H);
    int x1 = clamp_int(x + w,     0, FB_W);
    int y1 = clamp_int(y + h,     0, FB_H);
    for (int yy = y0; yy < y1; yy++) {
        pixel_t *row = &bb()[yy * FB_W];
        for (int xx = x0; xx < x1; xx++) row[xx] = color;
    }
}

void draw_gradient_v(int x, int y, int w, int h, pixel_t top, pixel_t bot) {
    int x0 = clamp_int(x,     0, FB_W);
    int y0 = clamp_int(y,     0, FB_H);
    int x1 = clamp_int(x + w, 0, FB_W);
    int y1 = clamp_int(y + h, 0, FB_H);
    int span = h > 0 ? h : 1;
    int tr = (top >> 16) & 0xff, tg = (top >> 8) & 0xff, tb = top & 0xff;
    int br = (bot >> 16) & 0xff, bg = (bot >> 8) & 0xff, bb_ = bot & 0xff;
    for (int yy = y0; yy < y1; yy++) {
        int t      = ((yy - y) * 1024) / span;
        int inv    = 1024 - t;
        uint32_t r = (tr * inv + br * t) / 1024;
        uint32_t g = (tg * inv + bg * t) / 1024;
        uint32_t b = (tb * inv + bb_ * t) / 1024;
        pixel_t color = (r << 16) | (g << 8) | b;
        pixel_t *row  = &bb()[yy * FB_W];
        for (int xx = x0; xx < x1; xx++) row[xx] = color;
    }
}

/* Filled rounded rectangle with cheap distance-based AA on the corners.
 * radius >= 1, otherwise falls back to plain rect. */
void draw_rect_rounded(int x, int y, int w, int h, int radius, pixel_t color) {
    if (radius < 1) { draw_rect(x, y, w, h, color); return; }
    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;

    /* Body strips. */
    draw_rect(x + radius, y,             w - 2 * radius, radius,         color);
    draw_rect(x,          y + radius,    w,              h - 2 * radius, color);
    draw_rect(x + radius, y + h - radius, w - 2 * radius, radius,        color);

    /* Four corner quadrants with AA via squared-distance threshold. */
    int r2_in  = (radius - 1) * (radius - 1);
    int r2_out = radius * radius;
    int corners[4][2] = {
        { x + radius,         y + radius },         /* TL */
        { x + w - radius - 1, y + radius },         /* TR */
        { x + radius,         y + h - radius - 1 }, /* BL */
        { x + w - radius - 1, y + h - radius - 1 }, /* BR */
    };
    for (int c = 0; c < 4; c++) {
        int cx = corners[c][0], cy = corners[c][1];
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                /* Only fill in the appropriate quadrant of this corner. */
                if ((c == 0 && (dx > 0  || dy > 0))  ||
                    (c == 1 && (dx < 0  || dy > 0))  ||
                    (c == 2 && (dx > 0  || dy < 0))  ||
                    (c == 3 && (dx < 0  || dy < 0))) continue;
                int d2 = dx * dx + dy * dy;
                if (d2 <= r2_in) {
                    draw_pixel(cx + dx, cy + dy, color);
                } else if (d2 <= r2_out) {
                    /* Linear AA between r-1 and r. */
                    int span    = r2_out - r2_in;
                    int alpha   = 255 - ((d2 - r2_in) * 255 / (span ? span : 1));
                    if (alpha < 0) alpha = 0;
                    if (alpha > 255) alpha = 255;
                    draw_blend_pixel(cx + dx, cy + dy, color, (uint8_t)alpha);
                }
            }
        }
    }
}
