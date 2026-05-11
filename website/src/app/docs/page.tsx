import type { Metadata } from "next";
import Link from "next/link";

export const metadata: Metadata = {
  title: "Documentation - ATOMiK",
  description:
    "ATOMiK public docs, evidence labels, proof artifacts, hardware validation, and roadmap concepts.",
};

const cards = [
  {
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/evidence-labels.md",
    title: "Evidence Labels",
    desc: "Definitions for LIVE_MEASURED, HARDWARE_VALIDATED, SOFTWARE_VALIDATED, SYNTHESIS_VALIDATED, PROJECTED, CONCEPTUAL, and ROADMAP.",
  },
  {
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/technical-proof.md",
    title: "Technical Proof",
    desc: "Artifact map for current proof, benchmark outputs, hardware validation, software proof, and concept material.",
  },
  {
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/hardware-validation.md",
    title: "Hardware Validation",
    desc: "Separation of live hardware proof, hardware validation, synthesis output, and roadmap hardware work.",
  },
  {
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/roadmap.md",
    title: "Roadmap",
    desc: "Live today, near-term evaluation path, and roadmap concepts with explicit proof boundaries.",
  },
  {
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/design-partners.md",
    title: "Design Partners",
    desc: "Evaluation access, paid technical evaluation, qualification questions, and expected deliverables.",
  },
  {
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml",
    title: "Claims Registry",
    desc: "Current source of truth for public claim labels, artifact paths, and publication status.",
  },
];

export default function DocsIndexPage() {
  return (
    <div className="mx-auto max-w-5xl px-6 py-16">
      <p className="text-sm font-semibold uppercase" style={{ color: "#22d3ee" }}>
        Developer documentation
      </p>
      <h1 className="mt-4 text-4xl font-bold md:text-5xl">Docs</h1>
      <p className="mt-4 max-w-3xl text-lg leading-8" style={{ color: "#8888a0" }}>
        Start with the evidence framework and proof map. Deep implementation docs remain available in the repository, but public claims should route through the labeled artifacts.
      </p>

      <div className="mt-8 grid grid-cols-1 gap-5 sm:grid-cols-2 lg:grid-cols-3">
        {cards.map((card) => (
          <Link
            key={card.href}
            href={card.href}
            className="rounded-lg p-6 no-underline"
            style={{ background: "#12121a", border: "1px solid #1d324a", color: "#f4f8ff" }}
          >
            <h2 className="text-lg font-bold" style={{ color: "#22d3ee" }}>{card.title}</h2>
            <p className="mt-3 text-sm leading-6" style={{ color: "#9fb1c7" }}>
              {card.desc}
            </p>
          </Link>
        ))}
      </div>
    </div>
  );
}
