/* dock.c — Capability Rail (left-anchored vertical dock).
 *
 * v0.34-A introduced the rail.  v0.38-J ran a polish pass that left
 * candy-colored solid squares with giant pixel-font letters — by
 * v0.38-K3 that was the weakest visible surface vs the rest of the
 * premium shell.  ChatGPT 2026-05-16 directive: "The rail should
 * feel like a capability system, not a shortcut column."
 *
 * v0.38-L1 rewrite — concept-aligned capability rail:
 *   - Dark glass cells (rgb(0x10,0x18,0x2C) body) instead of saturated
 *     color squares.  Lane color stays as the cell's BORDER + letter
 *     glyph color, not its FILL.
 *   - AA UI letter glyph (FONT_AA_UI 18 px) replaces the giant
 *     pixel-font scale-4 letter.  Deliberate placeholder for proper
 *     line-style iconography in a later slice.
 *   - Larger breathing room between cells (24 px gap).
 *   - Active app: 2 px outer alpha halo + saturated cell border.
 *   - Open app: small filled dot on the right edge of the cell.
 *   - Atom (Assistant) placeholder cell — visible 6th slot in violet,
 *     action=ACT_NONE until v0.39-A wires the overlay.
 *
 * v0.39-F — product-nav pass:
 *   - Replace single-letter placeholder with simple line-art icons
 *     drawn from draw_rect / draw_blend_pixel primitives.  One
 *     symbol per app (i-in-circle, bar chart, prompt, folder, lines,
 *     atom rosette).  Layered-stroke compliant — every visible
 *     stroke gets a soft alpha halo around it.
 *   - Add per-cell name label below the icon (AA atomik_14 font).
 *   - Cell taller (80×100) to fit icon + label.
 *   - Rail wider accordingly (112 px).  All other v0.38-L1 behavior
 *     preserved.
 *
 * v0.39-G — structure pass:
 *   - Compact "SYSTEM" header at the top of the rail body (atomik_14,
 *     dim slate).  Gives the rail a name so it reads as a structured
 *     capability surface, not an unframed app stack.
 *   - Thin divider line drawn between the second-to-last and last
 *     cells (where the Atom cell sits in the agent_score order), so
 *     the assistant slot visually separates from app slots.
 *   - Layered-stroke compliant (1 px stroke + alpha halo).
 *   - All other v0.39-F behavior preserved.
 */
#include "atomik_os.h"
#include <string.h>

/* === geometry === */
#define RAIL_X_MARGIN    ATOMIK_GRID_L
#define RAIL_TOP_MARGIN  (ATOMIK_SAFE_TOP + ATOMIK_PULSE_BAR_H + ATOMIK_GRID_L * 2)
#define ICON_SIZE        96      /* v0.40 agentic rail: small glyphs, room for labels */
#define CELL_HEIGHT      80      /* concept capability-rail proportions          */
#define GLYPH_REGION_H   46      /* upper portion holds the icon       */
#define LABEL_REGION_H   24      /* lower portion holds the AA label   */
#define ICON_GAP         12
#define RAIL_PADDING     ATOMIK_GRID_L
#define RAIL_RADIUS      18
#define LABEL_GAP        ATOMIK_GRID_S
#define HEADER_H         24      /* v0.39-G: space for "SYSTEM" label  */
#define HEADER_BELOW_GAP 8       /* v0.39-G: gap between header & first cell */
#define FOOTER_H         28      /* v0.40: bottom mini-control (diamond + seam) */

#define ACTIVE_HALO      3       /* alpha halo around active cell      */
#define OPEN_DOT_R       4       /* radius of the open-app right dot   */

