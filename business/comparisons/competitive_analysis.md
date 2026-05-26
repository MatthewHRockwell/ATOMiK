# ATOMiK Competitive / Status Quo Analysis

> **Publication status: INTERNAL COMPETITIVE MEMO / REVIEW REQUIRED.**
> Use this as positioning input, not publishable competitor copy. Third-party
> claims, performance comparisons, and legal/IP statements require current source
> checking and counsel review before external use.

## Current Positioning Rule

ATOMiK should not be framed as "faster than every alternative." The safe claim is
that ATOMiK evaluates whether less state needs to move for one constrained
workload, then compares that path against the customer's current baseline.

## Status Quo Alternatives

| Alternative | What it solves | What it does not solve | Why teams choose it | Where it becomes insufficient | How ATOMiK differs | Proof required |
|---|---|---|---|---|---|---|
| More compute | Adds headroom for slow paths | May leave redundant state movement intact | Familiar procurement path | Battery, heat, size, and cost budgets tighten | Evaluates whether less state work is needed | Baseline cycles/update, latency, operations avoided |
| Bigger batteries | Extends runtime | Adds size, weight, charge time, and cost | Direct fix for field runtime | Hardware envelope or weight cannot grow | Targets the work that drains budget first | Power proxy or instrumented workload measurement |
| More cooling | Handles thermal load | Does not remove the work producing heat | Operationally straightforward in infrastructure | Fanless, sealed, remote, or dense systems lack cooling margin | Treats thermal pressure as a downstream metric | Thermal proxy or measured power path |
| More bandwidth | Moves full state faster | Keeps recurring transfer cost | Easiest network-side upgrade | Remote links, radio budgets, or cloud egress costs bind | Evaluates bytes moved and full-state transfers avoided | Bytes moved/avoided vs baseline |
| Compression | Reduces payload size | Adds encode/decode cost and may miss semantic change | Mature and cheap | State churn or latency dominates | Tracks meaningful state change before payload movement | Payload size, CPU cost, latency, correctness |
| Caching | Avoids repeated fetches | Can go stale and may not reduce update churn | Common performance pattern | Invalidations dominate or state changes frequently | Maps state transition boundaries and deltas | Cache invalidation cost, updates avoided |
| Deduplication | Removes repeated identical data | May not catch repeated reconstruction or scans | Useful for storage/network redundancy | Workload has many small state transitions | Coalesces logical changes when the model fits | Duplicate transfers avoided, operations coalesced |
| Sync protocols | Coordinate distributed state | May still move large state or metadata | Established systems practice | Metadata, replay, or full-state repair dominates | Evaluates the constrained state path underneath sync | Sync payload, replay/reconstruction cost |
| Specialized accelerators | Speeds a known kernel | Often application-specific and integration-heavy | Strong when the kernel is stable | State movement around the kernel dominates | Targets state movement as the unit of work | End-to-end workload metric, not isolated kernel metric |
| Cloud offload | Moves compute off device | Adds latency, bandwidth, privacy, and availability dependencies | Reduces local hardware burden | Link or autonomy constraints matter | Keeps evaluation centered on local state path | Local latency, bandwidth, reliability constraints |
| Overbuilt hardware | Buys schedule margin | Increases BOM, power, size, and thermal load | Fastest path to ship | Unit economics or physical envelope fail | Finds whether architecture removes overbuild pressure | BOM/thermal/power pressure tied to metric |
| Manual optimization | Squeezes known bottlenecks | Expensive, fragile, and hard to repeat | Engineers can start immediately | Diminishing returns and maintenance cost | Provides a repeatable evaluation of state movement waste | Engineering time, metric moved, correctness preserved |
| Feature cuts | Reduces workload | Reduces product value | Last resort under constraints | Customers need the feature | Evaluates whether state-aware execution keeps the feature viable | Metric threshold tied to product decision |

## Direct Competitor Language

Direct competitors and named startups should be discussed carefully. For external
materials, avoid claiming another company has no working hardware, no proof, or a
specific funding status unless that statement is source-checked immediately
before use.

Safe external contrast:

> Many alternatives buy more margin: more compute, more cooling, more bandwidth,
> bigger batteries, or more manual optimization. ATOMiK asks whether the workload
> can move less state in the first place.

Avoid:

- unverified third-party performance or funding claims
- "ATOMiK replaces CPUs/GPUs/accelerators"
- unaudited theorem counts as moat language
- universal latency, scaling, memory, power, heat, or security claims

## Buyer Proof Standard

A prospect should be persuaded only when ATOMiK improves the pre-agreed metric
against the current baseline, preserves correctness, and connects that
improvement to a business constraint worth pursuing.
