/* assistant.c — v0.39-A summoned ATOMiK Assistant ("Atom") overlay.
 *
 * Per ChatGPT 2026-05-16 + Matt directive: silicon-octopus character
 * that EXPLAINS ATOMiK but never BECOMES ATOMiK.  Class B visual
 * asset + Class A event/metric context.  The character art is
 * pre-rendered (assets/assistant/assistant_idle_160.atomik_asset).
 * The speech is generated from deterministic templates that route
 * through fabric_active() + metric_get() — never invented.
 *
 * MVP behavior (v0.39-A):
 *   - Summon: 'O' on Atom rail cell (via dock action) OR '?'/'H' keys.
 *   - Render: glass speech bubble in lower-left workspace, 160 px
 *     character bust on the left, 2-line AA explanation on the right.
 *   - Dismiss: Esc, another summon, or auto after 12 s.
 *   - Never covers the Resource Fabric (right shelf).
 *
 * Speech templates:
 *   - SYNC active:   "SYNC is active. ATOMiK emitted N deltas and
 *                     skipped unchanged replica state."
 *   - STATE active:  "STATE is active. Repeated writes were
 *                     coalesced before commit."
 *   - AGENT active:  "AGENT is active. Hot context retained, cold
 *                     context skipped."
 *   - (no active):   "ATOMiK is idle, waiting for workload."
 *
 * Deterministic — no LLM.  Future v0.39-B will wire event-aware
 * triggers; v0.39-C will add sprite states.  This slice is the
 * static foundation. */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

#define ASSIST_AUTO_DISMISS_MS    12000
#define ASSIST_AUTO_RATELIMIT_MS   5000   /* v0.39-B */
#define ASSIST_DISMISS_COOLDOWN_MS 30000  /* v0.39-B */
#define ASSIST_BUBBLE_W           640
#define ASSIST_BUBBLE_H           220
#define ASSIST_AVATAR_PX          160
#define ASSIST_MARGIN             ATOMIK_GRID_L

static atomik_asset_t   s_avatar;
static int              s_avatar_loaded = 0;
static int              s_visible       = 0;
static unsigned long    s_shown_ms      = 0;
static assistant_mode_t   s_mode          = ASSIST_EXPLAIN;
static assistant_source_t s_src           = ASSIST_SRC_MANUAL;  /* v0.39-D */
/* v0.39-B auto-summon state. */
static unsigned long    s_last_auto_ms     = 0;
static unsigned long    s_last_dismiss_ms  = 0;
/* Last frame's bubble rect — used to mark dirty on dismiss so the
 * desktop is repainted cleanly. */
static int s_last_x, s_last_y, s_last_w, s_last_h;

int assistant_init(void) {
    if (s_avatar_loaded) return 1;
    int rc = atomik_asset_load(
        "/tmp/atomik_assets/assistant_idle_160.atomik_asset",
        &s_avatar);
    if (rc != 0) {
        memset(&s_avatar, 0, sizeof s_avatar);
        return 0;
    }
    s_avatar_loaded = 1;
    return 1;
}

void assistant_summon(void) {
    assistant_summon_mode(ASSIST_EXPLAIN);
}

void assistant_summon_mode(assistant_mode_t m) {
    if (s_visible) {
        /* Toggle off if already showing. */
        assistant_dismiss();
        return;
    }
    if (!s_avatar_loaded) assistant_init();
    s_mode     = m;
    s_src      = ASSIST_SRC_MANUAL;       /* v0.39-D — manual entry */
    s_visible  = 1;
    s_shown_ms = anim_now_ms();
}

void assistant_summon_capture_success(void) {
    /* v0.39-D capture path — bypasses auto-summon gates so the SUCCESS
     * halo / "<P> data live" title can be photographed deterministically.
     * If already visible, hide first so the dirty rect repaints
     * cleanly (else two halos can overlap during animation). */
    if (s_visible) { assistant_dismiss(); }
    if (!s_avatar_loaded) assistant_init();
    s_mode     = ASSIST_SUCCESS;
    s_src      = ASSIST_SRC_FIRST_LIVE;
    s_visible  = 1;
    s_shown_ms = anim_now_ms();
}

/* v0.42: report "animating" while the character overlay is up so the main
 * loop keeps repainting (float bob + breathing aura).  Rate-limited to ~6 fps
 * — smooth enough to read as alive, light enough for the soft CPU. */
static unsigned long s_last_anim_ms = 0;
int assistant_animating(void) {
    if (!s_visible || !s_avatar_loaded) return 0;
    unsigned long now = anim_now_ms();
    if (now - s_last_anim_ms < 160) return 0;
    s_last_anim_ms = now;
    return 1;
}

