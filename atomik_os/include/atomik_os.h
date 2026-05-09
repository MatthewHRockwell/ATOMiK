/* atomik_os.h — public types and constants shared across modules. */
#ifndef ATOMIK_OS_H
#define ATOMIK_OS_H

#include <stdint.h>
#include <stddef.h>

/* Single source of truth for the version string. Bumped whenever a deploy
 * carries a user-visible change. About window, status bar, and the
 * /tmp/atomik_os_version stamp all read from here so the screen output
 * NEVER lies about which build is running. */
#define AOS_VERSION "v0.34-C"

/* Display geometry — locked to 1920x1080 XRGB8888 since simplefb is fixed. */
#define FB_W       1920
#define FB_H       1080
#define FB_BPP     4
#define FB_STRIDE  (FB_W * FB_BPP)
#define FB_BYTES   (FB_W * FB_H * FB_BPP)

/* HDMI safe area — see feedback_hdmi_safe_area.md.
 *
 * The AX7020 → HDMI → monitor pipeline crops the top of the framebuffer
 * (verified 2026-05-07 by direct /dev/fb0 pixel readback + user
 * observation of half-cropped magenta diagnostic).  All chrome must
 * respect these inset constants.  Wallpaper ignores them — the cropped
 * wallpaper edge is invisible.  Marker pixels and corner indicators
 * must live INSIDE the safe area or they get clipped.
 *
 * v0.31 patch 6: bumped TOP from 32 to 48.  Initial 32 was the
 * conservative read of "top half of 64-px magenta bar visible";
 * empirically the user's monitor crops more than that (status-bar
 * top edge still appeared cut at SAFE_TOP=32).  48 gives a 16-px
 * margin above the bar so the bar's top doesn't sit right on the
 * crop boundary. */
#define ATOMIK_SAFE_TOP    48
#define ATOMIK_SAFE_BOT    16
#define ATOMIK_SAFE_LEFT   16
#define ATOMIK_SAFE_RIGHT  16

/* XRGB8888 helpers. Pixels stored as 0x00RRGGBB. Macros (not inline funcs)
 * so they can be used as compile-time-constant global initializers. */
typedef uint32_t pixel_t;
#define rgb(r, g, b)        ((pixel_t)(((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))
#define rgba(r, g, b, a)    ((pixel_t)(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))

/* Brand palette — locked so the OS has a coherent identity.
 *
 * Cyan is the ATOMiK identity. We do NOT migrate to Linear-indigo or any
 * SaaS-template palette: this OS is a new class of computational machine,
 * not another browser dashboard. Cyan + near-black = engineering-tool /
 * machine-intelligence aesthetic, intentional and distinct.
 *
 * v0.25: cyan is one color in a SEMANTIC palette. Each color means
 * something specific in the visual grammar of the OS. Apps should pick
 * tokens by meaning, not by aesthetic fit. */
#define ATOMIK_BG_TOP       rgb(0x0A, 0x0E, 0x1A)  /* near-black, slight blue */
#define ATOMIK_BG_BOT       rgb(0x12, 0x18, 0x28)  /* deep navy */
#define ATOMIK_ACCENT       rgb(0x4F, 0xC3, 0xFF)  /* electric cyan */
#define ATOMIK_ACCENT_DIM   rgb(0x2A, 0x6E, 0x95)
#define ATOMIK_FG           rgb(0xF2, 0xF5, 0xFA)
#define ATOMIK_FG_DIM       rgb(0xA8, 0xB2, 0xC4)
#define ATOMIK_DOCK_BG      rgba(0x1A, 0x20, 0x30, 0xC0) /* translucent dock */
#define ATOMIK_DOCK_BORDER  rgb(0x30, 0x3A, 0x52)

/* Semantic tokens (v0.25, ChatGPT design review 2026-05-06). Use these
 * BY MEANING, not by color preference. Adding a new "looks nice violet"
 * UI element is a smell — pick the semantic token that matches the state
 * the element represents. */
#define ATOMIK_SEM_HARDWARE ATOMIK_ACCENT             /* live ATOMiK / hardware / active */
#define ATOMIK_SEM_AGENT    rgb(0x9B, 0x7E, 0xE0)     /* agent reasoning / dynamic adapt */
#define ATOMIK_SEM_SAVINGS  rgb(0x46, 0xA7, 0x58)     /* efficiency wins, cycles saved */
#define ATOMIK_SEM_WASTE    rgb(0xF2, 0xC9, 0x7D)     /* contention, redundant compute */
#define ATOMIK_SEM_OVERLOAD rgb(0xE5, 0x48, 0x4D)     /* crash, stale, IRQ storm */

