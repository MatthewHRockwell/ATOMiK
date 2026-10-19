import { pageMetadata } from "@/lib/siteMetadata";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata = pageMetadata({
  title: 'Start Technical Evaluation - ATOMiK',
  description: 'Start a technical evaluation of ATOMiK with a workload, proof review, or design-partner conversation.',
  path: '/get-started',
  openGraphTitle: 'Start Technical Evaluation - ATOMiK',
});

const colors = {
  bg: "#070b12",
  panel: "#0d1420",
  border: "#1d324a",
  text: "#f4f8ff",
  muted: "#9fb1c7",
  cyan: "#22d3ee",
  blue: "#2563eb",
};

const steps = [
  {
    title: "Bring one workload",
    body: "Start with a concrete state-heavy path: sync, rollback, change detection, edge telemetry, or adaptive execution.",
  },
  {
    title: "Review the proof stack",
    body: "Separate live measurements, hardware validation, software proof, synthesis output, projections, concepts, and roadmap items.",
  },
  {
    title: "Scope the evaluation",
    body: "Define success criteria, required artifacts, environment assumptions, and the decision path: continue, refine, or stop.",
  },
];

const requestTypes = [
  ["Reserve Proof Review", "/pricing"],
  ["Request Evaluation Access", "/contact?intent=evaluation&source=get-started&cta=request-evaluation-access"],
  ["Discuss Design Partnership", "/contact?intent=design-partner&source=get-started&cta=discuss-design-partnership"],
];

export default function GetStartedPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav />

      <section className="mx-auto max-w-5xl px-6 pb-12 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Start technical evaluation
        </p>
        <h1 className="mt-4 max-w-3xl text-4xl font-bold md:text-5xl">
          Evaluate ATOMiK against a real state-heavy path.
        </h1>
        <p className="mt-5 max-w-2xl text-lg leading-8" style={{ color: colors.muted }}>
          The first step is not a generic install flow. It is a focused conversation around one workload, one bottleneck, and the evidence needed to decide whether state-aware execution is worth testing.
        </p>
      </section>

      <section className="mx-auto grid max-w-5xl gap-4 px-6 pb-12 md:grid-cols-3">
        {steps.map((step, index) => (
          <article key={step.title} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <div className="text-sm font-semibold" style={{ color: colors.cyan }}>
              0{index + 1}
            </div>
            <h2 className="mt-3 text-lg font-bold">{step.title}</h2>
            <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{step.body}</p>
          </article>
        ))}
      </section>

      <section className="mx-auto max-w-5xl px-6 pb-16">
        <div className="rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h2 className="text-2xl font-bold">Choose the right next step</h2>
          <div className="mt-5 flex flex-wrap gap-3">
            {requestTypes.map(([label, href]) => (
              <Link
                key={label}
                href={href}
                className="rounded-lg px-4 py-2 text-sm font-semibold no-underline transition-opacity hover:opacity-90"
                style={{ background: label === "Discuss Design Partnership" ? "transparent" : colors.blue, color: colors.text, border: label === "Discuss Design Partnership" ? `1px solid ${colors.border}` : "1px solid transparent" }}
              >
                {label}
              </Link>
            ))}
          </div>
          <p className="mt-5 text-xs leading-6" style={{ color: colors.muted }}>
Proof-review and technical-evaluation reservations hold focused review or scoping time. Larger design-partner and licensing work is still confirmed through written scope. Live screenshots show current prototypes, and performance claims are only stated when backed by measured artifacts.
          </p>
        </div>
      </section>
    </div>
  );
}
