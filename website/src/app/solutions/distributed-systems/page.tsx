import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title:
    "Distributed State Without Consensus — ATOMiK | CRDT Alternative, Lock-Free Convergence",
  description:
    "Replace Raft, Paxos, CRDTs, and event sourcing with XOR delta-state algebra. Consensus-free state sync with 99% bandwidth reduction, O(1) reconstruction, and 108 formal proofs. The distributed cache without a leader.",
  keywords: [
    "consensus-free state sync",
    "CRDT alternative",
    "distributed cache without leader",
    "lock-free convergence",
    "distributed state synchronization",
    "conflict-free replication",
    "delta-state algebra",
    "XOR convergence",
    "leaderless distributed systems",
    "event sourcing alternative",
  ],
  openGraph: {
    title: "Distributed State Without Consensus — ATOMiK",
    description:
      "Replace consensus protocols, CRDTs, and event sourcing with XOR algebra. 99% bandwidth reduction. O(1) reconstruction. 108 formal proofs.",
    type: "website",
  },
};

const painPoints = [
  {
    label: "Consensus Overhead",
    icon: "\u231B",
    problem:
      "Raft and Paxos require leader election, log replication, and quorum agreement for every state change. Latency scales with cluster size. A single slow node stalls the entire pipeline.",
    solution:
      "Delta commutativity eliminates ordering requirements. Every node accumulates XOR deltas independently \u2014 no leader, no quorum, no round-trips. Latency is O(1) regardless of cluster size.",
  },
  {
    label: "CRDT Complexity",
    icon: "\u2699",
    problem:
      "G-Counters, LWW-Registers, OR-Sets \u2014 each data type needs its own merge function, metadata overhead, and correctness proof. State bloat grows unbounded as tombstones accumulate.",
    solution:
      "One algebraic operation (XOR) handles all merge semantics. Self-inverse property means no tombstones: applying a delta twice cancels it. Zero metadata overhead per operation.",
  },
  {
    label: "Event Log Growth",
    icon: "\u2191",
    problem:
      "Event sourcing stores every mutation forever. Replaying 10M events to reconstruct current state takes minutes. Compaction is fragile. Storage costs grow linearly with history.",
    solution:
      "O(1) state reconstruction: current_state = initial_state XOR accumulator. No log replay. No compaction. The accumulator is a fixed-size summary of all deltas ever applied.",
  },
  {
    label: "Split-Brain Scenarios",
    icon: "\u26A1",
    problem:
      "Network partitions create divergent state. Rejoining requires conflict resolution, manual intervention, or data loss. Vector clocks add per-node metadata to every message.",
    solution:
      "XOR is associative and commutative \u2014 partitioned nodes accumulate deltas locally, then exchange them in any order upon reconnection. States converge automatically. No vector clocks needed.",
  },
];

const comparisonRows = [
  {
    metric: "Write Latency",
    atomik: "O(1) local",
    raft: "O(n) quorum RTT",
    crdt: "O(1) local",
    eventsource: "O(1) append",
  },
  {
    metric: "Sync Bandwidth",
    atomik: "Fixed-size delta",
    raft: "Full log entries",
    crdt: "State + metadata",
    eventsource: "Full event stream",
  },
  {
    metric: "Conflict Resolution",
    atomik: "None needed",
    raft: "Leader decides",
    crdt: "Type-specific merge",
    eventsource: "Manual / LWW",
  },
  {
    metric: "State Reconstruction",
    atomik: "O(1) XOR",
    raft: "Log replay",
    crdt: "Merge all replicas",
    eventsource: "Replay all events",
  },
  {
    metric: "Metadata Overhead",
    atomik: "Zero",
    raft: "Term + index/entry",
    crdt: "Vector clocks / dots",
    eventsource: "Sequence numbers",
  },
  {
    metric: "Partition Recovery",
    atomik: "Automatic",
    raft: "Re-election + catch-up",
    crdt: "Automatic (slow)",
    eventsource: "Conflict detection",
  },
  {
    metric: "Formal Proofs",
    atomik: "108 Lean4 theorems",
    raft: "TLA+ spec",
    crdt: "Per-type proofs",
    eventsource: "None standard",
  },
];

const metrics = [
  {
    value: "99%",
    label: "Bandwidth Reduction",
    detail: "Fixed-size deltas vs. full state replication",
  },
  {
    value: "O(1)",
    label: "Reconstruction",
    detail: "Single XOR operation, not log replay",
  },
  {
    value: "108",
    label: "Formal Proofs",
    detail: "Lean4-verified algebraic properties",
  },
  {
    value: "0",
    label: "Coordination Messages",
    detail: "No leader election, no quorum, no locking",
  },
];

