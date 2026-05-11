import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Technical Proof Brief - ATOMiK",
  description:
    "Artifact-first technical proof map for ATOMiK state-aware compute.",
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

const links = [
  ["Evidence labels", "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/evidence-labels.md"],
  ["Technical proof", "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/technical-proof.md"],
  ["Hardware validation", "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/hardware-validation.md"],
  ["Claims registry", "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml"],
];

export default function WhitepaperPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav />

      <section className="mx-auto max-w-5xl px-6 pb-12 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Technical proof brief
        </p>
        <h1 className="mt-4 max-w-3xl text-4xl font-bold md:text-5xl">
          Review ATOMiK by artifact, not by headline number.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          The public proof packet now separates live measurements, hardware validation, software validation, synthesis output, projections, concepts, and roadmap material.
        </p>
      </section>

      <section className="mx-auto grid max-w-5xl gap-4 px-6 pb-12 md:grid-cols-2">
        {links.map(([label, href]) => (
          <Link key={label} href={href} className="rounded-lg p-5 no-underline" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.text }}>
            <h2 className="text-lg font-bold">{label}</h2>
            <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>Open the public source document.</p>
          </Link>
        ))}
      </section>

      <section className="mx-auto max-w-5xl px-6 pb-16">
        <div className="rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h2 className="text-2xl font-bold">Request a tailored proof review</h2>
          <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
            If you have a state-heavy workload, the useful next step is a focused proof review or scoped evaluation rather than a generic white paper download.
          </p>
          <Link href="/contact?intent=evaluation" className="mt-5 inline-flex rounded-lg px-4 py-2 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Request Evaluation Access
          </Link>
        </div>
      </section>
    </div>
  );
}
