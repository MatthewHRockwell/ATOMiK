import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Solutions — ATOMiK",
  description:
    "ATOMiK solutions for distributed systems, IoT/edge computing, and financial infrastructure. Delta-state algebra for every industry.",
};

const solutions = [
  {
    href: "/solutions/distributed-systems",
    title: "Distributed Systems",
    subtitle: "Consensus-free state synchronization",
    description:
      "Replace Raft/Paxos coordination with commutative XOR deltas. Nodes converge regardless of message order — no leader election, no vector clocks.",
    color: "#4f8fff",
    keywords: ["State sync", "CRDTs", "Convergence"],
  },
  {
    href: "/solutions/iot-edge",
    title: "IoT & Edge Computing",
    subtitle: "99% bandwidth reduction at the edge",
    description:
      "Send 8-byte deltas instead of full sensor payloads. Fingerprint-based change detection skips unchanged data. Works over intermittent connections.",
    color: "#22c55e",
    keywords: ["Bandwidth", "Sensors", "Edge-cloud sync"],
  },
  {
    href: "/solutions/financial",
    title: "Financial Systems",
    subtitle: "Deterministic, timing-safe state management",
    description:
      "O(1) constant-time operations with zero timing jitter. Lock-free multi-venue aggregation. Self-inverse undo for instant trade cancellation.",
    color: "#f59e0b",
    keywords: ["Trading", "P&L", "Deterministic"],
  },
];

export default function SolutionsPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      <section className="max-w-4xl mx-auto px-6 pt-20 pb-8">
        <h1 className="text-4xl font-bold tracking-tight mb-4">Solutions</h1>
        <p className="text-lg" style={{ color: "#8888a0" }}>
          Delta-state algebra solves different problems in different domains.
          Same four operations, different value propositions.
        </p>
      </section>

      <section className="max-w-4xl mx-auto px-6 pb-24">
        <div className="space-y-6">
          {solutions.map((s) => (
            <Link
              key={s.href}
              href={s.href}
              className="block rounded-xl border p-8 transition-colors hover:border-opacity-60"
              style={{
                background: "#12121a",
                borderColor: "#1e1e2e",
              }}
            >
              <div className="flex items-center gap-3 mb-2">
                <h2 className="text-xl font-bold">{s.title}</h2>
                <span className="text-xs font-mono px-2 py-0.5 rounded" style={{ color: s.color, background: `${s.color}15`, border: `1px solid ${s.color}33` }}>
                  {s.subtitle}
                </span>
              </div>
              <p className="mb-4" style={{ color: "#8888a0" }}>{s.description}</p>
              <div className="flex gap-2">
                {s.keywords.map((k) => (
                  <span key={k} className="text-xs px-2 py-0.5 rounded-full" style={{ background: "rgba(255,255,255,0.05)", color: "#8888a0" }}>
                    {k}
                  </span>
                ))}
              </div>
            </Link>
          ))}
        </div>
      </section>
    </div>
  );
}
