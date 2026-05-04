/* agent.c — agentic usage logger + adaptive surfacing.
 *
 * The agent layer captures the user's behavior as a stream of typed
 * actions. It reasons over that stream to:
 *   1. count frequency per action
 *   2. track recency (last-seen logical timestamp)
 *   3. predict the most likely next action via a frequency × recency score
 *
 * This is the seed of the NemoClaw / OpenClaw direction — the OS is an
 * agent that adapts to YOU. v0.3 keeps everything in memory; v0.4 will
 * persist + compute Markov-style transitions for sharper prediction.
 *
 * Core insight from the ATOMiK architecture: every action IS a delta in
 * the user's behavior model. The accumulator (frequency table) is naturally
 * commutative + associative, so we could distribute the computation across
 * ATOMiK delta-slots if we ever want hardware-accelerated user-modeling.
 * For now, plain CPU bookkeeping does the job at zero overhead.
 */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Logical clock — incremented on every event. Used as an integer timestamp
 * for recency. */
static unsigned long s_clock                       = 0;
static int           s_count[ACT_MAX]              = {0};
static unsigned long s_last[ACT_MAX]               = {0};
/* Markov: count of (prev, curr) transitions. P(next=k|prev=j) ~ trans[j][k]/sum_k. */
static int           s_trans[ACT_MAX][ACT_MAX]     = {{0}};
static action_t      s_prev_action                 = ACT_NONE;
/* Persistence: written every N events. /tmp survives until reboot; we'll
 * move to /var/atomik_os/ when a writable mount is available. */
#define AGENT_STATE_PATH "/tmp/atomik_os_agent.state"
#define AGENT_SAVE_EVERY 8

static const char *NAMES[ACT_MAX] = {
    [ACT_NONE]         = "none",
    [ACT_OPEN_ABOUT]    = "Open About",
    [ACT_OPEN_MONITOR]  = "Open Monitor",
    [ACT_OPEN_TERMINAL] = "Open Terminal",
    [ACT_OPEN_FILES]    = "Open Files",
    [ACT_OPEN_NOTES]    = "Open Notes",
    [ACT_OPEN_CALENDAR] = "Open Calendar",
    [ACT_OPEN_TASKS]    = "Open Tasks",
    [ACT_OPEN_CODE]     = "Open Code",
    [ACT_OPEN_BRIEF]    = "Open Brief",
    [ACT_OPEN_CHAT]     = "Open Chat",
    [ACT_CLOSE_WINDOW]  = "Close window",
    [ACT_CYCLE_FOCUS]   = "Cycle focus",
    [ACT_DOCK_HOVER]    = "Hover dock",
    [ACT_QUIT]          = "Quit",
};

/* Compact on-disk format: a v1 magic header + clock + count[] + last[] +
 * trans[][]. Not a stable wire format — bump the magic when fields change. */
#define AGENT_STATE_MAGIC 0xA01D5742u  /* "ATOMiK agent v1" */

static void agent_load(void) {
    FILE *f = fopen(AGENT_STATE_PATH, "rb");
    if (!f) return;
    uint32_t magic = 0;
    if (fread(&magic, sizeof magic, 1, f) != 1 || magic != AGENT_STATE_MAGIC) {
        fclose(f); return;
    }
    /* Read each section; if any short-read we just keep zeroed defaults. */
    if (fread(&s_clock, sizeof s_clock, 1, f) != 1) { fclose(f); return; }
    if (fread(s_count,  sizeof s_count,  1, f) != 1) { fclose(f); return; }
    if (fread(s_last,   sizeof s_last,   1, f) != 1) { fclose(f); return; }
    if (fread(s_trans,  sizeof s_trans,  1, f) != 1) { fclose(f); return; }
    fclose(f);
}

static void agent_save(void) {
    FILE *f = fopen(AGENT_STATE_PATH, "wb");
    if (!f) return;
    uint32_t magic = AGENT_STATE_MAGIC;
    fwrite(&magic, sizeof magic, 1, f);
    fwrite(&s_clock, sizeof s_clock, 1, f);
    fwrite(s_count,  sizeof s_count, 1, f);
    fwrite(s_last,   sizeof s_last,  1, f);
    fwrite(s_trans,  sizeof s_trans, 1, f);
    fclose(f);
}

void agent_init(void) {
    s_clock      = 0;
    s_prev_action = ACT_NONE;
    memset(s_count, 0, sizeof s_count);
    memset(s_last,  0, sizeof s_last);
    memset(s_trans, 0, sizeof s_trans);
    agent_load();   /* restore from prior session if present */
}

void agent_log(action_t a) {
    if (a <= ACT_NONE || a >= ACT_MAX) return;
    s_clock++;
    s_count[a]++;
    s_last[a] = s_clock;
    if (s_prev_action > ACT_NONE && s_prev_action < ACT_MAX) {
        s_trans[s_prev_action][a]++;
    }
    s_prev_action = a;
    if ((s_clock % AGENT_SAVE_EVERY) == 0) agent_save();
}

int agent_count(action_t a) {
    return (a > ACT_NONE && a < ACT_MAX) ? s_count[a] : 0;
}

double agent_recency(action_t a) {
    if (a <= ACT_NONE || a >= ACT_MAX) return 0.0;
    if (s_clock == 0)                 return 0.0;
    return (double)s_last[a] / (double)s_clock;
}

double agent_score(action_t a) {
    /* frequency normalized to [0,1] across all actions × recency, with a
     * small constant so brand-new actions can still surface once. */
    if (a <= ACT_NONE || a >= ACT_MAX) return 0.0;
    int total = 0;
    for (int i = 1; i < ACT_MAX; i++) total += s_count[i];
    double freq    = total > 0 ? (double)s_count[a] / (double)total : 0.0;
    double recency = agent_recency(a);
    /* +0.05 floor so we still produce a non-zero ranking when the user has
     * done literally nothing yet. */
    return (freq + 0.05) * (recency + 0.1);
}

action_t agent_predict(void) {
    /* If we have a prior action and at least one transition observed from
     * it, prefer the Markov-conditioned argmax — context-aware prediction.
     * Otherwise fall back to the frequency × recency score. */
    if (s_prev_action > ACT_NONE && s_prev_action < ACT_MAX) {
        int total = 0;
        for (int k = 1; k < ACT_MAX; k++) total += s_trans[s_prev_action][k];
        if (total > 0) {
            int      best_k = 0;
            int      best_v = -1;
            for (int k = 1; k < ACT_MAX; k++) {
                if (s_trans[s_prev_action][k] > best_v) {
                    best_v = s_trans[s_prev_action][k];
                    best_k = k;
                }
            }
            if (best_v > 0) return (action_t)best_k;
        }
    }
    action_t best  = ACT_NONE;
    double   bestS = -1.0;
    for (int i = 1; i < ACT_MAX; i++) {
        double s = agent_score((action_t)i);
        if (s > bestS) {
            bestS = s;
            best  = (action_t)i;
        }
    }
    return best;
}

/* Public hook so main can flush state on clean exit. */
void agent_flush(void) { agent_save(); }

const char *agent_action_name(action_t a) {
    if (a < 0 || a >= ACT_MAX) return "?";
    return NAMES[a];
}
