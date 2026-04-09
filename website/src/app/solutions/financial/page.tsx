import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title:
    "Deterministic State for Financial Systems — ATOMiK | Trading State Sync, Real-Time P&L",
  description:
    "Constant-time state reconstruction for trading systems. Zero timing jitter eliminates side channels. Self-inverse undo for instant trade cancellations. Lock-free multi-venue P&L aggregation backed by 108 formal proofs.",
  keywords: [
    "deterministic state management",
    "trading system state sync",
    "financial data integrity",
    "real-time P&L tracking",
    "constant-time state reconstruction",
    "trading system latency",
    "financial state reconciliation",
    "timing side channel prevention",
    "lock-free aggregation",
    "multi-venue position reconciliation",
    "trade cancellation undo",
    "XOR fingerprinting",
    "deterministic latency trading",
    "financial audit trail",
  ],
  openGraph: {
    title: "Deterministic State for Financial Systems — ATOMiK",
    description:
      "O(1) state reconstruction with zero timing jitter. Eliminates side channels, enables instant undo, and provides lock-free multi-venue aggregation. 108 formal proofs.",
    type: "website",
  },
};

const painPoints = [
  {
    label: "State Reconstruction Latency",
    icon: "\u23F1",
    problem:
      "Rebuilding position state from trade logs takes O(n) time. During market hours, replaying thousands of events to answer \"what is my current exposure?\" introduces latency that costs real money. Snapshotting helps but creates consistency gaps.",
    solution:
      "current_state = initial_state XOR accumulator. One operation, constant time, regardless of how many trades have occurred. No log replay. No snapshot staleness. The accumulator is a fixed-size summary of every delta ever applied.",
  },
  {
    label: "Non-Deterministic Timing",
    icon: "\u26A0",
    problem:
      "Variable-latency state lookups create exploitable jitter. Cache misses, GC pauses, and speculative execution produce timing variations that sophisticated adversaries can measure and exploit for information leakage about positions and order flow.",
    solution:
      "Every ATOMiK operation completes in constant time by design. No caches, no speculation, no garbage collection in the critical path. Hardware-validated deterministic latency with stdev below 0.5 cycles eliminates timing side channels entirely.",
  },
  {
    label: "Audit Trail Overhead",
    icon: "\u2696",
    problem:
      "Regulatory compliance demands a complete, tamper-evident record of every state transition. Maintaining cryptographic audit trails alongside high-frequency state updates creates write amplification and verification bottlenecks during audits.",
    solution:
      "XOR accumulation is its own audit mechanism. The accumulator is a deterministic fingerprint of all applied deltas. Any single bit flip in any historical delta produces a different final state. Verification is O(1): re-derive and compare.",
  },
  {
    label: "Multi-Venue Reconciliation",
    icon: "\u2194",
    problem:
      "Positions span NYSE, NASDAQ, CBOE, dark pools. Reconciling state across venues requires ordered merge logic, conflict resolution, and expensive end-of-day batch processes. Real-time cross-venue exposure is approximated, not exact.",
    solution:
      "XOR commutativity means venue deltas can arrive in any order and the result is identical. Lock-free parallel accumulation from every venue produces exact real-time exposure. No ordered merges. No batch reconciliation. Mathematically guaranteed convergence.",
  },
];

const securityProperties = [
  {
    property: "Operation Timing",
    traditional: "Variable (cache-dependent)",
    atomik: "Constant (hardware-guaranteed)",
    impact: "Eliminates timing oracle attacks on position state",
  },
  {
    property: "Speculative Execution",
    traditional: "CPU speculates on branches",
    atomik: "No speculation in data path",
    impact: "No Spectre/Meltdown class vulnerabilities",
  },
  {
    property: "Cache Side Channels",
    traditional: "Cache hit/miss leaks access patterns",
    atomik: "No cache hierarchy in core",
    impact: "Access patterns cannot be inferred",
  },
  {
    property: "State Integrity",
    traditional: "Separate checksum layer",
    atomik: "XOR accumulator is the checksum",
    impact: "Corruption detected at zero additional cost",
  },
  {
    property: "Replay Attacks",
    traditional: "Sequence number tracking",
    atomik: "Self-inverse: double-apply cancels",
    impact: "Duplicate deltas are algebraically harmless",
  },
];

const metrics = [
  {
    value: "O(1)",
    label: "Deterministic Latency",
    detail: "Constant-time state reconstruction regardless of history depth",
  },
  {
    value: "0",
    label: "Timing Jitter",
    detail: "Sub-0.5 cycle stdev eliminates side channel exploitation",
  },
  {
    value: "108",
    label: "Formal Proofs",
    detail: "Lean4-verified algebraic properties guarantee correctness",
  },
  {
    value: "\u22A5\u00B9",
    label: "Self-Inverse Undo",
    detail: "Instant trade cancellation: apply the same delta to reverse it",
  },
];

