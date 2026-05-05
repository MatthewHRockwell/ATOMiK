/* llm.c — pluggable AI-provider abstraction + stub backend + audit log.
 *
 * v0.12: ships an offline "stub" provider that returns canned responses
 * based on simple keyword matching. The architecture is the real piece:
 * every call goes through llm_query(), the cost is estimated upfront,
 * the actual spend is appended to the audit log. Wiring a real
 * Anthropic/OpenAI/local backend in v1.0 = swap one function body. */
#include "atomik_os.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Provider registry. Cost numbers are illustrative and current as of
 * 2026-05; they're shown to the user in the cost preview, not used as
 * billing of record. */
static const llm_provider_t PROVIDERS[] = {
    /* name,                base_url,                       model,                in $/Mtok, out $/Mtok, stub */
    {  "stub",              "(offline)",                    "atomik-stub-v1",     0,           0,           1 },
    {  "local-intent",      "(on-device)",                  "atomik-trigram-v1",  0,           0,           1 },
    {  "claude-haiku-4.5",  "https://api.anthropic.com/v1", "claude-haiku-4.5",   1,           5,           1 },
    {  "claude-sonnet-4.6", "https://api.anthropic.com/v1", "claude-sonnet-4.6",  3,          15,           1 },
    {  "gpt-4o-mini",       "https://api.openai.com/v1",    "gpt-4o-mini",       0,           1,           1 },
    /* "is_stub" = 1 on all of them in v0.12 because we have no internet
     * on the board. v1.0 flips the relevant ones to 0 and fires real
     * HTTP. The cost map stays the same.
     *
     * v0.18: 'local-intent' is the on-device Jaccard trigram classifier.
     * is_stub stays 1 because no network call happens, but the user-
     * facing label distinguishes it from the canned-keyword stub. */
};
#define N_PROVIDERS ((int)(sizeof(PROVIDERS)/sizeof(PROVIDERS[0])))

const llm_provider_t *llm_default_provider(void) { return &PROVIDERS[0]; }
int  llm_provider_count(void)                    { return N_PROVIDERS; }
const llm_provider_t *llm_provider_at(int i)     {
    if (i < 0 || i >= N_PROVIDERS) return NULL;
    return &PROVIDERS[i];
}
const llm_provider_t *llm_provider_by_name(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < N_PROVIDERS; i++) {
        if (strcmp(PROVIDERS[i].name, name) == 0) return &PROVIDERS[i];
    }
    return NULL;
}

int llm_estimate_tokens(const char *prompt) {
    if (!prompt) return 0;
    /* Tiktoken-ish heuristic: ~4 chars per token. Close enough for
     * preview UX; real backends return the exact count after the call. */
    size_t n = strlen(prompt);
    return (int)((n + 3) / 4);
}

int llm_estimate_cost_uusd(const llm_provider_t *p, int tin, int tout) {
    if (!p) return 0;
    /* Per-million-token rates -> per-token micro-USD via /1000. */
    return (tin  * p->cost_in_uusd_per_token  +
            tout * p->cost_out_uusd_per_token) / 1000;
}

/* ---------- stub-backend response synthesis ---------- */

static int contains_word(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    for (const char *p = hay; *p; p++) {
        size_t i = 0;
        while (needle[i] &&
               tolower((unsigned char)p[i]) ==
               tolower((unsigned char)needle[i])) i++;
        if (needle[i] == 0) return 1;
    }
    return 0;
}

