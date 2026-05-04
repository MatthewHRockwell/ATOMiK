# ATOMiK OS — Schema Adapters

Adapters translate between foreign generative-UI manifest formats and
ATOMiK OS's binary field-delta wire format (`delta_log.c`). They embody
the partnership pitch: don't reinvent the schema, render someone else's
output cheaply at the edge.

## What ships

| File | Direction | Status |
|------|-----------|--------|
| `adaptivecards_to_atomik.py` | AdaptiveCards JSON → ATOMiK delta-log binary | ✅ shipped |
| `atomik_to_adaptivecards.py` | ATOMiK delta-log binary → AdaptiveCards JSON | ✅ shipped |
| `example.json` | Sample AdaptiveCards card used as a smoke-test fixture | ✅ |

Together with `tools/atomik_view.py` and `tools/atomik_pull.py`, these
make ATOMiK OS a **two-way bridge** for the AdaptiveCards ecosystem.

## Round-trip example

```sh
# AdaptiveCards JSON  ->  ATOMiK deltas (320 B)
python3 adapters/adaptivecards_to_atomik.py adapters/example.json /tmp/x.deltas

# Inspect parsed state without rendering
python3 tools/atomik_view.py --dump /tmp/x.deltas

# ATOMiK deltas  ->  AdaptiveCards JSON (1.1 KB)
python3 adapters/atomik_to_adaptivecards.py /tmp/x.deltas /tmp/x.json
```

A 587-byte AdaptiveCards card compresses to 320 bytes of ATOMiK deltas,
re-expands to 1135 bytes of AdaptiveCards JSON. **Same UI, both ends.**

## AdaptiveCards subset supported

The forward adapter (`adaptivecards_to_atomik.py`) recognizes these
top-level body element types:

| Element | Coverage | Maps to |
|---------|----------|---------|
| `TextBlock` | text + size + weight + isSubtle | flowed into title/subtitle/body fields by position |
| `Container` | recursively walked | flattens into the same fields |
| `ColumnSet` | walked column-by-column | text from each column appended; presence triggers GRID primitive |
| `FactSet` | facts list | each fact becomes a `"<title>: <value>"` row |
| `ActionSet` | action titles only | each title becomes `"[<title>]"` row |
| `Image`, `Media`, `RichTextBlock` | **ignored in v0.11** | Future: map Image to a glyph slot |

## Primitive selection heuristic

The adapter picks an ATOMiK primitive based on the card's structure:

| Card structure | Chosen primitive |
|----------------|------------------|
| Any `ColumnSet` present | `PRIM_GRID` |
| 4+ top-level `TextBlock`s | `PRIM_LIST` |
| Otherwise | `PRIM_CARD` |

`PRIM_FEED` and `PRIM_CONVO` aren't auto-selected — use `/import` then
`set primitive feed` / `set primitive convo` if you want those layouts
applied to AdaptiveCards content.

## Style → accent

`card.style` (an AdaptiveCards-defined enum) maps onto our accent
palette:

| AdaptiveCards style | ATOMiK accent | Hex |
|---------------------|---------------|-----|
| `default`, `emphasis`, `accent` | cyan | `#4FC3FF` |
| `good` | green | `#6EC46E` |
| `warning` | amber | `#FFCB4A` |
| `attention` | pink | `#FF6F91` |

The reverse adapter (`atomik_to_adaptivecards.py`) inverts this map.

## Field convention (delta-log → AdaptiveCards)

The reverse adapter assumes the standard 3-field convention used by
`eapp_render.c`:

| Field id | Used as |
|----------|---------|
| `0` | header / title |
| `1` | list of items |
| `2` | footer / subtitle / body |

These get emitted as `TextBlock`s in body order, with the list rendered
either as one TextBlock per row (LIST/FEED/CONVO/GRID) or skipped in
favor of `body` text (CARD).

## Why this isn't lossy in practice

For UIs that fit our 5 primitives, the round-trip is structurally
identity. Where AdaptiveCards has richer features (action invocation,
inline images, choice sets), v0.11 ignores them rather than fakes
them. v1.0+ will add primitive-extension hooks so partner schemas
can declare "this column should be rendered as <primitive>" without
modifying the core.

## Adding new schema adapters

Follow the AdaptiveCards pattern:

1. Read the foreign schema into a Python dict / dataclass
2. Decide a primitive via a small heuristic
3. Walk the schema, emit a sequence of `delta_emit_*` opcodes
4. Either write to a file (`/tmp/foo.deltas`) or stream over UART

Each adapter is one ~150-line Python file; no on-board changes
required. New targets to consider: Vercel v0 React-tree dumps,
Anthropic Artifacts, OpenAI structured-outputs JSON. See
`docs/SCHEMA_SURVEY.md` for the comparative scan.
