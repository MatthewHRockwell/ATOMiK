import type { Metadata } from "next";
import { DocsNav } from "../shared";
import UpgradeGate from "@/components/UpgradeGate";

export const metadata: Metadata = {
  title: "Hardware — ATOMiK Docs",
  description:
    "ATOMiK FPGA deployment: Tang Nano 9K at 94.5 Mops/s and Xilinx Zynq XC7Z020 at 69.7 Gops/s. Sub-linear LUT scaling across 512 parallel banks.",
};

const platforms = [
  {
    name: "Tang Nano 9K",
    chip: "Gowin GW1NR-9K",
    price: "$13.50",
    desc: "Production SoC with PicoRV32 @ 25.2 MHz + ATOMiK @ 81 MHz. Custom RV64I CPU with native ATOMiK ISA extensions. 1280x720 HDMI output.",
    stats: "94.5 Mops/s \u00b7 477 LUT \u00b7 81 MHz",
    color: "#22c55e",
    border: "border-green-500/30",
  },
  {
    name: "Xilinx Zynq XC7Z020",
    chip: "ALINX AX7020",
    price: "~$100",
    desc: "512 parallel banks at 135.6 MHz. 444 MHz single-bank. Full ceiling characterization across 6 configs with 4 Vivado strategies each.",
    stats: "69.7 Gops/s \u00b7 23,542 LUT \u00b7 512 banks",
    color: "#d4a843",
    border: "border-yellow-500/30",
  },
];

export default function HardwarePage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#38bdf8" }}
      >
        Silicon
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">Hardware</h1>
      <p className="text-lg mb-10" style={{ color: "#8888a0" }}>
        ATOMiK runs on real silicon. FPGA-validated across two platforms, from $13.50 to 69.7 Gops/s.
      </p>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-8">
        {platforms.map((p) => (
          <div
            key={p.name}
            className={`rounded-xl p-6 border ${p.border} transition-all duration-300 hover:scale-[1.01]`}
            style={{ background: "#12121a" }}
          >
            <div className="flex items-center justify-between mb-3">
              <h3 className="text-lg font-bold" style={{ color: p.color }}>
                {p.name}
              </h3>
              <span
                className="text-xs font-mono px-2 py-0.5 rounded"
                style={{ background: "#0d0d14", color: "#8888a0" }}
              >
                {p.price}
              </span>
            </div>
            <p className="text-xs font-mono mb-3" style={{ color: "#555570" }}>
              {p.chip}
            </p>
            <p className="text-sm mb-4 leading-relaxed" style={{ color: "#b0b0c0" }}>
              {p.desc}
            </p>
            <div
              className="text-xs font-mono px-3 py-2 rounded-lg"
              style={{ background: "#0d0d14", color: p.color }}
            >
              {p.stats}
            </div>
          </div>
        ))}
      </div>

      {/* Scaling note */}
      <div
        className="rounded-xl p-6 border"
        style={{
          background: "linear-gradient(135deg, rgba(34,197,94,0.06), rgba(212,168,67,0.06))",
          borderColor: "#22c55e30",
        }}
      >
        <p className="text-sm font-semibold mb-2" style={{ color: "#22c55e" }}>
          Sub-linear LUT scaling
        </p>
        <p className="text-sm" style={{ color: "#b0b0c0" }}>
          Each additional bank costs ~34 LUTs (at N=512). 16x throughput requires only 3.7x more
          logic. All configurations share a single BRAM. The architecture is designed for parallel
          scaling — the accumulator is a shared resource by design because XOR commutativity means
          multiple producers can feed deltas in any order and the result is identical.
        </p>
      </div>

      <UpgradeGate
        tier="enterprise"
        title="Deploy on your FPGA"
        description="Enterprise customers get pre-built bitstreams, device tree overlays, integration consulting, and 4-hour SLA."
        ctaText="Contact Sales"
      />

      <DocsNav
        prev={{ href: "/docs/examples", label: "Examples" }}
      />
    </div>
  );
}