static void stub_respond(const char *prompt, char *out, size_t cap) {
    /* Intent matching: simple keyword OR. The response is a script of
     * field-delta commands that the Document parser executes. Each
     * branch shows what an LLM would route given a similar prompt. */

    /* --- shape-shifting via primitive --- */
    if (contains_word(prompt, "calendar") || contains_word(prompt, "schedule") ||
        contains_word(prompt, "month") || contains_word(prompt, "week")) {
        snprintf(out, cap,
            "load calendar\n"
            "set primitive grid\n"
            "set accent cyan");
        return;
    }
    if (contains_word(prompt, "task") || contains_word(prompt, "todo") ||
        contains_word(prompt, "remind") || contains_word(prompt, "checklist")) {
        snprintf(out, cap,
            "load tasks\n"
            "set primitive list\n"
            "set accent green\n"
            "set header \"Today\"");
        return;
    }
    if (contains_word(prompt, "code") || contains_word(prompt, "pr") ||
        contains_word(prompt, "review") || contains_word(prompt, "merge") ||
        contains_word(prompt, "commit")) {
        snprintf(out, cap,
            "load code\n"
            "set primitive feed\n"
            "set accent pink");
        return;
    }
    if (contains_word(prompt, "summarize") || contains_word(prompt, "brief") ||
        contains_word(prompt, "summary") || contains_word(prompt, "digest")) {
        snprintf(out, cap,
            "load brief\n"
            "set primitive card\n"
            "set accent amber");
        return;
    }
    if (contains_word(prompt, "chat") || contains_word(prompt, "talk") ||
        contains_word(prompt, "conversation") || contains_word(prompt, "ask")) {
        snprintf(out, cap,
            "load chat\n"
            "set primitive convo\n"
            "set accent cyan");
        return;
    }

    /* --- simple field mutations --- */
    if (contains_word(prompt, "rename") || contains_word(prompt, "title")) {
        /* Pull anything inside double-quotes as the new title. */
        const char *q1 = strchr(prompt, '"');
        const char *q2 = q1 ? strchr(q1 + 1, '"') : NULL;
        if (q1 && q2 && q2 > q1 + 1) {
            char buf[160];
            snprintf(buf, sizeof buf, "%.*s",
                     (int)(q2 - q1 - 1), q1 + 1);
            snprintf(out, cap, "set header \"%s\"", buf);
            return;
        }
    }
    if (contains_word(prompt, "clear") || contains_word(prompt, "wipe") ||
        contains_word(prompt, "reset list") || contains_word(prompt, "empty")) {
        snprintf(out, cap, "clear list");
        return;
    }
    if (contains_word(prompt, "amber")  || contains_word(prompt, "yellow") ||
        contains_word(prompt, "warm")) {
        snprintf(out, cap, "set accent amber");
        return;
    }
    if (contains_word(prompt, "pink") || contains_word(prompt, "magenta")) {
        snprintf(out, cap, "set accent pink");
        return;
    }
    if (contains_word(prompt, "green")) {
        snprintf(out, cap, "set accent green");
        return;
    }
    if (contains_word(prompt, "cyan") || contains_word(prompt, "blue")) {
        snprintf(out, cap, "set accent cyan");
        return;
    }

    /* --- structural --- */
    if (contains_word(prompt, "kanban") || contains_word(prompt, "board")) {
        snprintf(out, cap,
            "set primitive grid\n"
            "set accent cyan\n"
            "set header \"Sprint\"");
        return;
    }
    if (contains_word(prompt, "feed") || contains_word(prompt, "timeline") ||
        contains_word(prompt, "activity")) {
        snprintf(out, cap, "set primitive feed");
        return;
    }
    if (contains_word(prompt, "card") || contains_word(prompt, "detail")) {
        snprintf(out, cap, "set primitive card");
        return;
    }

    snprintf(out, cap,
        "(stub) Try: 'show me a calendar', 'switch to tasks', "
        "'kanban for sprint 14', 'rename to \"Inbox\"', 'use amber accent', "
        "'summarize my day', 'open a chat'.");
}

/* ---------- v0.18 local-intent classifier ---------- */
/* Character-trigram bag-of-features with Jaccard similarity. Same logic
 * as tools/atomik_local_intent.py — proves the v0.18 path is real, all
 * on-device, no network, no model file. ~5 KB of code + table.
 *
 * Each label has a "training" string: a concatenation of example
 * utterances separated by spaces. At classify time we compute the
 * trigram set of the prompt and each label's training, pick the
 * highest Jaccard score above a 5% threshold. */