/* Window chrome geometry — v0.25 makes these explicit so the spacing
 * grid is enforceable. Allowed values for any layout literal in app
 * code: {4, 8, 12, 16, 24, 32, 48, 64}. Anything else is a smell. */
#define ATOMIK_GRID_S       4
#define ATOMIK_GRID_M       8
#define ATOMIK_GRID_L       16
#define ATOMIK_TITLEBAR_H   24    /* 1.5 * text-sm = 24 px */
#define ATOMIK_BORDER_W     1     /* 1 px window border */
#define ATOMIK_DOCK_GAP     8     /* visionOS-style gap from screen edge */

/* Motion budget — v0.25, ChatGPT design review 2026-05-06.
 *
 * Linear's "we removed animations because they cost more attention than
 * they earned" rule, applied to our delta-frame budget (~10–30 Hz on a
 * soft-CPU stack). Anything animating >32×32 px at >10 Hz outside the
 * focused widget is a budget violation.
 *
 * ATOMIK_ANIM_WINDOW_OPEN: window-open fade-in (cubic ease-out, 220ms,
 * scales the whole window's bounding rect from 96% to 100%). Per design
 * review, default OFF: windows snap open. The animation still works as
 * a 1-line code change — flip to 1 for marketing screenshots / videos
 * where the fade is intentional. The OS itself feels snappier without it.
 *
 * Other motion that IS in budget and stays on:
 *   - cursor blink (8×16 invert at 1 Hz)
 *   - agent activity dot (8×8 at 2 Hz, when LLM dispatch is in flight)
 *   - notification slide-in (24px right-edge translation, 200ms)
 *   - focus-flip (instant; no animation by design) */
#define ATOMIK_ANIM_WINDOW_OPEN 0

/* fb.c — back-buffer compositor. */
int  fb_open(void);
void fb_close(void);
pixel_t *fb_back(void);   /* writeable back buffer (FB_W*FB_H pixels) */
void fb_present(void);    /* memcpy back buffer to /dev/fb0 mmap */
void fb_clear(pixel_t color);
void fb_enable_scanout(int enable);  /* drives the LiteX VTG/DMA CSRs */

/* draw.c — primitives. All operate on the back buffer. */
void draw_rect(int x, int y, int w, int h, pixel_t color);
void draw_rect_rounded(int x, int y, int w, int h, int radius, pixel_t color);
void draw_gradient_v(int x, int y, int w, int h, pixel_t top, pixel_t bot);
void draw_pixel(int x, int y, pixel_t color);
void draw_blend_pixel(int x, int y, pixel_t color, uint8_t alpha);

/* font.c — text. */
int  font_init(void);
int  text_width(const char *s, int scale);
int  text_height(int scale);
void draw_text(int x, int y, const char *s, int scale, pixel_t color);

/* wallpaper.c */
void wallpaper_draw(void);

/* agent.c — agentic usage logger + adaptive surfacing.
 *
 * Every UI event (key press, window open, window close, app launch) is
 * recorded as a delta in a small ring buffer. The agent layer derives:
 *   - per-action frequency (how often)
 *   - per-action recency  (how recently)
 *   - per-action context  (what's currently focused)
 * and surfaces a single "predicted next action" hint that the WM/dock can
 * render. v0.3 is per-process in-memory only; v0.4 will persist to
 * /var/lib/atomik_os/usage.delta. */
typedef enum {
    ACT_NONE = 0,
    ACT_OPEN_ABOUT,
    ACT_OPEN_MONITOR,
    ACT_OPEN_TERMINAL,
    ACT_OPEN_FILES,
    ACT_OPEN_NOTES,
    ACT_OPEN_CALENDAR,
    ACT_OPEN_TASKS,
    ACT_OPEN_CODE,
    ACT_OPEN_BRIEF,
    ACT_OPEN_CHAT,
    ACT_OPEN_DOCUMENT,
    ACT_CLOSE_WINDOW,
    ACT_CYCLE_FOCUS,
    ACT_DOCK_HOVER,
    ACT_QUIT,
    ACT_MAX,
} action_t;
void        agent_init(void);
void        agent_log(action_t action);
action_t    agent_predict(void);                  /* most likely next action */
const char *agent_action_name(action_t a);
int         agent_count(action_t a);              /* lifetime count */
double      agent_recency(action_t a);            /* 0..1, 1=most recent */
double      agent_score(action_t a);              /* freq * recency */
void        agent_flush(void);                    /* persist now */

