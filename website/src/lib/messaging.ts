export const positioningStatement =
  "ATOMiK is a state-aware compute architecture that helps edge and embedded teams reduce wasted state movement by tracking meaningful change instead of repeatedly moving, scanning, syncing, replaying, or rebuilding full state.";

export const offerFormula = {
  give:
    "Give us one state-heavy workload, your current baseline, and the constraint that already hurts.",
  evaluate:
    "We will evaluate where state movement creates waste and whether ATOMiK can improve the path.",
  receive:
    "You will receive a workload map, baseline comparison, evidence map, fit/no-fit recommendation, and next-step plan.",
  success:
    "Success looks like a measured improvement against one agreed metric while preserving correctness and showing enough economic or technical value to justify a design-partner evaluation.",
};

export const primaryIcp =
  "Edge and embedded teams that can provide one representative state-heavy workload, one current baseline, and one painful constraint expensive enough to evaluate.";

export const intakeItems = [
  "Representative workload",
  "Current implementation or pseudocode",
  "State model, state size, and update frequency",
  "Current data or state movement path",
  "Current baseline measurements",
  "Painful constraint: battery, heat, bandwidth, latency, footprint, weight, reliability, cost, or compute density",
  "Target hardware or deployment environment",
  "Decision metric and threshold for continuing",
  "Available traces, logs, counters, power data, latency data, bandwidth data, or thermal data",
];

export const evaluationProcess = [
  {
    step: "1",
    title: "First response and qualification",
    body:
      "Confirm one measurable constraint, confirm the workload is state-heavy, and route to proof review, technical evaluation, licensing, investor diligence, or no-fit.",
  },
  {
    step: "2",
    title: "Intake call",
    body:
      "Anchor on one workload, identify the current baseline and painful constraint, choose one primary success metric, and decide whether an NDA is needed.",
  },
  {
    step: "3",
    title: "Evaluation package",
    body:
      "The customer provides enough about one constrained state path to evaluate relevance without exposing the entire product.",
  },
  {
    step: "4",
    title: "State-movement map",
    body:
      "ATOMiK looks for full-state movement, repeated scans, repeated replays, avoidable reconstruction, and opportunities for deltas, coalescing, local reconstruction, or cleaner state-transition boundaries.",
  },
  {
    step: "5",
    title: "Success criteria",
    body:
      "Before benchmark or prototype claims, define the baseline, measurement method, success metric, threshold, evidence tier, and decision the result will support.",
  },
  {
    step: "6",
    title: "Evaluation path",
    body:
      "Pick the right path: proof review, software exploration, benchmark exchange, workload mapping, hardware-backed demo, technical evaluation, design-partner evaluation, licensing/IP diligence, or investor diligence.",
  },
  {
    step: "7",
    title: "Readout",
    body:
      "Deliver the workload map, baseline comparison, evidence/proof map, results or evaluation plan, fit/no-fit recommendation, risks, claim boundaries, and next-step recommendation.",
  },
  {
    step: "8",
    title: "Final outcome",
    body:
      "The result can be no fit, proof review complete, technical evaluation recommended, design partnership recommended, licensing/IP diligence recommended, or investor diligence recommended.",
  },
];