typedef struct {
    const char *label;
    const char *response;   /* command-script emitted on match */
    const char *training;
} intent_pattern_t;

/* v0.19c: expanded training utterances per label. See docs/MINILM_EVALUATION.md
 * for why we expanded the trigram table instead of adopting MiniLM. ~1 KB of
 * extra static data buys us coverage on common paraphrases that didn't match
 * the original tight list. */
static const intent_pattern_t INTENTS[] = {
    { "load calendar",
      "load calendar\nset primitive grid\nset accent cyan",
      "show me a calendar  open calendar  whats on my schedule  "
      "view this month  calendar of may  schedule view  "
      "give me my agenda  what is coming up  upcoming events  "
      "calendar view  show appointments  weekly view  monthly view" },
    { "load tasks",
      "load tasks\nset primitive list\nset accent green",
      "show me my tasks  whats on my list  todo list  "
      "checklist  open tasks  what do i need to do  "
      "what should i do today  what is on my plate  "
      "show my todos  pull up my tasks  open my list" },
    { "load code",
      "load code\nset primitive feed\nset accent pink",
      "show open prs  review queue  code reviews  "
      "pull requests  what merges are pending  "
      "what needs review  show pull requests  show diffs  "
      "review backlog  whats in flight  open the review queue" },
    { "load brief",
      "load brief\nset primitive card\nset accent amber",
      "summarize my day  give me a brief  executive summary  "
      "daily digest  what should i know  "
      "what is the headline  whats important right now  "
      "give me a summary  one paragraph summary  status report" },
    { "load chat",
      "load chat\nset primitive convo\nset accent cyan",
      "open a chat  start a conversation  switch to chat mode  "
      "talk to the agent  "
      "let me chat  i want to chat  start chatting  "
      "open dialogue  talk mode" },
    { "set primitive grid",
      "set primitive grid",
      "make it a grid  tabular view  kanban  switch to grid  "
      "tile layout  spreadsheet view  matrix view  table view  "
      "show as a grid  arrange in tiles" },
    { "set primitive list",
      "set primitive list",
      "list view  make it a list  bullet list  rows  "
      "show as a list  flat list  vertical list  rows of items  "
      "make this look like notion" },
    { "set primitive feed",
      "set primitive feed",
      "feed view  timeline  activity stream  make it a feed  "
      "show as a feed  scrolling feed  newsfeed view  "
      "reverse chronological  twitter view" },
    { "set primitive card",
      "set primitive card",
      "card view  single card  make it one big card  "
      "show as a card  one big block  highlight view  hero view" },
    { "set primitive convo",
      "set primitive convo",
      "conversation view  chat bubbles  talk view  "
      "show as messages  threaded view  imessage style  "
      "discussion view" },
    { "set accent cyan",
      "set accent cyan",
      "use cyan  make it blue  blue accent  "
      "cool color  ocean blue  electric blue  default accent" },
    { "set accent green",
      "set accent green",
      "use green  green accent  "
      "make it green  forest accent  emerald color  fresh color" },
    { "set accent amber",
      "set accent amber",
      "use amber  yellow accent  warm color  "
      "gold accent  honey color  make it warm  sunset color" },
    { "set accent pink",
      "set accent pink",
      "use pink  magenta accent  make it pink  "
      "hot pink  fuchsia color  vibrant accent  loud color" },
    { "clear list",
      "clear list",
      "clear the list  wipe items  empty list  delete everything  "
      "remove all  start over  reset the list  blank slate" },
};
#define N_INTENTS ((int)(sizeof(INTENTS)/sizeof(INTENTS[0])))

#define MAX_TRIGRAMS 384

