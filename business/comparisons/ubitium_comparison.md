# ATOMiK vs. Universal-Processor Narratives

> **Publication status: INTERNAL COMPETITIVE MEMO / REVIEW REQUIRED.**
> This memo is safe as positioning guidance only. Do not publish named-company
> claims without current source checks and counsel review.

## Core Difference

Universal-processor narratives usually argue that one chip can run many workload
types. ATOMiK is narrower: it evaluates whether a constrained state path can do
less unnecessary state work by tracking meaningful change.

ATOMiK should be framed as complementary to processors and accelerators, not as a
blanket replacement.

## Comparison Framework

| Dimension | Universal processor thesis | ATOMiK thesis | Proof needed |
|---|---|---|---|
| Commercial promise | Consolidate compute types | Reduce wasted state movement in fit workloads | Customer workload baseline and fit/no-fit map |
| Buyer wedge | Platform replacement | Evaluation of one constrained state path | One workload, one metric, one decision threshold |
| Risk | Broad workload coverage and ecosystem adoption | Workload fit and integration path | Evidence map and claim boundaries |
| Integration | Potentially replaces existing processor choice | May sit beside existing CPU/GPU/accelerator paths | Architecture-specific integration plan |
| Evidence posture | Depends on company-specific artifacts | Label each claim: live measured, hardware validated, software validated, synthesis validated, build artifact, projected, conceptual, roadmap | Artifact-linked proof packet |

## Safe Investor Framing

> Some companies try to make one processor do everything. ATOMiK takes a more
> focused path: find one painful constrained state path, measure where state is
> being moved or rebuilt unnecessarily, and decide with evidence whether a
> state-aware architecture belongs in that system.

## What Not To Say

- Do not say ATOMiK replaces universal processors, CPUs, GPUs, DSPs, FPGAs, or accelerators.
- Do not say a named competitor lacks hardware or proof unless source-checked the same day.
- Do not use unaudited theorem counts as competitive moat language.
- Do not present synthesis results as production silicon.
- Do not claim guaranteed power, heat, battery, cooling, water, or footprint improvements.

## Proof Standard

The useful comparison is not "which architecture sounds bigger." It is whether
ATOMiK improves a customer's pre-agreed metric against their baseline while
preserving correctness.
