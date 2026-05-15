import type { Metadata } from "next";
import Link from "next/link";

export const metadata: Metadata = {
  title: "Hardware Proof - ATOMiK Docs",
  description:
    "ATOMiK hardware proof map separating live measurements, hardware validation, and synthesis output.",
};

const artifacts = [
  {
    label: "HARDWARE_VALIDATED",
    title: "Current v0.38-I live prototype screenshot",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/website/public/08-current-live-atomik-desk-v038i.png",
    body: "ATOMiK Desk v0.38-I prototype UI running on live hardware. UI claims are limited to what the screenshot shows.",
  },
  {
    label: "HARDWARE_VALIDATED",
    title: "Linux userspace validation",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/LINUX_USERSPACE_PROOF.md",
    body: "Hardware/software stack validation through Linux userspace to FPGA accelerator path.",
  },
  {
    label: "LIVE_MEASURED",
    title: "AX7020 board-run matrix",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/perf_matrix_ax7020_20260509.txt",
    body: "Raw board-run artifact. Interpret with documented caveats.",
  },
  {
    label: "SYNTHESIS_VALIDATED",
    title: "Synthesis ceilings",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/HARDWARE_SYNTHESIS.md",
    body: "Toolchain-derived ceilings. Do not present as live-board measurements.",
  },
];

export default function HardwarePage() {
  return (
    <div className="mx-auto max-w-5xl px-6 py-16">
      <p className="text-sm font-semibold uppercase" style={{ color: "#22d3ee" }}>
        Hardware proof
      </p>
      <h1 className="mt-4 text-4xl font-bold">Hardware validation map</h1>
      <p className="mt-4 max-w-3xl text-lg leading-8" style={{ color: "#8888a0" }}>
        This page separates hardware validation from synthesis output and roadmap language. Use exact performance numbers only when the linked artifact supports them.
      </p>

      <div className="mt-8 grid gap-4 md:grid-cols-2">
        {artifacts.map((artifact) => (
          <Link key={artifact.title} href={artifact.href} className="rounded-lg p-5 no-underline" style={{ background: "#12121a", border: "1px solid #1d324a", color: "#f4f8ff" }}>
            <p className="text-[11px] font-semibold uppercase" style={{ color: "#22d3ee" }}>{artifact.label}</p>
            <h2 className="mt-2 text-lg font-bold">{artifact.title}</h2>
            <p className="mt-2 text-sm leading-6" style={{ color: "#9fb1c7" }}>{artifact.body}</p>
          </Link>
        ))}
      </div>
    </div>
  );
}
