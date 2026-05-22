import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Use Cases - ATOMiK",
  description:
    "ATOMiK use cases for customers constrained by heat, battery life, bandwidth, latency, cooling cost, or hardware footprint.",
};

const colors = {
  bg: "#070b12",
  panel: "#0d1420",
  border: "#1d324a",
  text: "#f4f8ff",
  muted: "#9fb1c7",
  cyan: "#22d3ee",
  green: "#22c55e",
  violet: "#a78bfa",
  amber: "#f59e0b",
  blue: "#4f8fff",
};

const useCases = [
  {
    label: "Data centers",
    outcome: "Reduce cooling pressure before it becomes a facilities problem",
    pain: "Heat is not just a chip problem. It becomes rack density, cooling energy, water use, uptime risk, and real operating cost.",
    value: "ATOMiK is a fit to evaluate where repeated state movement and redundant reconstruction create avoidable heat and bandwidth pressure.",
    proof: "Start with workload mapping, then quote only measured or labeled artifacts.",
    color: colors.green,
  },
  {
    label: "Edge devices",
    outcome: "Extend useful work inside tight battery and thermal envelopes",
    pain: "Edge systems live with small batteries, intermittent links, limited cooling, and hard latency budgets.",
    value: "ATOMiK targets compact state deltas and local reconstruction so devices can move less data and spend more power on useful work.",
    proof: "Best first fit: telemetry, sync, sensor state, embedded control, and other change-heavy paths.",
    color: colors.cyan,
  },
  {
    label: "AI at the edge",
    outcome: "Keep context hot without pushing every state change through a larger system",
    pain: "Agent and AI products are increasingly constrained by context movement, latency, memory pressure, and network cost.",
    value: "ATOMiK evaluates the state-management layer around AI workloads: context retention, delta propagation, and fewer full-state transfers.",
    proof: "Current claims stay at the systems layer unless a workload-specific benchmark is created.",
    color: colors.violet,
  },
  {
    label: "Defense and remote operations",
    outcome: "Lower weight, lower power, and fewer fragile data paths",
    pain: "Remote systems care about every watt, ounce, packet, and minute of uptime. Connectivity and field service are not guaranteed.",
    value: "ATOMiK is relevant where deterministic state handling and reduced data movement can make a system smaller, cooler, or more reliable.",
    proof: "Evaluate against explicit mission constraints; do not infer production readiness from concept visuals.",
    color: colors.amber,
  },
];

const fitSignals = [
  "Your workload repeatedly asks what changed",
  "Bandwidth, heat, battery, latency, or hardware footprint is already painful",
  "Full-state sync or replay is more expensive than the actual business logic",
  "A smaller hardware profile would create product or deployment value",
];

export default function SolutionsPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Use Cases" />

      <section className="mx-auto max-w-6xl px-6 pb-12 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Customer use cases
        </p>
        <h1 className="mt-4 max-w-4xl text-4xl font-bold leading-tight md:text-5xl">
          ATOMiK matters when wasted state turns into heat, cost, latency, or bigger hardware.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          The same architecture creates different customer value depending on the constraint: data centers feel cooling and density, edge devices feel battery and bandwidth, AI systems feel context movement, and remote systems feel reliability.
        </p>
      </section>

      <section className="mx-auto grid max-w-6xl gap-5 px-6 pb-14 md:grid-cols-2">
        {useCases.map((item) => (
          <article key={item.label} className="rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <div className="mb-5 h-1 w-20 rounded" style={{ background: item.color }} />
            <p className="text-[11px] font-semibold uppercase" style={{ color: item.color }}>
              {item.label}
            </p>
            <h2 className="mt-2 text-2xl font-bold leading-snug">{item.outcome}</h2>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              <strong style={{ color: colors.text }}>Pain:</strong> {item.pain}
            </p>
            <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
              <strong style={{ color: colors.text }}>ATOMiK value:</strong> {item.value}
            </p>
            <p className="mt-3 text-xs leading-5" style={{ color: "#6f8097" }}>
              {item.proof}
            </p>
          </article>
        ))}
      </section>

      <section className="mx-auto max-w-6xl px-6 pb-16">
        <div className="grid gap-5 rounded-lg p-6 md:grid-cols-[0.9fr_1.1fr]" style={{ background: "#101a29", border: `1px solid ${colors.border}` }}>
          <div>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
              Strong fit signals
            </p>
            <h2 className="mt-3 text-3xl font-bold">Bring one workload where state waste is already expensive.</h2>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              ATOMiK should be evaluated against concrete pain. If the constraint is not measurable, the first step is to define the measurement rather than claim a result.
            </p>
          </div>
          <div className="grid gap-3 sm:grid-cols-2">
            {fitSignals.map((signal) => (
              <div key={signal} className="rounded-lg p-4 text-sm leading-6" style={{ background: colors.bg, border: `1px solid ${colors.border}`, color: colors.muted }}>
                <span style={{ color: colors.green }}>-</span> {signal}
              </div>
            ))}
          </div>
        </div>

        <div className="mt-8 flex flex-wrap gap-3">
          <Link href="/contact?intent=evaluation" className="rounded-lg px-5 py-3 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Request Technical Evaluation
          </Link>
          <Link href="/docs/hardware" className="rounded-lg px-5 py-3 text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
            View Hardware Proof
          </Link>
        </div>
      </section>
    </div>
  );
}
