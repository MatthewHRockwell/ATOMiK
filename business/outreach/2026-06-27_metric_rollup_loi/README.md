# ATOMiK Customer / LOI Outreach — High-Cardinality Metric Rollups

**Generated:** 2026-06-27
**Vertical (chosen, single focus):** high-cardinality aggregation — counter / metric / sketch rollups
**Goal:** first design-partner conversations → live silicon runs → conditional LOIs.

This pack is a deliberate narrowing. One vertical, one sharp story, the strongest
asset out front: *we run your exact workload on real silicon and you only commit
if it hits your bar.* It supersedes the 2026-05-29 robotics/edge "state-path"
pack — that was a different (vaguer) positioning.

## Why this vertical

The website funnel converts qualified interest; it does not generate it. This
pack generates it. We lead where the story is sharpest and fully backed:

- **The pain is real and felt:** keeping per-series / per-key counters exact
  under concurrent, out-of-order arrival forces a lock or a sharded hot path,
  which caps throughput exactly at peak load.
- **The differentiator is unique:** ATOMiK merges those counters
  order-independently and **lock-free**, **byte-identical** across parallel
  units. For abelian aggregations (sum, count, xor, or, min, max, HLL merge)
  this is architectural, not a tuning trick.
- **It's both measured and proven:** 800 M accumulations/s at 8 banks,
  byte-identical across 1/2/4/8 banks, measured on an AX7020 (XC7Z020) at
  100 MHz — *and* the order-independence is formally verified in Lean 4
  (108 theorems, zero `sorry`). No competitor leads with "and here's the proof."

## The offer (lead with this)

> "I'll run your exact rollup on our silicon and only ask you to commit to a
> pilot **if** it hits your bar. Conditional LOI — you sign nothing until it
> passes. If your operator isn't order-independent, I'll tell you it's a no-fit."

This is the design-partner program: `atomik.tech/design-partners`. It de-risks
the ask to almost zero, which is the whole point for a first LOI.

## Files

- `ICP_AND_SCORING.md` — who qualifies, the signals that find them, segment +
  example-company map, and how to reach the right person. (No fabricated
  contacts — signals and routes, not a fake CRM.)
- `EMAIL_TEMPLATES.md` — cold first-touch per sub-segment + follow-up sequence,
  each leading with the live-run/LOI offer and linking the matching funnel page.
- `DISCOVERY_CALL_SCRIPT.md` — the 20-minute call, question-by-question, mapped
  to the funnel pages you run live on the call.
- `ONE_PAGER.md` — the single-page leave-behind (proof + value + conditional LOI).
- `SEND_TRACKER.csv` — blank tracker.

## How to use it

1. **Build the list first** (ICP_AND_SCORING.md). 25–30 named accounts in this
   one vertical where you can identify a real workload match. Quality over volume.
2. **Find the workload owner**, not a VP — the engineer who owns the counter hot
   path / the ingestion-aggregation pipeline.
3. **Send 8–12 personalized first**, not all 30. Reference *their* specific pain
   (a talk, a blog post, a GitHub issue about sharding/contention).
4. **Goal of touch #1 is a 20-minute call**, not an LOI. The LOI ask comes after
   they acknowledge a real workload (DISCOVERY_CALL_SCRIPT.md).
5. **On the call, run the funnel live**: `/workloads` → their archetype →
   `/benchmark-proof` with their numbers → `/formal-verification` if they push on
   correctness → `/cost-of-lock` for the value → the conditional LOI.

## ✅ Live-run path is hardware-validated (2026-06-28)

`run_live()` was fired end-to-end on the AX7020 on 2026-06-28: the harness drove
`abench` over the board console and produced a witnessed record with the verbatim
transcript — `0x17e9fe29b5dc0aad` byte-identical across 1/2/4/8 banks, 800 M/s at
8 banks, reproducing the recorded reference exactly. Sample artifact (record +
LOI with the live run embedded) in `witnessed_run_2026-06-28/`.

So the offer is real and deliverable. Two operational notes for prospect runs:
- Console port is **/dev/ttyUSB1** in the current setup (the harness default is
  `/dev/ttyUSB2`; pass `--port /dev/ttyUSB1`).
- The board does not auto-boot Linux from cold; if it's powered down you need a
  boot session before a call. Keep it warm before a scheduled live run, or
  generate the record ahead of time and hand it over.

## Claim guardrails (hard rules — every word in this pack obeys them)

**Safe to say (all measured or proven):**
- 800 M accumulations/s at 8 banks; byte-identical across 1/2/4/8 banks;
  measured on AX7020 (XC7Z020) at 100 MHz. Reproduce: `abench 65537 0xDEADBEEF0BADF00D`.
- Order-independence formally verified in Lean 4 — 108 theorems, zero `sorry`,
  depends only on Lean's standard axioms.
- Lock-free / no serialization **for abelian aggregations only**.
- We evaluate fit and will return an honest no-fit.

**Never say in outreach:**
- "X% faster than your system" / any universal or end-to-end speedup.
- Any traffic-reduction multiplier (e.g. the 7,670×–916,000× range — no current artifact).
- Production-ready, signed design partners, customer traction, named customers.
- Guaranteed cost/latency savings — those are workload-specific and computed from
  the prospect's own baseline (the value memo), never asserted.

Do not describe any target as pipeline / customer / design partner / LOI until
there is a confirmed meeting, written interest, or a signed instrument.
