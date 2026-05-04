#!/usr/bin/env python3
"""AdaptiveCards JSON -> ATOMiK field-delta wire-format compiler.

Demonstrates the partnership pitch: ATOMiK OS doesn't need to invent the
manifest schema. Existing generative-UI players (Microsoft AdaptiveCards,
Vercel v0, Thesys, Anthropic Artifacts, OpenAI structured outputs) already
produce UI specs at the model layer. We render them at the OS layer for a
memcpy's worth of compute.

Pipeline:
    AdaptiveCards JSON  ->  this script  ->  .deltas binary
    .deltas binary      ->  scp/UART     ->  /tmp/atomik_os_document.deltas
    atomik_os boots     ->  delta_replay_file()  ->  on-screen UI

Usage:
    adaptivecards_to_atomik.py input.json output.deltas
    adaptivecards_to_atomik.py - <<< '{"type":"AdaptiveCard",...}'   # stdin

Wire format mirrors include/atomik_os.h:
    magic     "DELT" (be32, 0x44454C54)
    version   1     (be32)
    op stream:
        OP_RESET=6
        OP_SET_NAME=7  : be16 len + bytes
        OP_SET_SUBTITLE=8
        OP_SET_PRIMITIVE=1 : u8 (0=LIST 1=CARD 2=GRID 3=FEED 4=CONVO)
        OP_SET_ACCENT=2 : be32 0x00RRGGBB
        OP_SET_FIELD_STR=3 : u8 field_id, be16 len, bytes
        OP_LIST_APPEND=4 : u8 field_id, be16 len, bytes
        OP_LIST_CLEAR=5 : u8 field_id

Field-id convention used by eapp_render.c:
    field 0 = header / title
    field 1 = list of items (when primitive uses one)
    field 2 = footer / subtitle / body

This compiler keeps an extremely small AdaptiveCards subset: TextBlock,
Container, FactSet, Image (ignored), ColumnSet (flattened), and the
top-level AdaptiveCard's `body` array. The point isn't to ship a
production renderer — it's to PROVE the architecture by ingesting a
foreign schema and emitting our wire format.
"""
import json
import struct
import sys

# Op codes (must match include/atomik_os.h)
OP_SET_PRIMITIVE = 1
OP_SET_ACCENT    = 2
OP_SET_FIELD_STR = 3
OP_LIST_APPEND   = 4
OP_LIST_CLEAR    = 5
OP_RESET         = 6
OP_SET_NAME      = 7
OP_SET_SUBTITLE  = 8

PRIM_LIST  = 0
PRIM_CARD  = 1
PRIM_GRID  = 2
PRIM_FEED  = 3
PRIM_CONVO = 4

DELTA_LOG_MAGIC = 0x44454C54   # "DELT"
DELTA_LOG_VER   = 1


class DeltaWriter:
    def __init__(self):
        self.buf = bytearray()
        self.buf += struct.pack(">II", DELTA_LOG_MAGIC, DELTA_LOG_VER)

    def _str(self, s: str) -> bytes:
        b = s.encode("utf-8", errors="replace")[:65535]
        return struct.pack(">H", len(b)) + b

    def reset(self):           self.buf += bytes([OP_RESET])
    def name(self, s):         self.buf += bytes([OP_SET_NAME]) + self._str(s)
    def subtitle(self, s):     self.buf += bytes([OP_SET_SUBTITLE]) + self._str(s)
    def primitive(self, p):    self.buf += bytes([OP_SET_PRIMITIVE, p & 0xFF])
    def accent(self, rgb):     self.buf += bytes([OP_SET_ACCENT]) + struct.pack(">I", rgb & 0xFFFFFF)
    def field_str(self, fid, s):
        self.buf += bytes([OP_SET_FIELD_STR, fid & 0xFF]) + self._str(s)
    def list_append(self, fid, s):
        self.buf += bytes([OP_LIST_APPEND, fid & 0xFF]) + self._str(s)
    def list_clear(self, fid):
        self.buf += bytes([OP_LIST_CLEAR, fid & 0xFF])

    def bytes_(self):
        return bytes(self.buf)


