/* metrics.c — v0.38 truth-aware metric provider.
 *
 * Per ChatGPT 2026-05-09/05-10 directives.  Single global registry of
 * named metrics.  Each metric carries a getter callback and its own
 * source label (LIVE / DERIVED / SCENARIO / MOCK / STALE / WAITING).
 * The mode (DEV / DEMO / INVESTOR) gates which sources display.
 *
 * Built-in producers register themselves at metric_init() time so the
 * common ATOMiK metrics — STATE/SYNC/AGENT perf, event-bus counters,
 * personality state — are available by ID without each surface
 * touching their underlying APIs directly.  This is the data
 * contract that every concept-UI surface (Resource Fabric, State
 * Watch, Replica Flow, Memory Weave, Build Lane, Hero) reads through.
 *
 * Adding a new metric = one line of metric_register at init.  Adding
 * a new source class (e.g. METRIC_PROJECTED) = adjust the enum + the
 * mode visibility rules.  No surfaces need to change.
 */
#include "atomik_os.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define STALE_AGE_MS  4000     /* a LIVE getter that hasn't been called in 4s
                                  is downgraded to STALE on next get() */

static atomik_metric_t s_metrics[METRIC_REGISTRY_MAX];
static int             s_count = 0;
static metric_mode_t   s_mode  = METRIC_MODE_DEV;

static atomik_metric_t *find(const char *id) {
    if (!id) return NULL;
    for (int i = 0; i < s_count; i++) {
        if (strncmp(s_metrics[i].id, id, METRIC_ID_MAX) == 0) {
            return &s_metrics[i];
        }
    }
    return NULL;
}

void metric_register(const char *id, const char *label, const char *unit,
                     metric_getter_fn getter, void *ctx) {
    if (s_count >= METRIC_REGISTRY_MAX) return;
    /* Reject duplicate IDs — keeps the registry honest. */
    if (find(id)) return;
    atomik_metric_t *m = &s_metrics[s_count++];
    memset(m, 0, sizeof *m);
    strncpy(m->id, id, METRIC_ID_MAX - 1);
    strncpy(m->label, label ? label : id, METRIC_LABEL_MAX - 1);
    m->unit       = unit;     /* string pointer is owned by caller (literal) */
    m->source     = METRIC_WAITING;    /* until first refresh */
    m->getter     = getter;
    m->getter_ctx = ctx;
    m->value      = 0.0;
    m->updated_ms = 0;
}

const atomik_metric_t *metric_get(const char *id) {
    atomik_metric_t *m = find(id);
    if (!m) return NULL;
    if (m->getter) {
        metric_value_t v = m->getter(m->getter_ctx);
        m->value      = v.value;
        m->source     = v.source;
        m->updated_ms = anim_now_ms();
    }
    return m;
}

const atomik_metric_t *metric_peek(const char *id) {
    return find(id);
}

int metric_count(void) { return s_count; }

const atomik_metric_t *metric_at(int idx) {
    if (idx < 0 || idx >= s_count) return NULL;
    return &s_metrics[idx];
}

const char *metric_source_label(metric_source_t s) {
    switch (s) {
    case METRIC_LIVE:     return "LIVE";
    case METRIC_DERIVED:  return "DERIVED";
    case METRIC_SCENARIO: return "SCENARIO";
    case METRIC_MOCK:     return "MOCK";
    case METRIC_STALE:    return "STALE";
    case METRIC_WAITING:  return "WAITING";
    default:              return "?";
    }
}

pixel_t metric_source_color(metric_source_t s) {
    switch (s) {
    case METRIC_LIVE:     return ATOMIK_SEM_SAVINGS;     /* green */
    case METRIC_DERIVED:  return ATOMIK_SEM_HARDWARE;    /* cyan  */
    case METRIC_SCENARIO: return ATOMIK_SEM_AGENT;       /* violet */
    case METRIC_MOCK:     return ATOMIK_SEM_WASTE;       /* amber — visible warning */
    case METRIC_STALE:    return ATOMIK_SEM_WASTE;
    case METRIC_WAITING:
    default:              return ATOMIK_FG_DIM;
    }
}

void          metric_set_mode(metric_mode_t mode) { s_mode = mode; }
metric_mode_t metric_mode(void)                   { return s_mode; }

