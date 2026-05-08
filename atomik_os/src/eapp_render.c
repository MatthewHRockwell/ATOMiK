/* eapp_render.c — the invariant-frame renderer.
 *
 * One render function per primitive. All edge apps share these. Apps don't
 * write rendering code — they pick a primitive and stream typed fields.
 * That's the entire architectural reduction.
 */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

/* Field-id conventions (per primitive). The manifest determines which
 * fields exist; these constants document the slot meanings the renderer
 * looks at. Apps that don't define a slot just don't render that piece.
 */

/* PRIM_LIST: 0 = header str, 1 = list_t items, 2 = footer str (optional) */
static void render_list(edge_app_t *a, int x, int y, int wd, int ht) {
    /* Header */
    if (a->field_count > 0 && a->fields[0].type == FT_STR) {
        draw_text(x + 16, y + 14, a->fields[0].str, 2, ATOMIK_FG);
    }
    /* List body */
    if (a->field_count > 1 && a->fields[1].type == FT_LIST) {
        const field_value_t *items = &a->fields[1];
        int row_h     = 28;
        int max_rows  = (ht - 80) / row_h;
        int draw_n    = items->list_n < max_rows ? items->list_n : max_rows;
        for (int i = 0; i < draw_n; i++) {
            int yy = y + 60 + i * row_h;
            if (i % 2) draw_rect(x + 12, yy - 4, wd - 24, row_h, rgb(0x12, 0x18, 0x26));
            /* Accent bullet */
            draw_rect_rounded(x + 24, yy + 6, 6, 6, 2, a->accent);
            draw_text(x + 44, yy + 4, items->list[i], 1, ATOMIK_FG);
        }
        if (items->list_n > draw_n) {
            char more[64];
            snprintf(more, sizeof more, "+ %d more", items->list_n - draw_n);
            draw_text(x + 24, y + 60 + draw_n * row_h, more, 1, ATOMIK_FG_DIM);
        }
    }
    /* Footer (field 2) — REMOVED in v0.31 patch 7.  Vestigial: this
     * predated the unified eapp_draw meta footer below.  Both drew at
     * y+ht-24 and y+ht-22 respectively (2-px gap), overlapping
     * horizontally, producing the user-reported "jumbled text at the
     * bottom of Document" bug.  The eapp_draw meta footer covers the
     * same role.  Apps that have stale field 2 strings still keep them
     * in their schema (so old delta logs replay clean) — they just
     * don't render here. */
}

/* PRIM_CARD: 0 = title, 1 = subtitle, 2 = body */
static void render_card(edge_app_t *a, int x, int y, int wd, int ht) {
    /* Big accent stripe on the left */
    draw_rect(x, y, 6, ht, a->accent);

    if (a->field_count > 0 && a->fields[0].type == FT_STR) {
        draw_text(x + 24, y + 24, a->fields[0].str, 4, ATOMIK_FG);
    }
    if (a->field_count > 1 && a->fields[1].type == FT_STR) {
        draw_text(x + 24, y + 24 + text_height(4) + 8, a->fields[1].str, 1, a->accent);
    }
    if (a->field_count > 2 && a->fields[2].type == FT_STR) {
        /* Body — wrap by width into multiple lines */
        const char *body = a->fields[2].str;
        int char_w = 8;
        int cols   = (wd - 48) / char_w;
        int by     = y + 96;
        int bx     = x + 24;
        char line[256];
        int  lp    = 0;
        while (*body) {
            char c = *body++;
            int wrap = (c == '\n') || (lp >= cols && cols > 0);
            if (wrap) {
                line[lp] = 0;
                draw_text(bx, by, line, 1, ATOMIK_FG_DIM);
                by += text_height(1) + 4;
                lp = 0;
                if (c == '\n') continue;
            }
            if (lp < (int)sizeof line - 1) line[lp++] = c;
        }
        if (lp > 0) {
            line[lp] = 0;
            draw_text(bx, by, line, 1, ATOMIK_FG_DIM);
        }
    }
}

/* PRIM_GRID: 0 = title, 1 = list (each cell is one item) */
static void render_grid(edge_app_t *a, int x, int y, int wd, int ht) {
    if (a->field_count > 0 && a->fields[0].type == FT_STR) {
        draw_text(x + 16, y + 14, a->fields[0].str, 2, ATOMIK_FG);
    }
    if (a->field_count > 1 && a->fields[1].type == FT_LIST) {
        const field_value_t *items = &a->fields[1];
        int cell_w = 140;
        int cell_h = 80;
        int cols   = (wd - 32) / (cell_w + 12);
        if (cols < 1) cols = 1;
        for (int i = 0; i < items->list_n; i++) {
            int cx = x + 16 + (i % cols) * (cell_w + 12);
            int cy = y + 56 + (i / cols) * (cell_h + 12);
            if (cy + cell_h > y + ht - 24) break;
            draw_rect_rounded(cx, cy, cell_w, cell_h, 8, rgb(0x1A, 0x22, 0x32));
            draw_rect_rounded(cx, cy, cell_w, 4, 2, a->accent);
            draw_text(cx + 12, cy + 16, items->list[i], 1, ATOMIK_FG);
        }
    }
}

