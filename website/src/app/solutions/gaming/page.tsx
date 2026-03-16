import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title:
    "Real-Time Game State Sync at 5M Ops/Second — ATOMiK | Multiplayer Netcode, Deterministic Lockstep",
  description:
    "Replace full state snapshots and rollback netcode with XOR delta-state algebra. 8-byte updates, O(1) rollback, zero desync — mathematically guaranteed. The multiplayer state engine that eliminates reconciliation.",
  keywords: [
    "game state synchronization",
    "multiplayer netcode",
    "deterministic lockstep",
    "real-time state replication",
    "lag compensation",
    "rollback netcode",
    "game server state sync",
    "multiplayer state management",
    "client-server reconciliation",
    "game state prediction",
  ],
  openGraph: {
    title: "Real-Time Game State at 5M Ops/Second — ATOMiK",
    description:
      "Replace full state snapshots and rollback netcode with XOR delta-state algebra. 8-byte updates. O(1) rollback. Zero desync.",
    type: "website",
  },
};

const painPoints = [
  {
    label: "Full State Snapshots Waste Bandwidth",
    icon: "\uD83D\uDCE1",
    problem:
      "Traditional multiplayer engines broadcast entire game state every tick. A 64-player lobby sending full snapshots at 60 Hz generates gigabytes of redundant data. Most of that state hasn't changed — you're paying bandwidth for unchanged health bars, idle inventories, and static world objects.",
    solution:
      "ATOMiK sends only 8-byte XOR deltas per changed property. Position moved? Send the delta. Health changed? Send the delta. Everything else costs zero bandwidth. Validated at 7,670x to 916,000x traffic reduction across real workloads.",
  },
  {
    label: "Client-Server Desync",
    icon: "\uD83D\uDD04",
    problem:
      "Floating-point arithmetic is non-deterministic across platforms — different CPUs, compilers, and optimization levels produce different results. Client prediction diverges from the authoritative server. Players see rubberbanding, teleporting, and ghost hits.",
    solution:
      "XOR is bitwise-exact on every platform. No floating-point rounding, no platform-dependent behavior. Delta accumulation is deterministic by construction — every client and server converges to the identical state, bit for bit. Proven by 92 Lean4 theorems.",
  },
  {
    label: "Rollback Netcode Complexity",
    icon: "\u23EA",
    problem:
      "Rollback (GGPO-style) requires snapshotting game state every frame, detecting mispredictions, rewinding, and re-simulating. The implementation complexity is enormous — save/load serialization for every game object, plus CPU cost scales with rollback depth.",
    solution:
      "XOR self-inverse gives O(1) undo: applying a delta twice cancels it. Rollback is a single XOR operation, not a full state reload and re-simulation. No serialization, no snapshots, no frame-by-frame replay — just reverse the delta.",
  },
  {
    label: "State Prediction Failures",
    icon: "\uD83C\uDFAF",
    problem:
      "Client-side prediction assumes actions will be confirmed by the server. When they aren't — rejected movement, lag spike, packet reorder — the client must reconcile divergent state. Reconciliation logic is error-prone and game-specific.",
    solution:
      "Commutative accumulation means player actions apply in any order with the same result. Packet reorder doesn't cause divergence. Late-arriving deltas just get XOR'd in — the math guarantees convergence regardless of arrival order. No reconciliation logic needed.",
  },
];

const comparisonRows = [
  {
    metric: "Update Size",
    atomik: "8 bytes (fixed)",
    snapshot: "Full state (KB-MB)",
    deltaComp: "Variable (compressed diff)",
    lockstep: "Input only (small)",
  },
  {
    metric: "Bandwidth at 60 Hz",
    atomik: "480 B/s per property",
    snapshot: "KB-MB/s per client",
    deltaComp: "Varies with change rate",
    lockstep: "Low (inputs only)",
  },
  {
    metric: "Rollback Cost",
    atomik: "O(1) single XOR",
    snapshot: "O(n) state reload",
    deltaComp: "O(n) diff reapply",
    lockstep: "O(n) re-simulate",
  },
  {
    metric: "Platform Determinism",
    atomik: "Bitwise-exact (XOR)",
    snapshot: "N/A (authoritative)",
    deltaComp: "N/A (authoritative)",
    lockstep: "Requires IEEE 754 lockstep",
  },
  {
    metric: "Packet Reorder Tolerance",
    atomik: "Fully commutative",
    snapshot: "Sequence numbers required",
    deltaComp: "Ordered application",
    lockstep: "Must synchronize inputs",
  },
  {
    metric: "Late Join / Reconnect",
    atomik: "Send accumulator (8B)",
    snapshot: "Send full state",
    deltaComp: "Send full state + log",
    lockstep: "Replay from start",
  },
  {
    metric: "Implementation Complexity",
    atomik: "3 operations (load/accum/read)",
    snapshot: "Serialize entire state",
    deltaComp: "Diff + patch + compress",
    lockstep: "Deterministic simulation",
  },
];