int metric_visible(const atomik_metric_t *m) {
    if (!m) return 0;
    /* WAITING is always allowed (it's the honest "no data" state).
     * Everything else depends on mode. */
    switch (s_mode) {
    case METRIC_MODE_DEV:
        return 1;     /* dev sees everything */
    case METRIC_MODE_DEMO:
        return m->source != METRIC_MOCK;
    case METRIC_MODE_INVESTOR:
        return m->source == METRIC_LIVE
            || m->source == METRIC_DERIVED
            || m->source == METRIC_SCENARIO    /* must be labeled */
            || m->source == METRIC_WAITING;
    }
    return 1;
}

/* "Worst" = most dishonest currently registered.  WAITING/STALE are
 * not dishonest (they're honest "no data" / "old data" states), so
 * we report MOCK > SCENARIO > STALE > DERIVED > LIVE.  STALE and
 * WAITING beat DERIVED only because they tell the audience to look
 * at the badge if a panel goes quiet. */
metric_source_t metric_worst_source(void) {
    metric_source_t worst = METRIC_LIVE;
    /* Refresh all to capture current state. */
    for (int i = 0; i < s_count; i++) {
        if (s_metrics[i].getter) {
            metric_value_t v = s_metrics[i].getter(s_metrics[i].getter_ctx);
            s_metrics[i].value      = v.value;
            s_metrics[i].source     = v.source;
            s_metrics[i].updated_ms = anim_now_ms();
        }
        metric_source_t s = s_metrics[i].source;
        /* Order: LIVE < DERIVED < WAITING < STALE < SCENARIO < MOCK */
        int rank, worst_rank;
        switch (s) {
        case METRIC_LIVE:     rank = 0; break;
        case METRIC_DERIVED:  rank = 1; break;
        case METRIC_WAITING:  rank = 2; break;
        case METRIC_STALE:    rank = 3; break;
        case METRIC_SCENARIO: rank = 4; break;
        case METRIC_MOCK:     rank = 5; break;
        default:              rank = 0; break;
        }
        switch (worst) {
        case METRIC_LIVE:     worst_rank = 0; break;
        case METRIC_DERIVED:  worst_rank = 1; break;
        case METRIC_WAITING:  worst_rank = 2; break;
        case METRIC_STALE:    worst_rank = 3; break;
        case METRIC_SCENARIO: worst_rank = 4; break;
        case METRIC_MOCK:     worst_rank = 5; break;
        default:              worst_rank = 0; break;
        }
        if (rank > worst_rank) worst = s;
    }
    return worst;
}

/* v0.38-J++ aggregate label — what the top-bar DATA badge should say
 * when several lanes are alive and others are waiting.  Worst-source
 * still drives the color (so MOCK / SCENARIO still trigger amber),
 * but the LABEL becomes "MIXED" so the bar stops contradicting the
 * Resource Fabric.  ChatGPT 2026-05-15: "DATA: WAITING while Fabric
 * shows real values" was a perception contradiction. */
const char *metric_aggregate_label(void) {
    int n_live = 0, n_derived = 0, n_waiting = 0, n_stale = 0;
    int n_mock = 0, n_scenario = 0;
    for (int i = 0; i < s_count; i++) {
        if (s_metrics[i].getter) {
            metric_value_t v = s_metrics[i].getter(s_metrics[i].getter_ctx);
            s_metrics[i].value      = v.value;
            s_metrics[i].source     = v.source;
            s_metrics[i].updated_ms = anim_now_ms();
        }
        switch (s_metrics[i].source) {
        case METRIC_LIVE:     n_live++; break;
        case METRIC_DERIVED:  n_derived++; break;
        case METRIC_WAITING:  n_waiting++; break;
        case METRIC_STALE:    n_stale++; break;
        case METRIC_SCENARIO: n_scenario++; break;
        case METRIC_MOCK:     n_mock++; break;
        }
    }
    /* Trust-breaking sources still win: never hide them behind MIXED. */
    if (n_mock     > 0) return "MOCK";
    if (n_scenario > 0) return "SCENARIO";
    /* Pure live or derived. */
    if (n_waiting + n_stale == 0) {
        if (n_live > 0)    return "LIVE";
        if (n_derived > 0) return "DERIVED";
        return "WAITING";
    }
    /* All waiting / stale. */
    if (n_live + n_derived == 0) {
        if (n_stale > 0) return "STALE";
        return "WAITING";
    }
    /* Some live, some quiet — honest aggregate. */
    return "MIXED";
}

