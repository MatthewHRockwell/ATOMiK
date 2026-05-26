import type { Metadata } from "next";
import type { CSSProperties, ReactNode } from "react";
import Link from "next/link";
import Nav from "@/components/Nav";
import { proofToday } from "@/lib/messaging";

export const metadata: Metadata = {
  title: "Proof & Benchmarks - ATOMiK",
  description:
    "ATOMiK proof packet, evidence labels, claims registry, live measured artifacts, hardware validation, synthesis artifacts, and roadmap separation.",
  openGraph: {
    title: "ATOMiK Proof and Benchmarks",
    description:
      "How we know ATOMiK works today, what each artifact proves, and what it does not prove.",
    url: "https://atomik.tech/benchmarks",
    images: [{ url: "https://atomik.tech/09-current-live-atomik-desk-v039k.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK Proof and Benchmarks",
    description:
      "Quote the artifact, context, and caveat. Do not isolate the biggest number.",
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
};

const evidenceLabels = [
  ["LIVE_MEASURED", "Observed on a running system with recorded measurement artifacts."],
  ["HARDWARE_VALIDATED", "Demonstrated on physical hardware without implying production readiness."],
  ["SOFTWARE_VALIDATED", "Shown in software prototype, simulation, local runtime, or formal work."],
  ["SYNTHESIS_VALIDATED", "Supported by toolchain output, separate from live-board measurements."],
  ["BUILD_ARTIFACT", "Concrete local build output exists, but the full path is not promoted as live proof."],
  ["PROJECTED", "A model or estimate. It must not be phrased as an observed result."],
  ["CONCEPTUAL", "Explains direction or UX. It is not current functionality."],
  ["ROADMAP", "Planned work that may change."],
];

const artifactInterpretations = [
  {
    title: "AX7020 performance matrix",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/perf_matrix_ax7020_20260509.txt",
    label: "LIVE_MEASURED",
    proves:
      "A recorded board run produced software, direct hardware, batched, and profiled results on the AX7020 path.",
    doesNotProve:
      "It does not prove ATOMiK is always faster. The interpretation shows wins in specific coalesced/batched scenarios and losses in others.",
  },
  {
    title: "AX7020 interpretation",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/perf/20260509_matrix_interpretation.md",
    label: "LIVE_MEASURED",
    proves:
      "The honest readout: naive hardware access can lose, batching can help modestly, and coalescing can drive the meaningful win when the workload fits.",
    doesNotProve:
      "It does not validate SYNC bytes-avoided behavior in the current matrix because the document records that limitation.",
  },
  {
    title: "Linux userspace to FPGA validation",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/LINUX_USERSPACE_PROOF.md",
    label: "HARDWARE_VALIDATED",
    proves:
      "ATOMiK algebraic checks passed through a Linux userspace to kernel/MMIO/Wishbone/FPGA path.",
    doesNotProve:
      "It does not by itself prove production readiness, battery improvement, or thermal improvement.",
  },
  {
    title: "Hardware synthesis notes",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/HARDWARE_SYNTHESIS.md",
    label: "SYNTHESIS_VALIDATED",
    proves:
      "Toolchain and hardware/synthesis characterization exists for parallel accumulator configurations.",
    doesNotProve:
      "Synthesis ceilings must not be quoted as live-board performance unless a matching board-run artifact exists.",
  },
  {
    title: "Claims registry",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml",
    label: "CLAIM CONTROL",
    proves:
      "Public-safe claims, labels, artifacts, caveats, and notes are tracked in one registry.",
    doesNotProve:
      "The registry is a control surface, not a substitute for the linked raw artifacts.",
  },
  {
    title: "Formal proof work",
    href: "https://github.com/MatthewHRockwell/ATOMiK/tree/main/math/proofs",
    label: "SOFTWARE_VALIDATED",
    proves:
      "Formal proof work is present in the repository and can be reviewed in technical diligence.",
    doesNotProve:
      "Public pages should not repeat unaudited proof counts unless the count is verified across repo, site, and deck.",
  },
];

function EvidenceLink({
  href,
  className,
  style,
  children,
}: {
  href: string;
  className?: string;
  style?: CSSProperties;
  children: ReactNode;
}) {
  if (href.startsWith("http")) {
    return (
      <a href={href} className={className} style={style}>
        {children}
      </a>
    );
  }
  return (
    <Link href={href} className={className} style={style}>
      {children}
    </Link>
  );
}

export default function BenchmarksPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Proof" />

      <section className="mx-auto max-w-6xl px-6 pb-12 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          How we know ATOMiK works today
        </p>
        <h1 className="mt-4 max-w-4xl text-4xl font-bold leading-tight md:text-6xl">
          Quote the artifact, context, and caveat.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          ATOMiK proof is organized by evidence tier. Performance claims must be quoted only with their artifact, workload context, and caveat. Do not isolate the biggest number without the interpretation.
        </p>
        <div className="mt-7 flex flex-col gap-3 sm:flex-row">
          <Link href="/contact?intent=proof&source=proof-hero&cta=request-proof-review" className="rounded px-5 py-3 text-center text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Request Proof Review
          </Link>
          <Link href="/pricing" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
            Request Evaluation
          </Link>
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="max-w-3xl">
          <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Public proof packet</p>
          <h2 className="mt-3 text-3xl font-bold md:text-4xl">Current evidence is useful because it is labeled.</h2>
          <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
            The important interpretation is that ATOMiK is evaluated workload by workload. The architecture is strongest where state movement, repeated scans, full-state sync, replay, or reconstruction dominate the cost.
          </p>
        </div>
        <div className="mt-8 grid gap-4 md:grid-cols-3">
          {proofToday.map((item) => (
            <EvidenceLink key={item.title} href={item.href} className="rounded p-5 no-underline" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.text }}>
              <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{item.label}</p>
              <h3 className="mt-2 text-lg font-bold">{item.title}</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
            </EvidenceLink>
          ))}
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Evidence labels</p>
        <div className="mt-6 grid gap-4 md:grid-cols-4">
          {evidenceLabels.map(([label, body]) => (
            <article key={label} className="rounded p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
              <p className="text-[11px] font-semibold uppercase" style={{ color: colors.green }}>{label.replace(/_/g, " ")}</p>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{body}</p>
            </article>
          ))}
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>What each proof does and does not prove</p>
        <div className="mt-6 grid gap-4 md:grid-cols-2">
          {artifactInterpretations.map((artifact) => (
            <EvidenceLink key={artifact.title} href={artifact.href} className="rounded p-5 no-underline" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.text }}>
              <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{artifact.label}</p>
              <h2 className="mt-2 text-xl font-bold">{artifact.title}</h2>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
                <strong style={{ color: colors.text }}>What it supports:</strong> {artifact.proves}
              </p>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
                <strong style={{ color: colors.text }}>What it does not support:</strong> {artifact.doesNotProve}
              </p>
            </EvidenceLink>
          ))}
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="rounded p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
          <p className="text-sm font-semibold uppercase" style={{ color: colors.amber }}>Performance warning</p>
          <h2 className="mt-3 text-2xl font-bold">The AX7020 matrix is not a universal speedup claim.</h2>
          <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
            The current interpretation says ATOMiK can win when batching and coalescing match the workload, and can lose when naive hardware access or high unique-region ratios dominate. The honest claim is workload-specific: ATOMiK is strongest where state movement, repeated scans, full-state sync, replay, or reconstruction dominate the cost.
          </p>
          <div className="mt-5 grid gap-3 md:grid-cols-3">
            {[
              "Do not say ATOMiK is always faster.",
              "Do not claim proven heat, battery, water, or footprint improvements without artifact-backed measurement.",
              "Do not treat concept visuals or synthesis ceilings as live performance proof.",
            ].map((item) => (
              <p key={item} className="text-sm leading-6" style={{ color: colors.muted }}>
                <span style={{ color: colors.rose }}>-</span> {item}
              </p>
            ))}
          </div>
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="rounded p-8" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h2 className="text-3xl font-bold">Need proof for your workload?</h2>
          <p className="mt-4 max-w-3xl text-sm leading-6" style={{ color: colors.muted }}>
            Bring one workload, one baseline, and one constraint. ATOMiK will map the evidence tier required before making any public or diligence claim.
          </p>
          <Link href="/contact?intent=evaluation&source=proof-final&cta=request-evaluation" className="mt-6 inline-flex rounded px-5 py-3 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Request Evaluation
          </Link>
        </div>
      </section>
    </div>
  );
}
