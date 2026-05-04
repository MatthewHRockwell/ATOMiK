/* document.c — universal Document app.
 *
 * One app. Split-pane: invariant-frame primitive renderer on the left,
 * chat / command panel on the right. The user types a command; a small
 * grammar parser maps it to field-delta operations on the underlying
 * edge_app_t; the renderer redraws.
 *
 * v0.10: typed mini-language only. v0.12 swaps the parser for an LLM
 * call without touching anything below it. The agent is the
 * configuration interface.
 *
 * Grammar (case-insensitive, simple split-on-space):
 *   set primitive <list|card|grid|feed|convo>
 *   set accent <hex|cyan|amber|pink|green|white>
 *   set header "<text>"           (or: set header <text>)
 *   set subtitle "<text>"
 *   set body "<text>"
 *   clear list
 *   add "<item>"                  (or: add <item>)
 *   load <calendar|tasks|code|brief|chat>
 *   help
 */
#include "atomik_os.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Persistence: v0.11+ uses the field-delta wire format (delta_log.c).
 * The legacy fwrite-of-struct path stays available as a one-shot
 * fallback if the delta log is corrupt. */
#define DOC_DELTA_PATH  "/tmp/atomik_os_document.deltas"
#define DOC_STATE_PATH  "/tmp/atomik_os_document.state"
#define DOC_STATE_MAGIC 0xA01D0CE7u  /* "ATOMiK doc v1" */

#define DOC_HISTORY_LINES 32
#define DOC_LINE_MAX      160
#define DOC_INPUT_MAX     200

static edge_app_t s_doc;
static int        s_doc_init = 0;
static char       s_history[DOC_HISTORY_LINES][DOC_LINE_MAX];
static int        s_n_history = 0;
static char       s_input[DOC_INPUT_MAX];
static int        s_input_len = 0;

/* Persist the Document via the v0.11 delta wire format. Same encoding
 * works for streaming (just point delta_log_open() at a pipe / socket).
 * On replay we start from a fresh edge_app_t skeleton, then apply the
 * snapshot delta log to reconstruct state. */
static void doc_save_state(void) {
    delta_snapshot_to_file(DOC_DELTA_PATH, &s_doc);
}

static int doc_load_state(void) {
    /* Initialize a clean schema first so delta replay has slots to fill. */
    eapp_init(&s_doc, "Document",
              "type a command on the right -> the document morphs",
              PRIM_LIST, ATOMIK_ACCENT);
    eapp_add_field(&s_doc, FT_STR);
    eapp_add_field(&s_doc, FT_LIST);
    eapp_add_field(&s_doc, FT_STR);

    int n = delta_replay_file(DOC_DELTA_PATH, &s_doc);
    if (n > 0) return 1;

    /* Fallback: legacy struct dump from v0.10.1. Promote it to deltas on
     * the next save so the legacy file fades out naturally. */
    FILE *f = fopen(DOC_STATE_PATH, "rb");
    if (!f) return 0;
    uint32_t magic = 0;
    if (fread(&magic, sizeof magic, 1, f) == 1 && magic == DOC_STATE_MAGIC &&
        fread(&s_doc, sizeof s_doc, 1, f) == 1) {
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

/* Push a line into the chat history ring (newest at the end). */
static void hist_push(const char *prefix, const char *line) {
    char buf[DOC_LINE_MAX];
    snprintf(buf, sizeof buf, "%s%s", prefix ? prefix : "", line ? line : "");
    if (s_n_history >= DOC_HISTORY_LINES) {
        for (int i = 0; i < DOC_HISTORY_LINES - 1; i++)
            memcpy(s_history[i], s_history[i+1], DOC_LINE_MAX);
        s_n_history = DOC_HISTORY_LINES - 1;
    }
    snprintf(s_history[s_n_history++], DOC_LINE_MAX, "%s", buf);
}

static void doc_lazy_init(void) {
    if (s_doc_init) return;
    s_doc_init = 1;

    /* Try to restore prior state first. The whole point of the Document
     * model is that the user's morphs accumulate over time — closing and
     * reopening should not wipe them. */
    if (doc_load_state()) {
        hist_push("agent> ", "restored prior session.");
        return;
    }

    eapp_init(&s_doc, "Document",
              "type a command on the right -> the document morphs",
              PRIM_LIST, ATOMIK_ACCENT);
    /* Standard 3-field convention used by all primitives in eapp_render: */
    eapp_add_field(&s_doc, FT_STR);   /* 0: header  */
    eapp_add_field(&s_doc, FT_LIST);  /* 1: items   */
    eapp_add_field(&s_doc, FT_STR);   /* 2: footer / subtitle / body */
    eapp_set_str(&s_doc, 0, "New Document");
    eapp_list_append(&s_doc, 1, "Welcome to ATOMiK OS.");
    eapp_list_append(&s_doc, 1, "I am the agent. Tell me what to show.");
    eapp_list_append(&s_doc, 1, "Try: load calendar  /  set primitive feed");
    eapp_list_append(&s_doc, 1, "     add \"my new task\"  /  set accent cyan");
    eapp_list_append(&s_doc, 1, "     help");
    eapp_set_str(&s_doc, 2,
                 "primitive=list  -  fields=3  -  delta-streamed");

    hist_push("agent> ", "ready. say what you want to see.");
    doc_save_state();
}

/* Strip leading/trailing whitespace and outer quotes if present. */
static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n')) {
        e--; *e = 0;
    }
    if (*s == '"' && e > s && e[-1] == '"') { s++; e[-1] = 0; }
    return s;
}