static const struct {
    const char *label;
    const char *title;
    pixel_t     color;
    action_t    action;
} ICONS[] = {
    /* v0.40 agentic CAPABILITY RAIL (concept-01).  Monochrome cyan line-art;
     * fixed order (no adaptive reorder).  Surfaces that don't exist yet map to
     * ACT_NONE (no-op until built); DASHBOARD is the home/idle active state. */
    { "DASHBOARD", "ATOMiK Desk",     ATOMIK_ACCENT, ACT_NONE          },
    { "AGENTS",    "Atom",            ATOMIK_ACCENT, ACT_OPEN_ASSISTANT },
    { "WORKFLOWS", "Workflows",       ATOMIK_ACCENT, ACT_NONE          },
    { "KNOWLEDGE", "Knowledge",       ATOMIK_ACCENT, ACT_NONE          },
    { "STUDIO",    "Studio",          ATOMIK_ACCENT, ACT_NONE          },
    { "TERMINAL",  "Terminal",        ATOMIK_ACCENT, ACT_OPEN_TERMINAL },
    { "SYSTEM",    "ATOMiK Monitor",  ATOMIK_ACCENT, ACT_OPEN_MONITOR  },
};
#define N_ICONS ((int)(sizeof(ICONS) / sizeof(ICONS[0])))

int dock_count(void) { return N_ICONS; }

int dock_left_edge(void) { return RAIL_X_MARGIN; }
int dock_right_edge(void) {
    return RAIL_X_MARGIN + RAIL_PADDING * 2 + ICON_SIZE;
}

static int rail_x(void) { return RAIL_X_MARGIN; }
static int rail_y(void) { return RAIL_TOP_MARGIN; }
static int rail_w(void) { return RAIL_PADDING * 2 + ICON_SIZE; }
static int rail_h(void) {
    /* v0.39-G — extra HEADER_H + HEADER_BELOW_GAP for the SYSTEM header. */
    return RAIL_PADDING * 2 + HEADER_H + HEADER_BELOW_GAP +
           N_ICONS * CELL_HEIGHT + (N_ICONS - 1) * ICON_GAP + FOOTER_H;
}

static int s_last_order[N_ICONS] = {0,1,2,3,4,5,6};

/* Fixed agentic-rail order (concept capability rail is stable nav, not an
 * adaptively-reordered app dock). */
static void compute_order(int order[N_ICONS]) {
    for (int i = 0; i < N_ICONS; i++) order[i] = i;
}

static int find_window_by_title(const char *title) {
    if (!title) return -1;
    for (int i = 0; i < wm_count(); i++) {
        const window_t *w = wm_get(i);
        if (!w || !w->visible) continue;
        if (strncmp(w->title, title, sizeof w->title) == 0) return w->id;
    }
    return -1;
}

static int app_is_open(int icon_idx) {
    return find_window_by_title(ICONS[icon_idx].title) >= 0;
}

static int app_is_focused(int icon_idx) {
    const window_t *top = wm_topmost();
    if (!top) return 0;
    return strncmp(top->title, ICONS[icon_idx].title,
                   sizeof top->title) == 0;
}

/* v0.39-F line-icon helpers.  Each icon paints a 36×36 line-art
 * symbol centered in (cx, cy).  Strokes are LAYERED — every 1 px
 * line carries a 1 px alpha-50 halo on each side so the icons read
 * as solid weights at 1080p, never as naked single-pixel lines.
 * Layered-stroke discipline; ChatGPT 2026-05-15. */
