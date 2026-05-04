#!/usr/bin/env python3
"""ATOMiK Document viewer — laptop-side renderer for the field-delta
wire format defined in `include/atomik_os.h` and emitted by
`src/delta_log.c`.

Same encoding the on-board OS uses. Different render target. Proves
the manifest is portable across CPUs / OSes / displays — a Document
configured on the AX7020 can be visualized identically on the laptop
just by replaying its delta log.

Two modes:

    atomik_view.py file.deltas             # render snapshot once
    atomik_view.py --watch file.deltas     # rerender whenever the file changes

For live sync against a running atomik_os instance:

    1. The on-board Document writes to /tmp/atomik_os_document_<id>.deltas
       on every chat command.
    2. A laptop-side bridge (e.g. a periodic 'cat' over UART, or scp on a
       networked board) pulls that file to /tmp/board_doc.deltas.
    3. atomik_view.py --watch /tmp/board_doc.deltas re-renders.

End state: same Document, both screens, synchronized state.

Renderer is plain Tk for portability. Mirrors the v0.10/v0.11 invariant
frame's primitive set: LIST, CARD, GRID, FEED, CONVO.
"""
import argparse, os, struct, sys, time

DELTA_LOG_MAGIC = 0x44454C54
DELTA_LOG_VER   = 1

OP_SET_PRIMITIVE = 1
OP_SET_ACCENT    = 2
OP_SET_FIELD_STR = 3
OP_LIST_APPEND   = 4
OP_LIST_CLEAR    = 5
OP_RESET         = 6
OP_SET_NAME      = 7
OP_SET_SUBTITLE  = 8

PRIM_NAMES = ["list", "card", "grid", "feed", "convo"]


class EdgeApp:
    def __init__(self):
        self.name      = ""
        self.subtitle  = ""
        self.primitive = 0
        self.accent    = "#4FC3FF"
        self.fields    = {0: "", 1: [], 2: ""}

    def set_field_str(self, fid, s):
        self.fields[fid] = s

    def list_append(self, fid, s):
        if fid not in self.fields or not isinstance(self.fields[fid], list):
            self.fields[fid] = []
        self.fields[fid].append(s)

    def list_clear(self, fid):
        self.fields[fid] = []


def parse_deltas(path):
    with open(path, "rb") as f: data = f.read()
    if len(data) < 8: raise ValueError("file too short")
    magic, ver = struct.unpack(">II", data[:8])
    if magic != DELTA_LOG_MAGIC: raise ValueError(f"bad magic {magic:#x}")
    if ver   != DELTA_LOG_VER:   raise ValueError(f"unsupported ver {ver}")
    pos = 8
    app = EdgeApp()
    while pos < len(data):
        op = data[pos]; pos += 1
        if op == OP_SET_PRIMITIVE:
            app.primitive = data[pos]; pos += 1
        elif op == OP_SET_ACCENT:
            (rgb,) = struct.unpack(">I", data[pos:pos+4]); pos += 4
            app.accent = "#%06x" % (rgb & 0xFFFFFF)
        elif op == OP_SET_FIELD_STR:
            fid = data[pos]; pos += 1
            n,  = struct.unpack(">H", data[pos:pos+2]); pos += 2
            s   = data[pos:pos+n].decode("utf-8", errors="replace"); pos += n
            app.set_field_str(fid, s)
        elif op == OP_LIST_APPEND:
            fid = data[pos]; pos += 1
            n,  = struct.unpack(">H", data[pos:pos+2]); pos += 2
            s   = data[pos:pos+n].decode("utf-8", errors="replace"); pos += n
            app.list_append(fid, s)
        elif op == OP_LIST_CLEAR:
            fid = data[pos]; pos += 1
            app.list_clear(fid)
        elif op == OP_RESET:
            app = EdgeApp()
        elif op == OP_SET_NAME:
            n, = struct.unpack(">H", data[pos:pos+2]); pos += 2
            app.name = data[pos:pos+n].decode("utf-8", errors="replace"); pos += n
        elif op == OP_SET_SUBTITLE:
            n, = struct.unpack(">H", data[pos:pos+2]); pos += 2
            app.subtitle = data[pos:pos+n].decode("utf-8", errors="replace"); pos += n
        else:
            raise ValueError(f"unknown op {op} at {pos-1}")
    return app


