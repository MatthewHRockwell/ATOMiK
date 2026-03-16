import type { Metadata } from "next";
import { DocsNav } from "../shared";

export const metadata: Metadata = {
  title: "Compatibility Matrix — ATOMiK Docs",
  description:
    "ATOMiK platform support matrix, reference architectures, and cloud provider deployment benchmarks.",
};

/* ── Platform support data ─────────────────────────────────────────── */

const platforms = [
  {
    platform: "Python SDK",
    status: "GA",
    minVersion: "Python 3.9+",
    testedOn: "Linux, macOS, Windows",
  },
  {
    platform: "C99 Header",
    status: "GA",
    minVersion: "Any C99 compiler",
    testedOn: "GCC, Clang, MSVC, ARM GCC, RISC-V GCC",
  },
  {
    platform: "Linux Kernel Module",
    status: "GA",
    minVersion: "Linux 5.15+",
    testedOn: "Ubuntu 22.04/24.04, Debian 12, RHEL 9",
  },
  {
    platform: "FPGA (Tang Nano 9K)",
    status: "GA",
    minVersion: "Gowin EDA 1.9+",
    testedOn: "GW1NR-9K (8,640 LUT)",
  },
  {
    platform: "FPGA (Zynq)",
    status: "Beta",
    minVersion: "Vivado 2025.2+",
    testedOn: "XC7Z020 (53,200 LUT)",
  },
  {
    platform: "npm Package",
    status: "Preview",
    minVersion: "Node.js 16+",
    testedOn: "\u2014",
  },
];

const statusColor: Record<string, { bg: string; text: string; border: string }> = {
  GA: { bg: "#22c55e18", text: "#22c55e", border: "#22c55e40" },
  Beta: { bg: "#d4a84318", text: "#d4a843", border: "#d4a84340" },
  Preview: { bg: "#4f8fff18", text: "#4f8fff", border: "#4f8fff40" },
};

/* ── Reference architectures ───────────────────────────────────────── */

const refArchitectures = [
  {
    title: "Microservices State Sync",
    description: "Python SDK + SyncTable + Redis adapter",
    color: "#4f8fff",
  },
  {
    title: "Edge-to-Cloud Pipeline",
    description: "Kernel module + IoT sensors + cgroup tracking",
    color: "#22c55e",
  },
  {
    title: "High-Throughput Data Plane",
    description: "FPGA + AXI4-Lite + 512 parallel banks",
    color: "#d4a843",
  },
];

/* ── Cloud providers ───────────────────────────────────────────────── */

const cloudProviders = [
  {
    provider: "AWS",
    detail: "c7g.xlarge (Graviton3): 6.3M ops/s, $0.17/hr",
    color: "#ff9900",
  },
  {
    provider: "GCP",
    detail: "c3-standard-4: estimated 7M ops/s",
    color: "#4285f4",
  },
  {
    provider: "Azure",
    detail: "Dv5 series: estimated 6.5M ops/s",
    color: "#00bcf2",
  },
];

/* ── Page ──────────────────────────────────────────────────────────── */

export default function CompatibilityPage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#22d3ee" }}
      >
        Compatibility
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">
        Hardware &amp; Platform Compatibility
      </h1>
      <p className="text-lg mb-12" style={{ color: "#8888a0" }}>
        Supported platforms, reference architectures, and cloud deployment
        benchmarks.
      </p>

      {/* ─── Section 1: Platform Support Matrix ─────────────────── */}
      <h2 className="text-2xl font-bold mb-6">Platform Support Matrix</h2>

      <div
        className="rounded-xl overflow-hidden mb-12 border"
        style={{ background: "#12121a", borderColor: "#1e1e2e" }}
      >
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead>
              <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                {["Platform", "Status", "Min Version", "Tested On"].map(
                  (h) => (
                    <th
                      key={h}
                      className="text-left px-5 py-3 font-mono text-xs tracking-wider uppercase"
                      style={{ color: "#555570" }}
                    >
                      {h}
                    </th>
                  )
                )}
              </tr>
            </thead>
            <tbody>
              {platforms.map((p) => {
                const sc = statusColor[p.status];
                return (
                  <tr
                    key={p.platform}
                    style={{ borderBottom: "1px solid #1e1e2e" }}
                  >
                    <td
                      className="px-5 py-3 font-semibold"
                      style={{ color: "#e0e0e8" }}
                    >
                      {p.platform}
                    </td>
                    <td className="px-5 py-3">
                      <span
                        className="inline-block text-xs font-mono font-semibold px-2 py-0.5 rounded"
                        style={{
                          background: sc.bg,
                          color: sc.text,
                          border: `1px solid ${sc.border}`,
                        }}
                      >
                        {p.status}
                      </span>
                    </td>
                    <td
                      className="px-5 py-3 font-mono text-xs"
                      style={{ color: "#b0b0c0" }}
                    >
                      {p.minVersion}
                    </td>
                    <td
                      className="px-5 py-3 text-xs"
                      style={{ color: "#8888a0" }}
                    >
                      {p.testedOn}
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
      </div>

      {/* ─── Section 2: Reference Architectures ─────────────────── */}
      <h2 className="text-2xl font-bold mb-6">Reference Architectures</h2>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-12">
        {refArchitectures.map((arch) => (
          <div
            key={arch.title}
            className="rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div
              className="w-2 h-2 rounded-full mb-4"
              style={{ background: arch.color }}
            />
            <h3
              className="text-base font-bold mb-2"
              style={{ color: arch.color }}
            >
              {arch.title}
            </h3>
            <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
              {arch.description}
            </p>
          </div>
        ))}
      </div>

      {/* ─── Section 3: Cloud Provider Deployment ───────────────── */}
      <h2 className="text-2xl font-bold mb-6">Cloud Provider Deployment</h2>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-8">
        {cloudProviders.map((cp) => (
          <div
            key={cp.provider}
            className="rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <h3
              className="text-lg font-bold mb-3"
              style={{ color: cp.color }}
            >
              {cp.provider}
            </h3>
            <p
              className="text-sm font-mono leading-relaxed"
              style={{ color: "#b0b0c0" }}
            >
              {cp.detail}
            </p>
          </div>
        ))}
      </div>

      <DocsNav
        prev={{ href: "/docs/hardware", label: "Hardware" }}
      />
    </div>
  );
}