# AdaptiveCards style names -> our accent palette
STYLE_TO_ACCENT = {
    "default":     0x4FC3FF,   # cyan
    "emphasis":    0x4FC3FF,   # cyan
    "good":        0x6EC46E,   # green
    "warning":     0xFFCB4A,   # amber
    "attention":   0xFF6F91,   # pink
    "accent":      0x4FC3FF,
}

def flatten_text_blocks(items, out):
    """Walk an AdaptiveCards 'body' array recursively, accumulating any
    TextBlock content as plain strings into `out`. Containers and
    ColumnSets are flattened. FactSets become 'name: value' rows."""
    for it in items or []:
        t = it.get("type", "")
        if t == "TextBlock":
            text = it.get("text", "")
            if text: out.append(text)
        elif t == "Container":
            flatten_text_blocks(it.get("items"), out)
        elif t == "ColumnSet":
            for col in it.get("columns") or []:
                flatten_text_blocks(col.get("items"), out)
        elif t == "FactSet":
            for f in it.get("facts") or []:
                title = f.get("title", "")
                value = f.get("value", "")
                out.append(f"{title}: {value}" if title else value)
        elif t == "ActionSet":
            for a in it.get("actions") or []:
                title = a.get("title", "")
                if title: out.append(f"[{title}]")
        # Image, Media, RichTextBlock etc -> ignored for v1


def pick_primitive(card):
    """Heuristic: AdaptiveCards don't have a 'primitive' field. We pick:
       - GRID if any ColumnSet exists (looks tabular)
       - LIST if many TextBlocks at the top level (>4)
       - CARD otherwise (single big record)
    """
    body = card.get("body") or []
    has_columnset = any(it.get("type") == "ColumnSet" for it in body)
    if has_columnset:
        return PRIM_GRID
    text_blocks = sum(1 for it in body if it.get("type") == "TextBlock")
    if text_blocks >= 4:
        return PRIM_LIST
    return PRIM_CARD


def compile_card(card: dict) -> bytes:
    w = DeltaWriter()
    w.reset()

    title    = ""
    subtitle = ""
    body_text_blocks = []
    body = card.get("body") or []
    # Use the FIRST TextBlock with style/size hint as the title; the
    # next as subtitle; everything else as body content.
    for it in body:
        if it.get("type") == "TextBlock":
            text = it.get("text", "")
            if not title:
                title = text; continue
            if not subtitle:
                subtitle = text; continue
            body_text_blocks.append(text)
        elif it.get("type") in ("Container", "ColumnSet", "FactSet", "ActionSet"):
            flatten_text_blocks([it], body_text_blocks)

    w.name("Imported")
    w.subtitle(card.get("speak") or "via AdaptiveCards adapter")
    style = (card.get("style") or "default").lower()
    w.accent(STYLE_TO_ACCENT.get(style, 0x4FC3FF))
    w.primitive(pick_primitive(card))

    # Field 0 = title
    if title: w.field_str(0, title)
    # Field 1 = list (clear then append)
    w.list_clear(1)
    for line in body_text_blocks:
        w.list_append(1, line)
    # Field 2 = subtitle / body summary
    if subtitle: w.field_str(2, subtitle)

    return w.bytes_()


def main():
    if len(sys.argv) < 3:
        print(__doc__); sys.exit(2)
    inp, outp = sys.argv[1], sys.argv[2]
    raw = sys.stdin.read() if inp == "-" else open(inp).read()
    data = json.loads(raw)
    blob = compile_card(data)
    with open(outp, "wb") as f:
        f.write(blob)
    print(f"wrote {outp} ({len(blob)} bytes)")
    print(f"  title={data.get('body', [{}])[0].get('text','?') if data.get('body') else '?'}")


if __name__ == "__main__":
    main()
