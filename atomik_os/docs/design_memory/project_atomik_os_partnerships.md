---
name: ATOMiK OS — generative-UI partnership strategy
description: Don't invent the manifest schema. Render someone else's. Position as the substrate that makes their generated UIs cheap to deliver at the edge.
type: project
originSessionId: 0b1da619-fb37-4461-99b3-200ab2dbce84
---
The user surfaced this 2026-05-04 (cross-session: came from a Claude Code conversation he was running in parallel). The strategic instinct:

**Don't reinvent the manifest schema. Adopt or align with an existing generative-UI standard. Position ATOMiK OS as the canonical renderer for their output.**

## Players worth a conversation

### Direct architectural fit (closest to invariant-frame architecture)
- **Vercel v0** — text prompt → React/HTML UI. Their output gets massively compressed when reduced to our field-delta primitives.
- **Thesys / GenUI** — explicitly "generative UI as a service." API output is JSON UI specs, one transform away from our field-delta wire format.
- **Tempo Labs / GPT Engineer / Lovable** — same category, same fit.

### Adjacent, also worth talking to
- **Anthropic's "computer use" / Artifacts model** — Claude already emits structured UI hints. We could be the canonical edge renderer for those hints, zero-token scan-out cost.
- **OpenAI structured outputs / function-calling UIs** — same shape.
- **Microsoft AdaptiveCards** — older but well-thought-through JSON UI schema. Our 5 primitives map onto a subset cleanly. **Most mature interop candidate.**
- **Tonk / Arcade.dev / Block's open agentic stack** — agent frameworks looking for a UI layer.

## The pitch (uniform across partners)

> Your model outputs UI intent. Today that intent gets re-rendered as a full React tree on every device. On ATOMiK OS the same intent is a few hundred bytes of typed field deltas applied to a compiled invariant frame, runs on a $200 board, costs zero scan-out compute. You make the intent; we make it cheap to deliver.

**Why this is a partnership, not competition:** they produce UI specs at the model layer; we render UI specs at the OS layer. Their cost-per-render on a phone/browser is dominated by JS evaluation + DOM diffing. Ours is a memcpy.

The same shape applies to the parked dynamic-vproc idea: someone else has the workload (gen-UI, agent inference, edge ML); we have the substrate that makes it cheap. **Partnership-first, not OS-as-monolith.**

## Concrete near-term moves

1. **Adopt or align with one existing manifest schema** instead of inventing ours from scratch. AdaptiveCards is the most mature; Thesys is the most LLM-native. Our `edge_app_t` becomes a renderer for that schema — instant interop with anyone already using it.
2. **Land one design-partner conversation before v1.0 is locked.** Even a single chat with Vercel v0 or Thesys clarifies whether our manifest schema should be theirs.
3. **Do NOT rip out what we built.** The invariant frame + 5 primitives are correct. The only thing potentially negotiable is the manifest *format* (the front-end of `edge_app_t`).

## Architectural implication

The current `edge_app_t` + `delta_log.c` wire format is internal. We can keep it as the canonical on-the-wire encoding (it's tiny, ATOMiK-aligned) AND offer adapters that ingest:
- AdaptiveCards JSON → eapp_t
- Thesys schema → eapp_t
- Vercel v0 React tree subset → eapp_t

Each adapter is a small file that walks the foreign schema and emits `delta_emit_*()` calls. Done.

## How to apply

When designing v1.0+, every new manifest feature should check:
1. Is there a standard schema that already has this field?
2. If yes, can we map our internal type onto theirs?
3. If we ship a "new" field, is it because no standard covers it, or because we got tunnel vision?

When pitching, lead with **"render layer for your generated UI"** — not "another OS." We're not competing with macOS or Windows for app developers; we're competing with Electron / React Native / Flutter for **render bandwidth + compute cost**, which we win 10–1000× on by virtue of the architecture.
