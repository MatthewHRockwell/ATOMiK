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

static int s_init = 0;

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

static void demo_init_once(void) {
    if (s_init) return;
    s_init = 1;
    demo_calendar_init();
    demo_tasks_init();
    demo_code_init();
    demo_brief_init();
    demo_chat_init();
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