static int extract_trigrams(const char *s, uint32_t *out, int cap) {
    if (!s) return 0;
    char buf[512];
    int n = 0;
    buf[n++] = ' ';
    for (const char *p = s; *p && n < (int)sizeof(buf)-2; p++) {
        buf[n++] = (char)tolower((unsigned char)*p);
    }
    buf[n++] = ' ';
    int count = 0;
    for (int i = 0; i + 3 <= n && count < cap; i++) {
        unsigned char a = (unsigned char)buf[i];
        unsigned char b = (unsigned char)buf[i+1];
        unsigned char c = (unsigned char)buf[i+2];
        if (a == ' ' && b == ' ' && c == ' ') continue;
        uint32_t tg = ((uint32_t)a << 16) | ((uint32_t)b << 8) | (uint32_t)c;
        int dup = 0;
        for (int j = 0; j < count; j++) if (out[j] == tg) { dup = 1; break; }
        if (!dup) out[count++] = tg;
    }
    return count;
}

static int jaccard_permille(const uint32_t *a, int na,
                            const uint32_t *b, int nb) {
    int inter = 0;
    for (int i = 0; i < na; i++) {
        for (int j = 0; j < nb; j++) {
            if (a[i] == b[j]) { inter++; break; }
        }
    }
    int uni = na + nb - inter;
    if (uni <= 0) return 0;
    return (inter * 1000) / uni;
}

static void local_intent_respond(const char *prompt, char *out, size_t cap) {
    if (!prompt || !*prompt) {
        snprintf(out, cap, "(local-intent) need a prompt");
        return;
    }
    uint32_t pt[MAX_TRIGRAMS];
    int npt = extract_trigrams(prompt, pt, MAX_TRIGRAMS);

    int best_idx   = -1;
    int best_score = 0;
    uint32_t lt[MAX_TRIGRAMS];
    for (int i = 0; i < N_INTENTS; i++) {
        int nlt = extract_trigrams(INTENTS[i].training, lt, MAX_TRIGRAMS);
        int s   = jaccard_permille(pt, npt, lt, nlt);
        if (s > best_score) { best_score = s; best_idx = i; }
    }

    /* 50/1000 = 0.05 Jaccard threshold, matches the Python prototype. */
    if (best_idx < 0 || best_score < 50) {
        snprintf(out, cap,
            "(local-intent) no confident match -- try 'show me a calendar', "
            "'switch to tasks', 'use amber accent', 'kanban view'");
        return;
    }
    snprintf(out, cap, "%s", INTENTS[best_idx].response);
}

void llm_query(const llm_provider_t *p, const char *prompt, llm_response_t *out) {
    if (!out) return;
    memset(out, 0, sizeof *out);
    if (!p) p = llm_default_provider();
    out->is_stub  = p->is_stub;
    out->tokens_in = llm_estimate_tokens(prompt);
    if (p->is_stub) {
        if (strcmp(p->name, "local-intent") == 0) {
            local_intent_respond(prompt, out->text, sizeof out->text);
        } else {
            stub_respond(prompt, out->text, sizeof out->text);
        }
    } else {
        /* v1.0: real HTTP call here. */
        snprintf(out->text, sizeof out->text,
                 "(provider %s not yet wired in this build)", p->name);
    }
    out->tokens_out = llm_estimate_tokens(out->text);
    out->cost_uusd  = llm_estimate_cost_uusd(p, out->tokens_in, out->tokens_out);
    out->ok         = 1;
    llm_audit_append(prompt, out);
}

/* ---------- audit log ---------- */

#define LLM_AUDIT_PATH "/tmp/atomik_os_llm_audit.log"

void llm_audit_append(const char *prompt, const llm_response_t *r) {
    FILE *f = fopen(LLM_AUDIT_PATH, "a");
    if (!f) return;
    fprintf(f, "tin=%d tout=%d cost_uusd=%d stub=%d :: %.120s\n",
            r->tokens_in, r->tokens_out, r->cost_uusd, r->is_stub,
            prompt ? prompt : "");
    fclose(f);
}

int llm_audit_total_uusd(void) {
    FILE *f = fopen(LLM_AUDIT_PATH, "r");
    if (!f) return 0;
    int total = 0;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        const char *k = strstr(line, "cost_uusd=");
        if (!k) continue;
        total += atoi(k + 10);
    }
    fclose(f);
    return total;
}
