# ATOMiK Website Messaging Framework

## 1. One sentence

ATOMiK is a state-aware compute architecture that helps constrained edge and embedded teams do less unnecessary state work by tracking meaningful changes instead of repeatedly moving, scanning, syncing, replaying, or rebuilding full state.

## 1A. Buyer Narrative

The strongest buyer story is not "new computing architecture" first. It is "we are paying for unnecessary work" first. Lead with the paid pain: battery budget, heat, bandwidth, latency, reliability, and hardware footprint. Explain the architecture only after the prospect understands the waste ATOMiK is evaluating.

## 1B. How ATOMiK Works

CEO-safe answer: traditional systems often keep moving or rechecking whole state even when only a small part changed. ATOMiK starts from a known state, records the meaningful changes, and rebuilds only the state needed when it is needed. In the right workload, that can reduce unnecessary data movement and repeated work, which may improve performance or reduce power, heat, and bandwidth pressure.

Technical answer: LOAD sets a reference state, ACCUM XORs each new delta into an accumulator, READ reconstructs current state as reference XOR accumulator, and SWAP checkpoints the result. Because XOR deltas are commutative and associative, multiple logical updates can be batched or coalesced before they are applied; because they are self-inverse, duplicate or undo-style operations can be handled algebraically.

## 2. X/Y/Z/A positioning

ATOMiK is a state-aware compute architecture that helps constrained edge and embedded teams do less unnecessary state work by tracking meaningful changes instead of repeatedly moving, scanning, syncing, replaying, or rebuilding full state.

Give us one state-heavy workload, your current baseline, and the constraint that already hurts.
We will evaluate where state movement creates waste and whether ATOMiK can improve the path.
You will receive a workload map, baseline comparison, evidence map, fit/no-fit recommendation, and next-step plan.
Success looks like a measured improvement against one agreed metric while preserving correctness and showing enough economic or technical value to justify a design-partner evaluation.

## 3. Primary ICP

The first ICP is edge and embedded teams that can provide one representative state-heavy workload, one current baseline, and one painful constraint expensive enough to evaluate. Strong categories include AI at the edge, robotics, industrial systems, IoT, remote or field systems, defense-adjacent systems, and hardware-constrained teams where battery, heat, bandwidth, latency, size, weight, reliability, or cost is already measurable.

Direct first-order pains: battery or power budget, heat in sealed or fanless systems, bandwidth pressure, update/reconstruction latency, reliability, field-service, size, weight, and hardware-footprint pressure.

Derived or expansion pains: data-center power bills, cooling and water pressure, sustainability reporting, rack density, and infrastructure overbuild. Treat these as evaluation targets unless measured for the specific workload.

## 4. Evaluation offer

The offer is not generic product access. It is proof review, workload mapping, technical evaluation, design-partner evaluation, licensing/IP diligence, or investor diligence depending on fit.

The default customer offer is: bring one state-heavy workload, the current baseline, and the constraint that already hurts. ATOMiK maps wasted state movement, defines success criteria, and recommends the next step or a no-fit outcome.

## 5. Customer intake package

The customer should bring as many of the following as possible:

- Representative workload
- Current implementation or pseudocode
- State model, state size, and update frequency
- Current sync, replay, scan, reconstruction, or state movement path
- Current baseline measurements
- Target hardware or deployment environment
- Painful constraint: battery, heat, bandwidth, latency, footprint, weight, reliability, cost, or compute density
- Current workaround
- Decision threshold for continuing
- Available traces, logs, counters, power data, latency data, bandwidth data, or thermal data

Customers do not need to expose the entire product. They need enough about one constrained state path to evaluate whether ATOMiK is relevant.

## 6. Metrics matrix