export const metricMatrix = [
  {
    metric: "Bytes moved",
    matters: "Bandwidth-constrained links, radio duty cycle, sync-heavy paths",
    customerProvides: "Current transfer volume, state size, update cadence",
    atomikEvaluates: "Compare full-state movement to meaningful state deltas",
    meaningful: "Fewer bytes moved for the same correct outcome",
    whereSafe: "Homepage-safe as an evaluation target",
  },
  {
    metric: "Bytes avoided",
    matters: "Repeated sync, telemetry, agent context, remote systems",
    customerProvides: "Baseline payloads, repeat/update traces, last-shipped state if available",
    atomikEvaluates: "Estimate or measure avoided transfers after coalescing and skip rules",
    meaningful: "Avoided bytes tied to bandwidth, cost, latency, or power budget",
    whereSafe: "Proof-page-safe with artifact",
  },
  {
    metric: "Full-state transfers avoided",
    matters: "Systems that repeatedly send snapshots or rebuild full state",
    customerProvides: "Snapshot frequency, object size, transport path",
    atomikEvaluates: "Identify where deltas can replace full-state movement",
    meaningful: "Fewer full transfers without losing correctness",
    whereSafe: "Homepage-safe as an evaluation target",
  },
  {
    metric: "State scans avoided",
    matters: "Change detection that repeatedly asks what changed",
    customerProvides: "Scan logic, region count, region size, scan cadence",
    atomikEvaluates: "Map scans to tracked regions and update boundaries",
    meaningful: "Fewer or cheaper scans against the same state path",
    whereSafe: "Proof-page-safe with workload context",
  },
  {
    metric: "Replay or reconstruction cost",
    matters: "Log replay, local recovery, state rebuild, checkpoint restore",
    customerProvides: "Replay path, event volume, reconstruction frequency",
    atomikEvaluates: "Measure work avoided by preserving meaningful transition state",
    meaningful: "Lower reconstruction cost while preserving final state",
    whereSafe: "Proof-page-safe with artifact",
  },
  {
    metric: "Operations coalesced",
    matters: "Change-heavy workloads with repeated updates to the same regions",
    customerProvides: "Operation trace or representative generator",
    atomikEvaluates: "Compare raw operations to unique state-region operations",
    meaningful: "Fewer emitted operations for equivalent state",
    whereSafe: "Homepage-safe as an evaluation target",
  },
  {
    metric: "Unique-region ratio",
    matters: "Workloads where many logical operations hit fewer state regions",
    customerProvides: "Region model, trace, update distribution",
    atomikEvaluates: "Measure touched regions divided by total logical operations",
    meaningful: "Low ratio signals stronger coalescing potential",
    whereSafe: "Proof-page-safe with caveat",
  },
  {
    metric: "Cycles per update",
    matters: "Embedded processors, MMIO paths, local update loops",
    customerProvides: "Cycle counter baseline and target hardware",
    atomikEvaluates: "Measure software, direct hardware, batched, and profiled paths where possible",
    meaningful: "Lower cycles/update on the selected path",
    whereSafe: "Proof-page-safe with artifact",
  },
  {
    metric: "Update latency",
    matters: "Control loops, robotics, field systems, local state machines",
    customerProvides: "Latency target, update path, timing method",
    atomikEvaluates: "Measure time from state change to usable updated state",
    meaningful: "Lower latency against the current baseline",
    whereSafe: "Proof-page-safe with artifact",
  },
  {
    metric: "Response latency",
    matters: "AI-at-edge, robotics, user-facing embedded systems",
    customerProvides: "End-to-end path and response target",
    atomikEvaluates: "Isolate whether state movement is on the response path",
    meaningful: "Lower response latency if state movement dominates",
    whereSafe: "Diligence-only until measured",
  },
  {
    metric: "Duty cycle",
    matters: "Battery-limited and intermittently connected devices",
    customerProvides: "Wake/sleep pattern, transfer cadence, power budget",
    atomikEvaluates: "Estimate whether fewer transfers or scans reduce active time",
    meaningful: "Reduced active duty cycle with responsible measurement",
    whereSafe: "Diligence-only unless artifact-backed",
  },
  {
    metric: "Wake-up frequency",
    matters: "Radio-heavy IoT, sensor, and remote deployments",
    customerProvides: "Wake events, sync schedule, radio or processor activity",
    atomikEvaluates: "Look for state paths that trigger unnecessary wake-ups",
    meaningful: "Fewer wake-ups tied to a device-level budget",
    whereSafe: "Diligence-only unless artifact-backed",
  },
  {
    metric: "Memory/state footprint",
    matters: "Small devices, dense local state, constrained memory",
    customerProvides: "State layout, memory budget, history/checkpoint strategy",
    atomikEvaluates: "Map stored state, deltas, and reconstruction requirements",
    meaningful: "Lower footprint pressure without losing required state",
    whereSafe: "Proof-page-safe with artifact",
  },
  {
    metric: "Power proxy",
    matters: "Battery and thermal evaluations before instrumented power runs",
    customerProvides: "Cycle counts, bytes moved, active time, power model if available",
    atomikEvaluates: "Use measured proxies until instrumented power data exists",
    meaningful: "Proxy improves enough to justify power-instrumented testing",
    whereSafe: "Diligence-only unless clearly labeled",
  },
  {
    metric: "Thermal proxy",
    matters: "Enclosures, dense racks, fanless devices, field reliability",
    customerProvides: "Utilization, cycles, transfer volume, thermal limit",
    atomikEvaluates: "Use workload proxies to decide whether thermal measurement is worth running",
    meaningful: "Proxy reduction that supports a thermal test plan",
    whereSafe: "Diligence-only unless measured",
  },
  {
    metric: "Bandwidth pressure",
    matters: "Expensive, intermittent, congested, or tactical links",
    customerProvides: "Baseline bandwidth, link budget, transfer frequency",
    atomikEvaluates: "Compare baseline transfer pressure to state-aware transfer pressure",
    meaningful: "Less pressure on the constrained link",
    whereSafe: "Homepage-safe as an evaluation target",
  },
  {
    metric: "Correctness preservation",
    matters: "Every evaluation",
    customerProvides: "Expected outputs, invariants, test oracle, acceptance criteria",
    atomikEvaluates: "Verify that reduced movement or coalescing preserves required state",
    meaningful: "Same correct state or accepted behavior after optimization",
    whereSafe: "Homepage-safe",
  },
  {
    metric: "Target business outcome",
    matters: "Continuation decisions and paid evaluations",
    customerProvides: "Decision threshold, budget owner, economic or technical value",
    atomikEvaluates: "Connect measured improvement to the constraint that already hurts",
    meaningful: "Enough value to justify design-partner, licensing, or diligence work",
    whereSafe: "Diligence-only for customer-specific numbers",
  },
];