/* === built-in producers ===
 *
 * Each getter wraps an existing API and returns a metric_value_t with
 * the appropriate source label.  When the underlying producer has no
 * data yet, the getter downgrades to METRIC_WAITING so the consuming
 * surface can render "no data" honestly. */

static metric_value_t getter_personality_active(void *ctx) {
    (void)ctx;
    metric_value_t v = { (double)fabric_active(), METRIC_LIVE };
    return v;
}

/* Helper: wraps perf_last_for(p) lookup with WAITING fallback. */
typedef enum {
    PF_OPS_LOGICAL,
    PF_OPS_ISSUED,
    PF_REGIONS_TOUCHED,
    PF_REGIONS_UNIQUE,
    PF_BYTES_PROCESSED,
    PF_BYTES_AVOIDED,
    PF_CYCLES_TOTAL,
    PF_CYCLES_ATOMIK,
    PF_CYCLES_BASELINE,
} perf_field_t;

static metric_value_t perf_field(personality_t p, perf_field_t field) {
    const perf_sample_t *s = perf_last_for(p);
    metric_value_t v = { 0.0, METRIC_WAITING };
    if (!s) return v;
    v.source = METRIC_LIVE;
    switch (field) {
    case PF_OPS_LOGICAL:     v.value = (double)s->ops_logical; break;
    case PF_OPS_ISSUED:      v.value = (double)s->ops_issued; break;
    case PF_REGIONS_TOUCHED: v.value = (double)s->regions_touched; break;
    case PF_REGIONS_UNIQUE:  v.value = (double)s->regions_unique; break;
    case PF_BYTES_PROCESSED: v.value = (double)s->bytes_processed; break;
    case PF_BYTES_AVOIDED:   v.value = (double)s->bytes_avoided; break;
    case PF_CYCLES_TOTAL:    v.value = (double)s->cycles_total; break;
    case PF_CYCLES_ATOMIK:   v.value = (double)s->cycles_atomik; break;
    case PF_CYCLES_BASELINE: v.value = (double)s->cycles_software_baseline; break;
    }
    return v;
}

#define MK_PERF_GETTER(name, perso, field)                                  \
    static metric_value_t getter_##name(void *ctx) {                        \
        (void)ctx; return perf_field(perso, field);                         \
    }

MK_PERF_GETTER(state_ops_logical,    PERSONALITY_STATE, PF_OPS_LOGICAL)
MK_PERF_GETTER(state_ops_issued,     PERSONALITY_STATE, PF_OPS_ISSUED)
MK_PERF_GETTER(state_regions_unique, PERSONALITY_STATE, PF_REGIONS_UNIQUE)
MK_PERF_GETTER(state_cycles_total,   PERSONALITY_STATE, PF_CYCLES_TOTAL)

MK_PERF_GETTER(sync_ops_issued,      PERSONALITY_SYNC,  PF_OPS_ISSUED)
MK_PERF_GETTER(sync_regions_unique,  PERSONALITY_SYNC,  PF_REGIONS_UNIQUE)
MK_PERF_GETTER(sync_regions_touched, PERSONALITY_SYNC,  PF_REGIONS_TOUCHED)
MK_PERF_GETTER(sync_bytes_processed, PERSONALITY_SYNC,  PF_BYTES_PROCESSED)
MK_PERF_GETTER(sync_bytes_avoided,   PERSONALITY_SYNC,  PF_BYTES_AVOIDED)

MK_PERF_GETTER(agent_ops_issued,     PERSONALITY_AGENT, PF_OPS_ISSUED)
MK_PERF_GETTER(agent_bytes_avoided,  PERSONALITY_AGENT, PF_BYTES_AVOIDED)

/* DERIVED: STATE batch speedup.  Both cycle counts must be present. */
static metric_value_t getter_state_speedup(void *ctx) {
    (void)ctx;
    const perf_sample_t *s = perf_last_for(PERSONALITY_STATE);
    metric_value_t v = { 0.0, METRIC_WAITING };
    if (!s) return v;
    if (s->cycles_software_baseline == 0 || s->cycles_atomik == 0) return v;
    v.value  = perf_speedup(s);
    v.source = METRIC_DERIVED;
    return v;
}

