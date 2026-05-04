---
name: ATOMiK OS — universal Document app (chat-customizable UI)
description: One app. The document IS the data. AI chat morphs primitive + fields by natural language or speech. Replaces fixed app list.
type: project
originSessionId: 0b1da619-fb37-4461-99b3-200ab2dbce84
---
# The Document app — one app, infinite UIs

User insight (2026-05-03, immediately after the invariant-frame architecture landed): instead of shipping a fixed list of edge apps (Calendar, Tasks, Code, etc.), ship ONE universal **Document** app whose primitive and fields are mutable at runtime via chat / speech. The user describes what they want; the agent emits field-delta commands; the invariant frame renders.

This *replaces* the per-app dock list with a Document + a chat session. The chat session IS the configuration interface. Every existing demo app (Calendar, Tasks, Code, Brief, Chat) becomes "what the Document looks like in state X."

## Architecture

```
+--------------------------------------------------+
| Window: Document                                 |
|  +----------------------------------+----------+  |
|  | Document body (any primitive,    | Chat     |  |
|  | rendered by invariant frame)     | panel    |  |
|  |                                  |          |  |
|  | -> PRIM_GRID for calendar        | user> .. |  |
|  | -> PRIM_LIST for tasks           | agent> . |  |
|  | -> PRIM_FEED for activity        |          |  |
|  | -> ...                           | _        |  |
|  +----------------------------------+----------+  |
+--------------------------------------------------+
```

The chat panel takes typed (or spoken) commands. The agent parses them into field-delta operations on the Document's `edge_app_t`. The invariant-frame renderer redraws.

## Examples

| User says                                | Agent does                                 |
|------------------------------------------|--------------------------------------------|
| "show me a calendar of May"              | set primitive=GRID, fill day cells         |
| "make this a kanban for sprint 14"       | set primitive=LIST, accent=cyan, header    |
| "filter to PRs assigned to me"           | replace list[] with filtered subset        |
| "switch to chat mode"                    | set primitive=CONVO, preserve fields       |
| "show ATOMiK monitor"                    | bind to live /dev/mem read for slot data   |
| "summarize my day"                       | call brief generator, set primitive=CARD   |
| "split the screen with my tasks"         | open second Document with tasks state      |
| "make text bigger"                       | bump font scale field                      |
| "use the brand-cyan accent"              | set color field                            |

## Why this is the killer realization

1. **One binary, infinite apps.** No app store. No installer. No native code per app.
2. **Discovery is conversation, not menu hunting.** "what can this do?" → agent enumerates capabilities.
3. **Agent prediction surface is richer.** Instead of "next likely app" it predicts "next likely transformation of THIS document."
4. **Cross-device sync = stream the manifest deltas.** Same field-delta wire format we were already designing.
5. **Maps onto the delta-state algebra perfectly.** Document state = invariant_frame ⊕ Σ(chat-commanded deltas). XOR-clean. Self-inverse undo. Hardware-aligned.

## Roadmap update

Replaces v0.10 (3 demo edge apps with fake data) with v0.10 (Document app).

| Version | Scope |
|---------|-------|
| v0.9    | invariant-frame runtime + 5 reference edge apps (DONE) |
| v0.10   | **Document app**: split window with primitive renderer + chat panel; small command grammar that morphs primitive + fields. No real LLM yet — chat parses a typed mini-language. |
| v0.11   | Speech input: route an audio capture pipeline to a tiny on-device intent classifier (or stub for the demo). |
| v0.12   | Real LLM integration via API or local model. Same parse path; just smarter. |
| v0.13   | Multi-document workspace: split-screen, side-by-side, save/load named documents. |
| v1.0    | Cross-device sync: same Document on AX7020 and laptop, streaming deltas. |

## Implementation note

The first cut of v0.10 doesn't need an actual LLM. The agent's "parser" can be a small command grammar:

```
set primitive <list|card|grid|feed|convo>
set accent <hex|cyan|amber|pink|green>
set header "<text>"
set body "<text>"
clear list
add "<item>"
load <calendar|tasks|code|brief|chat>   # convenience: jump to a preset
```

That alone is enough to demonstrate the architecture on screen — and it cleanly tests the field-delta plumbing. v0.12 swaps the parser for an LLM without changing anything below it.

**This is the v1.0 demo.** Open Document. Type "show me a calendar." It becomes a calendar. Type "now show me my tasks." It becomes tasks. The audience watches the same window morph through every app type. Every existing OS makes you swap apps. ATOMiK OS makes you swap *intent*.