/* dock.c — Capability Rail (left-anchored vertical, v0.34). */
void dock_draw(int hover_index);
int  dock_hit_test(int mouse_x, int mouse_y);  /* returns icon index or -1 */
int  dock_count(void);
/* Rail geometry: left + right edge of the Capability Rail's outer
 * frame.  Other layout code (workspace_left() in main.c) reads these
 * so apps don't tuck under the Rail. */
int  dock_left_edge(void);
int  dock_right_edge(void);
/* Returns the action_t bound to the icon at visible slot `slot`, taking
 * into account the current adaptive ordering. ACT_NONE if the slot is a
 * placeholder app (Terminal/Files/Editor not yet implemented). */
action_t dock_action_for_slot(int slot);

/* wallet.c — local token wallet (data-only declarations; window_t-based
 * draw signature lives further down, AFTER wm.c declares window_t). */
typedef struct {
    int balance_uusd;       /* user-funded balance in micro-USD */
    int spent_today_uusd;
    int daily_cap_uusd;     /* 0 = no cap */
    int last_reset_day;
} wallet_state_t;

void wallet_init  (void);
void wallet_save  (void);
const wallet_state_t *wallet_get(void);
int  wallet_topup (int uusd);
int  wallet_charge(int uusd);
int  wallet_can_afford(int uusd);
void wallet_set_daily_cap(int uusd);

/* atomik.c — ATOMiK delta-state adapter access via /dev/mem */
#define ATOMIK_BASE_PS  0xF0020000UL    /* base via PS GP1 → PL */
#define ATOMIK_N_SLOTS  8
typedef struct {
    uint32_t cmd;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t rd;
} atomik_regs_t;
int  atomik_open(void);
void atomik_close(void);
/* Read each slot's accumulator. Fills `out[ATOMIK_N_SLOTS]`. */
int  atomik_read_slots(uint32_t *out);

/* perf_counter.c declarations live below the personality_t enum.
 * (See the fabric.c declaration block further down.) */

/* fabric.c declarations are below the wm.c block (window_t needed). */
typedef enum {
    PERSONALITY_NONE = 0,
    PERSONALITY_STATE,
    PERSONALITY_SYNC,
    PERSONALITY_AGENT,
} personality_t;

/* perf_counter.c — performance instrumentation, v0.33-A.
 *
 * Foundational measurement layer.  Every other v0.33 substance piece
 * (dynamic batching, software baselines, Resource Fabric live metrics,
 * compiler skeleton, replay engine) reads from this module.
 *
 * Design rule per ChatGPT 2026-05-08: "make performance observable
 * everywhere".  perf_sample_t captures everything we'd want to see in
 * a four-way comparison matrix — software baseline, ATOMiK direct,
 * ATOMiK batched, ATOMiK profile-selected.
 *
 * Cycle source: RV64 `csrr cycle` (1-cycle granularity, ~10 ns at our
 * 100 MHz fabric clock; deterministic, no syscalls).
 */
#define PERF_MAX_OPS 256

typedef struct {
    uint64_t cycles_total;
    uint64_t cycles_software_baseline;
    uint64_t cycles_atomik;

    uint32_t ops_issued;
    uint32_t ops_logical;
    uint32_t batch_count;

    uint32_t bytes_processed;
    uint32_t bytes_avoided;

    uint32_t regions_touched;
    uint32_t regions_unique;

    personality_t active_personality;

    uint64_t per_op_cycles[PERF_MAX_OPS];
    uint32_t per_op_count;
    uint64_t p50, p95, p99;
} perf_sample_t;

uint64_t perf_rdcycle(void);
void     perf_begin(perf_sample_t *s, personality_t p);
void     perf_op(perf_sample_t *s, uint32_t bytes_in, uint32_t bytes_out,
                 uint32_t region_id);