void assistant_dismiss(void) {
    if (!s_visible) return;
    s_visible = 0;
    s_last_dismiss_ms = anim_now_ms();
    /* Mark the previous bubble rect dirty so the desktop redraws
     * cleanly behind it on the next frame. */
    if (s_last_w > 0 && s_last_h > 0) {
        dirty_rect(s_last_x, s_last_y, s_last_w, s_last_h);
    }
}

/* v0.39-B.1 text-input suppression.  v0.39-C: drop Files — browsing
 * file lists isn't typing.  ChatGPT 2026-05-19: "Suppress only when
 * its search/filter input owns focus."  TODO: when WM gets a
 * WM_WINDOW_TEXT_INPUT flag, replace this title-prefix list. */
static int text_input_focused(void) {
    const window_t *top = wm_topmost();
    if (!top) return 0;
    if (strncmp(top->title, "Terminal", 8)  == 0) return 1;
    if (strncmp(top->title, "Notes",    5)  == 0) return 1;
    if (strncmp(top->title, "Document", 8)  == 0) return 1;
    return 0;
}

/* v0.39-B auto-summon gating.  Returns 1 if it's OK to fire an
 * auto-summon right now, 0 otherwise.  Respects four guards:
 *   - operator opt-out file /tmp/atomik_assist_auto containing "off"
 *   - 5 s rate-limit between auto-summons
 *   - 30 s cooldown after an explicit Esc/I dismiss
 *   - text-input window focused (v0.39-B.1) — don't steal focus
 *     attention while the operator is typing
 * Manual paths (assistant_summon() called from key router or rail
 * cell) skip this gate entirely. */
static int auto_summon_allowed(unsigned long now) {
    FILE *f = fopen("/tmp/atomik_assist_auto", "r");
    if (f) {
        char buf[8] = {0};
        if (fgets(buf, sizeof buf, f)) {
            if (strncmp(buf, "off", 3) == 0) { fclose(f); return 0; }
        }
        fclose(f);
    }
    if (text_input_focused()) return 0;
    if (s_last_dismiss_ms > 0 &&
        (now - s_last_dismiss_ms) < ASSIST_DISMISS_COOLDOWN_MS) {
        return 0;
    }
    if (s_last_auto_ms > 0 &&
        (now - s_last_auto_ms) < ASSIST_AUTO_RATELIMIT_MS) {
        return 0;
    }
    return 1;
}

void assistant_on_personality_change(personality_t old_p,
                                     personality_t new_p) {
    if (old_p == new_p) return;
    if (new_p == PERSONALITY_NONE) return;
    unsigned long now = anim_now_ms();
    if (!auto_summon_allowed(now)) return;
    s_last_auto_ms = now;
    if (!s_avatar_loaded) assistant_init();
    /* v0.39-C: personality switch is an explanation moment.
     * v0.39-D: title becomes "<P> workload detected". */
    s_mode     = ASSIST_EXPLAIN;
    s_src      = ASSIST_SRC_SWITCH;
    s_visible  = 1;
    s_shown_ms = now;
}

void assistant_on_first_live(personality_t p) {
    (void)p;
    unsigned long now = anim_now_ms();
    if (!auto_summon_allowed(now)) return;
    s_last_auto_ms = now;
    if (!s_avatar_loaded) assistant_init();
    /* v0.39-C: data started flowing — good news, success halo.
     * v0.39-D: title becomes "<P> data live". */
    s_mode     = ASSIST_SUCCESS;
    s_src      = ASSIST_SRC_FIRST_LIVE;
    s_visible  = 1;
    s_shown_ms = now;
}

int assistant_visible(void) { return s_visible; }

void assistant_tick(void) {
    if (!s_visible) return;
    unsigned long now = anim_now_ms();
    if (now - s_shown_ms > ASSIST_AUTO_DISMISS_MS) {
        assistant_dismiss();
    }
}

/* v0.39-A.5 speech composition.  Three pieces:
 *   - title   : dominant line in the active personality color
 *                ("SYNC active" / "STATE active" / "AGENT active").
 *   - detail  : Class A-safe explanation referencing one live metric.
 *   - color   : lane color for the title.  ATOMIK_FG_DIM if idle.
 * The "Atom" name is no longer the dominant header — per ChatGPT
 * 2026-05-18 "Atom explains ATOMiK; Atom does not become ATOMiK." */
