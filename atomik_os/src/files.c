/* files.c — minimal file browser app.
 *
 * Lists the contents of a directory. Up/down arrows (or k/j) move the
 * selection, Enter descends into directories, Backspace ascends.
 * No file operations yet — read-only navigation. v0.6.1 will add open-with
 * dispatch (text → editor app, ELF → "would you like to install this?"),
 * which is the seed of the apps installer.
 */
#include "atomik_os.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define FILES_MAX_ENTRIES 256
#define FILES_NAME_MAX    96

typedef struct {
    char name[FILES_NAME_MAX];
    int  is_dir;
} entry_t;

static char    s_cwd[256] = "/";
static entry_t s_entries[FILES_MAX_ENTRIES];
static int     s_n_entries = 0;
static int     s_selection = 0;
static int     s_scroll    = 0;

static int cmp_entries(const void *a, const void *b) {
    const entry_t *ea = a, *eb = b;
    if (ea->is_dir != eb->is_dir) return eb->is_dir - ea->is_dir;  /* dirs first */
    return strcmp(ea->name, eb->name);
}

static void files_reload(void) {
    s_n_entries = 0;
    s_selection = 0;
    s_scroll    = 0;
    DIR *d = opendir(s_cwd);
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d)) && s_n_entries < FILES_MAX_ENTRIES) {
        if (de->d_name[0] == '.' && de->d_name[1] == 0) continue; /* skip "." */
        snprintf(s_entries[s_n_entries].name, FILES_NAME_MAX, "%s", de->d_name);
        char path[512];
        snprintf(path, sizeof path, "%s/%s",
                 strcmp(s_cwd, "/") == 0 ? "" : s_cwd, de->d_name);
        struct stat st;
        s_entries[s_n_entries].is_dir =
            (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) ? 1 : 0;
        s_n_entries++;
    }
    closedir(d);
    qsort(s_entries, s_n_entries, sizeof s_entries[0], cmp_entries);
}

void files_open(void) {
    if (s_n_entries == 0) {
        snprintf(s_cwd, sizeof s_cwd, "/");
        files_reload();
    }
}

static void files_descend(void) {
    if (s_selection < 0 || s_selection >= s_n_entries) return;
    entry_t *e = &s_entries[s_selection];
    if (!e->is_dir) return;
    if (strcmp(e->name, "..") == 0) {
        /* Pop one component */
        char *slash = strrchr(s_cwd, '/');
        if (slash && slash != s_cwd) *slash = 0;
        else snprintf(s_cwd, sizeof s_cwd, "/");
    } else {
        char tmp[256];
        snprintf(tmp, sizeof tmp, "%s%s%s",
                 s_cwd,
                 strcmp(s_cwd, "/") == 0 ? "" : "/",
                 e->name);
        snprintf(s_cwd, sizeof s_cwd, "%s", tmp);
    }
    files_reload();
}

static void files_ascend(void) {
    char *slash = strrchr(s_cwd, '/');
    if (slash && slash != s_cwd) {
        *slash = 0;
    } else {
        snprintf(s_cwd, sizeof s_cwd, "/");
    }
    files_reload();
}

void files_handle_key(int key) {
    if (key == 'j' || key == 'J') {
        if (s_selection < s_n_entries - 1) s_selection++;
    } else if (key == 'k' || key == 'K') {
        if (s_selection > 0) s_selection--;
    } else if (key == '\n' || key == '\r') {
        files_descend();
    } else if (key == 0x7F /* DEL */ || key == '\b') {
        files_ascend();
    }
}

void files_draw(window_t *w, int x, int y, int wd, int ht) {
    (void)w;
    files_open();   /* lazy first-load */

    draw_rect(x, y, wd, ht, rgb(0x14, 0x1A, 0x28));

    /* Path bar */
    draw_rect(x, y, wd, 36, rgb(0x1F, 0x27, 0x38));
    char path_label[300];
    snprintf(path_label, sizeof path_label, "%s", s_cwd);
    draw_text(x + 12, y + 10, path_label, 1, ATOMIK_FG);

    /* List */
    int row_h     = 24;
    int max_rows  = (ht - 60) / row_h;
    if (s_selection < s_scroll) s_scroll = s_selection;
    if (s_selection >= s_scroll + max_rows)
        s_scroll = s_selection - max_rows + 1;

    int list_y = y + 44;
    for (int i = 0; i < max_rows && (s_scroll + i) < s_n_entries; i++) {
        int idx = s_scroll + i;
        int yy  = list_y + i * row_h;
        int sel = (idx == s_selection);
        if (sel) draw_rect(x + 4, yy - 2, wd - 8, row_h, ATOMIK_ACCENT_DIM);

        const entry_t *e = &s_entries[idx];
        /* Tiny "icon" — folder = filled square in amber, file = dim square */
        pixel_t icon_color = e->is_dir ? rgb(0xC9, 0x8C, 0x3C)
                                       : rgb(0x4F, 0x5C, 0x70);
        draw_rect_rounded(x + 16, yy + 4, 12, 12, 2, icon_color);

        draw_text(x + 36, yy + 4, e->name, 1,
                  sel ? ATOMIK_FG : ATOMIK_FG_DIM);
    }

    /* Help footer */
    char foot[128];
    snprintf(foot, sizeof foot,
             "%d entries  -  j/k move  -  Enter open  -  Backspace up",
             s_n_entries);
    draw_text(x + 12, y + ht - 22, foot, 1, ATOMIK_FG_DIM);
}