void     perf_hw_op(perf_sample_t *s);
void     perf_batch_summary(perf_sample_t *s, uint32_t logical, uint32_t unique);
void     perf_end(perf_sample_t *s);
double   perf_speedup(const perf_sample_t *s);
double   perf_bytes_avoided_pct(const perf_sample_t *s);
double   perf_coalesce_ratio(const perf_sample_t *s);
const perf_sample_t *perf_last_sample(void);
const perf_sample_t *perf_last_for(personality_t p);

/* atomik_batch.c — dynamic batching API, v0.33-B.
 *
 * Each personality gets a different batching strategy.  Callers don't
 * choose the strategy directly; they declare WHICH PERSONALITY their
 * workload class belongs to, and the batching layer routes
 * accordingly.  This is the architectural piece that makes Resource
 * Fabric REAL — every personality changes execution behavior, not
 * just UI state.
 *
 * Profile semantics:
 *   STATE — coalesce repeated writes to the same region.  XOR/sum
 *           into a per-region accumulator within the batch.  At
 *           commit, emit ONE hardware op per UNIQUE region with the
 *           final accumulated delta.  One fence per batch (not per
 *           op).  Optimizes: ops_issued, fence overhead.
 *   SYNC  — track dirty regions since last commit.  At commit, emit
 *           a compact (region_id, delta) list of CHANGED regions
 *           only.  Skip unchanged regions entirely.  Optimizes:
 *           bytes_avoided.
 *   AGENT — relevance-weighted memory work.  Per-region recency +
 *           access-frequency score.  At commit, emit ATOMiK ops
 *           weighted toward high-relevance regions; skip stale
 *           regions.  Optimizes: memory work avoided.
 *
 * The whole point: a Document edit that touches 47 sub-deltas across
 * 8 regions becomes 8 hardware ops + 1 fence under STATE, not 47 ops
 * + 47 fences.  Software baseline does the unbatched thing for direct
 * comparison via the perf_counter layer.
 */

#define ATOMIK_BATCH_MAX_REGIONS 64

typedef enum {
    ATOMIK_PROFILE_NONE  = 0,
    ATOMIK_PROFILE_STATE = 1,    /* coalesce + low latency */
    ATOMIK_PROFILE_SYNC  = 2,    /* compact delta packets, skip unchanged */
    ATOMIK_PROFILE_AGENT = 3,    /* relevance-weighted memory work */
} atomik_profile_t;

/* Map a Resource Fabric personality to its corresponding batch profile.
 * Sync inverse mapping in fabric.c so the visualization stays honest. */
atomik_profile_t atomik_profile_from_personality(personality_t p);

/* Batch lifecycle.  Caller is responsible for matching begin/commit
 * pairs; nested batches are NOT supported (would complicate coalesce). */
void atomik_batch_begin(atomik_profile_t profile);
/* Add a delta operation to the in-flight batch.  region_id selects
 * the per-region accumulator slot (0..ATOMIK_BATCH_MAX_REGIONS-1).
 * delta is the value to XOR/sum into that region. */
void atomik_batch_add(uint32_t region_id, uint32_t delta);
/* Commit: apply the accumulated batch to the ATOMiK hardware (or the
 * software fallback if /dev/mem isn't mapped).  Side-effect: publishes
 * a perf_sample_t via perf_end so Resource Fabric can render the
 * just-completed batch's metrics.  Returns count of hardware ops
 * actually issued (always <= number of atomik_batch_add calls; equals
 * the number of UNIQUE regions touched under STATE coalesce). */
int  atomik_batch_commit(void);

/* For the software baseline path (v0.33-D): same input sequence, but
 * apply each delta with no coalescing — emits one MMIO op per
 * atomik_batch_add call, one fence per op.  Used to measure speedup. */
int  atomik_batch_commit_baseline(void);

/* Inspector: peek at the current batch state without committing.
 * Returns 1 if a batch is in flight, 0 otherwise. */
int  atomik_batch_in_flight(void);
uint32_t atomik_batch_pending_ops(void);
uint32_t atomik_batch_pending_unique_regions(void);

/* perf_bench.c — four-way comparison harness, v0.33-D.
 *
 * software / atomik_direct / atomik_batched / atomik_profile.  Same
 * input across all four columns; deterministic seed per shape; full
 * perf_sample_t captured per column.  See project_v033_substance_plan.md
 * for the architectural-compounding framing. */
