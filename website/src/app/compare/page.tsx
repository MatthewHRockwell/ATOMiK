import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import DecisionTree from "@/components/DecisionTree";

export const metadata: Metadata = {
  title: "ATOMiK vs Redis vs Kafka vs CRDTs — Comparison",
  description:
    "How ATOMiK delta-state algebra compares to Redis, Kafka, CRDTs, event sourcing, checksums, and hardware accelerators. Honest trade-offs for developers.",
  openGraph: {
    title: "ATOMiK vs Redis vs Kafka vs CRDTs — Comparison",
    description:
      "Delta-state algebra vs the tools you already know. Honest comparison across state sync, stream processing, change detection, and hardware acceleration.",
    url: "https://atomik.tech/compare",
    siteName: "ATOMiK",
    images: [{ url: "https://atomik.tech/og-image.jpg", width: 1200, height: 630 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK vs Redis vs Kafka vs CRDTs — Comparison",
    description:
      "Delta-state algebra vs the tools you already know. Honest trade-offs for developers.",
    images: ["https://atomik.tech/og-image.jpg"],
  },
};

/* ------------------------------------------------------------------ */
/*  Data                                                               */
/* ------------------------------------------------------------------ */

const stateSyncHeaders = ["Feature", "ATOMiK", "Redis", "etcd", "ZooKeeper"];
const stateSyncRows: string[][] = [
  [
    "Consistency model",
    "Convergent (Abelian group)",
    "Eventual / strong (configurable)",
    "Linearizable (Raft)",
    "Linearizable (ZAB)",
  ],
  [
    "Sync bandwidth",
    "O(delta) \u2014 XOR of changes only",
    "Full key-value per write",
    "Full key-value per write",
    "Full znode per write",
  ],
  [
    "Coordination needed",
    "None \u2014 order-independent",
    "Leader for writes (Cluster)",
    "Leader election required",
    "Leader election required",
  ],
  [
    "Write latency",
    "Deterministic, constant-time",
    "Sub-ms (single node), variable (cluster)",
    "~10ms (Raft consensus round)",
    "~10ms (ZAB proposal round)",
  ],
  [
    "State reconstruction",
    "O(1) \u2014 initial \u2295 accumulator",
    "O(1) key lookup (stored in memory)",
    "O(1) key lookup (stored in memory)",
    "O(1) znode read (stored in memory)",
  ],
  [
    "Formal proofs",
    "108 Lean4 theorems",
    "No formal verification",
    "TLA+ spec (partial)",
    "No formal verification",
  ],
];

const streamHeaders = ["Feature", "ATOMiK", "Kafka", "Event Sourcing", "CRDTs"];
const streamRows: string[][] = [
  [
    "Storage growth",
    "Constant (single accumulator)",
    "Linear (log retention)",
    "Linear (event log)",
    "Proportional to state size",
  ],
  [
    "Replay cost",
    "O(1) \u2014 no replay needed",
    "O(n) \u2014 scan from offset",
    "O(n) \u2014 replay all events",
    "O(1) \u2014 merge states directly",
  ],
  [
    "Undo / rollback",
    "Re-apply same delta (self-inverse)",
    "Produce compensating event",
    "Produce compensating event",
    "Not generally supported",
  ],
  [
    "Conflict resolution",
    "Automatic (commutativity)",
    "Partition-based ordering",
    "Application-level logic",
    "Automatic (join-semilattice)",
  ],
  [
    "Bandwidth per update",
    "Size of changed fields only",
    "Full message payload",
    "Full event payload",
    "State or op payload (varies)",
  ],
  [
    "Ordering requirement",
    "None \u2014 commutative + associative",
    "Total order per partition",
    "Total order per aggregate",
    "None (op-based) / causal (state-based)",
  ],
];

const changeHeaders = ["Feature", "ATOMiK", "Checksums (SHA/MD5)", "Diff / Patch", "Merkle Trees"];
const changeRows: string[][] = [
  [
    "Detection complexity",
    "O(1) \u2014 compare accumulator to identity",
    "O(n) \u2014 hash full content",
    "O(n) \u2014 byte-by-byte comparison",
    "O(log n) \u2014 walk tree path",
  ],
  [
    "Per-page cost",
    "Single XOR (1 cycle on FPGA)",
    "Full hash computation",
    "Full content scan",
    "Hash per node on path",
  ],
  [
    "False negative rate",
    "Zero (algebraically proven)",
    "Negligible (collision probability)",
    "Zero",
    "Negligible (collision probability)",
  ],
  [
    "Deterministic timing",
    "Yes \u2014 constant-time, no branches",
    "No \u2014 data-dependent",
    "No \u2014 data-dependent",
    "Partially \u2014 fixed-depth trees only",
  ],
  [
    "Side channels",
    "None \u2014 no speculative execution",
    "Timing varies with input",
    "Timing varies with differences",
    "Path-dependent timing",
  ],
];

const hwHeaders = ["Feature", "ATOMiK FPGA", "GPU Computing", "ASIC (custom)"];
const hwRows: string[][] = [
  [
    "Operations/sec",
    "Synthesis-characterized Zynq scaling",
    "Tera-scale (matrix ops)",
    "Application-specific, highest",
  ],
  [
    "Dev board cost",
    "$13.50 (Tang Nano 9K)",
    "$200+ (entry-level)",
    "$100K+ (tape-out)",
  ],
  [
    "Power consumption",
    "< 1W (FPGA fabric)",
    "75\u2013350W (typical GPU)",
    "Lowest for given task",
  ],
  [
    "Formal verification",
    "108 Lean4 theorems + RTL sim",
    "Numerical verification only",
    "Full verification possible",
  ],
  [
    "Time to prototype",
    "Hours (parameterized RTL)",
    "Days (CUDA/OpenCL)",
    "12\u201318 months (fab cycle)",
  ],
];

const useCases = [
  "Distributed state synchronization where bandwidth is expensive (IoT, edge, satellite)",
  "Real-time change detection across large memory regions or page tables",
  "Lock-free parallel accumulation from multiple producers",
  "Embedded systems where deterministic, constant-time operations matter",
  "Applications that need undo/rollback without an event log",
  "Sensor fusion pipelines that merge readings from independent sources",
  "Reducing memory traffic in copy-on-write workloads (containers, VMs)",
  "Systems requiring formal correctness proof artifacts (108 Lean4 proofs)",
];

const antiPatterns = [
  {
    label: "Not a database",
    detail:
      "ATOMiK has no query language, no indexes, no transactions. If you need to store and query structured data, use PostgreSQL, Redis, or DynamoDB.",
  },
  {
    label: "Not a message queue",
    detail:
      "ATOMiK does not provide ordered delivery, consumer groups, or dead-letter queues. If you need reliable message routing, use Kafka, RabbitMQ, or SQS.",
  },
  {
    label: "Not cryptographic hashing",
    detail:
      "XOR-based accumulation is not collision-resistant in a cryptographic sense. Do not use ATOMiK as a substitute for SHA-256 in security contexts.",
  },
  {
    label: "No built-in audit trail",
    detail:
      "Delta-state algebra accumulates changes into a single value. Individual deltas are not stored. If you need a full event history, use event sourcing or an append-only log.",
  },
  {
    label: "Not for rich data transformations",
    detail:
      "ATOMiK tracks whether state changed and what the current state is. It does not map, filter, aggregate, or join data. Use Flink, Spark, or Kafka Streams for stream processing pipelines.",
  },
];

/* ------------------------------------------------------------------ */
/*  Reusable table component                                           */
/* ------------------------------------------------------------------ */

function ComparisonTable({
  headers,
  rows,
}: {
  headers: string[];
  rows: string[][];
}) {
  return (
    <div
      className="rounded-xl border overflow-x-auto"
      style={{ background: "#12121a", borderColor: "#1e1e2e" }}
    >
      <table className="w-full text-sm min-w-[640px]">
        <thead>
          <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
            {headers.map((h, i) => (
              <th
                key={h}
                className={`p-4 font-semibold ${i === 0 ? "text-left" : "text-left"}`}
                style={{ color: i === 1 ? "#8b5cf6" : "#8888a0" }}
              >
                {h}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {rows.map((row, ri) => (
            <tr
              key={ri}
              style={{
                borderBottom:
                  ri < rows.length - 1 ? "1px solid #1e1e2e" : "none",
                background:
                  ri % 2 === 0 ? "transparent" : "rgba(255,255,255,0.01)",
              }}
            >
              {row.map((cell, ci) => (
                <td
                  key={ci}
                  className="p-4"
                  style={{
                    color: ci === 0 ? "#b0b0c0" : ci === 1 ? "#d0d0e0" : "#8888a0",
                    fontWeight: ci === 0 ? 500 : 400,
                  }}
                >
                  {cell}
                </td>
              ))}
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  Page                                                               */
/* ------------------------------------------------------------------ */

export default function ComparePage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-4">
        <h1 className="text-4xl font-bold tracking-tight mb-4">
          How ATOMiK Compares
        </h1>
        <p
          className="text-lg max-w-2xl mx-auto leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          Delta-state algebra vs the tools you already know.
          <br />
          Honest trade-offs so you can pick the right tool for the job.
        </p>
      </section>

      {/* Decision Tree */}
      <DecisionTree />

      {/* Section 1: State Synchronization */}
      <section id="compare-state-sync" className="max-w-5xl mx-auto px-6 py-12 scroll-mt-20 rounded-xl transition-all duration-500">
        <h2 className="text-2xl font-bold mb-3">State Synchronization</h2>
        <p className="text-sm mb-6 max-w-3xl leading-relaxed" style={{ color: "#8888a0" }}>
          Redis, etcd, and ZooKeeper are production-grade data stores with rich
          ecosystems. ATOMiK is not a replacement for any of them. It solves a
          narrower problem: synchronizing state across nodes with minimal
          bandwidth and zero coordination. Where these tools store and serve
          full key-value pairs, ATOMiK transmits only the XOR delta of what
          changed, and reconstructs the current state in O(1) from a single
          accumulator.
        </p>
        <ComparisonTable headers={stateSyncHeaders} rows={stateSyncRows} />

        {/* Migration link */}
        <div
          className="mt-6 rounded-lg border p-4 flex flex-col sm:flex-row sm:items-center gap-3 sm:gap-4"
          style={{ background: "rgba(139,92,246,0.04)", borderColor: "#2d2d4a" }}
        >
          <p className="text-sm" style={{ color: "#b0b0c0" }}>
            Ready to switch?
          </p>
          <Link
            href="/docs/migrate-crdt"
            className="inline-flex items-center gap-2 text-sm font-semibold transition-opacity hover:opacity-80"
            style={{ color: "#8b5cf6" }}
          >
            Migration Guide: From CRDTs to ATOMiK
            <span aria-hidden="true">{"\u2192"}</span>
          </Link>
        </div>
      </section>

      {/* Section 2: Event / Stream Processing */}
      <section id="compare-stream" className="max-w-5xl mx-auto px-6 py-12 scroll-mt-20 rounded-xl transition-all duration-500">
        <h2 className="text-2xl font-bold mb-3">Event / Stream Processing</h2>
        <p className="text-sm mb-6 max-w-3xl leading-relaxed" style={{ color: "#8888a0" }}>
          Kafka and event sourcing give you ordered, replayable event logs with
          rich consumer ecosystems. CRDTs give you conflict-free convergence.
          ATOMiK overlaps with CRDTs conceptually (both converge without
          coordination), but uses a stricter algebraic structure (Abelian group
          vs. join-semilattice) that also provides free undo via self-inverse
          deltas. The trade-off: ATOMiK does not store individual events and
          has no concept of consumer groups or partitions.
        </p>
        <ComparisonTable headers={streamHeaders} rows={streamRows} />

        {/* Migration link */}
        <div
          className="mt-6 rounded-lg border p-4 flex flex-col sm:flex-row sm:items-center gap-3 sm:gap-4"
          style={{ background: "rgba(139,92,246,0.04)", borderColor: "#2d2d4a" }}
        >
          <p className="text-sm" style={{ color: "#b0b0c0" }}>
            Ready to switch?
          </p>
          <Link
            href="/docs/migrate-event-sourcing"
            className="inline-flex items-center gap-2 text-sm font-semibold transition-opacity hover:opacity-80"
            style={{ color: "#8b5cf6" }}
          >
            Migration Guide: From Event Sourcing to ATOMiK
            <span aria-hidden="true">{"\u2192"}</span>
          </Link>
        </div>
      </section>

      {/* Section 3: Change Detection */}
      <section id="compare-change" className="max-w-5xl mx-auto px-6 py-12 scroll-mt-20 rounded-xl transition-all duration-500">
        <h2 className="text-2xl font-bold mb-3">Change Detection</h2>
        <p className="text-sm mb-6 max-w-3xl leading-relaxed" style={{ color: "#8888a0" }}>
          Traditional change detection scans the full content (checksums, diff)
          or walks a tree (Merkle). ATOMiK detects changes in O(1) by checking
          whether the accumulator equals the identity element. On FPGA, this is
          a compact comparison in the modeled hardware path. Timing-side-channel
          claims still require a measured workload, threat model, and deployment
          boundary.
        </p>
        <ComparisonTable headers={changeHeaders} rows={changeRows} />

        {/* Migration link */}
        <div
          className="mt-6 rounded-lg border p-4 flex flex-col sm:flex-row sm:items-center gap-3 sm:gap-4"
          style={{ background: "rgba(139,92,246,0.04)", borderColor: "#2d2d4a" }}
        >
          <p className="text-sm" style={{ color: "#b0b0c0" }}>
            Ready to get started?
          </p>
          <Link
            href="/docs/quickstart"
            className="inline-flex items-center gap-2 text-sm font-semibold transition-opacity hover:opacity-80"
            style={{ color: "#8b5cf6" }}
          >
            Getting Started with ATOMiK Change Detection
            <span aria-hidden="true">{"\u2192"}</span>
          </Link>
        </div>
      </section>

      {/* Section 4: Hardware Acceleration */}
      <section className="max-w-5xl mx-auto px-6 py-12">
        <h2 className="text-2xl font-bold mb-3">Hardware Acceleration</h2>
        <p className="text-sm mb-6 max-w-3xl leading-relaxed" style={{ color: "#8888a0" }}>
          GPUs excel at massively parallel floating-point work (ML, graphics).
          ASICs deliver the highest performance per watt for a fixed function.
          ATOMiK targets FPGA because delta-state operations are
          simple, wide, and embarrassingly parallel&mdash;a natural fit for
          configurable fabric. A $13.50 Tang Nano 9K runs the full stack. A
          Zynq-7020 scaling path is documented as synthesis output, not as a
          current live-board benchmark claim.
        </p>
        <ComparisonTable headers={hwHeaders} rows={hwRows} />
      </section>

      {/* When to use ATOMiK */}
      <section className="max-w-5xl mx-auto px-6 py-12">
        <h2 className="text-2xl font-bold mb-6">When to use ATOMiK</h2>
        <div
          className="rounded-xl border p-6"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <ul className="space-y-3">
            {useCases.map((uc) => (
              <li key={uc} className="flex items-start gap-3 text-sm">
                <span className="mt-0.5 shrink-0" style={{ color: "#22c55e" }}>
                  {"\u2713"}
                </span>
                <span style={{ color: "#b0b0c0" }}>{uc}</span>
              </li>
            ))}
          </ul>
        </div>
      </section>

      {/* When NOT to use ATOMiK */}
      <section className="max-w-5xl mx-auto px-6 py-12">
        <h2 className="text-2xl font-bold mb-6">When NOT to use ATOMiK</h2>
        <div className="space-y-3">
          {antiPatterns.map((ap) => (
            <div
              key={ap.label}
              className="rounded-xl border p-5"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex items-start gap-3">
                <span className="mt-0.5 shrink-0" style={{ color: "#ef4444" }}>
                  {"\u2717"}
                </span>
                <div>
                  <p className="text-sm font-semibold mb-1">{ap.label}</p>
                  <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
                    {ap.detail}
                  </p>
                </div>
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* CTA */}
      <section className="text-center px-6 py-16 pb-24">
        <h2 className="text-2xl font-bold mb-3">Ready to evaluate it?</h2>
        <p className="text-sm mb-8 max-w-lg mx-auto" style={{ color: "#8888a0" }}>
          Start with the evidence framework, then bring a concrete workload if
          the algebra looks relevant.
        </p>
        <div className="flex items-center justify-center gap-4">
          <Link
            href="/get-started"
            className="inline-block px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#4f8fff", color: "#fff" }}
          >
            Request Evaluation Access
          </Link>
          <Link
            href="/docs"
            className="inline-block px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#e0e0e8",
              border: "1px solid #1e1e2e",
            }}
          >
            Read the Docs
          </Link>
        </div>
      </section>
    </div>
  );
}
