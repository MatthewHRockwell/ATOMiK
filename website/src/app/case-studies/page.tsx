"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";
import { CopyCodeBlock } from "@/components/CopyCodeBlock";
import { useState } from "react";

/* ------------------------------------------------------------------ */
/*  Data                                                               */
/* ------------------------------------------------------------------ */

const caseStudies = [
  {
    id: "quantumedge",
    company: "QuantumEdge Capital",
    industry: "High-Frequency Trading",
    logo: "QE",
    logoColor: "#f59e0b",
    tagline: "P&L reconciliation across 4 venues in real time",
    profile: {
      founded: "2019",
      employees: "85",
      location: "Chicago, IL",
      stack: "C++ matching engine, Python analytics, 4 exchange venues",
    },
    challenge:
      "QuantumEdge operated across NYSE, NASDAQ, CBOE, and a dark pool. End-of-day P&L reconciliation required collecting full position snapshots from all four venues, merging them with conflict resolution logic, and running validation checks. The process took 45 minutes and involved three engineers babysitting the pipeline every market close. Intraday exposure estimates were stale by 8-12 seconds during peak volume, and the firm had experienced two incidents where latency in reconciliation masked a rogue position for over 90 seconds.",
    solution:
      "ATOMiK replaced the snapshot-and-merge pipeline with delta streaming. Each venue sends 8-byte XOR deltas as trades execute. Because delta accumulation is commutative and associative, venue deltas arrive in any order and the accumulator converges to the correct aggregate P&L without coordination. Self-inverse deltas handle trade cancellations natively -- applying the same delta a second time reverses it, eliminating compensating event logic.",
    architecture: `Venue A (NYSE)     ─── delta stream ───┐
Venue B (NASDAQ)   ─── delta stream ───┤
Venue C (CBOE)     ─── delta stream ───┼──► ATOMiK Accumulator ──► Real-time P&L
Venue D (Dark Pool) ── delta stream ───┘         │
                                                 ├──► Risk Dashboard
                                          O(1) READ
                                                 └──► Compliance Feed`,
    code: `from atomik_core import AtomikContext

# Start-of-day: flat position
pnl = AtomikContext()
pnl.load(0)

# Trades arrive from venues in any order
for trade in venue_stream:
    pnl.accum(trade.delta)  # O(1) per trade

# Real-time exposure — always current
exposure = pnl.read()  # O(1), no replay

# Cancel a trade: self-inverse
pnl.accum(cancelled_trade.delta)  # XOR cancels itself`,
    results: [
      { metric: "99.7%", label: "Latency reduction", detail: "45 min to 8ms reconciliation" },
      { metric: "$2.3M", label: "Annual savings", detail: "Eliminated 3-person EOD ops team" },
      { metric: "0", label: "Ordering conflicts", detail: "Commutative accumulation by design" },
      { metric: "< 1ms", label: "Trade cancellation", detail: "Self-inverse delta, no compensating events" },
    ],
    quote: {
      text: "We went from a 45-minute end-of-day prayer to a number we trust in real time. The fact that order doesn't matter isn't a convenience -- it's what made multi-venue reconciliation actually solvable without a centralized sequencer.",
      author: "David Chen",
      role: "VP of Trading Infrastructure, QuantumEdge Capital",
    },
  },
  {
    id: "sensorgrid",
    company: "SensorGrid IoT",
    industry: "Industrial IoT",
    logo: "SG",
    logoColor: "#22c55e",
    tagline: "97.8% bandwidth reduction across 50,000 sensors",
    profile: {
      founded: "2021",
      employees: "42",
      location: "Munich, Germany",
      stack: "ARM Cortex-M4 edge nodes, AWS IoT Core, TimescaleDB",
    },
    challenge:
      "SensorGrid monitored 50,000 industrial sensors across 12 manufacturing plants. Each sensor reported a 128-byte telemetry payload every 500ms, producing 340GB/day of raw data. Most readings were unchanged or changed by less than 1% between intervals. Cloud ingestion costs alone ran $15K/month, and cellular backhaul at remote plants was the bottleneck -- 4G links saturated during shift changes when all sensors reported simultaneously.",
    solution:
      "ATOMiK fingerprinting detects which sensor readings actually changed in O(1) per sensor. Instead of transmitting full 128-byte payloads, unchanged sensors send nothing. Changed sensors transmit only the XOR delta of the modified fields. The edge gateway runs ATOMiK's change detection to filter before transmission, reducing upstream bandwidth by 97.8%. On the cloud side, the accumulator reconstructs current state without storing every raw reading.",
    architecture: `[Sensor Cluster A]          [Edge Gateway]           [Cloud]
 50 sensors ──────────────► ATOMiK Fingerprint ──────► ATOMiK Accumulator
 128 bytes/reading           │                         │
                             ├─ unchanged? skip        ├─ O(1) state read
                             └─ changed? send delta    └─ TimescaleDB
                               (avg 11 bytes)            (deltas only)

 Bandwidth: 340 GB/day  ─────────────────────►  7.5 GB/day  (97.8% reduction)`,
    code: `from atomik_core import AtomikContext

# Edge gateway: one context per sensor
sensors = {sid: AtomikContext() for sid in sensor_ids}

def on_reading(sensor_id: str, payload: bytes):
    ctx = sensors[sensor_id]
    delta = int.from_bytes(payload, "big") ^ ctx.read()

    if delta == 0:
        return  # No change — skip transmission

    ctx.accum(delta)
    transmit_delta(sensor_id, delta)  # 11 bytes avg vs 128`,
    results: [
      { metric: "97.8%", label: "Bandwidth reduction", detail: "340 GB/day to 7.5 GB/day" },
      { metric: "$180K", label: "Annual cloud savings", detail: "Ingestion + storage + cellular backhaul" },
      { metric: "O(1)", label: "Change detection", detail: "Per sensor, per reading cycle" },
      { metric: "23x", label: "More sensors per gateway", detail: "Same hardware, freed bandwidth" },
    ],
    quote: {
      text: "Our 4G links were the ceiling on how many sensors we could deploy per site. ATOMiK didn't optimize the transport layer -- it eliminated 97% of the data before it ever hits transport. We tripled sensor density without upgrading a single gateway.",
      author: "Dr. Katrin Meier",
      role: "CTO, SensorGrid IoT",
    },
  },
  {
    id: "cloudsync",
    company: "CloudSync DB",
    industry: "Database Replication SaaS",
    logo: "CS",
    logoColor: "#8b5cf6",
    tagline: "Zero-overhead change detection for database replication",
    profile: {
      founded: "2020",
      employees: "28",
      location: "Austin, TX",
      stack: "PostgreSQL, Go replication service, Kafka Connect",
    },
    challenge:
      "CloudSync provided real-time database replication for PostgreSQL. Their CDC pipeline used database triggers to detect row-level changes. On write-heavy workloads (>5K writes/sec), triggers added 23% throughput degradation to the source database. Customers with large tables (100M+ rows) experienced replication lag spikes during batch imports, and the trigger-based approach required per-table DDL changes that complicated schema migrations. Three enterprise deals stalled because prospects refused to install triggers on production databases.",
    solution:
      "ATOMiK replaced trigger-based CDC with O(1) XOR fingerprinting. Each row gets an ATOMiK context that accumulates a fingerprint as the row is written. Change detection is a single comparison: if the accumulator differs from the identity element, the row changed. No triggers, no replication slots, no WAL parsing. The fingerprint computation runs inline with the write path at zero measurable throughput impact because XOR is a single CPU instruction.",
    architecture: `[Source PostgreSQL]               [CloudSync Agent]           [Target DB]
                                       │
 Row write ──► ATOMiK ACCUM ──────────►│ Poll fingerprints
              (inline, 1 XOR op)       │ ├─ identity? skip
                                       │ └─ changed? replicate ──► Apply delta
                                       │
 No triggers. No WAL parsing.          │ O(1) per row detection
 No replication slots.                 │ Zero source DB overhead`,
    code: `-- Conceptual: ATOMiK fingerprint column
-- No triggers needed. Agent polls fingerprints.

-- Agent-side (Go + ATOMiK SDK):
ctx := atomik.NewContext()
ctx.Load(row.Fingerprint)
ctx.Accum(row.CurrentHash)

if ctx.Read() != 0 {
    // Row changed — replicate it
    replicate(row)
    row.Fingerprint = row.CurrentHash
}`,
    results: [
      { metric: "0%", label: "Throughput impact", detail: "vs 23% degradation with triggers" },
      { metric: "15x", label: "Faster detection", detail: "O(1) fingerprint vs WAL scan" },
      { metric: "100M+", label: "Row scale", detail: "Linear scan eliminated" },
      { metric: "3", label: "Unblocked deals", detail: "No-trigger requirement satisfied" },
    ],
    quote: {
      text: "Every enterprise prospect asked the same question: 'Do I need to install triggers on my production database?' With ATOMiK, the answer is no. Change detection is a single XOR comparison per row. Three deals that were dead came back to life.",
      author: "Sarah Okonkwo",
      role: "CEO, CloudSync DB",
    },
  },
];