| Metric | When it matters | What the customer provides | How ATOMiK evaluates it | Meaningful result | Public safety |
|---|---|---|---|---|---|
| Bytes moved | Bandwidth-constrained links, radio duty cycle, sync-heavy paths | Transfer volume, state size, update cadence | Compare full-state movement to deltas | Fewer bytes moved for same correct outcome | Homepage-safe as target |
| Bytes avoided | Repeated sync, telemetry, agent context, remote systems | Payloads, traces, last-shipped state | Estimate or measure avoided transfers | Avoided bytes tied to cost, latency, power, or bandwidth | Proof-page-safe with artifact |
| Full-state transfers avoided | Snapshot or full-state sync paths | Snapshot frequency, object size, transport path | Identify where deltas replace snapshots | Fewer full transfers with correctness | Homepage-safe as target |
| State scans avoided | Change detection paths | Scan logic, region count, region size, scan cadence | Map scans to tracked regions | Fewer or cheaper scans | Proof-page-safe with workload context |
| Replay/reconstruction cost | Log replay, recovery, state rebuild | Replay path, event volume, reconstruction frequency | Measure avoided reconstruction work | Lower reconstruction cost with same final state | Proof-page-safe with artifact |
| Operations coalesced | Repeated updates to same regions | Operation trace or generator | Compare raw operations to unique-region operations | Fewer emitted operations | Homepage-safe as target |
| Unique-region ratio | Workloads with many operations hitting few regions | Region model, trace, update distribution | Touched regions divided by operations | Low ratio signals coalescing potential | Proof-page-safe with caveat |
| Cycles per update | Embedded processors and local update loops | Cycle counter baseline, target hardware | Measure software, direct hardware, batched, profiled paths | Lower cycles/update | Proof-page-safe with artifact |
| Update latency | Control loops and local state machines | Latency target, timing method | Measure change-to-usable-state time | Lower update latency | Proof-page-safe with artifact |
| Response latency | AI-at-edge, robotics, user-facing embedded systems | End-to-end path and target | Isolate state movement on response path | Lower response latency if state movement dominates | Diligence-only until measured |
| Duty cycle | Battery-limited devices | Wake/sleep pattern and power budget | Estimate active-time impact | Reduced active duty cycle with measurement | Diligence-only unless artifact-backed |
| Wake-up frequency | Radio-heavy IoT and remote deployments | Wake events and sync schedule | Find unnecessary wake triggers | Fewer wake-ups tied to budget | Diligence-only unless artifact-backed |
| Memory/state footprint | Small devices and constrained memory | State layout and memory budget | Map state, deltas, and reconstruction needs | Lower footprint pressure | Proof-page-safe with artifact |
| Power proxy | Battery and thermal pretests | Cycles, bytes moved, active time, model | Use proxies until instrumented power exists | Proxy improves enough for power testing | Diligence-only unless labeled |
| Thermal proxy | Enclosures, dense racks, fanless devices | Utilization, cycles, transfer volume, limit | Use proxies to plan thermal measurement | Proxy reduction supports thermal test | Diligence-only unless measured |
| Bandwidth pressure | Expensive, intermittent, or tactical links | Baseline bandwidth and link budget | Compare baseline pressure to state-aware path | Less pressure on constrained link | Homepage-safe as target |
| Correctness preservation | Every evaluation | Expected outputs, invariants, test oracle | Verify optimized path preserves state | Same correct state or accepted behavior | Homepage-safe |
| Target business outcome | Continuation decisions | Threshold, budget owner, value model | Connect measured improvement to paid pain | Enough value to continue | Diligence-only for customer numbers |

## 7. Success definition

ATOMiK is successful for a customer evaluation when it improves the pre-agreed metric against the current baseline, preserves correctness, and connects that improvement to a business constraint worth pursuing.

Success examples include fewer bytes moved, fewer full-state transfers, lower update/reconstruction latency, fewer operations after coalescing, lower power or thermal proxy where responsibly measured, and clear fit/no-fit evidence.

Success does not mean universal speedup, guaranteed battery extension, guaranteed heat reduction, guaranteed water savings, guaranteed smaller hardware, generic better compute, or proof from unrelated workloads.

## 8. Buyer journey

1. First response and qualification: confirm one measurable constraint and route to proof review, technical evaluation, licensing, investor diligence, or no-fit.
2. Intake call: anchor on one workload, baseline, constraint, metric, and NDA need.
3. Customer evaluation package: collect workload, baseline, traces, target environment, and threshold.
4. State-movement map: identify full-state movement, scans, replays, reconstruction, deltas, coalescing, and transition boundaries.
5. Success criteria: define baseline, method, metric, threshold, evidence tier, and supported decision.
6. Evaluation path: choose proof review, software exploration, benchmark exchange, workload mapping, hardware-backed demo, technical evaluation, design-partner evaluation, licensing/IP diligence, or investor diligence.
7. Readout: deliver workload map, baseline comparison, evidence map, results or plan, recommendation, risks, and boundaries.
8. Final outcome: no fit, proof review complete, technical evaluation recommended, design partnership recommended, licensing/IP diligence recommended, or investor diligence recommended.

## 9. Proof strategy

Current proof to show today:

- Public proof packet
- Evidence-labeling framework
- Claims registry
- Live Zynq prototype evidence where appropriate
- Linux userspace to FPGA validation
- AX7020 board-run performance matrix
- Formal proof work, without unaudited proof counts
- Synthesis-validated artifacts where available
- Roadmap/concept materials clearly labeled as roadmap or conceptual

The AX7020 interpretation must be quoted honestly: ATOMiK can win in specific coalesced/batched scenarios and lose in others. The architecture is evaluated workload by workload and is strongest where state movement, repeated scans, full-state sync, replay, or reconstruction dominate the cost.

## 10. Competition and status quo alternatives

Customers often choose more compute, bigger batteries, more cooling, more bandwidth, compression, caching, deduplication, sync protocols, specialized accelerators, cloud offload, overbuilt hardware, manual optimization, or reducing feature scope.

ATOMiK differs by testing whether the paid pain comes from wasted state movement before adding more capacity or removing features. Persuasion requires workload-specific proof against the prospect's current baseline, not generic benchmark claims.

## 11. Language to use

Use: targets, evaluates, may reduce, designed to reduce, maps, measures, helps identify, workload-specific, evidence-bound, evaluation target, fit/no-fit, current baseline, preserves correctness, public proof packet, evidence tier.

Word-choice rule: use "does" only for artifact-backed facts, "can" for workload-specific capability ATOMiK is prepared to evaluate, and "may" for downstream business outcomes such as battery, thermal, cooling, water, and footprint.

## 12. Language to avoid

Avoid: guaranteed savings, universal speedup, production-ready unless directly proven, proven heat reduction for all workloads, proven battery improvement for all devices, water savings without artifact, smaller hardware without workload evidence, future of compute as a generic claim, revolutionary, always faster, replaces all compute, concept visuals as proof.