typedef struct {
    int               regions;
    int               ops;
    atomik_profile_t  profile;
    perf_sample_t     software;
    perf_sample_t     atomik_direct;
    perf_sample_t     atomik_batched;
    perf_sample_t     atomik_profile_col;
} perf_bench_result_t;

void perf_bench_run(int regions, int ops, atomik_profile_t profile,
                    perf_bench_result_t *out);
void perf_bench_print_header(void);
void perf_bench_print_row(const perf_bench_result_t *r);
void perf_bench_matrix(void);

/* atomik_event.c — workload event bus.  v0.31 patch 2/N.
 *
 * Apps emit semantic events; Resource Fabric (and future consumers)
 * subscribe via atomik_event_last_ts() with priority + decay rules.
 * This is THE architectural piece per ChatGPT review 2026-05-06 —
 * turns ATOMiK Desk from polling-based to adaptive event-driven.
 *
 * Producers: agent.c (state-delta, agent-context), document.c
 * (state-delta on buffer change), llm.c (agent-context on dispatch),
 * eapp_render.c (vis-render on heavy redraw), terminal.c (build-run
 * when compiler lane lands in v0.40).
 *
 * Consumer (today): fabric.c.  Anyone else can read the same bus
 * without touching producers. */
typedef enum {
    EVT_NONE          = 0,
    EVT_STATE_DELTA   = 1,    /* buffer/field change, document edit, agent_log activity */
    EVT_SYNC_REPLICA  = 2,    /* replica/cloud-sync activity */
    EVT_AGENT_CONTEXT = 3,    /* LLM dispatch, agent inference */
    EVT_VIS_RENDER    = 4,    /* heavy framebuffer redraw / animation */
    EVT_BUILD_RUN     = 5,    /* compiler lane build-and-run (v0.40+) */
    EVT_OVERRIDE      = 6,    /* demo-override (P key, not yet wired) */
} atomik_event_kind_t;

void          atomik_event_emit(atomik_event_kind_t kind, int detail);
unsigned long atomik_event_last_ts(atomik_event_kind_t kind);
int           atomik_event_last_detail(atomik_event_kind_t kind);
unsigned long atomik_event_total(void);
/* Walk the ring in chronological order.  Returns 1 if idx-th record
 * exists, 0 if not.  out_* may be NULL. */
int           atomik_event_iter(int idx, atomik_event_kind_t *out_kind,
                                unsigned long *out_ts, int *out_detail);

/* wm.c — window manager */
typedef struct window window_t;
typedef void (*win_draw_fn)(window_t *w, int content_x, int content_y,
                            int content_w, int content_h);

#define WM_MAX_WINDOWS 16
#define WM_TITLE_H     32
#define WM_BORDER      1

struct window {
    int           id;
    char          title[64];
    int           x, y, w, h;        /* window outer rect including title bar */
    int           visible;           /* 1 = drawn, 0 = hidden but kept in stack */
    int           z;                 /* stacking order (higher = front) */
    unsigned long opened_at_ms;      /* anim_now_ms() at wm_open time */
    win_draw_fn   draw_content;      /* called to render the inside of the window */
    void         *user;              /* per-window state */
};

void      wm_init(void);
window_t *wm_open(const char *title, int x, int y, int w, int h,
                  win_draw_fn draw_content, void *user);
void      wm_close(int id);
window_t *wm_focus(int id);
window_t *wm_topmost(void);
void      wm_draw_all(void);
int       wm_handle_key(int key);   /* returns 1 if key was consumed */

/* v0.31: enumeration so the status bar can render a per-window dot
 * strip showing every open window (focused = bright cyan, buried =
 * dim cyan).  Solves the "I opened R but it's buried under D" gap
 * surfaced by the v0.30 demo — buried windows now have a visible
 * affordance even when fully covered. */
int             wm_count(void);
const window_t *wm_get(int idx);    /* NULL if idx out of range */

/* v0.34-C: canonical card colors so app draw_content callbacks match
 * the WM-level chrome tone instead of hard-coding hex literals. */
pixel_t wm_card_bg(void);
pixel_t wm_card_border(void);

/* wallet_draw — declared here, after wm.c, because it uses window_t. */
void wallet_draw(window_t *w, int x, int y, int wd, int ht);