static int ieq(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* Pop first whitespace-delimited token; advances *p. Returns token start
 * (NUL-terminated in place) or NULL when exhausted. */
static char *pop_token(char **p) {
    char *s = *p;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == 0) { *p = s; return NULL; }
    if (*s == '"') {
        char *e = ++s;
        while (*e && *e != '"') e++;
        char *tok = s;
        if (*e == '"') { *e = 0; e++; }
        *p = e;
        return tok;
    }
    char *tok = s;
    while (*s && *s != ' ' && *s != '\t') s++;
    if (*s) { *s = 0; s++; }
    *p = s;
    return tok;
}

static pixel_t accent_by_name(const char *name) {
    if (ieq(name, "cyan"))   return rgb(0x4F, 0xC3, 0xFF);
    if (ieq(name, "amber"))  return rgb(0xFF, 0xCB, 0x4A);
    if (ieq(name, "pink"))   return rgb(0xFF, 0x6F, 0x91);
    if (ieq(name, "green"))  return rgb(0x6E, 0xC4, 0x6E);
    if (ieq(name, "white"))  return ATOMIK_FG;
    if (ieq(name, "lavender"))return rgb(0xB3, 0x88, 0xFF);
    /* Hex: #RRGGBB or RRGGBB */
    if (name[0] == '#') name++;
    if (strlen(name) == 6) {
        unsigned int v = 0;
        if (sscanf(name, "%x", &v) == 1) return v & 0xFFFFFF;
    }
    return ATOMIK_ACCENT;
}

static primitive_t prim_by_name(const char *name) {
    if (ieq(name, "list"))  return PRIM_LIST;
    if (ieq(name, "card"))  return PRIM_CARD;
    if (ieq(name, "grid"))  return PRIM_GRID;
    if (ieq(name, "feed"))  return PRIM_FEED;
    if (ieq(name, "convo") || ieq(name, "conversation") || ieq(name, "chat"))
        return PRIM_CONVO;
    return PRIM_LIST;
}

