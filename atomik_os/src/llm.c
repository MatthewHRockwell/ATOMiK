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
    {  "claude-haiku-4.5",  "https://api.anthropic.com/v1", "claude-haiku-4.5",   1,           5,           1 },
    {  "claude-sonnet-4.6", "https://api.anthropic.com/v1", "claude-sonnet-4.6",  3,          15,           1 },
    {  "gpt-4o-mini",       "https://api.openai.com/v1",    "gpt-4o-mini",       0,           1,           1 },
    /* "is_stub" = 1 on all of them in v0.12 because we have no internet
     * on the board. v1.0 flips the relevant ones to 0 and fires real
     * HTTP. The cost map stays the same. */
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

void llm_query(const llm_provider_t *p, const char *prompt, llm_response_t *out) {
    if (!out) return;
    memset(out, 0, sizeof *out);
    if (!p) p = llm_default_provider();
    out->is_stub  = p->is_stub;
    out->tokens_in = llm_estimate_tokens(prompt);
    if (p->is_stub) {
        stub_respond(prompt, out->text, sizeof out->text);
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