/* fabric.c — Resource Fabric panel.  v0.30 differentiator.
 *
 * Shows what ATOMiK is doing with its compute resources right now:
 * which workload-personality is currently active, how the bank/lane
 * allocation is split, the batch queue depth, and efficiency delta
 * vs a software baseline.  Three personalities for v0.30:
 *   STATE  — change detection, dirty-region tracking, cache invalidation
 *   SYNC   — replica updates, delta propagation, state reconciliation
 *   AGENT  — agent context tracking, memory compression, LLM dispatch
 *
 * Personality auto-detected from real signals: recent LLM dispatch →
 * AGENT, recent state-delta activity → STATE, default idle → SYNC.
 *
 * Architectural-claim discipline (per project_atomik_desk_vision): we
 * do NOT claim ATOMiK literally morphs silicon into a different
 * processor.  We claim ATOMiK organizes its execution resources into
 * workload-specific batching/scheduling personalities; the fabric
 * labels which one is active.  Hardware stays the same; resource
 * allocation and operation batching change. */
void          fabric_open(void);    /* opens / focuses the Resource Fabric window */
void          fabric_draw(window_t *w, int x, int y, int wd, int ht);
personality_t fabric_active(void);  /* current detected personality */
const char   *fabric_personality_name(personality_t p);
/* Tick the fabric on every frame so detection state has a chance to
 * reclassify based on recent activity. */
void          fabric_tick(void);

/* v0.32: presenter-override controls.  Demo backup so a workload
 * can be forced for a pitch even without LLM dispatch / activity.
 * Override decays after FABRIC_OVERRIDE_HOLD_MS so a forgotten
 * override doesn't permanently lie about system state. */
void          fabric_cycle_override(void);   /* AUTO → STATE → SYNC → AGENT → AUTO */
int           fabric_override_active(void);  /* 1 if manual override engaged */
personality_t fabric_override_personality(void);
/* Geometry of the pinned right-side shelf.  Other apps' open_*()
 * functions read these to place themselves in the LEFT workspace
 * (avoiding the shelf rect) instead of fully overlapping it. */
int           fabric_shelf_x(void);
int           fabric_shelf_y(void);
int           fabric_shelf_w(void);
int           fabric_shelf_h(void);

/* terminal.c — pty-backed terminal app. Must be declared AFTER wm.c
 * because terminal_draw signature uses window_t. */
int  terminal_start(void);
void terminal_stop(void);
void terminal_pump(void);
void terminal_send_key(int key);
void terminal_draw(window_t *w, int x, int y, int wd, int ht);

/* files.c — read-only directory browser */
void files_open(void);
void files_handle_key(int key);
void files_draw(window_t *w, int x, int y, int wd, int ht);

/* notes.c — minimal text editor.
 * Persists to /tmp/atomik_os_notes.txt. v0.8.1 will move to /var. */
void notes_open(void);
void notes_handle_key(int key);
void notes_draw(window_t *w, int x, int y, int wd, int ht);

/* notify.c — non-blocking toast notifications drawn over the desktop. */
void notify_post(const char *text);
void notify_draw(void);

/* status.c — top status bar (clock, CPU, prediction surface) */
void status_draw(void);

/* ============================================================
 * INVARIANT FRAME + FIELD-DELTA RUNTIME (v0.9)
 *
 * Every app's UI = invariant_frame XOR accumulated_field_deltas.
 * Apps ship as a manifest + a stream of typed field deltas, not as C
 * code. Mirrors the ATOMiK delta-state algebra at the application layer.
 * ============================================================ */

/* The five primitive views every app composes from. The invariant frame
 * supplies a renderer for each — apps just specify which one to use and
 * stream typed field deltas into it. */
typedef enum {
    PRIM_LIST = 0,    /* vertical list of textual rows                    */
    PRIM_CARD,        /* big single-record view (title + body + meta)     */
    PRIM_GRID,        /* calendar/album-style grid of cells               */
    PRIM_FEED,        /* timeline / activity stream                       */
    PRIM_CONVO,       /* chat-style alternating bubbles                   */
    PRIM_MAX,
} primitive_t;

/* A single typed field. Apps reference fields by id; the runtime maps id
 * to a slot in the app's field table. */
typedef enum {
    FT_NONE = 0,
    FT_STR,
    FT_INT,
    FT_LIST,         /* array of strings, length in v.list_n */
    FT_COLOR,
} field_type_t;

#define FIELD_LIST_MAX  32
#define FIELD_STR_MAX   128