/* Built-in presets so the user can leap to a known good state. */
static void load_preset(const char *name) {
    if (ieq(name, "calendar")) {
        s_doc.primitive = PRIM_GRID;
        s_doc.accent    = rgb(0x4F, 0xC3, 0xFF);
        eapp_set_str(&s_doc, 0, "May 2026");
        eapp_clear_list(&s_doc, 1);
        static const char *CELLS[] = {
            "27","28","29","30","1","2","3",
            "4","5","6 - sync","7","8","9","10 - lunch w/ bob",
            "11","12 - 1:1","13","14","15","16","17",
            "18","19","20 - sprint","21","22","23","24",
            "25","26","27 - launch","28","29","30","31"
        };
        for (size_t i = 0; i < sizeof CELLS / sizeof CELLS[0]; i++)
            eapp_list_append(&s_doc, 1, CELLS[i]);
    } else if (ieq(name, "tasks")) {
        s_doc.primitive = PRIM_LIST;
        s_doc.accent    = rgb(0x6E, 0xC4, 0x6E);
        eapp_set_str(&s_doc, 0, "Today");
        eapp_clear_list(&s_doc, 1);
        eapp_list_append(&s_doc, 1, "Ship Document app v0.10");
        eapp_list_append(&s_doc, 1, "Wire chat panel command grammar");
        eapp_list_append(&s_doc, 1, "Demo on HDMI: morph through 5 primitives");
        eapp_list_append(&s_doc, 1, "Plan v0.11 speech input");
    } else if (ieq(name, "code")) {
        s_doc.primitive = PRIM_FEED;
        s_doc.accent    = rgb(0xFF, 0x6F, 0x91);
        eapp_set_str(&s_doc, 0, "Review queue");
        eapp_clear_list(&s_doc, 1);
        eapp_list_append(&s_doc, 1, "PR #142  Document app skeleton");
        eapp_list_append(&s_doc, 1, "PR #141  invariant-frame runtime");
        eapp_list_append(&s_doc, 1, "PR #140  Markov agent + persistence");
    } else if (ieq(name, "brief")) {
        s_doc.primitive = PRIM_CARD;
        s_doc.accent    = rgb(0xFF, 0xCB, 0x4A);
        eapp_set_str(&s_doc, 0, "Tuesday, May 6 2026");
        eapp_set_str(&s_doc, 2,
            "3 events  -  6 tasks  -  5 PRs to review.\nFocus block 9-11am.");
    } else if (ieq(name, "chat")) {
        s_doc.primitive = PRIM_CONVO;
        s_doc.accent    = rgb(0x4F, 0xC3, 0xFF);
        eapp_set_str(&s_doc, 0, "ATOMiK Agent");
        eapp_clear_list(&s_doc, 1);
        eapp_list_append(&s_doc, 1, "agent: ready. what should I show?");
        eapp_list_append(&s_doc, 1, "you: morph the document into something new");
    } else {
        hist_push("agent> ", "unknown preset.");
        return;
    }
    char buf[64];
    snprintf(buf, sizeof buf, "loaded preset: %s", name);
    hist_push("agent> ", buf);
}

/* Parse one chat line and apply it as a delta to s_doc. */
static void apply_command(const char *raw_line) {
    char line[DOC_INPUT_MAX];
    snprintf(line, sizeof line, "%s", raw_line);
    char *p = line;
    char *cmd = pop_token(&p);
    if (!cmd) return;
    hist_push("you> ", raw_line);

    if (ieq(cmd, "help")) {
        hist_push("agent> ", "set primitive <list|card|grid|feed|convo>");
        hist_push("agent> ", "set accent <cyan|amber|pink|green|white|#hex>");
        hist_push("agent> ", "set header \"...\"  /  set body \"...\"");
        hist_push("agent> ", "clear list  /  add \"item\"");
        hist_push("agent> ", "load <calendar|tasks|code|brief|chat>");
        hist_push("agent> ", "save  /  reset");
        return;
    }
    if (ieq(cmd, "save")) {
        doc_save_state();
        hist_push("agent> ", "state persisted to disk.");
        return;
    }
    if (ieq(cmd, "reset")) {
        s_doc_init = 0;
        doc_lazy_init();        /* will rebuild defaults if no save exists */
        hist_push("agent> ", "reset to default state.");
        return;
    }
    if (ieq(cmd, "set")) {
        char *what = pop_token(&p);
        if (!what) { hist_push("agent> ", "set what?"); return; }
        char *val  = trim(p);
        if (ieq(what, "primitive")) {
            s_doc.primitive = prim_by_name(val);
            hist_push("agent> ", "primitive updated.");
        } else if (ieq(what, "accent")) {
            s_doc.accent = accent_by_name(val);
            hist_push("agent> ", "accent updated.");
        } else if (ieq(what, "header")) {
            eapp_set_str(&s_doc, 0, val);
            hist_push("agent> ", "header set.");
        } else if (ieq(what, "subtitle") || ieq(what, "footer") ||
                   ieq(what, "body")) {
            eapp_set_str(&s_doc, 2, val);
            hist_push("agent> ", "field set.");
        } else {
            hist_push("agent> ", "I don't know that field.");
        }
        return;
    }
    if (ieq(cmd, "clear")) {
        char *what = pop_token(&p);
        if (what && ieq(what, "list")) {
            eapp_clear_list(&s_doc, 1);
            hist_push("agent> ", "list cleared.");
        } else {
            hist_push("agent> ", "clear what? try: clear list");
        }
        return;
    }
    if (ieq(cmd, "add")) {
        char *val = trim(p);
        if (val && val[0]) {
            eapp_list_append(&s_doc, 1, val);
            hist_push("agent> ", "added.");
        }
        return;
    }
    if (ieq(cmd, "load")) {
        char *what = pop_token(&p);
        if (what) load_preset(what);
        else hist_push("agent> ", "load what? calendar/tasks/code/brief/chat");
        return;
    }
    hist_push("agent> ", "I didn't catch that. try 'help'.");
}