static const char *title_suffix_for_src(assistant_source_t src) {
    /* v0.39-D — title variant per WHY Atom appeared.
     *   MANUAL     → "<P> active"            (user opened it)
     *   SWITCH     → "<P> workload detected" (personality changed)
     *   FIRST_LIVE → "<P> data live"         (lane went LIVE)
     */
    switch (src) {
    case ASSIST_SRC_SWITCH:     return "workload detected";
    case ASSIST_SRC_FIRST_LIVE: return "data live";
    case ASSIST_SRC_MANUAL:
    default:                    return "active";
    }
}

static void compose_message(char *title, size_t tcap,
                            char *detail, size_t dcap,
                            pixel_t *color_out) {
    personality_t p = fabric_active();
    const char *suffix = title_suffix_for_src(s_src);
    title[0] = detail[0] = 0;
    pixel_t c = ATOMIK_FG_DIM;
    switch (p) {
    /* v0.39-C — per-personality trailing clauses, less repetition
     * than B.1's uniform "stays quiet" everywhere.
     * v0.39-D — title suffix varies with summon source. */
    case PERSONALITY_STATE: {
        const atomik_metric_t *coal = metric_get("state.coalesce_pct");
        snprintf(title, tcap, "STATE %s", suffix);
        c = ATOMIK_SEM_HARDWARE;
        if (coal && coal->source != METRIC_WAITING) {
            snprintf(detail, dcap,
                     "%.0f%% coalesced; unchanged regions stay quiet.",
                     coal->value);
        } else {
            snprintf(detail, dcap,
                     "Repeated writes coalesce before commit.");
        }
        break;
    }
    case PERSONALITY_SYNC: {
        const atomik_metric_t *ops = metric_get("sync.ops_issued");
        snprintf(title, tcap, "SYNC %s", suffix);
        c = ATOMIK_SEM_SAVINGS;
        if (ops && ops->source != METRIC_WAITING && ops->value > 0) {
            /* v0.39-D — "out of the frame" was too cinematic; collided
             * with framebuffer / render language.  ChatGPT 2026-05-19. */
            snprintf(detail, dcap,
                     "%u deltas emitted; unchanged replicas stay silent.",
                     (unsigned)ops->value);
        } else {
            snprintf(detail, dcap,
                     "Replica-tracking; unchanged replicas stay silent.");
        }
        break;
    }
    case PERSONALITY_AGENT: {
        const atomik_metric_t *ret = metric_get("agent.ops_issued");
        snprintf(title, tcap, "AGENT %s", suffix);
        c = ATOMIK_SEM_AGENT;
        if (ret && ret->source != METRIC_WAITING && ret->value > 0) {
            snprintf(detail, dcap,
                     "%u regions retained; cold context remains parked.",
                     (unsigned)ret->value);
        } else {
            snprintf(detail, dcap,
                     "Hot context retained; cold context remains parked.");
        }
        break;
    }
    default:
        snprintf(title, tcap, "ATOMiK idle");
        snprintf(detail, dcap, "Waiting for a workload to fire.");
        break;
    }
    if (color_out) *color_out = c;
}

