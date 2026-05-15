import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Evidence & Security Posture - ATOMiK",
  description:
    "ATOMiK public evidence posture, security-review notes, and claim-labeling policy.",
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

const posture = [
  {
    title: "Evidence labels",
    body: "Public claims are labeled as LIVE_MEASURED, HARDWARE_VALIDATED, SOFTWARE_VALIDATED, SYNTHESIS_VALIDATED, PROJECTED, CONCEPTUAL, or ROADMAP.",
  },
  {
    title: "Local evaluation path",
    body: "ATOMiK software and proof artifacts can be inspected locally from the public repository. Hosted lead forms capture only submitted contact and workload context.",
  },
  {
    title: "Security review boundary",
    body: "Formal proof work is a technical assurance input, not a substitute for a full product security review, compliance audit, or deployment assessment.",
  },
  {
    title: "Commercial claims",
    body: "SOC, ISO, customer, production, and enterprise-readiness claims should not be made until reviewed and backed by appropriate artifacts.",
  },
];

export default function CompliancePage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav />

      <section className="mx-auto max-w-5xl px-6 pb-12 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Evidence and security posture
        </p>
        <h1 className="mt-4 max-w-3xl text-4xl font-bold md:text-5xl">
          Public assurance starts with labeled evidence.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          ATOMiK separates technical proof, hardware validation, synthesis output, projections, concepts, and roadmap items. This page does not claim completed compliance certifications.
        </p>
      </section>

      <section className="mx-auto grid max-w-5xl gap-4 px-6 pb-12 md:grid-cols-2">
        {posture.map((item) => (
          <article key={item.title} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <h2 className="text-lg font-bold">{item.title}</h2>
            <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
          </article>
        ))}
      </section>

      <section className="mx-auto max-w-5xl px-6 pb-16">
        <div className="rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h2 className="text-2xl font-bold">Review the evidence framework</h2>
          <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
            Live screenshots show current prototypes. Concept visuals show product direction and are not represented as current commercial functionality. Performance claims are only stated when backed by measured artifacts.
          </p>
          <div className="mt-5 flex flex-wrap gap-3">
            <Link href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/evidence-labels.md" className="rounded-lg px-4 py-2 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
              Evidence Labels
            </Link>
            <Link href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml" className="rounded-lg px-4 py-2 text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
              Claims Registry
            </Link>
          </div>
        </div>
      </section>
    </div>
  );
}