export default function DistributedSystemsPage() {
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
          Solutions for Distributed Systems
        </p>
        <h1 className="text-4xl md:text-5xl font-bold tracking-tight mb-6">
          Distributed State{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{
              backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            Without Consensus
          </span>
        </h1>
        <p
          className="text-lg md:text-xl max-w-3xl mx-auto leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          ATOMiK replaces consensus protocols, CRDTs, and event sourcing with
          XOR delta-state algebra. Every node converges to the same state
          without leaders, quorums, or conflict resolution &mdash; mathematically
          guaranteed by 108 Lean4 proofs.
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
          The Problem with Distributed State
        </h2>
        <p
          className="text-center mb-12 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Every mainstream approach to distributed state pays a tax &mdash;
          in latency, bandwidth, complexity, or all three.
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

      {/* Architecture Diagram */}
      <section className="max-w-4xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          How Delta Convergence Works
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Three nodes send deltas in any order. XOR commutativity guarantees
          identical final state &mdash; no coordination required.
        </p>
        <div
          className="rounded-xl border p-6 md:p-10 overflow-x-auto"
          style={{
            background: "#12121a",
            borderColor: "#1e1e2e",
          }}
        >
          <pre
            className="text-sm md:text-base leading-relaxed"
            style={{
              color: "#8888a0",
              fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
            }}
          >
            <code>{`     Node A                  Node B                  Node C
  ┌──────────┐           ┌──────────┐           ┌──────────┐
  │ state(0) │           │ state(0) │           │ state(0) │
  │ = S_init │           │ = S_init │           │ = S_init │
  └────┬─────┘           └────┬─────┘           └────┬─────┘
       │                      │                      │
  accum(d_A)             accum(d_B)             accum(d_C)
       │                      │                      │
       ├───── send d_A ──────►├───── send d_B ──────►│
       │◄──── send d_B ───────┤◄──── send d_C ───────┤
       │◄──────────── send d_C ──────────────────────►│
       │               send d_A ─────────────────────►│
       │                      │◄──── send d_A ───────►│
       │                      │                      │
  ┌────┴─────┐           ┌────┴─────┐           ┌────┴─────┐
  │ S_init   │           │ S_init   │           │ S_init   │
  │ ⊕ d_A    │           │ ⊕ d_A    │           │ ⊕ d_A    │
  │ ⊕ d_B    │  ══════   │ ⊕ d_B    │  ══════   │ ⊕ d_B    │
  │ ⊕ d_C    │           │ ⊕ d_C    │           │ ⊕ d_C    │
  └──────────┘           └──────────┘           └──────────┘

  Order doesn't matter: d_A ⊕ d_B ⊕ d_C  =  d_C ⊕ d_A ⊕ d_B
  Algebraic guarantee:  Abelian group (commutative, associative, self-inverse)`}</code>
          </pre>
        </div>
      </section>

      {/* Code Example */}
      <section className="max-w-3xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          Convergence in 10 Lines
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Two nodes start with the same reference state. Each accumulates a
          different delta. Exchange deltas in any order &mdash; the result is
          always identical.
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
            distributed_cache.py
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
              <span style={{ color: "#e0e0e8" }}> DeltaStream</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Node A and Node B converge without coordination
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>stream_a = </span>
              <span style={{ color: "#4f8fff" }}>DeltaStream</span>
              <span style={{ color: "#e0e0e8" }}>()</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>stream_b = </span>
              <span style={{ color: "#4f8fff" }}>DeltaStream</span>
              <span style={{ color: "#e0e0e8" }}>()</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Both start from the same reference state
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>stream_a.</span>
              <span style={{ color: "#22d3ee" }}>load</span>
              <span style={{ color: "#e0e0e8" }}>(addr=</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>, initial_state=</span>
              <span style={{ color: "#f59e0b" }}>0xCAFEBABE</span>
              <span style={{ color: "#e0e0e8" }}>)</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>stream_b.</span>
              <span style={{ color: "#22d3ee" }}>load</span>
              <span style={{ color: "#e0e0e8" }}>(addr=</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>, initial_state=</span>
              <span style={{ color: "#f59e0b" }}>0xCAFEBABE</span>
              <span style={{ color: "#e0e0e8" }}>)</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Each node accumulates different deltas
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>stream_a.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(addr=</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>, delta=</span>
              <span style={{ color: "#f59e0b" }}>0x000000FF</span>
              <span style={{ color: "#e0e0e8" }}>)</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>stream_b.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(addr=</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>, delta=</span>
              <span style={{ color: "#f59e0b" }}>0x0000FF00</span>
              <span style={{ color: "#e0e0e8" }}>)</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Exchange deltas in any order -- result is identical
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>stream_a.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(addr=</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>, delta=</span>
              <span style={{ color: "#f59e0b" }}>0x0000FF00</span>
              <span style={{ color: "#e0e0e8" }}>)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# B&apos;s delta applied to A
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>stream_b.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(addr=</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>, delta=</span>
              <span style={{ color: "#f59e0b" }}>0x000000FF</span>
              <span style={{ color: "#e0e0e8" }}>)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# A&apos;s delta applied to B
              </span>
              {"\n\n"}
              <span style={{ color: "#8b5cf6" }}>assert</span>
              <span style={{ color: "#e0e0e8" }}> stream_a.</span>
              <span style={{ color: "#22d3ee" }}>read</span>
              <span style={{ color: "#e0e0e8" }}>(</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>) == stream_b.</span>
              <span style={{ color: "#22d3ee" }}>read</span>
              <span style={{ color: "#e0e0e8" }}>(</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# Always true
              </span>
            </code>
          </pre>
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
          Side-by-side comparison across the dimensions that matter for
          distributed state management.
        </p>
        <div
          className="rounded-xl border overflow-hidden overflow-x-auto"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <table className="w-full text-sm" style={{ minWidth: "640px" }}>
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
                  Raft / Paxos
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  CRDTs
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  Event Sourcing
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
                    {row.raft}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.crdt}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.eventsource}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
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

      {/* CTA */}
      <section
        className="text-center px-6 py-20"
        style={{ borderTop: "1px solid #1e1e2e" }}
      >
        <h2 className="text-3xl font-bold mb-4">
          Ready to eliminate consensus overhead?
        </h2>
        <p className="mb-8 max-w-xl mx-auto" style={{ color: "#8888a0" }}>
          Start with the free Python SDK. Scale to kernel-level optimization or
          FPGA hardware when you need 69.7 Gops/s throughput.
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