static void stroke_h(int x, int y, int w, pixel_t color) {
    draw_rect(x, y, w, 1, color);
    for (int dx = 0; dx < w; dx++) {
        draw_blend_pixel(x + dx, y - 1, color, 90);
        draw_blend_pixel(x + dx, y + 1, color, 90);
    }
}
static void stroke_v(int x, int y, int h, pixel_t color) {
    draw_rect(x, y, 1, h, color);
    for (int dy = 0; dy < h; dy++) {
        draw_blend_pixel(x - 1, y + dy, color, 90);
        draw_blend_pixel(x + 1, y + dy, color, 90);
    }
}
static void stroke_circle(int cx, int cy, int radius, pixel_t color) {
    /* Bresenham circle + per-octant halo neighbours. */
    int x = radius, y = 0, err = 0;
    while (x >= y) {
        const int pts[8][2] = {
            { cx + x, cy + y }, { cx - x, cy + y },
            { cx + x, cy - y }, { cx - x, cy - y },
            { cx + y, cy + x }, { cx - y, cy + x },
            { cx + y, cy - x }, { cx - y, cy - x },
        };
        for (int p = 0; p < 8; p++) {
            draw_pixel(pts[p][0], pts[p][1], color);
            draw_blend_pixel(pts[p][0] + 1, pts[p][1],     color, 70);
            draw_blend_pixel(pts[p][0] - 1, pts[p][1],     color, 70);
            draw_blend_pixel(pts[p][0],     pts[p][1] + 1, color, 70);
            draw_blend_pixel(pts[p][0],     pts[p][1] - 1, color, 70);
        }
        y += 1;
        if (err <= 0) { err += 2*y + 1; }
        if (err >  0) { x -= 1; err -= 2*x + 1; }
    }
}
static void stroke_disk(int cx, int cy, int radius, pixel_t color) {
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= r2) {
                draw_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}

/* === Icons ===
 * Each icon paints into a 36×36 box centred on (cx, cy).  Glyphs use
 * a single `color` (mode-aware: dim slate when unfocused, lane color
 * when focused). */
static void icon_about(int cx, int cy, pixel_t color) {
    /* "i" in circle. */
    stroke_circle(cx, cy, 17, color);
    /* Dot above the stem (the i's tittle). */
    stroke_disk(cx, cy - 7, 2, color);
    /* Vertical stem. */
    stroke_v(cx, cy - 2, 11, color);
}
static void icon_monitor(int cx, int cy, pixel_t color) {
    /* Three-bar vertical chart, increasing heights. */
    int base_y = cy + 10;
    int bw = 5;
    int gx = 8;
    int heights[3] = { 9, 14, 19 };
    for (int i = 0; i < 3; i++) {
        int bx = cx - gx + i * gx;
        int bh = heights[i];
        /* Filled rect for the bar (no halo — keep it solid). */
        draw_rect(bx - bw/2, base_y - bh, bw, bh, color);
        /* Faint alpha lift on each side so the bar reads layered. */
        for (int dy = 0; dy < bh; dy++) {
            draw_blend_pixel(bx - bw/2 - 1, base_y - bh + dy, color, 80);
            draw_blend_pixel(bx + bw/2,     base_y - bh + dy, color, 80);
        }
    }
    /* Baseline. */
    stroke_h(cx - 14, base_y + 2, 28, color);
}
static void icon_terminal(int cx, int cy, pixel_t color) {
    /* ">" arrow + underscore. */
    int x = cx - 11;
    /* Two diagonals making the > glyph. */
    for (int t = 0; t < 6; t++) {
        draw_pixel(x + t,         cy - 5 + t, color);
        draw_blend_pixel(x + t + 1, cy - 5 + t, color, 80);
        draw_blend_pixel(x + t,     cy - 5 + t + 1, color, 80);
        draw_pixel(x + t,         cy + 5 - t, color);
        draw_blend_pixel(x + t + 1, cy + 5 - t, color, 80);
        draw_blend_pixel(x + t,     cy + 5 - t + 1, color, 80);
    }
    /* Trailing underscore. */
    stroke_h(cx + 1, cy + 6, 11, color);
}
static void icon_files(int cx, int cy, pixel_t color) {
    /* Folder outline: tab + body. */
    int top    = cy - 11;
    int bottom = cy + 11;
    int left   = cx - 14;
    int right  = cx + 14;
    /* Tab on the left half of the top edge. */
    stroke_h(left,           top - 4, 12, color);
    stroke_v(left,           top - 4, 4,  color);
    stroke_v(left + 12,      top - 4, 4,  color);
    /* Body — rounded-ish rectangle (just 4 strokes). */
    stroke_h(left,           top, right - left,    color);
    stroke_h(left,           bottom, right - left, color);
    stroke_v(left,           top, bottom - top,    color);
    stroke_v(right,          top, bottom - top,    color);
}
static void icon_notes(int cx, int cy, pixel_t color) {
    /* Three stacked horizontal lines (note rule lines), inside a
     * thin page outline. */
    int left   = cx - 12;
    int right  = cx + 12;
    int top    = cy - 13;
    int bottom = cy + 13;
    stroke_h(left,  top,    right - left, color);
    stroke_h(left,  bottom, right - left, color);
    stroke_v(left,  top,    bottom - top, color);
    stroke_v(right, top,    bottom - top, color);
    /* Three rule lines inside. */
    stroke_h(left + 4, cy - 6, right - left - 8, color);
    stroke_h(left + 4, cy,     right - left - 8, color);
    stroke_h(left + 4, cy + 6, right - left - 8, color);
}
static void icon_atom(int cx, int cy, pixel_t color) {
    /* 3-dot rosette around a central node — placeholder for the
     * eventual chromakey'd Atom asset.  Reads as the assistant. */
    stroke_disk(cx, cy, 3, color);
    /* Three satellite dots arranged at 120° offsets. */
    /* Angles 90°, 210°, 330° → (0,1), (-0.866,-0.5), (0.866,-0.5).  */
    int dx_top = 0;     int dy_top = -12;
    int dx_bl  = -10;   int dy_bl  = 6;
    int dx_br  = 10;    int dy_br  = 6;
    stroke_disk(cx + dx_top, cy + dy_top, 2, color);
    stroke_disk(cx + dx_bl,  cy + dy_bl,  2, color);
    stroke_disk(cx + dx_br,  cy + dy_br,  2, color);
    /* Thin connector lines (single pixel + halo) from centre to
     * each satellite, gives the orbital feel. */
    for (int t = 4; t < 11; t++) {
        draw_blend_pixel(cx,     cy - t, color, 110);
        draw_blend_pixel(cx - t, cy + t * 6 / 10, color, 110);
        draw_blend_pixel(cx + t, cy + t * 6 / 10, color, 110);
    }
}

