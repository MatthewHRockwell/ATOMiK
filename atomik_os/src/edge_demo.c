/* edge_demo.c — three reference edge apps proving the invariant frame.
 *
 * Each app is JUST data: a manifest (name + primitive + accent) plus
 * field values populated with realistic mock content. They all render
 * through eapp_draw() — zero per-app rendering code. This is the v0.9
 * proof-of-concept for the field-delta architecture.
 */
#include "atomik_os.h"

static edge_app_t s_calendar;
static edge_app_t s_tasks;
static edge_app_t s_code;
static edge_app_t s_brief;
static edge_app_t s_chat;
static edge_app_t s_stocks;
static int        s_stocks_field = -1;
static int        s_stocks_tick  = 0;

static int s_init = 0;
static void demo_init_once(void);   /* fwd-decl for edge_stocks_draw */

static void demo_calendar_init(void) {
    eapp_init(&s_calendar, "Calendar", "May 2026  -  delta-streamed",
              PRIM_GRID, rgb(0x4F, 0xC3, 0xFF));
    int title = eapp_add_field(&s_calendar, FT_STR);   /* 0 */
    int days  = eapp_add_field(&s_calendar, FT_LIST);  /* 1 */
    eapp_set_str(&s_calendar, title, "May 2026");
    /* 35 mock cells; some have events */
    static const char *CELLS[] = {
        "27", "28", "29", "30",  "1",  "2",  "3",
         "4",  "5", "6 - sync",  "7",  "8",  "9", "10 - lunch w/ bob",
        "11", "12 - 1:1", "13", "14", "15", "16", "17",
        "18", "19", "20 - sprint", "21", "22", "23", "24",
        "25", "26", "27 - launch", "28", "29", "30", "31",
    };
    for (size_t i = 0; i < sizeof CELLS / sizeof CELLS[0]; i++)
        eapp_list_append(&s_calendar, days, CELLS[i]);
}

static void demo_tasks_init(void) {
    eapp_init(&s_tasks, "Tasks", "12 active  -  3 due today",
              PRIM_LIST, rgb(0x6E, 0xC4, 0x6E));
    int header = eapp_add_field(&s_tasks, FT_STR);
    int items  = eapp_add_field(&s_tasks, FT_LIST);
    int footer = eapp_add_field(&s_tasks, FT_STR);
    eapp_set_str(&s_tasks, header, "Today");
    eapp_list_append(&s_tasks, items, "Ship invariant-frame v0.9 to board");
    eapp_list_append(&s_tasks, items, "Wire edge_demo apps into main loop");
    eapp_list_append(&s_tasks, items, "Verify Calendar/Tasks/Code render via eapp_draw");
    eapp_list_append(&s_tasks, items, "Document field-delta wire format");
    eapp_list_append(&s_tasks, items, "Sketch agent capability matcher");
    eapp_list_append(&s_tasks, items, "Pick 3 real APIs for v1.0 demo");
    eapp_set_str(&s_tasks, footer,
                 "stream size: 1 manifest + 6 deltas, ~340 bytes");
}

static void demo_code_init(void) {
    eapp_init(&s_code, "Code", "5 PRs awaiting review",
              PRIM_FEED, rgb(0xFF, 0x6F, 0x91));
    int header = eapp_add_field(&s_code, FT_STR);
    int feed   = eapp_add_field(&s_code, FT_LIST);
    eapp_set_str(&s_code, header, "Review queue");
    eapp_list_append(&s_code, feed, "PR #142  invariant-frame: lock the chrome");
    eapp_list_append(&s_code, feed, "PR #141  agent: persist Markov transitions");
    eapp_list_append(&s_code, feed, "PR #140  dock: animate score-driven sort");
    eapp_list_append(&s_code, feed, "PR #139  notes: ctrl-S autosave debounce");
    eapp_list_append(&s_code, feed, "PR #138  monitor: read live ATOMiK slots");
}

static void demo_brief_init(void) {
    eapp_init(&s_brief, "Brief", "AI-summarized day  -  delta-streamed",
              PRIM_CARD, rgb(0xFF, 0xCB, 0x4A));
    int title    = eapp_add_field(&s_brief, FT_STR);
    int subtitle = eapp_add_field(&s_brief, FT_STR);
    int body     = eapp_add_field(&s_brief, FT_STR);
    eapp_set_str(&s_brief, title,    "Tuesday, May 6 2026");
    eapp_set_str(&s_brief, subtitle, "3 events  -  6 tasks  -  5 PRs to review");
    eapp_set_str(&s_brief, body,
        "Your morning is mostly free; the sprint sync at 10:00 needs a "
        "decision on the field-delta encoding format. Bob asked about lunch "
        "tomorrow. Your top task by recency is shipping invariant-frame v0.9 "
        "to the board.\n\nThe agent suggests starting with the wire format "
        "RFC since it unblocks both the cross-device sync and the laptop "
        "build. You typically focus best 9-11am, so block that window.");
}

static void demo_chat_init(void) {
    eapp_init(&s_chat, "Chat", "Agent conversation  -  alternating bubbles",
              PRIM_CONVO, rgb(0x4F, 0xC3, 0xFF));
    int title = eapp_add_field(&s_chat, FT_STR);
    int turns = eapp_add_field(&s_chat, FT_LIST);
    eapp_set_str(&s_chat, title, "ATOMiK Agent");
    eapp_list_append(&s_chat, turns, "agent: ready. what would you like to do?");
    eapp_list_append(&s_chat, turns, "you: schedule lunch with bob 1pm tomorrow");
    eapp_list_append(&s_chat, turns, "agent: dispatched to Calendar.create_event");
    eapp_list_append(&s_chat, turns, "you: any open PRs?");
    eapp_list_append(&s_chat, turns, "agent: 5 -- the highest priority is #142");
    eapp_list_append(&s_chat, turns, "you: review #142");
    eapp_list_append(&s_chat, turns, "agent: opened in Code -- ready when you are");
}