const metrics = [
  {
    value: "5M",
    label: "Ops/Second",
    detail: "Python SDK throughput per core",
  },
  {
    value: "8B",
    label: "Per Update",
    detail: "Fixed-size delta, any state change",
  },
  {
    value: "O(1)",
    label: "Rollback",
    detail: "Single XOR reversal, not re-simulation",
  },
  {
    value: "0",
    label: "Desync",
    detail: "Bitwise-exact convergence, all platforms",
  },
];

export default function GamingPage() {
  return (
    <div
      className="min-h-screen"
      style={{ background: "#0a0a0f", color: "#e0e0e8" }}
    >
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-16 max-w-4xl mx-auto">
        <p
          className="text-sm font-semibold uppercase tracking-widest mb-4"
          style={{ color: "#8b5cf6" }}
        >
          Solutions for Gaming &amp; Real-Time Applications
        </p>
        <h1 className="text-4xl md:text-5xl font-bold tracking-tight mb-6">
          Real-Time State at{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{
              backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            5M Ops/Second
          </span>
        </h1>
        <p
          className="text-lg md:text-xl max-w-3xl mx-auto leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          ATOMiK uses delta-state algebra to synchronize multiplayer game state,
          real-time collaboration, and live simulations. Player actions
          accumulate as 8-byte XOR deltas in any order &mdash; every client
          converges to identical state without reconciliation, rollback
          complexity, or bandwidth waste.
        </p>
        <div className="flex flex-wrap justify-center gap-4 mt-10">
          <Link
            href="/get-started"
            className="px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#8b5cf6", color: "#fff" }}
          >
            Get Started Free
          </Link>
          <Link
            href="/pricing"
            className="px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#4f8fff",
              border: "1px solid #4f8fff",
            }}
          >
            View Pricing
          </Link>
        </div>
      </section>

      {/* Problem / Solution */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          The Multiplayer State Problem
        </h2>
        <p
          className="text-center mb-12 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Game state synchronization forces a choice between bandwidth,
          determinism, complexity, and latency. ATOMiK eliminates the tradeoff.
        </p>

        <div className="grid md:grid-cols-2 gap-6">
          {painPoints.map((point) => (
            <div
              key={point.label}
              className="rounded-xl border p-6"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex items-center gap-3 mb-3">
                <span
                  className="w-10 h-10 rounded-lg flex items-center justify-center text-lg"
                  style={{
                    background: "rgba(139, 92, 246, 0.1)",
                    border: "1px solid rgba(139, 92, 246, 0.2)",
                  }}
                >
                  {point.icon}
                </span>
                <h3 className="text-lg font-semibold">{point.label}</h3>
              </div>
              <p className="text-sm mb-4" style={{ color: "#ff6b6b" }}>
                {point.problem}
              </p>
              <div
                className="rounded-lg p-4 text-sm"
                style={{
                  background: "rgba(34, 197, 94, 0.05)",
                  border: "1px solid rgba(34, 197, 94, 0.15)",
                  color: "#b0b0c0",
                }}
              >
                <span
                  className="font-semibold text-xs uppercase tracking-wider"
                  style={{ color: "#22c55e" }}
                >
                  ATOMiK Solution
                </span>
                <p className="mt-1">{point.solution}</p>
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Code Example */}
      <section className="max-w-3xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          Game State Sync in 12 Lines
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Server applies movement deltas from any client, in any order. Every
          client converges to the same position &mdash; no reconciliation
          needed.
        </p>
        <div
          className="rounded-xl border overflow-hidden"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <div
            className="px-4 py-2.5 border-b text-xs font-semibold flex items-center gap-2"
            style={{ borderColor: "#1e1e2e", color: "#8888a0" }}
          >
            <span
              className="w-2 h-2 rounded-full"
              style={{ background: "#22c55e" }}
            ></span>
            game_state_sync.py
          </div>
          <pre
            className="p-6 text-sm overflow-x-auto"
            style={{
              background: "#0a0a0f",
              fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
            }}
          >
            <code>
              <span style={{ color: "#8b5cf6" }}>from</span>
              <span style={{ color: "#e0e0e8" }}> atomik_core </span>
              <span style={{ color: "#8b5cf6" }}>import</span>
              <span style={{ color: "#e0e0e8" }}> AtomikContext</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Player position tracking
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>player = </span>
              <span style={{ color: "#4f8fff" }}>AtomikContext</span>
              <span style={{ color: "#e0e0e8" }}>()</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>player.</span>
              <span style={{ color: "#22d3ee" }}>load</span>
              <span style={{ color: "#e0e0e8" }}>(initial_position)</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Server applies movement deltas from any client, any order
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>player.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(movement_delta_1)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# Player A moved
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>player.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(movement_delta_2)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# Player B moved
              </span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # All clients converge to same state -- no reconciliation needed
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>position = player.</span>
              <span style={{ color: "#22d3ee" }}>read</span>
              <span style={{ color: "#e0e0e8" }}>()</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # O(1) rollback: undo any action with a single XOR
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>player.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(movement_delta_1)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# Self-inverse: XOR twice = cancel
              </span>
            </code>
          </pre>
        </div>
      </section>

      {/* Metrics */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div className="grid grid-cols-2 md:grid-cols-4 gap-6">
          {metrics.map((metric) => (
            <div
              key={metric.label}
              className="rounded-xl border p-6 text-center"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div
                className="text-3xl md:text-4xl font-bold mb-2 bg-clip-text text-transparent"
                style={{
                  backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
                }}
              >
                {metric.value}
              </div>
              <div className="text-sm font-semibold mb-1">{metric.label}</div>
              <div className="text-xs" style={{ color: "#8888a0" }}>
                {metric.detail}
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Comparison Table */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          How ATOMiK Compares
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Side-by-side comparison against the standard approaches to multiplayer
          game state synchronization.
        </p>
        <div
          className="rounded-xl border overflow-hidden overflow-x-auto"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <table className="w-full text-sm" style={{ minWidth: "720px" }}>
            <thead>
              <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  Metric
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8b5cf6" }}
                >
                  ATOMiK
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  Full State Snapshot
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  Delta Compression
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  Deterministic Lockstep
                </th>
              </tr>
            </thead>
            <tbody>
              {comparisonRows.map((row, i) => (
                <tr
                  key={row.metric}
                  style={{
                    borderBottom:
                      i < comparisonRows.length - 1
                        ? "1px solid #1e1e2e"
                        : "none",
                  }}
                >
                  <td
                    className="px-5 py-3.5 font-medium"
                    style={{ color: "#b0b0c0" }}
                  >
                    {row.metric}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#22c55e" }}>
                    {row.atomik}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.snapshot}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.deltaComp}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.lockstep}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>

      {/* CTA */}
      <section
        className="text-center px-6 py-20"
        style={{ borderTop: "1px solid #1e1e2e" }}
      >
        <h2 className="text-3xl font-bold mb-4">
          Ready to eliminate netcode complexity?
        </h2>
        <p className="mb-8 max-w-xl mx-auto" style={{ color: "#8888a0" }}>
          Start with the free Python SDK. Scale to FPGA hardware acceleration
          when you need 69.7 Gops/s throughput for dedicated game servers.
        </p>
        <div className="flex flex-wrap justify-center gap-4">
          <Link
            href="/get-started"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#8b5cf6", color: "#fff" }}
          >
            Get Started Free
          </Link>
          <Link
            href="/pricing"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#4f8fff",
              border: "1px solid #4f8fff",
            }}
          >
            View Pricing
          </Link>
        </div>
      </section>
    </div>
  );
}
