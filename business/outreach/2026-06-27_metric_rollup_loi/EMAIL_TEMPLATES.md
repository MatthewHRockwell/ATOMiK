# Cold Outreach Templates — Metric / Counter Rollups

Rules: under ~110 words. One specific hook about *their* pain. One differentiator.
The de-risked offer. A soft ask. One link (the matching archetype page). No
attachments on touch #1. Replace every `[bracket]`. A named, personalized message
beats a template 10:1 — these are scaffolds, not send-as-is.

The spine of every message:
**their pain → lock-free + byte-identical + proven → "I'll run yours, you only commit if it passes" → 20 min?**

---

## A. Metrics / observability infrastructure

**Subject:** lock-free metric rollups — proven on silicon, run on your workload

Hi [name],

[Saw your KubeCon talk on sharding the counter write path / read your post on
high-cardinality rollups]. The reason that path shards is that keeping per-series
counts exact under concurrent, out-of-order arrival needs a lock or a hot-shard —
which caps throughput right when load spikes.

ATOMiK merges those counters **order-independently and lock-free, byte-identical**
across parallel units — measured at 800 M accumulations/s on a $200 FPGA, and the
order-independence is **formally proven in Lean 4**, not just benchmarked.

I'll run your exact rollup on the board and only ask you to commit to a pilot
**if** it hits your bar — conditional LOI, you sign nothing until it passes.

Worth 20 minutes? atomik.tech/workloads/observability-metrics-rollup

— Matthew Rockwell, ATOMiK

---

## B. Ad-tech (exchange / DSP / SSP)

**Subject:** exact per-campaign counts at serve rate — without the lock

Hi [name],

Per-campaign impression/click/spend counters have to be exact for pacing and
frequency capping — but keeping them exact across concurrent serve nodes means a
per-campaign lock/CAS, or eating overspend from stale counts. Hot campaigns
contend exactly at peak traffic.

ATOMiK accumulates those counters **lock-free and byte-identical** regardless of
which node served first — order-independence that's measured on silicon (800 M
accumulations/s) and **formally proven in Lean 4**.

I'll run your counter workload live on our hardware and only ask for a commitment
**if** it clears your bar — a conditional LOI. No-fit if it doesn't.

20 minutes this week? atomik.tech/workloads/adtech-impression-aggregation

— Matthew Rockwell, ATOMiK

---

## C. Fraud / risk / rate-limiting

**Subject:** exact per-entity counters under attack traffic, lock-free

Hi [name],

Per-IP/user/card counters drive allow/deny/challenge, so they have to be exact —
but exact counts under concurrent edge updates mean a distributed lock or
serialized hot path, and hot keys (a botnet IP) contend precisely when the
decision needs to be fastest.

ATOMiK merges those counters **order-independently, lock-free, byte-identical** —
800 M accumulations/s measured on a $200 FPGA, with the correctness **formally
verified in Lean 4**.

I'll run your exact counter path on the board and only ask you to commit **if** it
hits your bar — conditional LOI, nothing signed until it passes.

Open to 20 minutes? atomik.tech/workloads/realtime-fraud-counters

— Matthew Rockwell, ATOMiK

---

## D. Generic / short (founder or unknown owner)

**Subject:** run your aggregation on our silicon — commit only if it passes

Hi [name],

If [company] keeps counters, sums, or sketches exact under concurrent, out-of-order
load, you're probably paying for it with a lock, a sharded hot path, or a
serialized reducer.

ATOMiK does that merge **lock-free and byte-identical** for any order-independent
aggregation — measured at 800 M/s on a $200 FPGA and **formally proven in Lean 4**.

The offer is simple: I run your workload live on our hardware, and you only sign a
(conditional) LOI **if** it hits your bar. Honest no-fit otherwise.

Who owns your aggregation path — and could we grab 20 minutes?

atomik.tech/design-partners — Matthew Rockwell, ATOMiK

---

## Follow-up sequence (no response)

Send from the same thread. Stop after #3 if silent — silence is a no, move on.

**Follow-up 1 (≈ day 4):** one new proof point.
> Quick follow-up — the thing that usually surprises people: the correctness
> isn't just measured, it's machine-checked. 108 Lean 4 theorems, zero `sorry`,
> depending only on Lean's standard axioms: atomik.tech/formal-verification.
> Still happy to run your workload live. 20 min?

**Follow-up 2 (≈ day 10):** make it concrete with the value angle.
> Last note from me — built a quick way to see what the lock actually costs you,
> from your own numbers: atomik.tech/cost-of-lock. If counters aren't a pain worth
> 20 minutes right now, just say so and I'll close the loop.

**Follow-up 3 (≈ day 18):** the breakup.
> I'll assume the timing's off and stop here. If a hot-key / cardinality counter
> problem shows up later, the standing offer holds: I run it live, you commit only
> if it passes. — Matthew

## After a positive reply

- Don't send the deck. Send the **one-pager** (`ONE_PAGER.md`) or a single
  archetype link, and book the 20-minute call.
- Bring the call to the funnel (`DISCOVERY_CALL_SCRIPT.md`). The LOI ask comes
  only after they name a real workload and bar.
