import { pageMetadata } from "@/lib/siteMetadata";
export const metadata = pageMetadata({
  title: 'Evaluation Compatibility — ATOMiK Docs',
  description: 'ATOMiK platform review matrix with evidence-scoped software, hardware, and evaluation surfaces.',
  path: '/docs/compatibility',
  openGraphTitle: 'ATOMiK Evaluation Compatibility',
});

/* ── Platform support data ─────────────────────────────────────────── */

const platforms = [
  {
    platform: "Python SDK",
    status: "Public",
    minVersion: "Python 3.9+",
    testedOn: "Public software path; verify package version before quoting support.",
  },
  {
    platform: "C99 Header",
    status: "Public",
    minVersion: "Any C99 compiler",
    testedOn: "Source-review path; compiler claims require current CI or local artifacts.",
  },
  {
    platform: "Linux Kernel Module",
    status: "Evaluation",
    minVersion: "Linux 5.15+",
    testedOn: "Request-based technical evaluation surface.",
  },
  {
    platform: "FPGA (Tang Nano 9K)",
    status: "Archived",
    minVersion: "Gowin EDA 1.9+",
    testedOn: "Historical prototype path; use current proof notes for public claims.",
  },
  {
    platform: "FPGA (Zynq)",
    status: "Validated",
    minVersion: "Vivado 2025.2+",
    testedOn: "AX7020 / XC7Z020 artifacts with proof-level labels.",
  },
  {
    platform: "npm Package",
    status: "Roadmap",
    minVersion: "Node.js 16+",
    testedOn: "Do not quote as a current public product path.",
  },
];

const statusColor: Record<string, { bg: string; text: string; border: string }> = {
  Public: { bg: "#22c55e18", text: "#22c55e", border: "#22c55e40" },
  Evaluation: { bg: "#4f8fff18", text: "#4f8fff", border: "#4f8fff40" },
  Archived: { bg: "#8b5cf618", text: "#8b5cf6", border: "#8b5cf640" },
  Validated: { bg: "#22d3ee18", text: "#22d3ee", border: "#22d3ee40" },
  Roadmap: { bg: "#d4a84318", text: "#d4a843", border: "#d4a84340" },
};

/* ── Reference architectures ───────────────────────────────────────── */

const refArchitectures = [
  {
    title: "State-heavy service path",
    description: "Review where repeated state diffing, replay, or sync work dominates.",
    color: "#4f8fff",
  },
  {
    title: "Edge / embedded path",
    description: "Scope battery, heat, backhaul, and local-latency constraints before claims.",
    color: "#22c55e",
  },
  {
    title: "Hardware proof path",
    description: "Map the workload to measured, hardware-validated, or synthesis-labeled artifacts.",
    color: "#d4a843",
  },
];

/* ── Cloud providers ───────────────────────────────────────────────── */

const cloudProviders = [
  {
    provider: "Software baseline",
    detail: "c7g.xlarge (Graviton3): run local benchmark before quoting throughput",
    color: "#ff9900",
  },
  {
    provider: "Hardware baseline",
    detail: "Use AX7020 artifacts and interpretation notes before quoting board behavior.",
    color: "#4285f4",
  },
  {
    provider: "Deployment target",
    detail: "Cloud, edge, and rack claims require a workload-specific benchmark exchange.",
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
        Evaluation Compatibility
      </h1>
      <p className="text-lg mb-12" style={{ color: "#8888a0" }}>
        Public review surfaces, hardware paths, and deployment targets with
        evidence boundaries. Treat this as an evaluation map, not a production
        support contract.
      </p>

      {/* ─── Section 1: Platform Review Matrix ─────────────────── */}
      <h2 className="text-2xl font-bold mb-6">Platform Review Matrix</h2>

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

      {/* ─── Section 2: Evaluation Patterns ─────────────────── */}
      <h2 className="text-2xl font-bold mb-6">Evaluation Patterns</h2>

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

      {/* ─── Section 3: Benchmark Boundary ───────────────── */}
      <h2 className="text-2xl font-bold mb-6">Benchmark Boundary</h2>

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

    </div>
  );
}