/* Auto-save after every apply_command — every keystroke trip into the
 * Document is a delta and we don't want to lose any. The file is small
 * (~bytes) so this is cheap. */
static void apply_and_persist(const char *line) {
    apply_command(line);
    doc_save_state();
}

void document_open(void) { doc_lazy_init(); }

void document_handle_key(int key) {
    doc_lazy_init();
    if (key == '\n' || key == '\r') {
        if (s_input_len > 0) {
            s_input[s_input_len] = 0;
            apply_and_persist(s_input);
            s_input_len = 0;
            s_input[0]  = 0;
        }
        return;
    }
    if (key == 0x7F || key == '\b') {
        if (s_input_len > 0) {
            s_input_len--;
            s_input[s_input_len] = 0;
        }
        return;
    }
    if (key >= 32 && key < 127 && s_input_len < DOC_INPUT_MAX - 1) {
        s_input[s_input_len++] = (char)key;
        s_input[s_input_len]   = 0;
    }
}

/* Two-pane render: document body left, chat panel right. */
void document_draw(window_t *w, int x, int y, int wd, int ht) {
    doc_lazy_init();

    int chat_w   = 360;
    int doc_w    = wd - chat_w - 8;
    int chat_x   = x + doc_w + 8;

    /* Left pane: render the document via the invariant frame */
    eapp_draw(w, &s_doc, x, y, doc_w, ht);

    /* Right pane: chat panel */
    draw_rect(chat_x, y, chat_w, ht, rgb(0x0E, 0x12, 0x1C));
    /* Vertical divider */
    draw_rect(chat_x - 1, y, 1, ht, rgb(0x22, 0x2A, 0x3A));

    /* Chat header */
    draw_text(chat_x + 14, y + 12, "Agent", 2, ATOMIK_ACCENT);
    draw_text(chat_x + 14, y + 14 + text_height(2),
              "type a command -> the doc morphs",
              1, ATOMIK_FG_DIM);

    /* Chat history (newest at bottom; scroll if overflow) */
    int hist_top = y + 14 + text_height(2) + text_height(1) + 16;
    int line_h   = text_height(1) + 6;
    int visible  = (ht - 80 - (hist_top - y)) / line_h;
    if (visible < 1) visible = 1;
    int start    = s_n_history > visible ? s_n_history - visible : 0;
    for (int i = 0; i < s_n_history - start; i++) {
        int yy = hist_top + i * line_h;
        const char *line = s_history[start + i];
        pixel_t color = (line[0] == 'a') ? ATOMIK_FG : ATOMIK_ACCENT;
        /* Wrap long lines crudely */
        char buf[80];
        snprintf(buf, sizeof buf, "%s", line);
        draw_text(chat_x + 14, yy, buf, 1, color);
    }

    /* Input line */
    int input_y = y + ht - 60;
    draw_rect_rounded(chat_x + 8, input_y, chat_w - 16, 36, 8,
                      rgb(0x1A, 0x22, 0x32));
    draw_text(chat_x + 18, input_y + 10, "> ", 1, ATOMIK_ACCENT);
    draw_text(chat_x + 36, input_y + 10, s_input, 1, ATOMIK_FG);
    /* Cursor */
    int cur_x = chat_x + 36 + text_width(s_input, 1);
    draw_rect(cur_x, input_y + 10, 8, text_height(1), ATOMIK_ACCENT);

    /* Footer */
    draw_text(chat_x + 14, y + ht - 18,
              "Enter run  -  Backspace edit", 1, ATOMIK_FG_DIM);
}
