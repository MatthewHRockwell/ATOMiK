# ATOMiK Memory Traffic Reduction - Historical Analysis Status

> Current diligence status: INTERNAL / RE-MEASUREMENT REQUIRED. Do not quote
> historical memory-traffic ratios, speedups, power, heat, water, or battery
> implications externally unless the raw artifact, methodology, interpretation,
> and evidence label are attached.

## Why This File Changed

Earlier drafts contained specific memory-traffic and throughput figures from
historical experiments. Those figures may still be useful for internal
engineering review, but they should not be used in the Aggie Angel pitch or sent
as investor proof until they are reconciled against the current claims registry
and reproduced with a clean artifact package.

## Current Investor-Safe Position

ATOMiK targets workloads where redundant state movement is expensive. The
customer-value hypothesis is that eligible workloads may benefit through:

- fewer bytes moved
- lower power draw
- lower thermal output
- lower bandwidth pressure
- faster local state handling
- smaller hardware or cooling budgets

These are evaluation targets, not current public results, unless a current
artifact proves the exact claim.

## Required Evidence Package Before Reuse

A refreshed memory-traffic claim must include:

| Requirement | Why it matters |
|---|---|
| Workload definition | Investors need to know what was actually measured. |
| Baseline implementation | The comparison must be fair and reproducible. |
| ATOMiK implementation | The tested path must be clear: software, FPGA, Zynq, or synthesis. |
| Raw output | Claims must trace to recorded artifacts. |
| Interpretation note | Caveats, limits, and failure modes must be explicit. |
| Evidence label | `LIVE_MEASURED`, `HARDWARE_VALIDATED`, `SYNTHESIS_VALIDATED`, or `PROJECTED`. |
| Date and commit | The result must be tied to a specific code state. |

## Recommended Next Measurement Targets

For the Friday investor narrative, prioritize measurements that map directly to
customer value:

1. Bytes moved avoided on one state-heavy workload.
2. Power draw difference on the board or evaluation platform.
3. Thermal trend under a repeatable workload.
4. Latency delta for update-heavy state handling.
5. Bandwidth avoided across a constrained link or replica path.

## Public Wording Until Refreshed

Safe:

> ATOMiK targets wasted state movement and is being evaluated for workloads
> where fewer state transfers could reduce heat, power, bandwidth, latency, or
> hardware footprint.

Avoid:

- exact memory-traffic ratios from historical drafts
- exact throughput numbers without current source artifacts
- any claim that heat, water, battery, or footprint savings have already been
  measured end to end