export default function FinancialSolutionsPage() {
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
          Solutions for Financial Technology
        </p>
        <h1 className="text-4xl md:text-5xl font-bold tracking-tight mb-6">
          Deterministic State for{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{
              backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            Financial Systems
          </span>
        </h1>
        <p
          className="text-lg md:text-xl max-w-3xl mx-auto leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          O(1) state reconstruction with zero timing jitter. ATOMiK&apos;s
          delta-state algebra eliminates timing side channels, enables instant
          trade cancellations via self-inverse undo, and provides lock-free
          multi-venue aggregation &mdash; all backed by 108 Lean4 formal proofs.
        </p>
        <div className="flex flex-wrap justify-center gap-4 mt-10">
          <Link
            href="/contact"
            className="px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#8b5cf6", color: "#fff" }}
          >
            Contact Sales
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
          Why Trading Systems Need Deterministic State
        </h2>
        <p
          className="text-center mb-12 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Financial infrastructure demands guarantees that conventional state
          management cannot provide &mdash; constant timing, provable
          correctness, and real-time multi-venue consistency.
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
          Real-Time P&amp;L Tracking
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Trades arrive from multiple venues in any order. The accumulator
          converges to the correct P&amp;L without ordering, locking, or
          reconciliation.
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
            realtime_pnl.py
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
                # Initialize P&amp;L tracker -- start of day
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>pnl = </span>
              <span style={{ color: "#4f8fff" }}>AtomikContext</span>
              <span style={{ color: "#e0e0e8" }}>()</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>pnl.</span>
              <span style={{ color: "#22d3ee" }}>load</span>
              <span style={{ color: "#e0e0e8" }}>(</span>
              <span style={{ color: "#f59e0b" }}>0</span>
              <span style={{ color: "#e0e0e8" }}>)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# Start of day: flat
              </span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Trades arrive from multiple venues in any order
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>pnl.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(trade_delta_nyse)</span>
              <span style={{ color: "#6a6a80" }}>
                {"    "}# +$1,234
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>pnl.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(trade_delta_nasdaq)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# -$567
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>pnl.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(trade_delta_cboe)</span>
              <span style={{ color: "#6a6a80" }}>
                {"    "}# +$890
              </span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # O(1), deterministic, order-independent
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>current_pnl = pnl.</span>
              <span style={{ color: "#22d3ee" }}>read</span>
              <span style={{ color: "#e0e0e8" }}>()</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Cancel a trade: self-inverse means apply the same delta again
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>pnl.</span>
              <span style={{ color: "#22d3ee" }}>accum</span>
              <span style={{ color: "#e0e0e8" }}>(trade_delta_nasdaq)</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# Undo: -$567 XOR -$567 = 0
              </span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Verify integrity: accumulator is the fingerprint
              </span>
              {"\n"}
              <span style={{ color: "#8b5cf6" }}>assert</span>
              <span style={{ color: "#e0e0e8" }}> pnl.</span>
              <span style={{ color: "#22d3ee" }}>read</span>
              <span style={{ color: "#e0e0e8" }}>() == expected_state</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# O(1) verification
              </span>
            </code>
          </pre>
        </div>
      </section>

      {/* Security Properties Table */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          Security Properties for Financial Infrastructure
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          ATOMiK&apos;s security guarantees are architectural, not bolted on.
          Constant-time operations and the absence of speculative execution
          eliminate entire classes of side-channel attacks.
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
                  Property
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  Traditional Systems
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
                  Impact
                </th>
              </tr>
            </thead>
            <tbody>
              {securityProperties.map((row, i) => (
                <tr
                  key={row.property}
                  style={{
                    borderBottom:
                      i < securityProperties.length - 1
                        ? "1px solid #1e1e2e"
                        : "none",
                  }}
                >
                  <td
                    className="px-5 py-3.5 font-medium"
                    style={{ color: "#b0b0c0" }}
                  >
                    {row.property}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#ff6b6b" }}>
                    {row.traditional}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#22c55e" }}>
                    {row.atomik}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.impact}
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

      {/* Case Study CTA */}
      <section className="max-w-4xl mx-auto px-6 pb-20">
        <Link
          href="/case-studies"
          className="block rounded-xl border p-8 transition-all hover:border-opacity-80"
          style={{
            background: "linear-gradient(135deg, rgba(245,158,11,0.06), rgba(139,92,246,0.06))",
            borderColor: "#f59e0b40",
          }}
        >
          <p
            className="text-xs font-semibold uppercase tracking-widest mb-3"
            style={{ color: "#f59e0b" }}
          >
            See it in action
          </p>
          <h3 className="text-xl font-bold mb-2">
            See how QuantumEdge Capital achieved 99.7% latency reduction with ATOMiK
          </h3>
          <p className="text-sm mb-4" style={{ color: "#8888a0" }}>
            QuantumEdge replaced their 45-minute end-of-day P&amp;L reconciliation pipeline
            with real-time delta streaming across 4 trading venues &mdash; saving $2.3M annually.
          </p>
          <span
            className="text-sm font-semibold"
            style={{ color: "#f59e0b" }}
          >
            Read case study &rarr;
          </span>
        </Link>
      </section>

      {/* CTA */}
      <section
        className="text-center px-6 py-20"
        style={{ borderTop: "1px solid #1e1e2e" }}
      >
        <h2 className="text-3xl font-bold mb-4">
          Ready for deterministic financial infrastructure?
        </h2>
        <p className="mb-8 max-w-xl mx-auto" style={{ color: "#8888a0" }}>
          Talk to our team about integrating ATOMiK into your trading
          systems, risk engines, and multi-venue reconciliation pipelines.
        </p>
        <div className="flex flex-wrap justify-center gap-4">
          <Link
            href="/contact"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#8b5cf6", color: "#fff" }}
          >
            Contact Sales
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
