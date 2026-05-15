import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Evidence & Benchmarks - ATOMiK",
  description:
    "ATOMiK benchmark and proof artifact map with evidence labels.",
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

const artifacts = [
  {
    label: "LIVE_MEASURED",
    title: "AX7020 performance matrix",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/perf_matrix_ax7020_20260509.txt",
    body: "Raw board-run output. Read with the interpretation notes before quoting any result.",
  },
  {
    label: "LIVE_MEASURED",
    title: "AX7020 interpretation",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/perf/20260509_matrix_interpretation.md",
    body: "Documents where ATOMiK helps, where software wins, and known measurement limitations.",
  },
  {
    label: "HARDWARE_VALIDATED",
    title: "Live AX7020/Zynq prototype path",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/LINUX_USERSPACE_PROOF.md",
    body: "Validation path through Linux userspace, kernel, bus, and FPGA accelerator. Live demo results are being consolidated into reproducible artifacts.",
  },
  {
    label: "SYNTHESIS_VALIDATED",
    title: "Zynq synthesis ceiling characterization",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/HARDWARE_SYNTHESIS.md",
    body: "Toolchain output and synthesis characterization. Keep separate from live-board claims.",
  },
  {
    label: "HARDWARE_VALIDATED",
    title: "ATOMiK Desk / Resource Fabric prototype",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/website/public/08-current-live-atomik-desk-v038i.png",
    body: "Current live prototype screenshot. Use as visual proof of the demo surface, not as benchmark or maturity evidence.",
  },
  {
    label: "ROADMAP",
    title: "First-silicon evaluation path",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/roadmap.md",
    body: "First-silicon work is preparation toward an evaluation chip. Do not describe it as a completed silicon artifact.",
  },
];

export default function BenchmarksPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Proof" />

      <section className="mx-auto max-w-5xl px-6 pb-12 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Evidence and benchmarks
        </p>
        <h1 className="mt-4 max-w-3xl text-4xl font-bold md:text-5xl">
          Quote the artifact, not the biggest number.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          ATOMiK benchmark material is organized by evidence tier. Measured board runs, hardware validation, software validation, synthesis output, projections, concepts, and roadmap items must stay separate. Zynq synthesis ceilings, live AX7020/Zynq prototype status, ATOMiK Desk demo proof, and first-silicon roadmap work are intentionally distinct.
        </p>
      </section>

      <section className="mx-auto max-w-5xl px-6 pb-8">
        <div className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h2 className="text-xl font-bold">Current Zynq / AX7020 status</h2>
          <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
            Zynq work is no longer described as simply pending. Public copy now separates synthesis-validated ceiling characterization, live AX7020/Zynq prototype progress, the current ATOMiK Desk / Resource Fabric demo surface, and roadmap work toward first silicon. Live AX7020/Zynq hardware demo results are being consolidated into reproducible artifacts before broader benchmark claims are published.
          </p>
        </div>
      </section>

      <section className="mx-auto grid max-w-5xl gap-4 px-6 pb-12 md:grid-cols-2">
        {artifacts.map((artifact) => (
          <Link key={artifact.title} href={artifact.href} className="rounded-lg p-5 no-underline" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.text }}>
            <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{artifact.label}</p>
            <h2 className="mt-2 text-lg font-bold">{artifact.title}</h2>
            <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{artifact.body}</p>
          </Link>
        ))}
      </section>

      <section className="mx-auto max-w-5xl px-6 pb-16">
        <div className="rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
          <h2 className="text-2xl font-bold">Need a workload-specific benchmark?</h2>
          <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
            Bring a workload, baseline, and target environment. ATOMiK will label any result by evidence tier and publish only approved artifacts.
          </p>
          <Link href="/contact?intent=evaluation" className="mt-5 inline-flex rounded-lg px-4 py-2 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Request Evaluation Access
          </Link>
        </div>
      </section>
    </div>
  );
}
