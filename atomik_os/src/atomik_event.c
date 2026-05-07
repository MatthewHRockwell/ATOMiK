/* atomik_event.c — workload event bus.  v0.31 patch 2/N.
 *
 * The architectural piece that turns ATOMiK Desk from "framebuffer
 * demo with apps painted on it" into "adaptive compute environment"
 * (per ChatGPT review 2026-05-06).  Apps emit semantic workload
 * events; Resource Fabric subscribes with priority + decay rules to
 * derive its active processor personality.  No polling, no random
 * state sampling — events flow up, decisions flow down.
 *
 * v0.31 keeps the bus minimal: a tiny ring buffer of (kind, timestamp)
 * tuples and a single getter for "most-recent timestamp of kind X".
 * That's enough for fabric.c to do priority + decay.  Future
 * subscribers (notify.c, agent.c, status.c) can read the same buffer
 * without changing the producer side.
 *
 * Event kinds match the project_v031_plan.md taxonomy:
 *   STATE_DELTA   — buffer change in Document, Notes, etc.
 *   SYNC_REPLICA  — replica/cloud-sync activity
 *   AGENT_CONTEXT — LLM dispatch, agent action log
 *   VIS_RENDER    — heavy framebuffer redraw / animation tick
 *   BUILD_RUN     — compiler lane (v0.40+)
 *
 * Plus a synthetic OVERRIDE kind so the v0.31 demo-override key (`P`,
 * not yet wired) can force a personality without violating the bus
 * abstraction. */
#include "atomik_os.h"
#include <string.h>

#define EVT_RING_SZ 64

typedef struct {
    atomik_event_kind_t kind;
    unsigned long       ts_ms;
    int                 detail;     /* per-kind small payload, -1 = none */
} evt_record_t;

static evt_record_t s_ring[EVT_RING_SZ];
static int          s_head = 0;          /* next write slot */
static int          s_count = 0;         /* # records actually written */

/* Per-kind "most recent timestamp" cache so consumers don't have to
 * walk the ring every frame.  Keyed by the enum's integer value;
 * sized to comfortably hold all kinds (8 is generous for v0.31). */
#define EVT_N_KINDS 8
static unsigned long s_last_ts[EVT_N_KINDS] = {0};
static int           s_last_detail[EVT_N_KINDS] = {0};
static unsigned long s_total_emits = 0;

void atomik_event_emit(atomik_event_kind_t kind, int detail) {
    unsigned long now = anim_now_ms();
    s_ring[s_head].kind   = kind;
    s_ring[s_head].ts_ms  = now;
    s_ring[s_head].detail = detail;
    s_head = (s_head + 1) % EVT_RING_SZ;
    if (s_count < EVT_RING_SZ) s_count++;
    if (kind >= 0 && kind < EVT_N_KINDS) {
        s_last_ts[kind]     = now;
        s_last_detail[kind] = detail;
    }
    s_total_emits++;
}

unsigned long atomik_event_last_ts(atomik_event_kind_t kind) {
    if (kind < 0 || kind >= EVT_N_KINDS) return 0;
    return s_last_ts[kind];
}

int atomik_event_last_detail(atomik_event_kind_t kind) {
    if (kind < 0 || kind >= EVT_N_KINDS) return 0;
    return s_last_detail[kind];
}

unsigned long atomik_event_total(void) { return s_total_emits; }

int atomik_event_iter(int idx, atomik_event_kind_t *out_kind,
                      unsigned long *out_ts, int *out_detail) {
    if (idx < 0 || idx >= s_count) return 0;
    /* Walk the ring in chronological order.  Oldest = (head - count) mod size. */
    int slot = (s_head - s_count + idx + EVT_RING_SZ) % EVT_RING_SZ;
    if (out_kind)   *out_kind   = s_ring[slot].kind;
    if (out_ts)     *out_ts     = s_ring[slot].ts_ms;
    if (out_detail) *out_detail = s_ring[slot].detail;
    return 1;
}
