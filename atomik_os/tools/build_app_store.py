#!/usr/bin/env python3
"""Generate the seed app-store manifests in /tmp/atomik_apps/.

Each "installed app" is a single .deltas file matching the on-board
wire format. The on-board chat command `/store install <name>` reads
/tmp/atomik_apps/<name>.deltas and reconstructs the Document.

This script writes 5 reference manifests using the same schema the
on-board edge_demo.c populates: Calendar, Tasks, Code, Brief, Chat.
Each is a few hundred bytes — the entire UI of an "app" sized like a
single network packet. That's the distribution-channel pitch.

Usage:
    python3 build_app_store.py [outdir]

Default outdir is /tmp/atomik_apps/.
"""
import os, struct, sys

DELTA_LOG_MAGIC  = 0x44454C54
DELTA_LOG_VER    = 1
OP_SET_PRIMITIVE = 1
OP_SET_ACCENT    = 2
OP_SET_FIELD_STR = 3
OP_LIST_APPEND   = 4
OP_LIST_CLEAR    = 5
OP_RESET         = 6
OP_SET_NAME      = 7
OP_SET_SUBTITLE  = 8

PRIM_LIST, PRIM_CARD, PRIM_GRID, PRIM_FEED, PRIM_CONVO = 0, 1, 2, 3, 4


class W:
    def __init__(self):
        self.b = bytearray(struct.pack(">II", DELTA_LOG_MAGIC, DELTA_LOG_VER))
    def _s(self, s): b = s.encode()[:65535]; return struct.pack(">H", len(b)) + b
    def reset(self):              self.b += bytes([OP_RESET])
    def name(self, s):            self.b += bytes([OP_SET_NAME]) + self._s(s)
    def subtitle(self, s):        self.b += bytes([OP_SET_SUBTITLE]) + self._s(s)
    def primitive(self, p):       self.b += bytes([OP_SET_PRIMITIVE, p])
    def accent(self, rgb):        self.b += bytes([OP_SET_ACCENT]) + struct.pack(">I", rgb)
    def fstr(self, fid, s):       self.b += bytes([OP_SET_FIELD_STR, fid]) + self._s(s)
    def lapp(self, fid, s):       self.b += bytes([OP_LIST_APPEND, fid]) + self._s(s)
    def lclr(self, fid):          self.b += bytes([OP_LIST_CLEAR, fid])
    def write(self, path):
        with open(path, "wb") as f: f.write(self.b)
        return len(self.b)


def emit_calendar():
    w = W(); w.reset()
    w.name("Calendar"); w.subtitle("May 2026 - delta-streamed")
    w.primitive(PRIM_GRID); w.accent(0x4FC3FF)
    w.fstr(0, "May 2026"); w.lclr(1)
    cells = ["27","28","29","30","1","2","3",
             "4","5","6 - sync","7","8","9","10 - lunch w/ bob",
             "11","12 - 1:1","13","14","15","16","17",
             "18","19","20 - sprint","21","22","23","24",
             "25","26","27 - launch","28","29","30","31"]
    for c in cells: w.lapp(1, c)
    return w


def emit_tasks():
    w = W(); w.reset()
    w.name("Tasks"); w.subtitle("Today's queue")
    w.primitive(PRIM_LIST); w.accent(0x6EC46E)
    w.fstr(0, "Today"); w.lclr(1)
    for it in [
        "Ship app store v1",
        "Wire /store list/install on board",
        "Document the manifest format",
        "Demo on HDMI: install via chat",
        "Plan v0.18 manifest registry over network",
    ]:
        w.lapp(1, it)
    w.fstr(2, "stream size: ~300 bytes per app")
    return w


def emit_code():
    w = W(); w.reset()
    w.name("Code"); w.subtitle("PR queue")
    w.primitive(PRIM_FEED); w.accent(0xFF6F91)
    w.fstr(0, "Review queue"); w.lclr(1)
    for it in [
        "PR #142  /store list + /store install",
        "PR #141  invariant-frame v0.9",
        "PR #140  Markov agent + persistence",
        "PR #139  Notes ctrl-S autosave",
        "PR #138  Monitor reads ATOMiK slots",
    ]:
        w.lapp(1, it)
    return w


def emit_brief():
    w = W(); w.reset()
    w.name("Brief"); w.subtitle("AI-summarized day")
    w.primitive(PRIM_CARD); w.accent(0xFFCB4A)
    w.fstr(0, "Tuesday, May 6 2026")
    w.fstr(2,
        "3 events  -  6 tasks  -  5 PRs to review.\n"
        "Sprint sync at 10:00 needs a decision on the wire format.\n"
        "Bob asked about lunch tomorrow.\n"
        "Top task by recency: ship app store v1.\n"
        "Focus block 9-11am, deep work.")
    return w


def emit_chat():
    w = W(); w.reset()
    w.name("Chat"); w.subtitle("Agent conversation")
    w.primitive(PRIM_CONVO); w.accent(0x4FC3FF)
    w.fstr(0, "ATOMiK Agent"); w.lclr(1)
    for line in [
        "agent: ready. what would you like to do?",
        "you: install the calendar manifest",
        "agent: dispatched: /store install calendar",
        "you: morph into a tasks list",
        "agent: dispatched: /store install tasks",
        "you: nice. now show me the PR queue",
        "agent: dispatched: /store install code",
    ]:
        w.lapp(1, line)
    return w


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "/tmp/atomik_apps"
    os.makedirs(outdir, exist_ok=True)
    apps = {
        "calendar": emit_calendar(),
        "tasks":    emit_tasks(),
        "code":     emit_code(),
        "brief":    emit_brief(),
        "chat":     emit_chat(),
    }
    print(f"writing to {outdir}/")
    for name, w in apps.items():
        path = os.path.join(outdir, f"{name}.deltas")
        sz = w.write(path)
        print(f"  {name:10s}  {sz:5d} bytes  ->  {path}")
    total = sum(len(w.b) for w in apps.values())
    print(f"5 apps, total {total} bytes — distribute by URL, USB stick, or UART")


if __name__ == "__main__":
    main()