export const proofToday = [
  {
    title: "Public proof packet",
    label: "Artifact map",
    body:
      "The public site links to the evidence and benchmark page, hardware proof map, claims registry, and evidence labels.",
    href: "/benchmarks",
  },
  {
    title: "Evidence-labeling framework",
    label: "Evidence labels",
    body:
      "Claims are separated as live measured, hardware validated, software validated, formal proof, synthesis validated, build artifact, projected, conceptual, or roadmap.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/evidence-labels.md",
  },
  {
    title: "Claims registry",
    label: "Claim control",
    body:
      "The registry records public-safe claims, artifacts, caveats, and overclaim risks for current public materials.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml",
  },
  {
    title: "Live Zynq prototype evidence",
    label: "Hardware validated",
    body:
      "ATOMiK Desk v0.40-A is the current UI captured live from /dev/fb0 on Zynq hardware, with on-screen metrics driven by real measured on-board data. It is not proof of power, thermal, uptime, or production maturity.",
    href: "/10-current-live-atomik-desk-v040a.png",
  },
  {
    title: "Parallel-bank throughput on AX7020",
    label: "Live measured",
    body:
      "An 8-bank XOR accumulator on the AX7020 scaled linearly with allocated banks (1/2/4/8x fewer cycles, about 100-800 Mdeltas/s at 100 MHz), measured with an on-chip cycle counter. The result was byte-identical across every bank count and matched a software recompute - order-independent and lock-free by construction. It is the throughput substrate, not a customer-workload speedup or power claim.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/hardware/zynq/results/PARALLEL_BANKS_HARDWARE_VALIDATED.md",
  },
  {
    title: "Live Workloads demo on hardware",
    label: "Live measured",
    body:
      "The Workloads surface (real-time telemetry aggregation) runs on the AX7020 over HDMI driven by live measured throughput from the parallel-bank engine: 800 Mevents/s at 8 banks, with byte-identical results across configs. The first customer-facing workload demo proven end-to-end on hardware. Throughput is measured; it is a static capture, not an interactive session.",
    href: "/12-workloads-live-ax7020.png",
  },
  {
    title: "Verified savings on hardware: 92% less data, 87% fewer writes",
    label: "Live measured",
    body:
      "The redesigned Workloads surface shows conventional-vs-ATOMiK side by side on the AX7020: an edge-telemetry sync tick moves 2,048 bytes conventionally vs 160 bytes as deltas (92% less data moved), and 256 control updates coalesce into 32 net writes (87% fewer). Every delta was verified on the ATOMiK adapter with exact 64-bit round-trips using a falsification-tested harness, and the demo self-drives through all three scenarios untethered. Figures are for deterministic test patterns (ratios scale with change density and locality); static captures, not an interactive session.",
    href: "/13-workloads-telemetry-verified-ax7020.png",
  },
  {
    title: "Linux userspace to FPGA validation",
    label: "Hardware validated",
    body:
      "The Linux userspace proof validates the path from user process through Linux, MMIO, Wishbone CSR bus, and FPGA accelerator for algebraic checks.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/LINUX_USERSPACE_PROOF.md",
  },
  {
    title: "AX7020 board-run performance matrix",
    label: "Live measured",
    body:
      "The matrix shows ATOMiK can win in specific coalesced/batched scenarios and lose in others. Use it with the interpretation caveats.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/perf_matrix_ax7020_20260509.txt",
  },
  {
    title: "Formal proof work",
    label: "Formal proof",
    body:
      "Formal proof work is present in the repository. Public pages should avoid unaudited proof counts unless the count is verified across repo, site, and deck.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/tree/main/math/proofs",
  },
  {
    title: "Synthesis-validated artifacts",
    label: "Synthesis validated",
    body:
      "Synthesis and toolchain outputs are useful evidence but must not be blended with live-board measurements.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/HARDWARE_SYNTHESIS.md",
  },
  {
    title: "Roadmap and concept materials",
    label: "Roadmap / conceptual",
    body:
      "Concept visuals explain product direction only. They are not proof of current commercial functionality.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/visual-asset-manifest.md",
  },
];

