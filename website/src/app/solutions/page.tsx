import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import {
  fitSignals,
  noFitSignals,
  statusQuoAlternatives,
} from "@/lib/messaging";

export const metadata: Metadata = {
  title: "Use Cases - ATOMiK",
  description:
    "ATOMiK evaluation paths for edge, embedded, AI-at-the-edge, remote, industrial, robotics, IoT, data center, and licensing teams.",
  openGraph: {
    title: "ATOMiK Use Cases - Edge and Embedded State Movement",
    description:
      "Use-case pages centered on what the customer brings, what ATOMiK evaluates, metrics, fit signals, no-fit signals, and proof needed.",
    url: "https://atomik.tech/solutions",
    images: [{ url: "https://atomik.tech/09-current-live-atomik-desk-v039k.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK Use Cases - Edge and Embedded State Movement",
    description:
      "Start with one state-heavy workload, one baseline, and one painful constraint.",
    images: ["https://atomik.tech/09-current-live-atomik-desk-v039k.png"],
  },
};

const colors = {
  bg: "#070807",
  panel: "#10130f",
  panel2: "#17130f",
  border: "#2d3a34",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  cyan: "#22d3ee",
  blue: "#2563eb",
  green: "#2dd36f",
  amber: "#f5a524",
  rose: "#fb7185",
  violet: "#c084fc",
};

const useCases = [
  {
    label: "Lead ICP",
    title: "Edge and embedded systems",
    pain:
      "Battery, heat, bandwidth, latency, footprint, weight, reliability, or field-service constraints are already measurable.",
    bring:
      "A representative state-heavy workload, current baseline, state size, update cadence, target hardware, and decision threshold.",
    evaluates:
      "Full-state movement, repeated scans, repeated replays, sync overhead, reconstruction cost, and coalescing potential.",
    metrics:
      "Bytes moved, full-state transfers avoided, operations coalesced, cycles/update, latency, power proxy, thermal proxy, correctness.",
    proof:
      "Workload map first, then proof review or benchmark exchange. Public claims require linked artifact and evidence label.",
    fit: "The state path is the expensive part and correctness can be checked.",
    noFit: "The workload is mostly stateless compute or the baseline cannot be measured.",
    href: "/contact?intent=evaluation&source=solutions-edge&cta=request-evaluation",
    cta: "Request Evaluation",
    accent: colors.green,
  },
  {
    label: "Lead ICP",
    title: "AI at the edge and agent context",
    pain:
      "Context movement, state retention, local response latency, memory pressure, or network dependence constrains the product.",
    bring:
      "Context/state model, update trace, response target, baseline path, target device, and correctness criteria.",
    evaluates:
      "Whether meaningful changes can be tracked without repeatedly moving or rebuilding full context/state.",
    metrics:
      "Bytes moved, update latency, response latency, memory footprint, operations coalesced, correctness preservation.",
    proof:
      "Current proof is systems-layer and workload-specific. Do not imply board-integrated AI inference without a matching artifact.",
    fit: "The bottleneck is context/state movement around the AI path.",
    noFit: "The bottleneck is only model math and not state movement.",
    href: "/contact?intent=evaluation&source=solutions-ai-edge&cta=request-evaluation",
    cta: "Request Evaluation",
    accent: colors.violet,
  },
  {
    label: "Lead ICP",
    title: "Robotics, industrial, and field control",
    pain:
      "Local update latency, reliability, power envelope, and disconnected operation create hard constraints.",
    bring:
      "Control/state path, timing budget, update cadence, logs or traces, target controller, and failure modes.",
    evaluates:
      "State-transition boundaries, replay/reconstruction cost, avoidable scans, and local reconstruction opportunities.",
    metrics:
      "Update latency, cycles/update, state scans avoided, reconstruction cost, duty cycle, correctness.",
    proof:
      "A responsible path starts with software/workload mapping before any hardware-backed claim.",
    fit: "The system repeatedly rebuilds or scans state to make local decisions.",
    noFit: "The process cannot tolerate any architecture change or lacks a test oracle.",
    href: "/contact?intent=evaluation&source=solutions-industrial&cta=request-evaluation",
    cta: "Request Evaluation",
    accent: colors.amber,
  },
  {
    label: "Secondary path",
    title: "Remote, IoT, and defense-adjacent systems",
    pain:
      "Every packet, watt, ounce, and field-service event matters when connectivity and power are limited.",
    bring:
      "Telemetry path, sync cadence, link budget, power model if available, target hardware, and reliability constraints.",
    evaluates:
      "Bytes avoided, wake-up frequency, duty cycle, local state preservation, and sync/replay cost.",
    metrics:
      "Bandwidth pressure, bytes avoided, wake-up frequency, duty cycle, power proxy, footprint pressure, correctness.",
    proof:
      "Claims must stay mission- and workload-specific. Concept visuals are not proof of deployment readiness.",
    fit: "State movement causes bandwidth, power, reliability, or weight pain.",
    noFit: "The constraint is regulatory or mechanical with no state-path lever.",
    href: "/contact?intent=evaluation&source=solutions-remote&cta=request-evaluation",
    cta: "Request Evaluation",
    accent: colors.cyan,
  },
  {
    label: "Strategic path",
    title: "Data center and infrastructure partners",
    pain:
      "Cooling, water, density, bandwidth, utilization, and infrastructure cost can become expensive when state movement scales.",
    bring:
      "A specific state-heavy service path, baseline counters, utilization data, transfer volume, and the constraint to evaluate.",
    evaluates:
      "Whether state movement rather than generic compute is creating measurable waste.",
    metrics:
      "Bytes moved, operations coalesced, cycles/update, memory footprint, bandwidth pressure, thermal proxy.",
    proof:
      "Thermal, water, and power language must remain evaluation-target language until measured for the workload.",
    fit: "A state path is driving a cost center large enough to evaluate.",
    noFit: "The workload is dominated by unrelated compute, storage, or facilities constraints.",
    href: "/contact?intent=evaluation&source=solutions-infrastructure&cta=request-evaluation",
    cta: "Request Evaluation",
    accent: colors.rose,
  },
  {
    label: "Strategic path",
    title: "Chip, IP, and licensing partners",
    pain:
      "A partner needs to know whether state-aware execution belongs in an accelerator, embedded IP block, or architecture roadmap.",
    bring:
      "Target integration context, workload class, baseline architecture, proof requirements, and commercial decision path.",
    evaluates:
      "IP fit, evidence gaps, ASIC feasibility, synthesis artifacts, and workload-specific proof requirements.",
    metrics:
      "Cycles/update, operation coalescing, area/footprint pressure, synthesis evidence, correctness, integration risk.",
    proof:
      "Separate live measured, hardware validated, synthesis validated, projected, conceptual, and roadmap claims.",
    fit: "The partner has a state-heavy architecture problem and a defined diligence path.",
    noFit: "The ask depends on production-ready IP claims not supported by current evidence.",
    href: "/contact?intent=licensing&source=solutions-licensing&cta=discuss-licensing",
    cta: "Discuss Licensing",
    accent: colors.blue,
  },
];

export default function SolutionsPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Use Cases" />

      <section className="mx-auto max-w-6xl px-6 pb-12 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Use cases
        </p>
        <h1 className="mt-4 max-w-4xl text-4xl font-bold leading-tight md:text-6xl">
          ATOMiK is for teams whose state movement already hurts.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          The lead path is edge and embedded evaluation. Data center, infrastructure, licensing, and investor routes remain important, but the first question is always the same: can the team provide one workload, one baseline, and one expensive constraint?
        </p>
        <div className="mt-7 flex flex-col gap-3 sm:flex-row">
          <Link href="/contact?intent=evaluation&source=solutions-hero&cta=request-evaluation" className="rounded px-5 py-3 text-center text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Request Evaluation
          </Link>
          <Link href="/benchmarks" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
            Review Proof
          </Link>
        </div>
      </section>

      <section className="mx-auto grid max-w-6xl gap-5 px-6 pb-14 md:grid-cols-2">
        {useCases.map((item) => (
          <article key={item.title} className="rounded p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <div className="mb-5 h-1 w-20 rounded" style={{ background: item.accent }} />
            <p className="text-[11px] font-semibold uppercase" style={{ color: item.accent }}>{item.label}</p>
            <h2 className="mt-2 text-2xl font-bold leading-snug">{item.title}</h2>
            <div className="mt-5 space-y-3 text-sm leading-6" style={{ color: colors.muted }}>
              <p><strong style={{ color: colors.text }}>Pain:</strong> {item.pain}</p>
              <p><strong style={{ color: colors.text }}>What customer brings:</strong> {item.bring}</p>
              <p><strong style={{ color: colors.text }}>What ATOMiK evaluates:</strong> {item.evaluates}</p>
              <p><strong style={{ color: colors.text }}>Likely metrics:</strong> {item.metrics}</p>
              <p><strong style={{ color: colors.text }}>Proof needed:</strong> {item.proof}</p>
              <p><strong style={{ color: colors.text }}>Fit signal:</strong> {item.fit}</p>
              <p><strong style={{ color: colors.text }}>No-fit signal:</strong> {item.noFit}</p>
            </div>
            <Link href={item.href} className="mt-6 inline-flex rounded px-4 py-2 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
              {item.cta}
            </Link>
          </article>
        ))}
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="grid gap-5 md:grid-cols-2">
            <article className="rounded p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
              <p className="text-sm font-semibold uppercase" style={{ color: colors.green }}>Strong fit signals</p>
              <div className="mt-5 grid gap-3">
                {fitSignals.map((signal) => (
                  <p key={signal} className="text-sm leading-6" style={{ color: colors.muted }}>{signal}</p>
                ))}
              </div>
            </article>
            <article className="rounded p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
              <p className="text-sm font-semibold uppercase" style={{ color: colors.rose }}>No-fit signals</p>
              <div className="mt-5 grid gap-3">
                {noFitSignals.map((signal) => (
                  <p key={signal} className="text-sm leading-6" style={{ color: colors.muted }}>{signal}</p>
                ))}
              </div>
            </article>
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Competition and status quo</p>
            <h2 className="mt-3 text-3xl font-bold md:text-4xl">The real alternative is often more of the old workaround.</h2>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              ATOMiK should be compared against what teams actually do today: add compute, batteries, cooling, bandwidth, compression, caching, sync machinery, accelerators, cloud offload, overbuilt hardware, manual optimization, or feature cuts.
            </p>
          </div>
          <div className="mt-8 overflow-x-auto rounded" style={{ border: `1px solid ${colors.border}` }}>
            <table className="w-full min-w-[1180px] border-collapse text-left text-sm">
              <thead style={{ background: colors.panel2 }}>
                <tr>
                  <th className="px-4 py-3 font-semibold">Status quo</th>
                  <th className="px-4 py-3 font-semibold">Solves</th>
                  <th className="px-4 py-3 font-semibold">Does not solve</th>
                  <th className="px-4 py-3 font-semibold">Why teams choose it</th>
                  <th className="px-4 py-3 font-semibold">Where it breaks</th>
                  <th className="px-4 py-3 font-semibold">ATOMiK difference</th>
                  <th className="px-4 py-3 font-semibold">Proof required</th>
                </tr>
              </thead>
              <tbody>
                {statusQuoAlternatives.map((item) => (
                  <tr key={item.name} style={{ borderTop: `1px solid ${colors.border}` }}>
                    <td className="px-4 py-3 font-semibold" style={{ color: colors.text }}>{item.name}</td>
                    <td className="px-4 py-3" style={{ color: colors.muted }}>{item.solves}</td>
                    <td className="px-4 py-3" style={{ color: colors.muted }}>{item.notSolve}</td>
                    <td className="px-4 py-3" style={{ color: colors.muted }}>{item.whyTeamsChoose}</td>
                    <td className="px-4 py-3" style={{ color: colors.muted }}>{item.insufficient}</td>
                    <td className="px-4 py-3" style={{ color: colors.muted }}>{item.atomikDiffers}</td>
                    <td className="px-4 py-3" style={{ color: colors.faint }}>{item.proofRequired}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="rounded p-8" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
          <h2 className="text-3xl font-bold">Start with the constrained state path.</h2>
          <p className="mt-4 max-w-3xl text-sm leading-6" style={{ color: colors.muted }}>
            The best first call is not a broad demo. It is a decision about whether one state-heavy workload has enough pain and evidence to justify deeper evaluation.
          </p>
          <div className="mt-6 flex flex-col gap-3 sm:flex-row">
            <Link href="/contact?intent=evaluation&source=solutions-final&cta=request-evaluation" className="rounded px-5 py-3 text-center text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
              Request Evaluation
            </Link>
            <Link href="/contact?intent=licensing&source=solutions-final&cta=discuss-licensing" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
              Discuss Licensing
            </Link>
          </div>
        </div>
      </section>
    </div>
  );
}