/* === v0.40 agentic line-art helpers === */
static void stroke_line(int x0, int y0, int x1, int y1, pixel_t c) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx - dy;
    for (;;) {
        draw_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}
/* pointy-top hexagon outline, "radius" r */
static void stroke_hex(int cx, int cy, int r, pixel_t c) {
    int hx = r * 866 / 1000, hy = r / 2;
    int vx[6] = { cx,      cx + hx, cx + hx, cx,      cx - hx, cx - hx };
    int vy[6] = { cy - r,  cy - hy, cy + hy, cy + r,  cy + hy, cy - hy };
    for (int i = 0; i < 6; i++)
        stroke_line(vx[i], vy[i], vx[(i+1)%6], vy[(i+1)%6], c);
}

/* === v0.40 agentic capability glyphs (monochrome line-art, ~36px box) === */
static void icon_dashboard(int cx, int cy, pixel_t c) {
    stroke_hex(cx, cy, 15, c);
    stroke_disk(cx, cy, 2, c);
    stroke_disk(cx, cy - 8, 1, c); stroke_disk(cx + 7, cy + 4, 1, c);
    stroke_disk(cx - 7, cy + 4, 1, c);
    stroke_line(cx, cy, cx, cy - 7, c);
    stroke_line(cx, cy, cx + 6, cy + 4, c);
    stroke_line(cx, cy, cx - 6, cy + 4, c);
}
static void icon_agents(int cx, int cy, pixel_t c) {   /* share / network */
    int tx = cx, ty = cy - 11, blx = cx - 11, bly = cy + 9, brx = cx + 11, bry = cy + 9;
    stroke_line(tx, ty, blx, bly, c);
    stroke_line(tx, ty, brx, bry, c);
    stroke_line(blx, bly, brx, bry, c);
    stroke_circle(tx, ty, 4, c);
    stroke_circle(blx, bly, 4, c);
    stroke_circle(brx, bry, 4, c);
}
static void icon_workflows(int cx, int cy, pixel_t c) {  /* branching flow */
    int ny = cy - 11;
    stroke_circle(cx, ny, 3, c);
    stroke_line(cx, ny + 3, cx, cy, c);
    stroke_line(cx, cy, cx - 10, cy, c);
    stroke_line(cx, cy, cx + 10, cy, c);
    stroke_line(cx - 10, cy, cx - 10, cy + 7, c);
    stroke_line(cx + 10, cy, cx + 10, cy + 7, c);
    stroke_circle(cx - 10, cy + 10, 3, c);
    stroke_circle(cx + 10, cy + 10, 3, c);
}
static void icon_knowledge(int cx, int cy, pixel_t c) {  /* iso cube */
    int r = 11, h = r * 866 / 1000;
    int top_x = cx, top_y = cy - r;
    int rgt_x = cx + h, rgt_y = cy - r/2;
    int rb_x  = cx + h, rb_y  = cy + r/2;
    int bot_x = cx, bot_y = cy + r;
    int lb_x  = cx - h, lb_y  = cy + r/2;
    int lt_x  = cx - h, lt_y  = cy - r/2;
    stroke_line(top_x,top_y, rgt_x,rgt_y, c); stroke_line(rgt_x,rgt_y, rb_x,rb_y, c);
    stroke_line(rb_x,rb_y, bot_x,bot_y, c);   stroke_line(bot_x,bot_y, lb_x,lb_y, c);
    stroke_line(lb_x,lb_y, lt_x,lt_y, c);     stroke_line(lt_x,lt_y, top_x,top_y, c);
    stroke_line(top_x,top_y, cx,cy, c); stroke_line(cx,cy, rb_x,rb_y, c);
    stroke_line(cx,cy, lb_x,lb_y, c);
}
static void icon_studio(int cx, int cy, pixel_t c) {     /* 3x3 app grid */
    for (int r = -1; r <= 1; r++)
        for (int q = -1; q <= 1; q++)
            stroke_disk(cx + q * 9, cy + r * 9, 2, c);
}
static void icon_system(int cx, int cy, pixel_t c) {     /* nested hexagons */
    stroke_hex(cx, cy, 15, c);
    stroke_hex(cx, cy, 7, c);
    stroke_disk(cx, cy, 1, c);
}