void assistant_draw(void) {
    if (!s_visible) return;

    /* Position: lower-left of the workspace, anchored above the
     * bottom-of-screen edge.  Never overlaps the Fabric shelf. */
    int x = dock_right_edge() + ATOMIK_GRID_L * 2;
    int y = FB_H - ASSIST_BUBBLE_H - ATOMIK_GRID_L * 2;
    int w = ASSIST_BUBBLE_W;
    int h = ASSIST_BUBBLE_H;
    if (x + w > fabric_shelf_x() - ATOMIK_GRID_L) {
        w = fabric_shelf_x() - ATOMIK_GRID_L - x;
        if (w < 320) w = 320;
    }

    /* v0.39-D: store the EXPANDED rect (including aura) so dismiss
     * repaints the full breathing halo footprint. */
    s_last_x = x - 56; s_last_y = y - 28;
    s_last_w = w + 56 + 12; s_last_h = h + 56;

    /* Glass bubble body — same dark navy palette as other chrome. */
    pixel_t body   = rgb(0x10, 0x18, 0x2C);
    pixel_t accent = ATOMIK_SEM_AGENT;         /* violet for the assistant */
    int radius = 18;
    draw_rect_rounded(x, y, w, h, radius, body);

    /* Layered alpha halo around the bubble — gives the "summoned"
     * lift without being a hard outline. */
    for (int g = 1; g <= 4; g++) {
        uint8_t a = (uint8_t)(36 - (g - 1) * 8);
        for (int sx = 0; sx < w + 2 * g; sx++) {
            draw_blend_pixel(x - g + sx, y - g,           accent, a);
            draw_blend_pixel(x - g + sx, y + h - 1 + g,   accent, a);
        }
        for (int sy = 0; sy < h + 2 * g; sy++) {
            draw_blend_pixel(x - g,           y - g + sy, accent, a);
            draw_blend_pixel(x + w - 1 + g,   y - g + sy, accent, a);
        }
    }
    /* Bright 1 px violet border around the rounded body. */
    draw_rect(x + radius, y,              w - radius * 2, 1, accent);
    draw_rect(x + radius, y + h - 1,      w - radius * 2, 1, accent);
    draw_rect(x,              y + radius, 1, h - radius * 2, accent);
    draw_rect(x + w - 1,      y + radius, 1, h - radius * 2, accent);

    /* Avatar — 160 px Atom bust on the left, vertically centered.
     * v0.39-C: paint a mode-tinted alpha halo behind Atom so the
     * assistant visibly reflects WHY it appeared.  Same asset, just
     * a different colored aura.  Halo also pulses with anim_now_ms
     * so the still-pose character feels alive. */
    int av_x = x + ATOMIK_GRID_L + 4;
    int av_y = y + (h - ASSIST_AVATAR_PX) / 2;

    /* v0.42 Atom float: a gentle vertical bob (+/-4 px, 2.4 s period) synced
     * to the aura's breathing phase so the character "hovers" alive on the
     * card.  Driven by anim_now_ms; the main loop repaints while the overlay
     * is visible (assistant_animating).  Taylor sine — no math.h here. */
    double assist_sn;
    {
        unsigned long now0 = anim_now_ms();
        double ph = (double)(now0 % 2400) / 2400.0 * 6.2831853;
        while (ph > 3.14159265) ph -= 6.2831853;
        while (ph < -3.14159265) ph += 6.2831853;
        double p3 = ph*ph*ph, p5 = p3*ph*ph;
        assist_sn = ph - p3/6.0 + p5/120.0;
    }
    av_y += (int)(4.0 * assist_sn);

    /* v0.39-E aura palette.
     * Cyan is the brand identity for Atom; SUCCESS keeps cyan for
     * the INNER two rings and paints the OUTER ring emerald so the
     * frame reads "alive with good news" instead of "alert".  Other
     * modes use a single color across all rings — only SUCCESS gets
     * the rim-split treatment.  ChatGPT 2026-05-19: "Smallest fix is
     * not more green everywhere, it is a visible emerald outer rim
     * while keeping the cyan core."
     */
    pixel_t core_aura, rim_aura;
    switch (s_mode) {
    case ASSIST_SUCCESS:
        core_aura = ATOMIK_SEM_HARDWARE;        /* cyan core    */
        rim_aura  = ATOMIK_SEM_SAVINGS;          /* emerald rim  */
        break;
    case ASSIST_THINKING:
        core_aura = rim_aura = ATOMIK_SEM_AGENT;     /* violet   */
        break;
    case ASSIST_WARNING:
        core_aura = rim_aura = ATOMIK_SEM_WASTE;     /* amber    */
        break;
    case ASSIST_IDLE:
        core_aura = rim_aura = ATOMIK_FG_DIM;
        break;
    case ASSIST_EXPLAIN:
    default:
        core_aura = rim_aura = ATOMIK_SEM_HARDWARE;  /* cyan     */
        break;
    }

    /* Concentric alpha rings centered on the avatar bounding box.
     * Three layers: wide soft halo, mid ring, tight inner bloom.
     * v0.39-E — layer 2 (outermost) uses rim_aura; layers 0/1 use
     * core_aura.  For EXPLAIN both are cyan, so no visual change.
     * For SUCCESS the outer ring is emerald, identity stays cyan.
     * Pulse amplitude driven by anim_now_ms so Atom breathes. */
    {
        double pulse = 0.7 + 0.3 * assist_sn;   /* synced with the float bob */
        int cx = av_x + ASSIST_AVATAR_PX / 2;
        int cy = av_y + ASSIST_AVATAR_PX / 2;
        for (int layer = 0; layer < 3; layer++) {
            int   r_out  = ASSIST_AVATAR_PX / 2 + 18 + layer * 14;
            int   r_in   = r_out - 12;
            uint8_t base = (uint8_t)((50 - layer * 14) * pulse);
            if (base == 0) continue;
            int r_out2 = r_out * r_out;
            int r_in2  = r_in  * r_in;
            /* Outer ring (layer 2) takes the rim color; inner rings
             * take the core color.  v0.39-E split for SUCCESS state. */
            pixel_t layer_color = (layer == 2) ? rim_aura : core_aura;
            /* When the rim differs from the core (SUCCESS), give the
             * outer rim a touch more alpha so green is more visible
             * than the cyan it sits beside.  Otherwise keep the
             * gentle fall-off. */
            uint8_t layer_alpha = base;
            if (layer == 2 && rim_aura != core_aura) {
                layer_alpha = (uint8_t)((unsigned)base * 7 / 4);
                if (layer_alpha > 80) layer_alpha = 80;
            }
            for (int dy = -r_out; dy <= r_out; dy++) {
                for (int dx = -r_out; dx <= r_out; dx++) {
                    int d2 = dx*dx + dy*dy;
                    if (d2 <= r_out2 && d2 >= r_in2) {
                        draw_blend_pixel(cx + dx, cy + dy, layer_color,
                                         layer_alpha);
                    }
                }
            }
        }
    }

    if (s_avatar_loaded) {
        atomik_asset_blit(&s_avatar, av_x, av_y);
    } else {
        /* Fallback — violet placeholder rectangle with "Atom" text. */
        int aw = ASSIST_AVATAR_PX;
        int ah = ASSIST_AVATAR_PX;
        draw_rect_rounded(av_x, av_y, aw, ah, 12,
                          rgb(0x1E, 0x16, 0x40));
        const char *nm = "ATOM";
        if (font_aa_loaded(FONT_AA_UI)) {
            int tw = text_width_aa(FONT_AA_UI, nm);
            int th = text_height_aa(FONT_AA_UI);
            draw_text_aa(FONT_AA_UI, av_x + (aw - tw) / 2,
                         av_y + (ah - th) / 2, nm, accent);
        }
    }

    /* v0.39-A.5 hierarchy:
     *   small dim "Atom" label up top (subordinate)
     *   dominant `<PERSONALITY> active` line in lane color
     *   supporting detail line (Class A-safe wording)
     *   "Esc to dismiss" hint
     * Atom no longer claims the dominant header. */
    char title[48], detail[160];
    pixel_t personality_col = accent;
    compose_message(title, sizeof title, detail, sizeof detail,
                    &personality_col);

    int tx     = av_x + ASSIST_AVATAR_PX + ATOMIK_GRID_L * 2;

    /* Small "Atom" label — dim slate. */
    int label_y = y + ATOMIK_GRID_L + 4;
    if (font_aa_loaded(FONT_AA_LABEL)) {
        draw_text_aa(FONT_AA_LABEL, tx, label_y, "Atom",
                     rgb(0x6A, 0x76, 0x92));
        label_y += text_height_aa(FONT_AA_LABEL) + 4;
    } else {
        draw_text(tx, label_y, "Atom", 1, rgb(0x6A, 0x76, 0x92));
        label_y += text_height(1) + 4;
    }

    /* Dominant personality line in lane color. */
    int title_y = label_y;
    if (font_aa_loaded(FONT_AA_DISPLAY)) {
        draw_text_aa(FONT_AA_DISPLAY, tx, title_y, title,
                     personality_col);
        title_y += text_height_aa(FONT_AA_DISPLAY) + ATOMIK_GRID_S;
    } else {
        draw_text(tx, title_y, title, 3, personality_col);
        title_y += text_height(3) + ATOMIK_GRID_S;
    }

    /* Supporting detail line in dim FG. */
    if (font_aa_loaded(FONT_AA_UI)) {
        draw_text_aa(FONT_AA_UI, tx, title_y, detail, ATOMIK_FG_DIM);
    } else {
        draw_text(tx, title_y, detail, 2, ATOMIK_FG_DIM);
    }

    /* "Esc to dismiss" hint, quiet slate. */
    if (font_aa_loaded(FONT_AA_LABEL)) {
        const char *hint = "Esc to dismiss";
        draw_text_aa(FONT_AA_LABEL, tx,
                     y + h - text_height_aa(FONT_AA_LABEL) - 8,
                     hint, rgb(0x5A, 0x66, 0x82));
    }

    /* Mark dirty.  v0.39-D — the aura halo extends ~46 px LEFT of the
     * bubble (avatar is on the left, outer aura ring radius is 126
     * pixels around an 80-pixel-radius avatar) and ~20 px above /
     * below.  6-px pad from v0.39-C left stale pulse pixels on the
     * upswing.  Use 56 px left, 28 px top/bot, 12 px right to cover
     * the full breathing aura without painting the whole screen. */
    dirty_rect(x - 56, y - 28, w + 56 + 12, h + 56);
}
