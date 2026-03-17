import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Benchmarks — ATOMiK",
  description: "Verified ATOMiK benchmark results from real hardware. Reproducible Python benchmarks, FPGA synthesis data, and hardware test suites.",
};

/* ── Verified results ─────────────────────────────────────────────── */

const verifiedResults = [
  {
    platform: "AMD Ryzen 7 5700U (8 GB)",
    config: "Python 3.12 / Kubuntu 24.04",
    tier: "python" as const,
    results: [
      { metric: "Throughput (ACCUM)", value: "~5 M ops/s" },
      { metric: "Rollback (10k undo)", value: "~140x faster" },
      { metric: "Change detection", value: "~330,000x less memory" },
      { metric: "Bandwidth (1 MB state)", value: "131,072x reduction" },
    ],
    methodology: "python -m atomik_core benchmark",
    badge: "Verified" as const,
    note: "Developer's machine. Run the same command on your hardware to compare.",
  },
  {
    platform: "Tang Nano 9K (GW1NR-9K, $13.50)",
    config: "Single bank, N=1, 81 MHz core clock",
    tier: "fpga" as const,
    results: [
      { metric: "Throughput", value: "94.5 Mops/s" },
      { metric: "LOAD latency", value: "64 cycles" },
      { metric: "ACCUM latency", value: "70 cycles" },
      { metric: "READ latency", value: "99 cycles" },
      { metric: "LUT usage", value: "477 (5.5%)" },
    ],
    methodology: "Synthesis reports + 80/80 hardware tests",
    badge: "Verified" as const,
    note: "Production SoC: PicoRV32 @ 25.2 MHz + ATOMiK @ 81 MHz. Deterministic latency (stdev < 0.5 cycles).",
  },
  {
    platform: "Tang Nano 9K — 16 banks",
    config: "N=16 parallel banks, 66 MHz",
    tier: "fpga" as const,
    results: [
      { metric: "Throughput", value: "1,056 Mops/s" },
      { metric: "LUT usage", value: "1,779 (20.6%)" },
      { metric: "LUT scaling", value: "3.7x for 16x throughput" },
    ],
    methodology: "Synthesis sweep + 80/80 hardware tests",
    badge: "Verified" as const,
    note: "Sub-linear LUT scaling: ~65 LUT + 64 FF per additional bank.",
  },
  {
    platform: "Xilinx Zynq XC7Z020",
    config: "N=512 parallel banks, 135.6 MHz",
    tier: "fpga" as const,
    results: [
      { metric: "Throughput", value: "69.7 Gops/s" },
      { metric: "LUT usage", value: "23,542 (44.3%)" },
      { metric: "Fmax (N=1)", value: "444.4 MHz" },
      { metric: "Marginal LUT/bank", value: "~34" },
    ],
    methodology: "Vivado synthesis ceiling characterization (6 configs, 4 strategies each)",
    badge: "Projected" as const,
    note: "Synthesis-validated. Board integration pending (AX7020 in hand). N=512 is the XC7Z020 placement limit.",
  },
];

const badgeStyles: Record<string, { bg: string; text: string }> = {
  Verified: { bg: "rgba(34,197,94,0.12)", text: "#22c55e" },
  Projected: { bg: "rgba(234,179,8,0.12)", text: "#eab308" },
};

const tierColors: Record<string, string> = {
  python: "#4f8fff",
  fpga: "#8b5cf6",
};

/* ── Page ─────────────────────────────────────────────────────────── */