/* v0.23: Stocks ticker — second showcase edge-app proving the
 * field-delta vs native-binary contrast. Same eapp_draw() pipeline
 * the other 5 apps use, just different field values + a tick handler
 * that mutates one row per second to visibly demonstrate
 * field-delta-driven updates against a static compiled frame. */
typedef struct { const char *sym; int price_cents; int chg_bp; long volume; } stock_t;
static stock_t s_stocks_data[] = {
    { "ATOMK",  4250,  +320, 1840000 },
    { "AAPL", 22895,   +45, 5210000 },
    { "NVDA", 89712,  -180,  920000 },
    { "MSFT", 41218,   +12, 3140000 },
    { "GOOGL",18504,   +88, 2210000 },
    { "AMZN", 21030,   -22, 1780000 },
    { "META", 60145,  +234,  990000 },
    { "TSLA", 17812,  -415, 4860000 },
};
#define N_STOCKS ((int)(sizeof(s_stocks_data)/sizeof(s_stocks_data[0])))

static void format_row(const stock_t *t, char *buf, size_t cap) {
    /* "ATOMK   $42.50   +3.20%   1.8M" — fixed-width, monospace-friendly. */
    int dollars = t->price_cents / 100;
    int cents   = t->price_cents % 100;
    int chg_pct = t->chg_bp / 100;          /* basis-points -> percent */
    int chg_dec = (t->chg_bp >= 0 ? t->chg_bp : -t->chg_bp) % 100;
    char sign   = (t->chg_bp >= 0 ? '+' : '-');
    long vol_m  = t->volume / 100000;       /* tenths of a million */
    snprintf(buf, cap, "%-6s  $%4d.%02d   %c%d.%02d%%   %ld.%ldM",
             t->sym, dollars, cents,
             sign, chg_pct < 0 ? -chg_pct : chg_pct, chg_dec,
             vol_m / 10, vol_m % 10);
}

static void demo_stocks_init(void) {
    eapp_init(&s_stocks, "Stocks", "live ticker  -  field-delta demo",
              PRIM_FEED, rgb(0xFF, 0x6F, 0x91));
    eapp_add_field(&s_stocks, FT_STR);                  /* 0 = header */
    s_stocks_field = eapp_add_field(&s_stocks, FT_LIST); /* 1 = tickers */
    eapp_set_str(&s_stocks, 0, "MARKET  -  same chrome, different fields");
    char row[80];
    for (int i = 0; i < N_STOCKS; i++) {
        format_row(&s_stocks_data[i], row, sizeof row);
        eapp_list_append(&s_stocks, s_stocks_field, row);
    }
}

void stocks_tick(void) {
    /* Called from the main frame loop. Once a second-ish, perturb one
     * row's price and re-render that field. The visible result: the
     * compiled chrome stays put, the row text mutates — exactly the
     * field-delta pitch in motion. */
    if (s_stocks_field < 0) return;
    s_stocks_tick++;
    if ((s_stocks_tick % 10) != 0) return;       /* ~1 Hz at 100ms poll */
    int idx = (s_stocks_tick / 10) % N_STOCKS;
    stock_t *t = &s_stocks_data[idx];
    /* Pseudo-random walk: move price by -50..+50 cents, update bp. */
    static unsigned long rng = 0x12345678ul;
    rng = rng * 1103515245ul + 12345ul;
    int delta = (int)((rng >> 8) % 101) - 50;
    t->price_cents += delta;
    if (t->price_cents < 100) t->price_cents = 100;
    t->chg_bp += delta * 7;
    if (t->chg_bp >  9999) t->chg_bp =  9999;
    if (t->chg_bp < -9999) t->chg_bp = -9999;
    /* Rewrite ALL rows so the list field carries fresh content. The
     * eapp engine treats this as an atomic list-replace; on the wire
     * format it's one CLEAR + N APPENDs. */
    eapp_clear_list(&s_stocks, s_stocks_field);
    char row[80];
    for (int i = 0; i < N_STOCKS; i++) {
        format_row(&s_stocks_data[i], row, sizeof row);
        eapp_list_append(&s_stocks, s_stocks_field, row);
    }
}

void edge_stocks_draw(window_t *w, int x, int y, int wd, int ht) {
    if (!s_init) demo_init_once();      /* first-touch lazy init */
    eapp_draw(w, &s_stocks, x, y, wd, ht);
}

static void demo_init_once(void) {
    if (s_init) return;
    s_init = 1;
    demo_calendar_init();
    demo_tasks_init();
    demo_code_init();
    demo_brief_init();
    demo_chat_init();
    demo_stocks_init();
}

void edge_calendar_draw(window_t *w, int x, int y, int wd, int ht) {
    demo_init_once();
    eapp_draw(w, &s_calendar, x, y, wd, ht);
}

void edge_tasks_draw(window_t *w, int x, int y, int wd, int ht) {
    demo_init_once();
    eapp_draw(w, &s_tasks, x, y, wd, ht);
}

void edge_code_draw(window_t *w, int x, int y, int wd, int ht) {
    demo_init_once();
    eapp_draw(w, &s_code, x, y, wd, ht);
}

void edge_brief_draw(window_t *w, int x, int y, int wd, int ht) {
    demo_init_once();
    eapp_draw(w, &s_brief, x, y, wd, ht);
}

void edge_chat_draw(window_t *w, int x, int y, int wd, int ht) {
    demo_init_once();
    eapp_draw(w, &s_chat, x, y, wd, ht);
}
