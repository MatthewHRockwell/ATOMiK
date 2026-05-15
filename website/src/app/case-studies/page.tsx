import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Target Workloads - ATOMiK",
  description:
    "Illustrative ATOMiK target workloads and design-partner discovery paths, not external deployments.",
};

const colors = {
  bg: "#070b12",
  panel: "#0d1420",
  border: "#1d324a",
  text: "#f4f8ff",
  muted: "#9fb1c7",
  cyan: "#22d3ee",
  blue: "#4f8fff",
};

const workloads = [
  {
    label: "TARGET APPLICATION",
    title: "Order book / financial state replication",
    body: "Example workload where fast-changing state needs careful evidence boundaries, auditability, and workload-specific validation.",
  },
  {
    label: "TARGET APPLICATION",
    title: "Edge sensor delta telemetry",
    body: "Constrained systems where full payloads, repeated scans, and limited backhaul make state movement expensive.",
  },
  {
    label: "TARGET APPLICATION",
    title: "Distributed database replica sync",
    body: "Systems that repeatedly ask what changed across nodes, logs, or replicas before applying useful work.",
  },
  {
    label: "TARGET APPLICATION",
    title: "Agent memory / context updates",
    body: "Agentic systems where context churn, memory updates, and relevance pruning may benefit from delta-aware evaluation.",
  },
  {
    label: "TARGET APPLICATION",
    title: "Display / UI dirty-region rendering",
    body: "Interactive surfaces where meaningful visual changes can be tracked without treating concept art as current product proof.",
  },
  {
    label: "TARGET APPLICATION",
    title: "Rollback-sensitive execution",
    body: "Runtime paths where undo, replay, checkpoints, or recovery logic add operational and latency overhead.",
  },
];

export default function CaseStudiesPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav />

      <section className="mx-auto max-w-5xl px-6 pb-12 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Target workloads
        </p>
        <h1 className="mt-4 max-w-3xl text-4xl font-bold md:text-5xl">
          Where ATOMiK applies.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          These are illustrative target applications for discovery and evaluation. They are not external deployments, production results, logos, savings claims, or named references.
        </p>
      </section>

      <section className="mx-auto grid max-w-5xl gap-4 px-6 pb-12 md:grid-cols-3">
        {workloads.map((workload) => (
          <article key={workload.title} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{workload.label}</p>
            <h2 className="mt-3 text-lg font-bold">{workload.title}</h2>
            <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{workload.body}</p>
          </article>
        ))}
      </section>

      <section className="mx-auto max-w-5xl px-6 pb-16">
        <div className="rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h2 className="text-2xl font-bold">Bring a real workload</h2>
          <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
            A design-partner evaluation starts with baseline architecture, current bottleneck metrics, state size, update cadence, rollback or recovery path, and deployment constraints.
          </p>
          <Link
            href="/contact?intent=design-partner"
            className="mt-5 inline-flex rounded-lg px-4 py-2 text-sm font-semibold text-white no-underline"
            style={{ background: colors.blue }}
          >
            Discuss Design Partnership
          </Link>
        </div>
      </section>
    </div>
  );
}