def render_tk(app, root=None):
    import tkinter as tk
    if root is None: root = tk.Tk()
    root.title(f"ATOMiK Document — {app.name or 'untitled'}")
    root.configure(bg="#0e121c")
    root.geometry("960x600")

    for w in root.winfo_children(): w.destroy()

    header = tk.Frame(root, bg="#1a2232", height=56)
    header.pack(fill="x")
    tk.Label(header, text=app.name or "Document",
             fg="#f2f5fa", bg="#1a2232",
             font=("Helvetica", 18, "bold")).pack(side="left", padx=18, pady=12)
    tk.Label(header, text=f"primitive: {PRIM_NAMES[app.primitive]}  -  "
                          f"{app.subtitle or 'delta-streamed'}",
             fg="#a8b2c4", bg="#1a2232",
             font=("Helvetica", 10)).pack(side="left", padx=12)

    body = tk.Frame(root, bg="#10161e")
    body.pack(fill="both", expand=True, padx=18, pady=18)

    title = app.fields.get(0, "")
    items = app.fields.get(1, [])
    body_str = app.fields.get(2, "")

    if title:
        tk.Label(body, text=title, fg=app.accent, bg="#10161e",
                 font=("Helvetica", 22, "bold")).pack(anchor="w", pady=(0, 12))

    if app.primitive == 0:    # LIST
        for it in items:
            row = tk.Frame(body, bg="#10161e")
            row.pack(fill="x", pady=2)
            tk.Label(row, text="•", fg=app.accent, bg="#10161e",
                     font=("Helvetica", 14)).pack(side="left", padx=(2, 8))
            tk.Label(row, text=it, fg="#f2f5fa", bg="#10161e",
                     font=("Helvetica", 12)).pack(side="left")
    elif app.primitive == 1:  # CARD
        if body_str:
            tk.Label(body, text=body_str, fg="#a8b2c4", bg="#10161e",
                     font=("Helvetica", 12), wraplength=860, justify="left"
                     ).pack(anchor="w")
    elif app.primitive == 2:  # GRID
        wrap = tk.Frame(body, bg="#10161e")
        wrap.pack(fill="both")
        for i, it in enumerate(items):
            cell = tk.Label(wrap, text=it, fg="#f2f5fa",
                            bg="#1a2232", padx=10, pady=14, width=12,
                            font=("Helvetica", 10))
            cell.grid(row=i // 7, column=i % 7, padx=4, pady=4, sticky="nsew")
    elif app.primitive == 3:  # FEED
        for it in items:
            card = tk.Frame(body, bg="#16202a")
            card.pack(fill="x", pady=4, padx=2)
            tk.Label(card, text=" ", bg=app.accent).pack(side="left", fill="y", padx=(0, 12))
            tk.Label(card, text=it, fg="#f2f5fa", bg="#16202a",
                     font=("Helvetica", 11)).pack(side="left", padx=8, pady=10)
    elif app.primitive == 4:  # CONVO
        for i, it in enumerate(items):
            row = tk.Frame(body, bg="#10161e")
            row.pack(fill="x", pady=4)
            anchor = "e" if (i & 1) else "w"
            bg     = app.accent if (i & 1) else "#1a2232"
            fg     = "#0e121c" if (i & 1) else "#f2f5fa"
            tk.Label(row, text=it, fg=fg, bg=bg,
                     padx=14, pady=8,
                     font=("Helvetica", 11)).pack(anchor=anchor, padx=12)

    foot = tk.Label(root,
                    text=f"streamed-by-delta  -  primitive: {PRIM_NAMES[app.primitive]}"
                         f"  -  fields: {len(app.fields)}  -  source: invariant frame",
                    fg="#a8b2c4", bg="#0e121c",
                    font=("Helvetica", 9))
    foot.pack(side="bottom", pady=8)
    return root


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path", help=".deltas file")
    ap.add_argument("--watch", action="store_true",
                    help="Re-render whenever the file mtime changes "
                         "(simulates cross-device sync)")
    args = ap.parse_args()

    if not os.path.exists(args.path):
        sys.exit(f"missing {args.path}")

    if not args.watch:
        try:
            app = parse_deltas(args.path)
        except Exception as e:
            sys.exit(f"parse error: {e}")
        root = render_tk(app)
        root.mainloop()
        return

    import tkinter as tk
    root = tk.Tk()
    last_mtime = [0]
    def tick():
        try:
            m = os.path.getmtime(args.path)
            if m != last_mtime[0]:
                last_mtime[0] = m
                app = parse_deltas(args.path)
                render_tk(app, root)
        except Exception as e:
            print(f"[view] {e}", file=sys.stderr, flush=True)
        root.after(500, tick)
    tick()
    root.mainloop()


if __name__ == "__main__":
    main()
