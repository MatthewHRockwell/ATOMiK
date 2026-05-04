/* document.c — universal Document app.
 *
 * One app, N concurrent instances. Each Document window owns its own
 * doc_state_t (allocated on the heap, hung off window_t.user). The chat
 * panel mutates the local edge_app_t via the same field-delta grammar
 * the v0.10 hand-rolled parser used; v0.12 swaps that parser for an
 * LLM call without touching anything below it.
 *
 * Persistence: each instance writes /tmp/atomik_os_document_<id>.deltas
 * via the v0.11 wire format. Closing and reopening recovers state
 * per-instance.
 *
 * Grammar (case-insensitive, simple split-on-space):
 *   set primitive <list|card|grid|feed|convo>
 *   set accent <hex|cyan|amber|pink|green|white>
 *   set header "<text>"
 *   set subtitle "<text>"
 *   set body "<text>"
 *   clear list
 *   add "<item>"
 *   load <calendar|tasks|code|brief|chat>
 *   /ai <prompt>
 *   /export <path>  /  /import <path>
 *   help / save / reset
 */
#include "atomik_os.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DOC_HISTORY_LINES 32
#define DOC_LINE_MAX      160
#define DOC_INPUT_MAX     200

typedef struct doc_state {
    int          doc_id;                       /* 1, 2, 3, ... */
    edge_app_t   app;
    char         history[DOC_HISTORY_LINES][DOC_LINE_MAX];
    int          n_history;
    char         input[DOC_INPUT_MAX];
    int          input_len;
} doc_state_t;

static int s_next_doc_id = 1;

static void doc_path(int id, char *out, size_t cap) {
    snprintf(out, cap, "/tmp/atomik_os_document_%d.deltas", id);
}

static void doc_save(doc_state_t *d) {
    char path[80];
    doc_path(d->doc_id, path, sizeof path);
    delta_snapshot_to_file(path, &d->app);
}

static void hist_push(doc_state_t *d, const char *prefix, const char *line) {
    char buf[DOC_LINE_MAX];
    snprintf(buf, sizeof buf, "%s%s", prefix ? prefix : "", line ? line : "");
    if (d->n_history >= DOC_HISTORY_LINES) {
        for (int i = 0; i < DOC_HISTORY_LINES - 1; i++)
            memcpy(d->history[i], d->history[i+1], DOC_LINE_MAX);
        d->n_history = DOC_HISTORY_LINES - 1;
    }
    snprintf(d->history[d->n_history++], DOC_LINE_MAX, "%s", buf);
}

static void doc_init_default(doc_state_t *d) {
    eapp_init(&d->app, "Document",
              "type a command on the right -> the document morphs",
              PRIM_LIST, ATOMIK_ACCENT);
    eapp_add_field(&d->app, FT_STR);   /* 0: header  */
    eapp_add_field(&d->app, FT_LIST);  /* 1: items   */
    eapp_add_field(&d->app, FT_STR);   /* 2: footer  */
    char title[32];
    snprintf(title, sizeof title, "Document #%d", d->doc_id);
    eapp_set_str(&d->app, 0, title);
    eapp_list_append(&d->app, 1, "Welcome to ATOMiK OS.");
    eapp_list_append(&d->app, 1, "I am the agent. Tell me what to show.");
    eapp_list_append(&d->app, 1, "Try: load calendar  /  set primitive feed");
    eapp_list_append(&d->app, 1, "     add \"my new task\"  /  set accent cyan");
    eapp_list_append(&d->app, 1, "     /ai show me my morning  /  help");
    eapp_set_str(&d->app, 2,
                 "primitive=list  -  fields=3  -  delta-streamed");
    hist_push(d, "agent> ", "ready. say what you want to see.");
}