export const successExamples = [
  "Fewer bytes moved for a bandwidth-constrained workload",
  "Fewer full-state transfers for a sync-heavy workload",
  "Lower update or reconstruction latency for a local-execution workload",
  "Fewer operations after coalescing for a change-heavy workload",
  "Lower power or thermal proxy where responsible measurement exists",
  "Clear fit/no-fit evidence that supports a design-partner decision",
];

export const notSuccessClaims = [
  "Universal speedup",
  "Guaranteed battery extension",
  "Guaranteed heat or cooling reduction",
  "Guaranteed water or power-bill savings",
  "Guaranteed smaller hardware or footprint",
  "Generic better compute",
  "Proof from unrelated workloads",
];

export const fitSignals = [
  "The workload repeatedly moves, scans, syncs, replays, or rebuilds state.",
  "The team can provide a current baseline and a decision threshold.",
  "Battery, heat, bandwidth, latency, footprint, weight, reliability, or cost is already painful.",
  "Correctness can be checked against an expected state, invariant, or output.",
  "The economic or technical value is large enough to justify evaluation work.",
];

export const noFitSignals = [
  "There is no measurable constraint.",
  "The workload is mostly stateless compute.",
  "The customer cannot share even representative state behavior.",
  "The baseline is unknown and cannot be measured.",
  "Correctness cannot be verified.",
  "The claimed pain is not expensive enough to support evaluation.",
];

export const buyerRoutes = [
  {
    route: "Proof review",
    for: "Prospects who need to understand current evidence before deeper work.",
    outcome: "Proof packet, claim boundaries, and fit/no-fit recommendation.",
  },
  {
    route: "Technical evaluation",
    for: "Teams with one workload, one baseline, and one painful constraint.",
    outcome: "Workload map, metric plan, and evaluation readout.",
  },
  {
    route: "Design-partner evaluation",
    for: "Qualified teams where an early ATOMiK path could affect product or architecture decisions.",
    outcome: "Scoped collaboration around measured value and integration risk.",
  },
  {
    route: "Licensing/IP diligence",
    for: "Chip, embedded, infrastructure, and strategic partners evaluating ATOMiK as IP.",
    outcome: "Proof-bound IP, hardware, ASIC feasibility, and licensing review.",
  },
  {
    route: "Investor diligence",
    for: "Investors reviewing the transition from proof to evaluated customer/IP opportunities.",
    outcome: "Evidence map, paid-evaluation strategy, IP status, and ASIC feasibility path.",
  },
];