typedef struct {
    field_type_t type;
    char         str[FIELD_STR_MAX];
    int          i;
    char         list[FIELD_LIST_MAX][FIELD_STR_MAX];
    int          list_n;
    pixel_t      color;
} field_value_t;

#define EAPP_MAX_FIELDS 16
#define EAPP_NAME_MAX   48

/* Edge-app instance — pairs a manifest (primitive choice + field schema)
 * with the live accumulated field state. Sending a "delta" means writing
 * eapp_set_field(); the local renderer recomputes from there. */
typedef struct edge_app {
    char          name[EAPP_NAME_MAX];
    char          subtitle[EAPP_NAME_MAX];
    primitive_t   primitive;
    pixel_t       accent;
    field_value_t fields[EAPP_MAX_FIELDS];
    int           field_count;
} edge_app_t;

/* eapp.c */
void        eapp_init(edge_app_t *a, const char *name, const char *subtitle,
                      primitive_t prim, pixel_t accent);
int         eapp_add_field(edge_app_t *a, field_type_t t);
void        eapp_set_str(edge_app_t *a, int field_id, const char *s);
void        eapp_set_int(edge_app_t *a, int field_id, int v);
void        eapp_set_color(edge_app_t *a, int field_id, pixel_t c);
void        eapp_clear_list(edge_app_t *a, int field_id);
void        eapp_list_append(edge_app_t *a, int field_id, const char *s);
const char *eapp_primitive_name(primitive_t p);

/* llm.c — pluggable AI-provider abstraction + token meter.
 *
 * Every code path that wants AI inference goes through here. The
 * provider is a struct (name + base URL + model + token-cost map);
 * routing is a configurable choice; cost preview happens BEFORE the
 * request fires; audit log captures every spend.
 *
 * v0.12 ships a "stub" provider (offline, hand-mapped responses with
 * realistic token counts) plus the abstraction needed to plug a real
 * Anthropic / OpenAI / local backend in v1.0.
 *
 * The substring "stub" in the response is the user-visible signal that
 * no real network call happened — never hide an offline answer behind a
 * cost number. */
typedef struct {
    const char *name;
    const char *base_url;          /* informational only in v0.12 */
    const char *model;
    /* Cost in micro-dollars per token (1e-6 USD). */
    int         cost_in_uusd_per_token;
    int         cost_out_uusd_per_token;
    int         is_stub;           /* 1 = offline canned responses */
} llm_provider_t;

typedef struct {
    char        text[2048];        /* response text */
    int         tokens_in;
    int         tokens_out;
    int         cost_uusd;          /* total micro-USD this call */
    int         is_stub;
    int         ok;
} llm_response_t;

/* Cost preview without firing the request. token_in_estimate uses
 * 4-chars-per-token heuristic. */
int llm_estimate_tokens   (const char *prompt);
int llm_estimate_cost_uusd(const llm_provider_t *p, int tokens_in,
                           int tokens_out);

/* Synchronous call. v0.12 returns canned stub responses; v1.0 wires real
 * HTTP. Always logs to the audit trail. */
void llm_query(const llm_provider_t *p, const char *prompt, llm_response_t *out);

/* Provider registry helpers. */
const llm_provider_t *llm_default_provider(void);
const llm_provider_t *llm_provider_by_name(const char *name);
int                   llm_provider_count(void);
const llm_provider_t *llm_provider_at(int idx);

/* Audit log: list of every spend, persisted across runs. */
void llm_audit_append (const char *prompt, const llm_response_t *r);
int  llm_audit_total_uusd(void);          /* lifetime cost in micro-USD */

/* delta_log.c — versioned binary wire format for edge-app field deltas.
 *
 * Every mutation an app accumulates is encoded as one opcode + payload.
 * The same encoding works for:
 *   - on-disk persistence (one snapshot file)
 *   - on-disk replay log (append-only)
 *   - network/UART streaming (write to a pipe instead of a file)
 *
 * v0.11 ships only the on-disk encoder/decoder — streaming hookup is
 * added when we wire networking. */
typedef enum {
    OP_NONE          = 0,
    OP_SET_PRIMITIVE = 1,    /* u8 primitive          */
    OP_SET_ACCENT    = 2,    /* be32 rgb              */
    OP_SET_FIELD_STR = 3,    /* u8 id, be16 len, len bytes */
    OP_LIST_APPEND   = 4,    /* u8 id, be16 len, len bytes */
    OP_LIST_CLEAR    = 5,    /* u8 id                 */
    OP_RESET         = 6,    /* (no payload)          */
    OP_SET_NAME      = 7,    /* be16 len, len bytes   */
    OP_SET_SUBTITLE  = 8,    /* be16 len, len bytes   */
} delta_op_t;