export default function BenchmarksPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Header */}
      <section className="max-w-5xl mx-auto px-6 pt-20 pb-4">
        <h1 className="text-4xl font-bold tracking-tight mb-2">Benchmarks</h1>
        <p className="text-lg mb-6" style={{ color: "#8888a0" }}>
          Every number on this page is reproducible. No simulated hardware, no fabricated results.
        </p>
      </section>

      {/* Disclaimer */}
      <section className="max-w-5xl mx-auto px-6 pb-8">
        <div className="rounded-xl border p-5" style={{ background: "rgba(79,143,255,0.04)", borderColor: "rgba(79,143,255,0.15)" }}>
          <p className="text-sm" style={{ color: "#b0b0c0" }}>
            <strong style={{ color: "#e0e0e8" }}>Reproducibility guarantee:</strong>{" "}
            All Python results can be verified by running{" "}
            <code style={{ color: "#22d3ee" }}>python -m atomik_core benchmark</code>{" "}
            on your own machine. FPGA results are from Gowin/Vivado synthesis reports and hardware test suites
            (80/80 parallel bank tests, 9/9 ATOMiK core tests, 6/6 display pipeline tests). Synthesis reports
            and test logs are in the{" "}
            <a href="https://github.com/MatthewHRockwell/ATOMiK" style={{ color: "#4f8fff", textDecoration: "underline" }}>
              source repository
            </a>.
          </p>
        </div>
      </section>

      {/* Verified Results */}
      <section className="max-w-5xl mx-auto px-6 pb-12">
        <h2 className="text-2xl font-bold mb-6">Verified Results</h2>
        <div className="space-y-6">
          {verifiedResults.map((r, i) => (
            <div key={i} className="rounded-xl border overflow-hidden" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
              {/* Card header */}
              <div className="flex flex-wrap items-center gap-3 p-5 pb-0">
                <span className="inline-block w-2.5 h-2.5 rounded-full shrink-0" style={{ background: tierColors[r.tier] }} />
                <h3 className="font-semibold text-lg">{r.platform}</h3>
                <span
                  className="text-xs font-semibold px-2.5 py-0.5 rounded-full"
                  style={{ background: badgeStyles[r.badge].bg, color: badgeStyles[r.badge].text }}
                >
                  {r.badge}
                </span>
              </div>
              <div className="px-5 pt-1 pb-4">
                <p className="text-sm" style={{ color: "#8888a0" }}>{r.config}</p>
              </div>

              {/* Results table */}
              <div className="px-5 pb-4">
                <table className="w-full text-sm">
                  <tbody>
                    {r.results.map((m, j) => (
                      <tr key={j} style={{ borderTop: j === 0 ? "1px solid #1e1e2e" : "1px solid rgba(30,30,46,0.5)" }}>
                        <td className="py-2.5 pr-4" style={{ color: "#8888a0" }}>{m.metric}</td>
                        <td className="py-2.5 text-right font-mono font-bold" style={{ color: "#22d3ee" }}>{m.value}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>

              {/* Footer */}
              <div className="px-5 py-3" style={{ borderTop: "1px solid #1e1e2e", background: "rgba(255,255,255,0.01)" }}>
                <p className="text-xs" style={{ color: "#666" }}>
                  <strong style={{ color: "#8888a0" }}>Methodology:</strong> {r.methodology}
                </p>
                {r.note && (
                  <p className="text-xs mt-1" style={{ color: "#555" }}>{r.note}</p>
                )}
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Hardware acceleration tiers */}
      <section className="max-w-3xl mx-auto px-6 pb-12">
        <h2 className="text-2xl font-bold mb-4 text-center">Hardware acceleration tiers</h2>
        <p className="text-sm text-center mb-6" style={{ color: "#8888a0" }}>
          Same algebra at every tier. Code once, accelerate anywhere.
        </p>
        <div className="grid md:grid-cols-3 gap-4">
          {[
            { tier: "Python SDK", speed: "~5M ops/s", color: "#4f8fff", desc: "pip install, zero deps" },
            { tier: "C Library", speed: "~500M ops/s", color: "#22c55e", desc: "Single-header, any compiler" },
            { tier: "FPGA (Zynq N=512)", speed: "69.7G ops/s", color: "#8b5cf6", desc: "512 parallel banks, synthesis-validated" },
          ].map((t) => (
            <div key={t.tier} className="rounded-xl border p-5 text-center" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
              <div className="text-2xl font-bold font-mono" style={{ color: t.color }}>{t.speed}</div>
              <div className="font-semibold mt-1">{t.tier}</div>
              <div className="text-xs mt-1" style={{ color: "#8888a0" }}>{t.desc}</div>
            </div>
          ))}
        </div>
      </section>

      {/* Run it yourself */}
      <section className="max-w-3xl mx-auto px-6 pb-12">
        <h2 className="text-2xl font-bold mb-6 text-center">Run it yourself</h2>
        <div className="space-y-4">
          {[
            { step: "1", title: "Install", code: "pip install atomik-core" },
            { step: "2", title: "Benchmark", code: "python -m atomik_core benchmark" },
            { step: "3", title: "JSON output", code: "python -m atomik_core benchmark --json" },
          ].map((s) => (
            <div key={s.step} className="flex items-start gap-4 rounded-xl border p-5" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
              <span className="shrink-0 w-8 h-8 rounded-lg flex items-center justify-center text-sm font-bold" style={{ background: "rgba(79,143,255,0.1)", color: "#4f8fff" }}>{s.step}</span>
              <div>
                <div className="font-semibold mb-1">{s.title}</div>
                <code className="text-sm" style={{ color: "#22d3ee" }}>{s.code}</code>
              </div>
            </div>
          ))}
        </div>
        <p className="text-sm text-center mt-4" style={{ color: "#8888a0" }}>
          The benchmark suite runs five tests: rollback, change detection, multi-node convergence, bandwidth, and raw throughput.
          All operations are O(1) -- constant time regardless of state history.
        </p>
      </section>

      {/* CTA */}
      <section className="max-w-3xl mx-auto px-6 pb-24">
        <div className="rounded-xl border p-8 text-center" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
          <h3 className="text-xl font-bold mb-3">Run the benchmark on your machine and share your results</h3>
          <pre className="rounded-lg px-4 py-3 text-sm mb-6 inline-block" style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", color: "#22d3ee", fontFamily: "monospace" }}>
            pip install atomik-core && python -m atomik_core benchmark
          </pre>
          <div className="flex flex-wrap justify-center gap-4">
            <Link href="/get-started" className="px-6 py-2.5 rounded-lg text-sm font-semibold" style={{ background: "#4f8fff", color: "#fff" }}>Get Started</Link>
            <Link href="/pricing" className="px-6 py-2.5 rounded-lg text-sm font-semibold" style={{ background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" }}>View Pricing</Link>
          </div>
        </div>
      </section>
    </div>
  );
}
