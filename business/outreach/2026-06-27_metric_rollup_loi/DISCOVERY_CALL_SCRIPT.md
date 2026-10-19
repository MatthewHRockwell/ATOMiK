# 20-Minute Discovery Call — Run the Funnel Live

The site is your demo. Share your screen and walk the prospect through their own
workload on the real pages. Goal of the call: **a named workload + a stated bar +
agreement to a conditional, live-run LOI.** Not a close — a qualified yes to the
next step.

## Before the call (2 min prep)
- Pick the archetype page that matches them (`/workloads/<slug>`).
- Have the benchmark tool ready with a guess at their operator + rate.
- Know their public pain hook (the talk/post that got them on the call).

## Minute-by-minute

**0–3 · Frame + confirm the pain.**
"Before I show anything — tell me how you keep [counters/rollups] exact today.
Lock, shard, serialized reducer? Where does it hurt at peak?"
→ You're scoring signals 1–3 live. Let them describe the lock. Don't pitch yet.

**3–7 · Their archetype, their words.**
Open `/workloads/<their-slug>`. "This is the pattern as we hear it — is this you?"
Let them correct it. The 'today pain' / 'with ATOMiK' framing does the selling.

**7–11 · Their numbers, live.**
Open `/benchmark-proof`. Enter *their* operator and *their* rate. Show the verdict
compute. Then scroll to the **Recorded reference run** — "this isn't a projection,
it's read off this measured ladder; 800 M/s at 8 banks, byte-identical."
→ If they push on "is it really order-independent?": open `/formal-verification`.
"It's not just measured — it's machine-proven. 108 Lean 4 theorems, zero `sorry`,
depends only on Lean's standard axioms." This is your trump card on correctness.

**11–14 · What it's worth (their baseline).**
Open `/cost-of-lock`. Enter their shard count / instance cost / % on coordination.
"Every line is tagged — ours is measured, the savings are arithmetic on *your*
numbers. This is the lock tax order-independence removes." Let them react to a
real dollar/latency figure derived from their own inputs.

**14–18 · The de-risked offer.**
"Here's how we'd work: I run *your* exact workload on the AX7020 and hand you the
dated transcript. We draft the bar; you counter-sign a conditional LOI that only
binds if the live run hits it. If your operator isn't order-independent, I tell you
it's a no-fit. That's the design-partner program." (`/design-partners`)

**18–20 · The ask + next step.**
"Can you get me one sanitized workload spec — the operator, the rate, the
correctness bar — and the right person if it's not you? I'll come back with the
verdict and a draft conditional LOI." Book the follow-up before you hang up.

## Honesty moves (these build trust, use them)
- If they name a median/percentile/ordered-log workload: "That one's order-
  dependent — we wouldn't claim order-independence there. Honest no-fit." (Then
  ask if there's a counter/sketch workload nearby.)
- Throughput is the **engine's accumulation step**, not their end-to-end system.
  Say so before they ask. "What we remove is the coordination lock; your ingest
  path is yours."
- Never quote a speedup-vs-their-system number. You don't have it and won't fake it.

## What "good" looks like leaving the call
- ✅ A sanitized workload spec promised (operator + rate + bar).
- ✅ The workload owner identified (if it wasn't them).
- ✅ Agreement to receive a verdict + draft conditional LOI.
- ⛔ Not "good": a vague "send me the deck." Re-anchor on one workload + one bar.

## Immediately after
- Log it in `SEND_TRACKER.csv` (real status only — a call is not pipeline).
- Generate their verdict + LOI from the spec (`contract_bench` CLI: `--loi`).
- When the board is up, produce the witnessed live run (`--live-run`) and attach it.