/* Dispatch table — by icon_idx (matches ICONS[] order). */
static void draw_icon(int icon_idx, int cx, int cy, pixel_t color) {
    switch (icon_idx) {
    case 0: icon_dashboard(cx, cy, color); break;
    case 1: icon_agents(cx, cy, color);    break;
    case 2: icon_workflows(cx, cy, color); break;
    case 3: icon_knowledge(cx, cy, color); break;
    case 4: icon_studio(cx, cy, color);    break;
    case 5: icon_terminal(cx, cy, color);  break;
    case 6: icon_system(cx, cy, color);    break;
    default:                               break;
    }
}

/* Outer alpha halo around (x, y, w, h) — `thickness` rings, each at a
 * decreasing alpha.  Used for the active-cell glow and hover lift. */
static void halo_ring(int x, int y, int w, int h, int thickness,
                      pixel_t color, uint8_t base_alpha) {
    for (int t = 1; t <= thickness; t++) {
        uint8_t a = (uint8_t)(base_alpha - (t - 1) * (base_alpha / (thickness + 1)));
        for (int sx = 0; sx < w + 2 * t; sx++) {
            draw_blend_pixel(x - t + sx, y - t,           color, a);
            draw_blend_pixel(x - t + sx, y + h - 1 + t,   color, a);
        }
        for (int sy = 0; sy < h + 2 * t; sy++) {
            draw_blend_pixel(x - t,           y - t + sy, color, a);
            draw_blend_pixel(x + w - 1 + t,   y - t + sy, color, a);
        }
    }
}