#define DELTA_LOG_MAGIC 0x44454C54u   /* "DELT" */
#define DELTA_LOG_VER   1

/* Open a log for append; writes header on first open. Returns 0 on success. */
int  delta_log_open(const char *path);
void delta_log_close(void);

/* Encode operations into the open log (and/or stream sink). */
int  delta_emit_set_primitive (primitive_t p);
int  delta_emit_set_accent    (pixel_t accent);
int  delta_emit_set_field_str (int field_id, const char *s);
int  delta_emit_list_append   (int field_id, const char *s);
int  delta_emit_list_clear    (int field_id);
int  delta_emit_reset         (void);
int  delta_emit_set_name      (const char *s);
int  delta_emit_set_subtitle  (const char *s);

/* Replay every op in a file onto the given edge_app_t. Returns count of
 * ops applied (negative on read error). */
int  delta_replay_file        (const char *path, edge_app_t *a);

/* Snapshot the current state of an edge_app_t as a complete log file
 * (RESET + SET_PRIMITIVE + SET_ACCENT + SET_NAME + SET_SUBTITLE +
 * SET_FIELD_STR / LIST_APPEND for every field). The output is
 * self-contained — replaying it produces an identical edge_app_t. */
int  delta_snapshot_to_file   (const char *path, const edge_app_t *a);

/* eapp_render.c — the invariant-frame renderer. Draws the universal
 * window-content surface for any edge app, picking the right primitive
 * and pulling field values out of the app's accumulator. */
void eapp_draw(window_t *w, edge_app_t *a, int x, int y, int wd, int ht);

/* edge_demo.c — five reference edge apps shipped as data only */
void edge_calendar_draw(window_t *w, int x, int y, int wd, int ht);
void edge_tasks_draw(window_t *w, int x, int y, int wd, int ht);
void edge_code_draw(window_t *w, int x, int y, int wd, int ht);
void edge_brief_draw(window_t *w, int x, int y, int wd, int ht);
void edge_chat_draw(window_t *w, int x, int y, int wd, int ht);
void edge_stocks_draw(window_t *w, int x, int y, int wd, int ht);
int  stocks_tick(void);   /* returns 1 if a row mutated this call */

/* document.c — universal Document app. v0.13: multi-instance.
 *
 * Each Document window owns a heap-allocated doc_state_t (opaque). The
 * caller obtains one via document_open_new() and stores it in
 * window_t.user so document_draw_for / document_handle_key_for can
 * recover per-window state.
 *
 * Legacy single-doc shims preserved for backward compatibility. */
typedef struct doc_state doc_state_t;

doc_state_t *document_open_new(void);
void         document_close(doc_state_t *d);
void         document_handle_key_for(doc_state_t *d, int key);
void         document_draw_for(window_t *w, int x, int y, int wd, int ht);

/* Legacy singleton API (backed by a process-wide doc_state_t). */
void         document_open(void);
void         document_handle_key(int key);
void         document_draw(window_t *w, int x, int y, int wd, int ht);

/* anim.c — minimal animation runtime.
 * Returns monotonic milliseconds for use in tween functions, plus a few
 * common easing curves. The compositor checks anim_dirty() each frame to
 * decide whether to schedule another redraw. */
unsigned long anim_now_ms(void);
double        anim_ease_out(double t);  /* t in [0,1] */
double        anim_lerp(double a, double b, double t);
/* Drive open-window age tracking. Returns 0..1 for the fade-in tween. */
double        anim_window_age(int win_id, unsigned long opened_at_ms);
/* Returns 1 if any animation is currently in progress. */
int           anim_active(void);
/* Bump the animation tick — called when something visually changes. */
void          anim_tick(void);

/* input.c */
typedef enum {
    EV_NONE = 0,
    EV_KEY,
    EV_QUIT,
} event_kind_t;
typedef struct {
    event_kind_t kind;
    int key;          /* ASCII for now */
} event_t;
int  input_open(void);
void input_close(void);
event_t input_poll(int timeout_ms);

#endif /* ATOMIK_OS_H */