/* DERIVED: STATE coalesce ratio (ops_issued / ops_logical * 100 = % kept). */
static metric_value_t getter_state_coalesce_pct(void *ctx) {
    (void)ctx;
    const perf_sample_t *s = perf_last_for(PERSONALITY_STATE);
    metric_value_t v = { 0.0, METRIC_WAITING };
    if (!s || s->ops_logical == 0) return v;
    v.value  = 100.0 * (1.0 - (double)s->ops_issued / (double)s->ops_logical);
    v.source = METRIC_DERIVED;
    return v;
}

/* === event-bus counters === */
#define MK_EVENT_GETTER(name, kind)                                        \
    static metric_value_t getter_##name(void *ctx) {                       \
        (void)ctx;                                                         \
        unsigned long c = atomik_event_count(kind);                        \
        metric_value_t v = { (double)c,                                    \
                             c > 0 ? METRIC_LIVE : METRIC_WAITING };       \
        return v;                                                          \
    }

MK_EVENT_GETTER(event_state_count,      EVT_STATE_DELTA)
MK_EVENT_GETTER(event_sync_count,       EVT_SYNC_REPLICA)
MK_EVENT_GETTER(event_agent_count,      EVT_AGENT_CONTEXT)
MK_EVENT_GETTER(event_visual_count,     EVT_VIS_RENDER)
MK_EVENT_GETTER(event_build_count,      EVT_BUILD_RUN)
MK_EVENT_GETTER(event_override_count,   EVT_OVERRIDE)

static metric_value_t getter_event_total(void *ctx) {
    (void)ctx;
    unsigned long c = atomik_event_total();
    metric_value_t v = { (double)c,
                         c > 0 ? METRIC_LIVE : METRIC_WAITING };
    return v;
}

/* === TEXT: glyph cache hit-rate (v0.38-B) ===
 *
 * Pre-rendered glyph cache exposes per-frame counters: how many
 * glyphs we drew this session, how many came from the fast path
 * (mask blit, no per-pixel bounds), and the derived hit rate.  Lets
 * the audience see a real text-rendering perf claim, not a vibe. */
extern unsigned long font_glyphs_drawn(void);
extern unsigned long font_glyphs_cached(void);
extern unsigned long font_glyphs_clipped(void);

static metric_value_t getter_text_glyphs_drawn(void *ctx) {
    (void)ctx;
    unsigned long n = font_glyphs_drawn();
    metric_value_t v = { (double)n,
                         n > 0 ? METRIC_LIVE : METRIC_WAITING };
    return v;
}
static metric_value_t getter_text_glyphs_cached(void *ctx) {
    (void)ctx;
    unsigned long n = font_glyphs_cached();
    metric_value_t v = { (double)n,
                         font_glyphs_drawn() > 0 ? METRIC_LIVE : METRIC_WAITING };
    return v;
}
static metric_value_t getter_text_cache_hit_pct(void *ctx) {
    (void)ctx;
    unsigned long total  = font_glyphs_drawn();
    unsigned long cached = font_glyphs_cached();
    metric_value_t v = { 0.0, METRIC_WAITING };
    if (total > 0) {
        v.value = 100.0 * (double)cached / (double)total;
        v.source = METRIC_DERIVED;
    }
    return v;
}

/* === VISUAL: tile-based dirty-region tracker (v0.38-A) ===
 *
 * Uses internal accessors from dirty.c — kept private to this file
 * so dirty.c doesn't have to know about the metric_value_t shape. */
extern double dirty_metric_tiles_dirty(void);
extern double dirty_metric_tiles_avoided(void);
extern double dirty_metric_pct_avoided(void);
extern double dirty_metric_frame_count(void);

#define MK_DIRTY_GETTER(name, fn)                                          \
    static metric_value_t getter_##name(void *ctx) {                       \
        (void)ctx;                                                         \
        metric_value_t v = { fn(),                                         \
                             dirty_metric_frame_count() > 0                \
                                ? METRIC_LIVE : METRIC_WAITING };          \
        return v;                                                          \
    }

MK_DIRTY_GETTER(visual_tiles_dirty,    dirty_metric_tiles_dirty)
MK_DIRTY_GETTER(visual_tiles_avoided,  dirty_metric_tiles_avoided)
MK_DIRTY_GETTER(visual_pct_avoided,    dirty_metric_pct_avoided)
MK_DIRTY_GETTER(visual_frames,         dirty_metric_frame_count)

