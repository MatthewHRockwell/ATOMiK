import type { Metadata } from "next";
import Link from "next/link";

export const metadata: Metadata = {
  title: "Hardware Proof - ATOMiK Docs",
  description:
    "ATOMiK hardware proof map separating live measurements, hardware validation, build artifacts, and synthesis output.",
  openGraph: {
    title: "ATOMiK Hardware Proof Map",
    description:
      "Live prototype, hardware validation, board-run measurements, synthesis output, and current standalone boot build artifacts.",
    url: "https://atomik.tech/docs/hardware",
    images: [{ url: "https://atomik.tech/10-current-live-atomik-desk-v040a.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK Hardware Proof Map",
    description:
      "Hardware validation and synthesis artifacts kept separate from roadmap claims.",
    images: ["https://atomik.tech/10-current-live-atomik-desk-v040a.png"],
  },
};

const artifacts = [
  {
    label: "HARDWARE_VALIDATED",
    title: "Current v0.40-A live prototype screenshot",
    href: "/10-current-live-atomik-desk-v040a.png",
    body: "ATOMiK Desk v0.40-A UI captured live from /dev/fb0 on Zynq hardware, with on-screen metrics driven by real measured on-board data. HARDWARE_VALIDATED UI ARTIFACT - NOT A CUSTOMER WORKLOAD BENCHMARK.",
  },
  {
    label: "BUILD_ARTIFACT",
    title: "Standalone SD boot build artifacts",
    href: "#standalone-boot",
    body: "Local BOOT.bin and probe notes exist for the standalone SD boot path. Keep this as a build artifact until public power-on logs support a stronger claim.",
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
        This page separates hardware validation, build artifacts, synthesis output, UI artifacts, and roadmap language. Use exact performance numbers only when the linked artifact supports them.
      </p>

      <div className="mt-8 grid gap-4 md:grid-cols-2">
        {artifacts.map((artifact) => (
          <Link key={artifact.title} href={artifact.href} className="rounded-lg p-5 no-underline" style={{ background: "#12121a", border: "1px solid #1d324a", color: "#f4f8ff" }}>
            <p className="text-[11px] font-semibold uppercase" style={{ color: "#22d3ee" }}>{artifact.label.replace(/_/g, " ")}</p>
            <h2 className="mt-2 text-lg font-bold">{artifact.title}</h2>
            <p className="mt-2 text-sm leading-6" style={{ color: "#9fb1c7" }}>{artifact.body}</p>
          </Link>
        ))}
      </div>

      <section id="standalone-boot" className="mt-8 rounded-lg p-5" style={{ background: "#12121a", border: "1px solid #1d324a" }}>
        <p className="text-[11px] font-semibold uppercase" style={{ color: "#22d3ee" }}>
          CURRENT HARDWARE GATE
        </p>
        <h2 className="mt-2 text-lg font-bold">Standalone SD boot artifact boundary</h2>
        <p className="mt-2 text-sm leading-6" style={{ color: "#9fb1c7" }}>
          Local build output and probe notes exist for the standalone SD boot path. This public page does not link to unpublished probe logs; once power-on logs, FSBL exception handling, final autonomous handoff, and public artifacts are all present, the evidence link can be promoted.
        </p>
      </section>
    </div>
  );
}