void dock_draw(int hover_index) {
    int rx = rail_x();
    int ry = rail_y();
    int rw = rail_w();
    int rh = rail_h();

    /* Rail background — translucent dark slab.  v0.38-L1: use the
     * same dark-glass body color as the Pulse Bar pills + Fabric
     * card body so the chrome reads as one design system. */
    pixel_t rail_body = rgb(0x10, 0x18, 0x2C);
    draw_rect_rounded(rx, ry, rw, rh, RAIL_RADIUS, rail_body);
    /* Subtle border — 1 px dim cyan along the rail edges. */
    draw_rect(rx + RAIL_RADIUS / 2, ry, rw - RAIL_RADIUS, 1,
              ATOMIK_DOCK_BORDER);
    draw_rect(rx + RAIL_RADIUS / 2, ry + rh - 1, rw - RAIL_RADIUS, 1,
              ATOMIK_DOCK_BORDER);
    /* v0.40 glass polish — side borders + top sheen + inner accent rim so
     * the rail reads as the same glass system as the Pulse Bar / panels. */
    draw_rect(rx,          ry + RAIL_RADIUS / 2, 1, rh - RAIL_RADIUS, ATOMIK_DOCK_BORDER);
    draw_rect(rx + rw - 1, ry + RAIL_RADIUS / 2, 1, rh - RAIL_RADIUS, ATOMIK_DOCK_BORDER);
    for (int sy = 0; sy < 10; sy++)
        for (int sx = RAIL_RADIUS; sx < rw - RAIL_RADIUS; sx++)
            draw_blend_pixel(rx + sx, ry + 1 + sy, ATOMIK_FG, (uint8_t)(10 - sy));
    for (int sx = RAIL_RADIUS; sx < rw - RAIL_RADIUS; sx++)
        draw_blend_pixel(rx + sx, ry + 2, ATOMIK_ACCENT, 20);

    int order[N_ICONS];
    compute_order(order);
    memcpy(s_last_order, order, sizeof order);

    /* v0.39-G — "SYSTEM" header at the top of the rail body.  Sits
     * inside the glass slab, above the first cell.  Centered to the
     * rail width.  Tiny letter-spacing via FONT_AA_LABEL keeps it
     * quiet — no need to compete with the cell labels below. */
    const char *hdr = "CAPABILITY RAIL";
    pixel_t hdr_col = rgb(0x7A, 0x86, 0xA0);
    int header_y = ry + RAIL_PADDING + (HEADER_H -
                       (font_aa_loaded(FONT_AA_LABEL)
                        ? text_height_aa(FONT_AA_LABEL)
                        : text_height(1))) / 2;
    if (font_aa_loaded(FONT_AA_LABEL)) {
        int tw = text_width_aa(FONT_AA_LABEL, hdr);
        draw_text_aa(FONT_AA_LABEL,
                     rx + (rw - tw) / 2,
                     header_y, hdr, hdr_col);
    } else {
        int tw = text_width(hdr, 1);
        draw_text(rx + (rw - tw) / 2, header_y, hdr, 1, hdr_col);
    }

    /* DASHBOARD (slot 0) is the active capability when no app window is
     * focused — i.e. the home/idle state (concept's glowing DASHBOARD tile). */
    int any_app_focused = 0;
    for (int k = 0; k < N_ICONS; k++)
        if (app_is_focused(k)) { any_app_focused = 1; break; }

    int ix = rx + RAIL_PADDING;
    int iy = ry + RAIL_PADDING + HEADER_H + HEADER_BELOW_GAP;
    for (int slot = 0; slot < N_ICONS; slot++) {
        int i        = order[slot];
        int hover    = (slot == hover_index);
        int focused  = app_is_focused(i) || (i == 0 && !any_app_focused);
        int open     = app_is_open(i);
        int icon_x   = ix;
        int icon_y   = iy;
        int cw       = ICON_SIZE;
        int ch       = CELL_HEIGHT;
        pixel_t accent = ICONS[i].color;

        /* === concept-01 clean rail: NO per-cell box or border.  Icons + labels
         * float in the rail; only the ACTIVE item gets a soft glow behind the
         * glyph + a small cyan indicator dot on the right rail edge.  Hover gets
         * a faint glow only. === */
        int gcx0 = icon_x + cw / 2;
        int gcy0 = icon_y + GLYPH_REGION_H / 2 + 4;
        if (focused || hover) {
            int R = focused ? 28 : 22; long R2 = (long)R * R;
            uint8_t peak = focused ? 34 : 14;
            for (int dy = -R; dy <= R; dy++) {
                int yy = gcy0 + dy; if (yy < 0 || yy >= FB_H) continue;
                for (int dx = -R; dx <= R; dx++) {
                    int xx = gcx0 + dx; if (xx < 0 || xx >= FB_W) continue;
                    long d2 = (long)dx*dx + (long)dy*dy; if (d2 >= R2) continue;
                    uint8_t a = (uint8_t)((long)peak * (R2 - d2) / R2);
                    if (a) draw_blend_pixel(xx, yy, accent, a);
                }
            }
        }
        if (focused) {
            int dot_x = rx + rw - 5;
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++)
                    if (dx*dx + dy*dy <= 4) draw_blend_pixel(dot_x + dx, gcy0 + dy, accent, 235);
            for (int g = 1; g <= 4; g++) {
                uint8_t a = (uint8_t)(70 - (g - 1) * 16);
                draw_blend_pixel(dot_x, gcy0 - 2 - g, accent, a);
                draw_blend_pixel(dot_x, gcy0 + 2 + g, accent, a);
            }
        }

        /* === v0.39-F: line-art icon centred in the upper portion
         * of the cell.  Replaces the v0.38-L1 single-letter
         * placeholder glyph. */
        pixel_t glyph_col = focused ? accent : rgb(0x9A, 0xA6, 0xC0);
        int icon_cx = icon_x + cw / 2;
        int icon_cy = icon_y + GLYPH_REGION_H / 2 + 4;
        draw_icon(i, icon_cx, icon_cy, glyph_col);

        /* === v0.39-F: per-cell label below the icon.  Strip the
         * leading '?' that the rail uses internally for Atom (so the
         * "?Atom" first-letter glyph showed `?` in the prior slice).
         * Now that we have proper icons, just show "Atom". */
        const char *lab = ICONS[i].label;
        if (lab[0] == '?') lab++;
        pixel_t label_col = focused ? accent : rgb(0x7A, 0x86, 0xA0);
        if (font_aa_loaded(FONT_AA_LABEL)) {
            int tw  = text_width_aa(FONT_AA_LABEL, lab);
            int lab_y = icon_y + GLYPH_REGION_H +
                        (LABEL_REGION_H - text_height_aa(FONT_AA_LABEL)) / 2;
            draw_text_aa(FONT_AA_LABEL,
                         icon_x + (cw - tw) / 2,
                         lab_y, lab, label_col);
        } else {
            int tw = text_width(lab, 1);
            int lab_y = icon_y + GLYPH_REGION_H +
                        (LABEL_REGION_H - text_height(1)) / 2;
            draw_text(icon_x + (cw - tw) / 2, lab_y, lab, 1, label_col);
        }

        /* === Open-app indicator — small filled dot on the right
         * edge.  Lane color, half-radius outer halo for soft glow. */
        if (open) {
            int dot_x = icon_x + cw + 4;
            int dot_y = icon_y + GLYPH_REGION_H / 2;
            int r = OPEN_DOT_R;
            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    if (dx*dx + dy*dy <= r*r) {
                        draw_pixel(dot_x + dx, dot_y + dy, accent);
                    }
                }
            }
            for (int dy = -r - 2; dy <= r + 2; dy++) {
                for (int dx = -r - 2; dx <= r + 2; dx++) {
                    int d2 = dx*dx + dy*dy;
                    if (d2 > r*r && d2 <= (r + 2) * (r + 2)) {
                        draw_blend_pixel(dot_x + dx, dot_y + dy,
                                         accent, 80);
                    }
                }
            }
        }

        /* === Predicted-next pulse — small violet dot to the right.
         * Kept from v0.34-A but smaller and dimmer to suit the new
         * cell scale. */
        if (ICONS[i].action != ACT_NONE && agent_predict() == ICONS[i].action) {
            unsigned long t  = anim_now_ms();
            double phase = ((t % 1500) / 1500.0) * 6.2831853;
            double s = phase;
            double s3 = s*s*s, s5 = s3*s*s;
            double sinv = s - s3/6.0 + s5/120.0;
            if (sinv >  1.0) sinv =  1.0;
            if (sinv < -1.0) sinv = -1.0;
            double bright = 0.6 + 0.4 * sinv;
            if (bright < 0.0) bright = 0.0;
            if (bright > 1.0) bright = 1.0;
            uint8_t r = (uint8_t)(0xB0 * bright);
            uint8_t g = (uint8_t)(0x8C * bright);
            uint8_t b = (uint8_t)(0xFF * bright);
            pixel_t pulse_color = rgb(r, g, b);
            int dot_x = icon_x + cw + LABEL_GAP + 12;
            int dot_y = icon_y + GLYPH_REGION_H / 2;
            int radius = 3;
            for (int dy2 = -radius; dy2 <= radius; dy2++)
                for (int dx2 = -radius; dx2 <= radius; dx2++)
                    if (dx2*dx2 + dy2*dy2 <= radius*radius)
                        draw_pixel(dot_x + dx2, dot_y + dy2, pulse_color);
            anim_tick();
        }

        iy += CELL_HEIGHT + ICON_GAP;
    }

    /* v0.40 bottom mini-control — cyan diamond + cyan→violet gradient seam
     * (concept rail footer).  Pure decorative chrome, no state. */
    {
        int fy  = ry + rh - FOOTER_H / 2 - 4;
        int fcx = rx + rw / 2;
        for (int dy = -4; dy <= 4; dy++) {
            int wdt = 4 - (dy < 0 ? -dy : dy);
            for (int dx = -wdt; dx <= wdt; dx++)
                draw_blend_pixel(fcx + dx, fy + dy, ATOMIK_ACCENT, 200);
        }
        int lw = rw - RAIL_PADDING * 2;
        int lx = rx + RAIL_PADDING;
        for (int sx = 0; sx < lw; sx++) {
            double t = (double)sx / (double)lw;
            uint8_t r = (uint8_t)(0x6E * (1 - t) + 0xB0 * t);
            uint8_t g = (uint8_t)(0xDD * (1 - t) + 0x8C * t);
            draw_blend_pixel(lx + sx, fy + 9, rgb(r, g, 0xFF), 150);
        }
    }
}

action_t dock_action_for_slot(int slot) {
    if (slot < 0 || slot >= N_ICONS) return ACT_NONE;
    int icon_index = s_last_order[slot];
    return ICONS[icon_index].action;
}

int dock_hit_test(int mouse_x, int mouse_y) {
    int rx = rail_x();
    int ry = rail_y();
    if (mouse_x < rx + RAIL_PADDING ||
        mouse_x >= rx + RAIL_PADDING + ICON_SIZE) return -1;
    int iy = ry + RAIL_PADDING;
    for (int i = 0; i < N_ICONS; i++) {
        if (mouse_y >= iy && mouse_y < iy + CELL_HEIGHT) return i;
        iy += CELL_HEIGHT + ICON_GAP;
    }
    return -1;
}
