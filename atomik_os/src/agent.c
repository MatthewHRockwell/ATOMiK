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
#include <string.h>

/* Logical clock — incremented on every event. Used as an integer timestamp
 * for recency. */
static unsigned long s_clock           = 0;
static int           s_count[ACT_MAX]  = {0};
static unsigned long s_last[ACT_MAX]   = {0};

static const char *NAMES[ACT_MAX] = {
    [ACT_NONE]         = "none",
    [ACT_OPEN_ABOUT]   = "Open About",
    [ACT_OPEN_MONITOR] = "Open Monitor",
    [ACT_CLOSE_WINDOW] = "Close window",
    [ACT_CYCLE_FOCUS]  = "Cycle focus",
    [ACT_DOCK_HOVER]   = "Hover dock",
    [ACT_QUIT]         = "Quit",
};

void agent_init(void) {
    s_clock = 0;
    memset(s_count, 0, sizeof s_count);
    memset(s_last,  0, sizeof s_last);
}

void agent_log(action_t a) {
    if (a <= ACT_NONE || a >= ACT_MAX) return;
    s_clock++;
    s_count[a]++;
    s_last[a] = s_clock;
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

const char *agent_action_name(action_t a) {
    if (a < 0 || a >= ACT_MAX) return "?";
    return NAMES[a];
}