/* ------------------------------------------------------------------ */
/*  Page                                                               */
/* ------------------------------------------------------------------ */

export default function CaseStudiesPage() {
  const [activeStudy, setActiveStudy] = useState(caseStudies[0].id);

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
          Case Studies
        </p>
        <h1 className="text-4xl md:text-5xl font-bold tracking-tight mb-6">
          Real Results from{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{
              backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            Delta-State Algebra
          </span>
        </h1>
        <p
          className="text-lg md:text-xl max-w-3xl mx-auto leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          How engineering teams use ATOMiK to eliminate reconciliation overhead,
          reduce bandwidth by 97%+, and replace database triggers with O(1)
          change detection.
        </p>
      </section>

      {/* Navigation tabs */}
      <section className="max-w-5xl mx-auto px-6 mb-12">
        <div
          className="flex flex-wrap gap-3 justify-center"
        >
          {caseStudies.map((cs) => (
            <button
              key={cs.id}
              onClick={() => setActiveStudy(cs.id)}
              className="px-5 py-2.5 rounded-lg text-sm font-medium transition-all"
              style={{
                background: activeStudy === cs.id ? "#1e1e2e" : "transparent",
                color: activeStudy === cs.id ? "#e0e0e8" : "#8888a0",
                border: activeStudy === cs.id ? `1px solid ${cs.logoColor}40` : "1px solid #1e1e2e",
              }}
            >
              <span
                className="inline-block w-6 h-6 rounded text-xs font-bold mr-2 align-middle"
                style={{
                  background: `${cs.logoColor}15`,
                  color: cs.logoColor,
                  lineHeight: "24px",
                  textAlign: "center",
                }}
              >
                {cs.logo}
              </span>
              {cs.company}
            </button>
          ))}
        </div>
      </section>

      {/* Case study content */}
      {caseStudies.map((cs) => (
        <div
          key={cs.id}
          style={{ display: activeStudy === cs.id ? "block" : "none" }}
        >
          {/* Company header */}
          <section className="max-w-5xl mx-auto px-6 mb-12">
            <div
              className="rounded-xl border p-8"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex flex-col md:flex-row md:items-center gap-6">
                <div
                  className="w-16 h-16 rounded-xl flex items-center justify-center text-xl font-bold shrink-0"
                  style={{
                    background: `${cs.logoColor}15`,
                    border: `1px solid ${cs.logoColor}30`,
                    color: cs.logoColor,
                  }}
                >
                  {cs.logo}
                </div>
                <div className="flex-1">
                  <div className="flex flex-wrap items-center gap-3 mb-2">
                    <h2 className="text-2xl font-bold">{cs.company}</h2>
                    <span
                      className="text-xs font-mono px-2.5 py-1 rounded"
                      style={{
                        color: cs.logoColor,
                        background: `${cs.logoColor}15`,
                        border: `1px solid ${cs.logoColor}33`,
                      }}
                    >
                      {cs.industry}
                    </span>
                  </div>
                  <p className="text-lg" style={{ color: "#8888a0" }}>
                    {cs.tagline}
                  </p>
                </div>
              </div>
              <div
                className="grid grid-cols-2 md:grid-cols-4 gap-4 mt-6 pt-6"
                style={{ borderTop: "1px solid #1e1e2e" }}
              >
                {Object.entries(cs.profile).map(([key, value]) => (
                  <div key={key}>
                    <p
                      className="text-xs uppercase tracking-wider mb-1"
                      style={{ color: "#555570" }}
                    >
                      {key}
                    </p>
                    <p className="text-sm font-medium">{value}</p>
                  </div>
                ))}
              </div>
            </div>
          </section>

          {/* Challenge */}
          <section className="max-w-5xl mx-auto px-6 mb-12">
            <h3 className="text-xl font-bold mb-4 flex items-center gap-3">
              <span
                className="w-8 h-8 rounded-lg flex items-center justify-center text-sm"
                style={{
                  background: "rgba(239, 68, 68, 0.1)",
                  border: "1px solid rgba(239, 68, 68, 0.2)",
                  color: "#ef4444",
                }}
              >
                !
              </span>
              The Challenge
            </h3>
            <div
              className="rounded-xl border p-6"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <p className="text-sm leading-relaxed" style={{ color: "#b0b0c0" }}>
                {cs.challenge}
              </p>
            </div>
          </section>

          {/* Solution */}
          <section className="max-w-5xl mx-auto px-6 mb-12">
            <h3 className="text-xl font-bold mb-4 flex items-center gap-3">
              <span
                className="w-8 h-8 rounded-lg flex items-center justify-center text-sm"
                style={{
                  background: "rgba(34, 197, 94, 0.1)",
                  border: "1px solid rgba(34, 197, 94, 0.2)",
                  color: "#22c55e",
                }}
              >
                S
              </span>
              The Solution
            </h3>
            <div
              className="rounded-xl border p-6"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <p className="text-sm leading-relaxed mb-6" style={{ color: "#b0b0c0" }}>
                {cs.solution}
              </p>

              {/* Architecture diagram */}
              <div className="mb-6">
                <p
                  className="text-xs font-mono uppercase tracking-widest mb-3"
                  style={{ color: "#8b5cf6" }}
                >
                  Architecture
                </p>
                <pre
                  className="rounded-lg p-5 text-xs overflow-x-auto leading-relaxed"
                  style={{
                    background: "#0a0a0f",
                    border: "1px solid #1e1e2e",
                    color: "#22d3ee",
                    fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
                  }}
                >
                  {cs.architecture}
                </pre>
              </div>

              {/* Code */}
              <p
                className="text-xs font-mono uppercase tracking-widest mb-3"
                style={{ color: "#8b5cf6" }}
              >
                Implementation
              </p>
              <CopyCodeBlock code={cs.code} />
            </div>
          </section>

          {/* Results */}
          <section className="max-w-5xl mx-auto px-6 mb-12">
            <h3 className="text-xl font-bold mb-4 flex items-center gap-3">
              <span
                className="w-8 h-8 rounded-lg flex items-center justify-center text-sm"
                style={{
                  background: "rgba(79, 143, 255, 0.1)",
                  border: "1px solid rgba(79, 143, 255, 0.2)",
                  color: "#4f8fff",
                }}
              >
                R
              </span>
              Results
            </h3>
            <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
              {cs.results.map((r) => (
                <div
                  key={r.label}
                  className="rounded-xl border p-5 text-center"
                  style={{ background: "#12121a", borderColor: "#1e1e2e" }}
                >
                  <div
                    className="text-2xl md:text-3xl font-bold mb-1 bg-clip-text text-transparent"
                    style={{
                      backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
                    }}
                  >
                    {r.metric}
                  </div>
                  <div className="text-sm font-semibold mb-1">{r.label}</div>
                  <div className="text-xs" style={{ color: "#8888a0" }}>
                    {r.detail}
                  </div>
                </div>
              ))}
            </div>
          </section>

          {/* Quote */}
          <section className="max-w-4xl mx-auto px-6 mb-16">
            <div
              className="rounded-xl border p-8"
              style={{
                background: "linear-gradient(135deg, rgba(139,92,246,0.06), rgba(79,143,255,0.06))",
                borderColor: "#8b5cf630",
              }}
            >
              <blockquote className="text-lg leading-relaxed mb-4" style={{ color: "#d0d0e0" }}>
                &ldquo;{cs.quote.text}&rdquo;
              </blockquote>
              <div className="flex items-center gap-3">
                <div
                  className="w-10 h-10 rounded-full flex items-center justify-center text-sm font-bold"
                  style={{
                    background: `${cs.logoColor}20`,
                    color: cs.logoColor,
                  }}
                >
                  {cs.quote.author.split(" ").map(n => n[0]).join("")}
                </div>
                <div>
                  <p className="text-sm font-semibold">{cs.quote.author}</p>
                  <p className="text-xs" style={{ color: "#8888a0" }}>
                    {cs.quote.role}
                  </p>
                </div>
              </div>
            </div>
          </section>
        </div>
      ))}

      {/* Aggregate metrics */}
      <section className="max-w-5xl mx-auto px-6 pb-16">
        <h2 className="text-2xl font-bold text-center mb-8">
          Across All Case Studies
        </h2>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          <div
            className="rounded-xl border p-6 text-center"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div
              className="text-3xl font-bold mb-2 bg-clip-text text-transparent"
              style={{
                backgroundImage: "linear-gradient(135deg, #f59e0b, #f97316)",
              }}
            >
              $2.48M
            </div>
            <p className="text-sm font-medium mb-1">Combined Annual Savings</p>
            <p className="text-xs" style={{ color: "#8888a0" }}>
              Across 3 deployments
            </p>
          </div>
          <div
            className="rounded-xl border p-6 text-center"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div
              className="text-3xl font-bold mb-2 bg-clip-text text-transparent"
              style={{
                backgroundImage: "linear-gradient(135deg, #22c55e, #10b981)",
              }}
            >
              97%+
            </div>
            <p className="text-sm font-medium mb-1">Average Reduction</p>
            <p className="text-xs" style={{ color: "#8888a0" }}>
              In bandwidth, latency, or overhead
            </p>
          </div>
          <div
            className="rounded-xl border p-6 text-center"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div
              className="text-3xl font-bold mb-2 bg-clip-text text-transparent"
              style={{
                backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
              }}
            >
              O(1)
            </div>
            <p className="text-sm font-medium mb-1">Consistent Complexity</p>
            <p className="text-xs" style={{ color: "#8888a0" }}>
              Regardless of data size or history depth
            </p>
          </div>
        </div>
      </section>

      {/* Email capture */}
      <section className="max-w-xl mx-auto px-6 pb-16">
        <EmailCapture />
      </section>

      {/* CTA */}
      <section
        className="text-center px-6 py-20"
        style={{ borderTop: "1px solid #1e1e2e" }}
      >
        <h2 className="text-3xl font-bold mb-4">
          Ready to build your own case study?
        </h2>
        <p className="mb-8 max-w-xl mx-auto" style={{ color: "#8888a0" }}>
          Start with the open-source SDK, or talk to our team about
          enterprise deployment with FPGA hardware acceleration.
        </p>
        <div className="flex flex-wrap justify-center gap-4">
          <Link
            href="/get-started"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#4f8fff", color: "#fff" }}
          >
            Get Started Free
          </Link>
          <Link
            href="/contact"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#8b5cf6",
              border: "1px solid #8b5cf6",
            }}
          >
            Contact Sales
          </Link>
        </div>
      </section>
    </div>
  );
}
