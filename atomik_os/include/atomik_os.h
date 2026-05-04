/* atomik_os.h — public types and constants shared across modules. */
#ifndef ATOMIK_OS_H
#define ATOMIK_OS_H

#include <stdint.h>
#include <stddef.h>

/* Display geometry — locked to 1920x1080 XRGB8888 since simplefb is fixed. */
#define FB_W       1920
#define FB_H       1080
#define FB_BPP     4
#define FB_STRIDE  (FB_W * FB_BPP)
#define FB_BYTES   (FB_W * FB_H * FB_BPP)

/* XRGB8888 helpers. Pixels stored as 0x00RRGGBB. Macros (not inline funcs)
 * so they can be used as compile-time-constant global initializers. */
typedef uint32_t pixel_t;
#define rgb(r, g, b)        ((pixel_t)(((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))
#define rgba(r, g, b, a)    ((pixel_t)(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))

/* Brand palette — locked so the OS has a coherent identity. */
#define ATOMIK_BG_TOP       rgb(0x0A, 0x0E, 0x1A)  /* near-black, slight blue */
#define ATOMIK_BG_BOT       rgb(0x12, 0x18, 0x28)  /* deep navy */
#define ATOMIK_ACCENT       rgb(0x4F, 0xC3, 0xFF)  /* electric cyan */
#define ATOMIK_ACCENT_DIM   rgb(0x2A, 0x6E, 0x95)
#define ATOMIK_FG           rgb(0xF2, 0xF5, 0xFA)
#define ATOMIK_FG_DIM       rgb(0xA8, 0xB2, 0xC4)
#define ATOMIK_DOCK_BG      rgba(0x1A, 0x20, 0x30, 0xC0) /* translucent dock */
#define ATOMIK_DOCK_BORDER  rgb(0x30, 0x3A, 0x52)

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

/* dock.c */
void dock_draw(int hover_index);
int  dock_hit_test(int mouse_x, int mouse_y);  /* returns icon index or -1 */
int  dock_count(void);
/* Returns the action_t bound to the icon at visible slot `slot`, taking
 * into account the current adaptive ordering. ACT_NONE if the slot is a
 * placeholder app (Terminal/Files/Editor not yet implemented). */
action_t dock_action_for_slot(int slot);

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

/* wm.c — window manager */
typedef struct window window_t;
typedef void (*win_draw_fn)(window_t *w, int content_x, int content_y,
                            int content_w, int content_h);

#define WM_MAX_WINDOWS 16
#define WM_TITLE_H     32
#define WM_BORDER      1

struct window {
    int          id;
    char         title[64];
    int          x, y, w, h;       /* window outer rect including title bar */
    int          visible;          /* 1 = drawn, 0 = hidden but kept in stack */
    int          z;                /* stacking order (higher = front) */
    win_draw_fn  draw_content;     /* called to render the inside of the window */
    void        *user;             /* per-window state */
};

void      wm_init(void);
window_t *wm_open(const char *title, int x, int y, int w, int h,
                  win_draw_fn draw_content, void *user);
void      wm_close(int id);
window_t *wm_focus(int id);
window_t *wm_topmost(void);
void      wm_draw_all(void);
int       wm_handle_key(int key);   /* returns 1 if key was consumed */

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