static metric_value_t getter_visual_tiles_total(void *ctx) {
    (void)ctx;
    metric_value_t v = { (double)dirty_total(), METRIC_LIVE };
    return v;
}

/* === init: register every built-in metric === */

void metric_init(void) {
    s_count = 0;
    /* personality */
    metric_register("personality.active", "active personality", NULL,
                    getter_personality_active, NULL);
    /* STATE */
    metric_register("state.ops_logical",    "STATE ops logical",   "ops",
                    getter_state_ops_logical, NULL);
    metric_register("state.ops_issued",     "STATE ops issued",    "ops",
                    getter_state_ops_issued, NULL);
    metric_register("state.regions_unique", "STATE regions",       "rgn",
                    getter_state_regions_unique, NULL);
    metric_register("state.cycles_total",   "STATE cycles total",  "cyc",
                    getter_state_cycles_total, NULL);
    metric_register("state.speedup",        "STATE speedup vs sw", "x",
                    getter_state_speedup, NULL);
    metric_register("state.coalesce_pct",   "STATE coalesce",      "%",
                    getter_state_coalesce_pct, NULL);
    /* SYNC */
    metric_register("sync.ops_issued",      "SYNC ops emitted",    "ops",
                    getter_sync_ops_issued, NULL);
    metric_register("sync.regions_unique",  "SYNC regions",        "rgn",
                    getter_sync_regions_unique, NULL);
    metric_register("sync.regions_touched", "SYNC regions touched","rgn",
                    getter_sync_regions_touched, NULL);
    metric_register("sync.bytes_processed", "SYNC bytes",          "B",
                    getter_sync_bytes_processed, NULL);
    metric_register("sync.bytes_avoided",   "SYNC bytes avoided",  "B",
                    getter_sync_bytes_avoided, NULL);
    /* AGENT */
    metric_register("agent.ops_issued",     "AGENT retained",      "rgn",
                    getter_agent_ops_issued, NULL);
    metric_register("agent.bytes_avoided",  "AGENT bytes avoided", "B",
                    getter_agent_bytes_avoided, NULL);
    /* EVENT bus */
    metric_register("event.total",          "events on bus",       "ev",
                    getter_event_total, NULL);
    metric_register("event.state_count",    "STATE events",        "ev",
                    getter_event_state_count, NULL);
    metric_register("event.sync_count",     "SYNC events",         "ev",
                    getter_event_sync_count, NULL);
    metric_register("event.agent_count",    "AGENT events",        "ev",
                    getter_event_agent_count, NULL);
    metric_register("event.visual_count",   "VIS_RENDER events",   "ev",
                    getter_event_visual_count, NULL);
    metric_register("event.build_count",    "BUILD events",        "ev",
                    getter_event_build_count, NULL);
    metric_register("event.override_count", "OVERRIDE events",     "ev",
                    getter_event_override_count, NULL);
    /* VISUAL — tile-based dirty-region tracker (v0.38-A).
     * These were previously MOCK proxies via EVT_VIS_RENDER count;
     * now LIVE measurements of OS redraw waste. */
    metric_register("visual.tiles_dirty",     "tiles dirty / frame",   "tile",
                    getter_visual_tiles_dirty, NULL);
    metric_register("visual.tiles_avoided",   "tiles avoided / frame", "tile",
                    getter_visual_tiles_avoided, NULL);
    metric_register("visual.tiles_total",     "tiles in framebuffer",  "tile",
                    getter_visual_tiles_total, NULL);
    metric_register("visual.frame_pct_avoided","framebuffer % avoided", "%",
                    getter_visual_pct_avoided, NULL);
    metric_register("visual.frames",          "frames painted",        "fr",
                    getter_visual_frames, NULL);
    /* TEXT — glyph cache (v0.38-B). */
    metric_register("text.glyphs_drawn",  "glyphs drawn",        "g",
                    getter_text_glyphs_drawn, NULL);
    metric_register("text.glyphs_cached", "glyphs from cache",   "g",
                    getter_text_glyphs_cached, NULL);
    metric_register("text.cache_hit_pct", "glyph cache hit",     "%",
                    getter_text_cache_hit_pct, NULL);
}
