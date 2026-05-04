#!/usr/bin/env python3
"""ATOMiK delta-log -> Microsoft AdaptiveCards JSON.

Reverse direction of `adaptivecards_to_atomik.py`. Reads our binary
field-delta wire format, emits an AdaptiveCard JSON document that any
mature renderer (Outlook, Teams, the AdaptiveCards visualizer) can
display.

Together with the forward adapter, ATOMiK OS becomes a two-way bridge
for the AdaptiveCards ecosystem: ingest any AdaptiveCard, render it
cheaply on edge hardware AS deltas; export any on-board Document AS
AdaptiveCards JSON for browser / Outlook / Teams consumption. Same
typed UI, multiple render targets.

Pipeline:
    /tmp/board_doc.deltas
        ->  atomik_to_adaptivecards.py  ->  card.json
        ->  AdaptiveCards.io visualizer / Outlook / Teams

Usage:
    atomik_to_adaptivecards.py /tmp/board_doc_1.deltas card.json
    atomik_to_adaptivecards.py - card.json   < /tmp/board_doc_1.deltas
"""
import json
import os
import struct
import sys

# Mirror of include/atomik_os.h
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


def parse_deltas(data: bytes):
    if len(data) < 8: raise ValueError("file too short")
    magic, ver = struct.unpack(">II", data[:8])
    if magic != DELTA_LOG_MAGIC: raise ValueError(f"bad magic {magic:#x}")
    if ver   != DELTA_LOG_VER:   raise ValueError(f"unsupported ver {ver}")
    pos = 8
    name = subtitle = ""
    primitive = PRIM_LIST
    accent_rgb = 0x4FC3FF
    fields = {0: "", 1: [], 2: ""}
    while pos < len(data):
        op = data[pos]; pos += 1
        if op == OP_SET_PRIMITIVE:
            primitive = data[pos]; pos += 1
        elif op == OP_SET_ACCENT:
            accent_rgb, = struct.unpack(">I", data[pos:pos+4]); pos += 4
            accent_rgb &= 0xFFFFFF
        elif op == OP_SET_FIELD_STR:
            fid = data[pos]; pos += 1
            n,  = struct.unpack(">H", data[pos:pos+2]); pos += 2
            s   = data[pos:pos+n].decode("utf-8", errors="replace"); pos += n
            fields[fid] = s
        elif op == OP_LIST_APPEND:
            fid = data[pos]; pos += 1
            n,  = struct.unpack(">H", data[pos:pos+2]); pos += 2
            s   = data[pos:pos+n].decode("utf-8", errors="replace"); pos += n
            if not isinstance(fields.get(fid), list): fields[fid] = []
            fields[fid].append(s)
        elif op == OP_LIST_CLEAR:
            fid = data[pos]; pos += 1
            fields[fid] = []
        elif op == OP_RESET:
            fields = {0: "", 1: [], 2: ""}
        elif op == OP_SET_NAME:
            n, = struct.unpack(">H", data[pos:pos+2]); pos += 2
            name = data[pos:pos+n].decode("utf-8", errors="replace"); pos += n
        elif op == OP_SET_SUBTITLE:
            n, = struct.unpack(">H", data[pos:pos+2]); pos += 2
            subtitle = data[pos:pos+n].decode("utf-8", errors="replace"); pos += n
        else:
            raise ValueError(f"unknown op {op} at {pos-1}")
    return name, subtitle, primitive, accent_rgb, fields


def accent_to_style(rgb):
    """Map our accent palette back to AdaptiveCards style buckets."""
    palette = {
        0x4FC3FF: "emphasis",
        0x6EC46E: "good",
        0xFFCB4A: "warning",
        0xFF6F91: "attention",
        0xF2F5FA: "default",
    }
    return palette.get(rgb & 0xFFFFFF, "default")


def to_adaptive_card(name, subtitle, primitive, accent_rgb, fields):
    """Build an AdaptiveCard 1.5 JSON object."""
    title = fields.get(0, "")
    items = fields.get(1, []) or []
    body  = fields.get(2, "")

    body_blocks = []
    if title:
        body_blocks.append({
            "type": "TextBlock", "size": "ExtraLarge", "weight": "Bolder",
            "text": title,
        })
    if subtitle:
        body_blocks.append({
            "type": "TextBlock", "isSubtle": True, "wrap": True,
            "text": subtitle,
        })

    if primitive == PRIM_LIST or primitive == PRIM_FEED or primitive == PRIM_CONVO:
        for it in items:
            body_blocks.append({"type": "TextBlock", "wrap": True, "text": it})
    elif primitive == PRIM_CARD:
        if body:
            body_blocks.append({"type": "TextBlock", "wrap": True, "text": body})
    elif primitive == PRIM_GRID:
        # Render the grid as a ColumnSet of 7-wide columns of TextBlocks.
        rows = [items[i:i+7] for i in range(0, len(items), 7)]
        for row in rows:
            cols = []
            for cell in row:
                cols.append({
                    "type": "Column",
                    "items": [{"type": "TextBlock", "text": cell}],
                    "width": "stretch",
                })
            body_blocks.append({"type": "ColumnSet", "columns": cols})

    if body and primitive != PRIM_CARD:
        # Footer-ish text; render as subtle below the body.
        body_blocks.append({
            "type": "TextBlock", "wrap": True, "isSubtle": True,
            "text": body,
        })

    card = {
        "type": "AdaptiveCard",
        "version": "1.5",
        "$schema": "http://adaptivecards.io/schemas/adaptive-card.json",
        "speak": name or "ATOMiK Document",
        "style": accent_to_style(accent_rgb),
        "body": body_blocks,
    }
    return card


def main():
    if len(sys.argv) < 3:
        print(__doc__); sys.exit(2)
    inp, outp = sys.argv[1], sys.argv[2]
    raw = sys.stdin.buffer.read() if inp == "-" else open(inp, "rb").read()
    name, subtitle, prim, accent, fields = parse_deltas(raw)
    card = to_adaptive_card(name, subtitle, prim, accent, fields)
    out = json.dumps(card, indent=2)
    with open(outp, "w") as f: f.write(out)
    print(f"wrote {outp} ({len(out)} bytes; {len(card['body'])} blocks)")


if __name__ == "__main__":
    main()