export const statusQuoAlternatives = [
  {
    name: "More compute",
    solves: "Adds headroom quickly.",
    notSolve: "Does not remove redundant state movement.",
    whyTeamsChoose: "It is familiar and available.",
    insufficient: "When power, heat, cost, or size becomes the limiter.",
    atomikDiffers: "Targets the state movement that creates wasted work.",
    proofRequired: "Show that state movement, not raw compute, dominates the bottleneck.",
  },
  {
    name: "Bigger batteries",
    solves: "Extends runtime by adding stored energy.",
    notSolve: "Does not reduce work, heat, or data movement.",
    whyTeamsChoose: "It avoids architecture changes.",
    insufficient: "When weight, volume, charging, or field service is constrained.",
    atomikDiffers: "Evaluates whether the workload can spend less energy on state churn.",
    proofRequired: "Proxy or measured power improvement tied to the target workload.",
  },
  {
    name: "More cooling",
    solves: "Removes heat after it is created.",
    notSolve: "Does not reduce the work that creates heat.",
    whyTeamsChoose: "Thermal fixes are easier to budget than architecture changes.",
    insufficient: "When fanless, sealed, dense, or water-constrained deployments hit limits.",
    atomikDiffers: "Evaluates upstream redundant state work before promising thermal impact.",
    proofRequired: "Measured workload improvement and a responsible thermal test plan.",
  },
  {
    name: "More bandwidth",
    solves: "Moves larger state faster.",
    notSolve: "Does not reduce transfer volume or intermittent-link risk.",
    whyTeamsChoose: "Network upgrades are straightforward when available.",
    insufficient: "When links are expensive, congested, remote, or unavailable.",
    atomikDiffers: "Focuses on moving meaningful change instead of full state.",
    proofRequired: "Bytes moved, bytes avoided, and correctness preservation.",
  },
  {
    name: "Compression",
    solves: "Shrinks payloads.",
    notSolve: "May still move full state and can add CPU cost.",
    whyTeamsChoose: "It is a proven component-level optimization.",
    insufficient: "When unchanged state is still repeatedly encoded and moved.",
    atomikDiffers: "Changes the unit of movement before compression is considered.",
    proofRequired: "Compare compressed baseline to state-aware movement on the same workload.",
  },
  {
    name: "Caching",
    solves: "Keeps frequently used state closer.",
    notSolve: "Can add invalidation complexity and stale-state risk.",
    whyTeamsChoose: "It is familiar and often effective.",
    insufficient: "When updates, invalidation, or replay dominate the cost.",
    atomikDiffers: "Tracks meaningful change boundaries instead of only storing copies.",
    proofRequired: "Show reduced invalidation, replay, or sync work with correctness intact.",
  },
  {
    name: "Deduplication",
    solves: "Avoids repeated identical data.",
    notSolve: "May not model state transitions or coalesced updates.",
    whyTeamsChoose: "It is useful for storage and transfer systems.",
    insufficient: "When the painful cost is change propagation and reconstruction.",
    atomikDiffers: "Evaluates transition-aware work, not only duplicate payloads.",
    proofRequired: "Trace-level comparison of duplicates avoided versus operations coalesced.",
  },
  {
    name: "Sync protocols",
    solves: "Coordinates state across nodes.",
    notSolve: "Can still require scans, conflict handling, or large payloads.",
    whyTeamsChoose: "They are standard integration surfaces.",
    insufficient: "When protocol overhead or full-state reconciliation dominates.",
    atomikDiffers: "Targets the state path beneath or beside the protocol.",
    proofRequired: "Baseline protocol trace and state-aware alternative map.",
  },
  {
    name: "Specialized accelerators",
    solves: "Speeds a defined compute kernel.",
    notSolve: "May ignore state movement around the kernel.",
    whyTeamsChoose: "They fit known workloads such as AI, DSP, or crypto.",
    insufficient: "When the expensive work is bookkeeping, sync, or reconstruction.",
    atomikDiffers: "Focuses on state-aware execution and data movement boundaries.",
    proofRequired: "Show the state path is the bottleneck around the accelerator.",
  },
  {
    name: "Cloud offload",
    solves: "Moves compute and storage to larger infrastructure.",
    notSolve: "Adds latency, bandwidth dependence, and reliability exposure.",
    whyTeamsChoose: "It reduces device complexity.",
    insufficient: "When local, disconnected, private, or low-latency operation matters.",
    atomikDiffers: "Evaluates keeping more useful state work local.",
    proofRequired: "Latency, bandwidth, and correctness comparison for local execution.",
  },
  {
    name: "Overbuilt hardware",
    solves: "Buys margin across unknowns.",
    notSolve: "Raises cost, size, power, and thermal burden.",
    whyTeamsChoose: "It reduces near-term engineering risk.",
    insufficient: "When BOM, enclosure, or deployment economics are already tight.",
    atomikDiffers: "Looks for architectural waste before adding more hardware.",
    proofRequired: "Workload map showing avoided state work and footprint pressure.",
  },
  {
    name: "Manual optimization or feature cuts",
    solves: "Reduces load by hand or removes functionality.",
    notSolve: "Can be brittle, slow, or product-limiting.",
    whyTeamsChoose: "It is under the team's direct control.",
    insufficient: "When optimization becomes recurring engineering drag.",
    atomikDiffers: "Evaluates a reusable state-aware path for the constrained workload.",
    proofRequired: "Measured improvement compared with the current optimized baseline.",
  },
];