/* PRIM_FEED: 0 = title, 1 = list (one entry per feed item) */
static void render_feed(edge_app_t *a, int x, int y, int wd, int ht) {
    if (a->field_count > 0 && a->fields[0].type == FT_STR) {
        draw_text(x + 16, y + 14, a->fields[0].str, 2, ATOMIK_FG);
    }
    if (a->field_count > 1 && a->fields[1].type == FT_LIST) {
        const field_value_t *items = &a->fields[1];
        int row_h    = 60;
        int max_rows = (ht - 80) / row_h;
        int draw_n   = items->list_n < max_rows ? items->list_n : max_rows;
        for (int i = 0; i < draw_n; i++) {
            int yy = y + 60 + i * row_h;
            /* Card body */
            draw_rect_rounded(x + 16, yy, wd - 32, row_h - 8, 8,
                              rgb(0x16, 0x1C, 0x2A));
            /* Accent dot */
            for (int dy = -3; dy <= 3; dy++)
                for (int dx = -3; dx <= 3; dx++)
                    if (dx*dx + dy*dy <= 9)
                        draw_pixel(x + 32 + dx, yy + (row_h - 8)/2 + dy, a->accent);
            draw_text(x + 48, yy + 14, items->list[i], 1, ATOMIK_FG);
        }
    }
}

/* PRIM_CONVO: 0 = title, 1 = list (alternating user/agent bubbles) */
static void render_convo(edge_app_t *a, int x, int y, int wd, int ht) {
    if (a->field_count > 0 && a->fields[0].type == FT_STR) {
        draw_text(x + 16, y + 14, a->fields[0].str, 2, ATOMIK_FG);
    }
    if (a->field_count > 1 && a->fields[1].type == FT_LIST) {
        const field_value_t *items = &a->fields[1];
        int row_h    = 56;
        int max_rows = (ht - 80) / row_h;
        int draw_n   = items->list_n < max_rows ? items->list_n : max_rows;
        for (int i = 0; i < draw_n; i++) {
            int yy = y + 60 + i * row_h;
            int is_user = (i & 1);
            int bubble_w = (wd - 64) * 3 / 4;
            int bx = is_user ? (x + wd - 16 - bubble_w) : (x + 16);
            pixel_t bg = is_user ? a->accent : rgb(0x22, 0x2A, 0x3A);
            draw_rect_rounded(bx, yy, bubble_w, row_h - 12, 14, bg);
            draw_text(bx + 16, yy + 14, items->list[i], 1, ATOMIK_FG);
        }
    }
}

void eapp_draw(window_t *w, edge_app_t *a, int x, int y, int wd, int ht) {
    (void)w;
    /* Universal app surface — body fill + a "powered by ATOMiK" footer that
     * reminds the viewer this is a single shared frame. */
    draw_rect(x, y, wd, ht, rgb(0x10, 0x16, 0x22));

    switch (a->primitive) {
        case PRIM_LIST:  render_list (a, x, y, wd, ht); break;
        case PRIM_CARD:  render_card (a, x, y, wd, ht); break;
        case PRIM_GRID:  render_grid (a, x, y, wd, ht); break;
        case PRIM_FEED:  render_feed (a, x, y, wd, ht); break;
        case PRIM_CONVO: render_convo(a, x, y, wd, ht); break;
        default: break;
    }

    /* "Streamed by delta" footer — always present, demonstrating the
     * invariant frame is what's drawing this.
     *
     * v0.31 patch 4: shortened format + hard truncation to the panel
     * width.  The pre-patch format ("%s - primitive: %s - fields: %d
     * - delta-state UI") could exceed wd-32 px and the unclipped
     * draw_text would leak the tail INTO the next panel (Document
     * chat side, etc.) leaving fragments like "- primitive: list..."
     * visible at the bottom of an adjacent window.  Truncate first
     * to ensure the rendered string fits inside wd-32 at scale 1
     * (8 px/char). */
    char meta[160];
    snprintf(meta, sizeof meta,
             "%s | %s | %d fields",
             a->subtitle, eapp_primitive_name(a->primitive), a->field_count);
    int max_chars = (wd - 32) / 8;
    if (max_chars > 0 && (int)strlen(meta) > max_chars) {
        meta[max_chars - 3] = meta[max_chars - 2] = meta[max_chars - 1] = '.';
        meta[max_chars] = 0;
    }
    draw_text(x + 16, y + ht - 22, meta, 1, ATOMIK_FG_DIM);
}
