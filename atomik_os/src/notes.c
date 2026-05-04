/* notes.c — minimal single-buffer text editor.
 *
 * v0.8: one ring buffer of plain text, persisted to disk on every write.
 * No multiple files yet, no syntax highlighting, no scroll — just a place
 * to scratch a thought and have it survive a reboot. This is the seed of
 * proving that the OS can host real, persistent user data. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NOTES_PATH       "/tmp/atomik_os_notes.txt"
#define NOTES_BUF_SIZE   8192

static char s_buf[NOTES_BUF_SIZE];
static int  s_len     = 0;
static int  s_loaded  = 0;
static int  s_dirty   = 0;

static void notes_save(void) {
    FILE *f = fopen(NOTES_PATH, "wb");
    if (!f) return;
    fwrite(s_buf, 1, s_len, f);
    fclose(f);
    s_dirty = 0;
}

static void notes_load(void) {
    if (s_loaded) return;
    s_loaded = 1;
    FILE *f = fopen(NOTES_PATH, "rb");
    if (!f) return;
    s_len = (int)fread(s_buf, 1, NOTES_BUF_SIZE - 1, f);
    fclose(f);
    if (s_len < 0) s_len = 0;
    s_buf[s_len] = 0;
}

void notes_open(void) { notes_load(); }

void notes_handle_key(int key) {
    notes_load();
    if (key == 0x7F || key == '\b') {
        if (s_len > 0) { s_len--; s_buf[s_len] = 0; s_dirty = 1; }
        return;
    }
    if (key == '\n' || key == '\r') {
        if (s_len < NOTES_BUF_SIZE - 1) { s_buf[s_len++] = '\n'; s_dirty = 1; }
        return;
    }
    /* Ctrl-S explicit save (also auto-save below) */
    if (key == 0x13) { notes_save(); return; }
    if (key >= 32 && key < 127) {
        if (s_len < NOTES_BUF_SIZE - 1) {
            s_buf[s_len++] = (char)key;
            s_dirty = 1;
        }
    }
    /* Auto-save every ~64 keystrokes so a crash loses at most one line. */
    if (s_dirty && (s_len & 0x3F) == 0) notes_save();
}

void notes_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    notes_load();
    draw_rect(x, y, wd, ht, rgb(0x0F, 0x14, 0x1E));

    /* Status strip */
    char hdr[80];
    snprintf(hdr, sizeof hdr, "%d / %d bytes  %s",
             s_len, NOTES_BUF_SIZE - 1,
             s_dirty ? "[unsaved]" : "saved");
    draw_text(x + 12, y + 8, hdr, 1,
              s_dirty ? rgb(0xFF, 0xCB, 0x4A) : ATOMIK_FG_DIM);

    /* Render the buffer with simple line wrapping at the window width. */
    int char_w   = 8;
    int char_h   = 16;
    int margin   = 16;
    int cols     = (wd - 2 * margin) / char_w;
    if (cols < 8) cols = 8;
    int row      = 0;
    int col      = 0;
    char line[256];
    int  lp = 0;
    int  base_y = y + 36;
    for (int i = 0; i < s_len; i++) {
        char c = s_buf[i];
        int wrap = (c == '\n') || (col >= cols);
        if (wrap) {
            line[lp] = 0;
            draw_text(x + margin, base_y + row * char_h, line, 1, ATOMIK_FG);
            row++;
            col = 0;
            lp  = 0;
            if (c == '\n') continue;
        }
        if (lp < (int)sizeof line - 1) line[lp++] = c;
        col++;
        if (base_y + row * char_h > y + ht - char_h) break;
    }
    if (lp > 0) {
        line[lp] = 0;
        draw_text(x + margin, base_y + row * char_h, line, 1, ATOMIK_FG);
    }

    /* Footer hint */
    draw_text(x + 12, y + ht - 22,
              "type to edit  -  Ctrl-S save  -  auto-saves periodically",
              1, ATOMIK_FG_DIM);
}