static int doc_load(doc_state_t *d) {
    /* Init clean schema first so delta replay has slots to fill. */
    eapp_init(&d->app, "Document",
              "type a command on the right -> the document morphs",
              PRIM_LIST, ATOMIK_ACCENT);
    eapp_add_field(&d->app, FT_STR);
    eapp_add_field(&d->app, FT_LIST);
    eapp_add_field(&d->app, FT_STR);
    char path[80];
    doc_path(d->doc_id, path, sizeof path);
    int n = delta_replay_file(path, &d->app);
    return n > 0 ? 1 : 0;
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
    if (ieq(name, "cyan"))    return rgb(0x4F, 0xC3, 0xFF);
    if (ieq(name, "amber"))   return rgb(0xFF, 0xCB, 0x4A);
    if (ieq(name, "pink"))    return rgb(0xFF, 0x6F, 0x91);
    if (ieq(name, "green"))   return rgb(0x6E, 0xC4, 0x6E);
    if (ieq(name, "white"))   return ATOMIK_FG;
    if (ieq(name, "lavender"))return rgb(0xB3, 0x88, 0xFF);
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

static void load_preset(doc_state_t *d, const char *name) {
    if (ieq(name, "calendar")) {
        d->app.primitive = PRIM_GRID;
        d->app.accent    = rgb(0x4F, 0xC3, 0xFF);
        eapp_set_str(&d->app, 0, "May 2026");
        eapp_clear_list(&d->app, 1);
        static const char *CELLS[] = {
            "27","28","29","30","1","2","3",
            "4","5","6 - sync","7","8","9","10 - lunch w/ bob",
            "11","12 - 1:1","13","14","15","16","17",
            "18","19","20 - sprint","21","22","23","24",
            "25","26","27 - launch","28","29","30","31"
        };
        for (size_t i = 0; i < sizeof CELLS / sizeof CELLS[0]; i++)
            eapp_list_append(&d->app, 1, CELLS[i]);
    } else if (ieq(name, "tasks")) {
        d->app.primitive = PRIM_LIST;
        d->app.accent    = rgb(0x6E, 0xC4, 0x6E);
        eapp_set_str(&d->app, 0, "Today");
        eapp_clear_list(&d->app, 1);
        eapp_list_append(&d->app, 1, "Ship Document app v0.10");
        eapp_list_append(&d->app, 1, "Wire chat panel command grammar");
        eapp_list_append(&d->app, 1, "Demo on HDMI: morph through 5 primitives");
        eapp_list_append(&d->app, 1, "Plan v0.11 speech input");
    } else if (ieq(name, "code")) {
        d->app.primitive = PRIM_FEED;
        d->app.accent    = rgb(0xFF, 0x6F, 0x91);
        eapp_set_str(&d->app, 0, "Review queue");
        eapp_clear_list(&d->app, 1);
        eapp_list_append(&d->app, 1, "PR #142  Document app skeleton");
        eapp_list_append(&d->app, 1, "PR #141  invariant-frame runtime");
        eapp_list_append(&d->app, 1, "PR #140  Markov agent + persistence");
    } else if (ieq(name, "brief")) {
        d->app.primitive = PRIM_CARD;
        d->app.accent    = rgb(0xFF, 0xCB, 0x4A);
        eapp_set_str(&d->app, 0, "Tuesday, May 6 2026");
        eapp_set_str(&d->app, 2,
            "3 events  -  6 tasks  -  5 PRs to review.\nFocus block 9-11am.");
    } else if (ieq(name, "chat")) {
        d->app.primitive = PRIM_CONVO;
        d->app.accent    = rgb(0x4F, 0xC3, 0xFF);
        eapp_set_str(&d->app, 0, "ATOMiK Agent");
        eapp_clear_list(&d->app, 1);
        eapp_list_append(&d->app, 1, "agent: ready. what should I show?");
        eapp_list_append(&d->app, 1, "you: morph the document into something new");
    } else {
        hist_push(d, "agent> ", "unknown preset.");
        return;
    }
    char buf[64];
    snprintf(buf, sizeof buf, "loaded preset: %s", name);
    hist_push(d, "agent> ", buf);
}

static void apply_command(doc_state_t *d, const char *raw_line) {
    char line[DOC_INPUT_MAX];
    snprintf(line, sizeof line, "%s", raw_line);
    char *p = line;
    char *cmd = pop_token(&p);
    if (!cmd) return;
    hist_push(d, "you> ", raw_line);

    if (ieq(cmd, "help")) {
        hist_push(d, "agent> ", "set primitive <list|card|grid|feed|convo>");
        hist_push(d, "agent> ", "set accent <cyan|amber|pink|green|white|#hex>");
        hist_push(d, "agent> ", "set header \"...\"  /  set body \"...\"");
        hist_push(d, "agent> ", "clear list  /  add \"item\"");
        hist_push(d, "agent> ", "load <calendar|tasks|code|brief|chat>");
        hist_push(d, "agent> ", "/ai <prompt>  /  /export <path>  /  /import <path>");
        hist_push(d, "agent> ", "save  /  reset");
        return;
    }
    if (ieq(cmd, "save")) { doc_save(d); hist_push(d, "agent> ", "state persisted."); return; }
    if (ieq(cmd, "reset")) {
        doc_init_default(d);
        hist_push(d, "agent> ", "reset to default state.");
        return;
    }
    if (ieq(cmd, "/export") || ieq(cmd, "export")) {
        char *path = trim(p);
        if (!path || !path[0]) path = "/tmp/doc_export.deltas";
        int rc = delta_snapshot_to_file(path, &d->app);
        char buf[160];
        snprintf(buf, sizeof buf, "%s exported to %s",
                 rc == 0 ? "OK" : "FAILED", path);
        hist_push(d, "agent> ", buf);
        return;
    }
    if (ieq(cmd, "/import") || ieq(cmd, "import")) {
        char *path = trim(p);
        if (!path || !path[0]) {
            hist_push(d, "agent> ", "import what? give a path.");
            return;
        }
        eapp_init(&d->app, "Document", "imported from manifest",
                  PRIM_LIST, ATOMIK_ACCENT);
        eapp_add_field(&d->app, FT_STR);
        eapp_add_field(&d->app, FT_LIST);
        eapp_add_field(&d->app, FT_STR);
        int n = delta_replay_file(path, &d->app);
        char buf[160];
        if (n > 0) snprintf(buf, sizeof buf, "imported %d ops from %s", n, path);
        else       snprintf(buf, sizeof buf, "no ops applied from %s", path);
        hist_push(d, "agent> ", buf);
        doc_save(d);
        return;
    }
    if (cmd[0] == '/' && ieq(cmd, "/ai")) {
        const char *prompt = trim(p);
        const llm_provider_t *prov = llm_default_provider();
        int est_in   = llm_estimate_tokens(prompt);
        int est_out  = est_in;
        int est_cost = llm_estimate_cost_uusd(prov, est_in, est_out);
        char preview[120];
        snprintf(preview, sizeof preview,
                 "estimate: %d in + ~%d out tokens, ~%d.%03d uUSD (%s)",
                 est_in, est_out, est_cost / 1000, est_cost % 1000,
                 prov->name);
        hist_push(d, "agent> ", preview);
        llm_response_t r;
        llm_query(prov, prompt, &r);
        char actual[120];
        snprintf(actual, sizeof actual,
                 "spent: %d in + %d out, %d.%03d uUSD%s",
                 r.tokens_in, r.tokens_out,
                 r.cost_uusd / 1000, r.cost_uusd % 1000,
                 r.is_stub ? " (stub)" : "");
        hist_push(d, "agent> ", actual);
        char *resp = r.text;
        while (resp && *resp) {
            char *eol = strchr(resp, '\n');
            if (eol) *eol = 0;
            if (resp[0]) apply_command(d, resp);
            if (!eol) break;
            resp = eol + 1;
        }
        return;
    }
    if (ieq(cmd, "set")) {
        char *what = pop_token(&p);
        if (!what) { hist_push(d, "agent> ", "set what?"); return; }
        char *val  = trim(p);
        if (ieq(what, "primitive")) {
            d->app.primitive = prim_by_name(val);
            hist_push(d, "agent> ", "primitive updated.");
        } else if (ieq(what, "accent")) {
            d->app.accent = accent_by_name(val);
            hist_push(d, "agent> ", "accent updated.");
        } else if (ieq(what, "header")) {
            eapp_set_str(&d->app, 0, val);
            hist_push(d, "agent> ", "header set.");
        } else if (ieq(what, "subtitle") || ieq(what, "footer") ||
                   ieq(what, "body")) {
            eapp_set_str(&d->app, 2, val);
            hist_push(d, "agent> ", "field set.");
        } else {
            hist_push(d, "agent> ", "I don't know that field.");
        }
        return;
    }
    if (ieq(cmd, "clear")) {
        char *what = pop_token(&p);
        if (what && ieq(what, "list")) {
            eapp_clear_list(&d->app, 1);
            hist_push(d, "agent> ", "list cleared.");
        } else {
            hist_push(d, "agent> ", "clear what? try: clear list");
        }
        return;
    }
    if (ieq(cmd, "add")) {
        char *val = trim(p);
        if (val && val[0]) {
            eapp_list_append(&d->app, 1, val);
            hist_push(d, "agent> ", "added.");
        }
        return;
    }
    if (ieq(cmd, "load")) {
        char *what = pop_token(&p);
        if (what) load_preset(d, what);
        else hist_push(d, "agent> ", "load what? calendar/tasks/code/brief/chat");
        return;
    }
    hist_push(d, "agent> ", "I didn't catch that. try 'help'.");
}

static void apply_and_persist(doc_state_t *d, const char *line) {
    apply_command(d, line);
    doc_save(d);
}

/* Public API. Each call to document_open() returns a heap-allocated
 * doc_state_t* — caller stores it in window_t.user. */
doc_state_t *document_open_new(void) {
    doc_state_t *d = (doc_state_t *)calloc(1, sizeof *d);
    if (!d) return NULL;
    d->doc_id = s_next_doc_id++;
    if (!doc_load(d)) doc_init_default(d);
    doc_save(d);
    return d;
}

void document_close(doc_state_t *d) {
    if (!d) return;
    doc_save(d);
    free(d);
}

void document_handle_key_for(doc_state_t *d, int key) {
    if (!d) return;
    if (key == '\n' || key == '\r') {
        if (d->input_len > 0) {
            d->input[d->input_len] = 0;
            apply_and_persist(d, d->input);
            d->input_len = 0;
            d->input[0]  = 0;
        }
        return;
    }
    if (key == 0x7F || key == '\b') {
        if (d->input_len > 0) {
            d->input_len--;
            d->input[d->input_len] = 0;
        }
        return;
    }
    if (key >= 32 && key < 127 && d->input_len < DOC_INPUT_MAX - 1) {
        d->input[d->input_len++] = (char)key;
        d->input[d->input_len]   = 0;
    }
}

void document_draw_for(window_t *w, int x, int y, int wd, int ht) {
    doc_state_t *d = (doc_state_t *)(w ? w->user : NULL);
    if (!d) {
        draw_rect(x, y, wd, ht, rgb(0x10, 0x16, 0x22));
        draw_text(x + 16, y + 16, "(empty document)", 1, ATOMIK_FG_DIM);
        return;
    }
    int chat_w = 360;
    int doc_w  = wd - chat_w - 8;
    int chat_x = x + doc_w + 8;

    /* Left pane: render the document via the invariant frame */
    eapp_draw(w, &d->app, x, y, doc_w, ht);

    /* Right pane: chat panel */
    draw_rect(chat_x, y, chat_w, ht, rgb(0x0E, 0x12, 0x1C));
    draw_rect(chat_x - 1, y, 1, ht, rgb(0x22, 0x2A, 0x3A));

    char hdr[64];
    snprintf(hdr, sizeof hdr, "Agent #%d", d->doc_id);
    draw_text(chat_x + 14, y + 12, hdr, 2, ATOMIK_ACCENT);
    draw_text(chat_x + 14, y + 14 + text_height(2),
              "type a command -> the doc morphs",
              1, ATOMIK_FG_DIM);

    int hist_top = y + 14 + text_height(2) + text_height(1) + 16;
    int line_h   = text_height(1) + 6;
    int visible  = (ht - 80 - (hist_top - y)) / line_h;
    if (visible < 1) visible = 1;
    int start    = d->n_history > visible ? d->n_history - visible : 0;
    for (int i = 0; i < d->n_history - start; i++) {
        int yy = hist_top + i * line_h;
        const char *line = d->history[start + i];
        pixel_t color = (line[0] == 'a') ? ATOMIK_FG : ATOMIK_ACCENT;
        draw_text(chat_x + 14, yy, line, 1, color);
    }

    int input_y = y + ht - 60;
    draw_rect_rounded(chat_x + 8, input_y, chat_w - 16, 36, 8,
                      rgb(0x1A, 0x22, 0x32));
    draw_text(chat_x + 18, input_y + 10, "> ", 1, ATOMIK_ACCENT);
    draw_text(chat_x + 36, input_y + 10, d->input, 1, ATOMIK_FG);
    int cur_x = chat_x + 36 + text_width(d->input, 1);
    draw_rect(cur_x, input_y + 10, 8, text_height(1), ATOMIK_ACCENT);

    draw_text(chat_x + 14, y + ht - 18,
              "Enter run  -  Backspace edit", 1, ATOMIK_FG_DIM);
}

/* Backward-compatible legacy single-doc shims so older callers still
 * work — these proxy through a process-wide singleton. New code should
 * use document_open_new()/document_handle_key_for()/document_draw_for(). */
static doc_state_t *s_legacy_doc = NULL;

void document_open(void) {
    if (!s_legacy_doc) s_legacy_doc = document_open_new();
}

void document_handle_key(int key) {
    if (!s_legacy_doc) document_open();
    document_handle_key_for(s_legacy_doc, key);
}

void document_draw(window_t *w, int x, int y, int wd, int ht) {
    if (!s_legacy_doc) document_open();
    /* If the WM passed a window without a user pointer, fall through to
     * the singleton to preserve old behavior. */
    if (w && !w->user) w->user = s_legacy_doc;
    document_draw_for(w, x, y, wd, ht);
}
