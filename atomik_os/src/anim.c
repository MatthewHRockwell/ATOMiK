/* anim.c — minimal animation runtime.
 *
 * The compositor is fundamentally event-driven (redraws on input). For
 * animations we need a tick clock so a tween can run from t=0 to t=1
 * over wall-clock time. We use CLOCK_MONOTONIC (immune to wall-clock
 * jumps) and ~16ms target frame interval = 60 Hz cap. */
#include "atomik_os.h"
#include <time.h>

#define ANIM_FRAME_MS 16
#define WINDOW_FADE_MS 220   /* opening-window fade-in duration */

static unsigned long s_last_tick_ms = 0;
static int           s_active       = 0;

/* Host repro harness (ATOMIK_PREVIEW_LIVELOOP): when sim mode is on, the clock
 * returns an injected value so the live event loop can be fast-forwarded under
 * a sanitizer to reproduce time-triggered crashes (e.g. Atom auto-dismiss). */
static int           s_sim_mode   = 0;
static unsigned long s_sim_now_ms = 0;
void anim_set_sim_time(unsigned long ms) { s_sim_mode = 1; s_sim_now_ms = ms; }

unsigned long anim_now_ms(void) {
    if (s_sim_mode) return s_sim_now_ms;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)ts.tv_sec * 1000UL +
           (unsigned long)(ts.tv_nsec / 1000000UL);
}

double anim_lerp(double a, double b, double t) {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return a + (b - a) * t;
}

/* Cubic ease-out: 1 - (1-t)^3.  Snappy at start, settles smoothly. */
double anim_ease_out(double t) {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    double inv = 1.0 - t;
    return 1.0 - inv * inv * inv;
}

double anim_window_age(int win_id, unsigned long opened_at_ms) {
    (void)win_id;
    unsigned long now = anim_now_ms();
    if (now < opened_at_ms) return 1.0;
    unsigned long age = now - opened_at_ms;
    if (age >= WINDOW_FADE_MS) return 1.0;
    return (double)age / (double)WINDOW_FADE_MS;
}

int anim_active(void) {
    /* Active for one frame after a tick so the caller knows to schedule
     * a follow-up redraw. The s_active flag is cleared if no tick comes
     * within ~3 frames. */
    unsigned long now = anim_now_ms();
    if (now - s_last_tick_ms > ANIM_FRAME_MS * 3) s_active = 0;
    return s_active;
}

void anim_tick(void) {
    s_last_tick_ms = anim_now_ms();
    s_active       = 1;
}
