---
name: ATOMiK OS — token-pay AI-app economics
description: Pay per token for AI-mediated work, not per-month for subscriptions. Local primitives free; cloud LLM opt-in metered with cost preview.
type: project
originSessionId: 0b1da619-fb37-4461-99b3-200ab2dbce84
---
The OS's pricing model is **token-pay**, not subscription. Confirmed by the user 2026-05-03:

> "this creates a AI specific (you can choose different AI APIs depending on task) and instead of paying traditional subscription fees you pay in tokens for what you use when you use it. So imagine creating a document but only paying (plus the upcharge) for the tokens used to do minimal work compared to a conventional computational heavy app"

## Core mechanics

1. **Pluggable AI providers.** Each provider has a (name, base URL, model, token-cost map). User configures per-task routing — "rewrite this paragraph" goes to a cheap fast model; "summarize my whole inbox" goes to the bigger one.

2. **Per-action cost preview.** Before any LLM call, the user sees "this will cost ~N tokens (~$X). [Confirm] [Cancel]." Audit log persists every spend.

3. **Local primitives are free forever.** Markov agent, dock sort, hand-rolled parser, persistence, framebuffer — all run on-device with zero token cost. The cheapest path is the most private path; not coincidental — it's the architecture.

4. **Margin via small markup.** ATOMiK OS marks raw provider cost (e.g. 25%) and pockets the spread. BYO-API-key option bypasses markup but a flat platform fee for the OS itself remains (similar to phone-carrier device-vs-service split).

## Why this is structurally cheaper than subscription SaaS

A subscription Calendar at $5/mo charges the same whether the user opened it once or 1000 times. The vendor prices for the average; light users overpay so heavy users underpay. Token-pay flips this:
- Light user (twice/week): ~$0.30/mo. Saves $4.70.
- Heavy user (live transcript summarization): $30/mo. Pays for what they got, gets more than the SaaS allows.

Both groups win because the "subsidize the heavy users" contract is eliminated.

## Why field-delta architecture makes token-pricing uniquely possible

1. UIs are tiny (manifest + ~bytes of deltas), not framebuffers/MB-streams. The agent's interpretation IS the cost; the rendering is free.
2. Accumulator survives — once a Document is morphed into a calendar, you don't re-pay every frame. You pay when intent changes.
3. AI provider routing is a manifest parameter, not bound at compile-time.

## Roadmap landing points

| Version | What lands |
|---------|------------|
| v0.12 | AI provider abstraction + token meter + cost preview |
| v0.15 | Local user profile + token wallet + daily/monthly cap |
| v1.0  | First real cloud service shipped (e.g. Google Calendar) |

## How to apply

When designing v0.12 onward, every LLM-touching code path must:
1. Fire through the provider abstraction (no hardcoded API URLs)
2. Surface the projected cost BEFORE the request
3. Log the actual cost AFTER the request to the audit trail
4. Allow the user to cancel mid-stream

Public-facing copy should always lead with "pay for what you use," NOT with a feature list. The pricing model IS the feature.

Full doc: `atomik_os/docs/BUSINESS_MODEL.md`.
