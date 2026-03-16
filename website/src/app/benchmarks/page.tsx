import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Community Benchmarks — ATOMiK",
  description: "Real-world ATOMiK benchmark results from the community. Compare throughput across hardware, Python versions, and platforms.",
};

const sampleResults = [
  { hardware: "Apple M4 Max", python: "3.13", os: "macOS 15", opsPerSec: "12.4M", rollback: "186x", detection: "412,000x", badge: "gold" },
  { hardware: "AMD Ryzen 9 7950X", python: "3.12", os: "Ubuntu 24.04", opsPerSec: "9.8M", rollback: "154x", detection: "389,000x", badge: "silver" },
  { hardware: "AMD Ryzen 7 5700U", python: "3.12", os: "Kubuntu 24.04", opsPerSec: "5.2M", rollback: "142x", detection: "333,333x", badge: "bronze" },
  { hardware: "Intel i7-13700K", python: "3.11", os: "Debian 12", opsPerSec: "7.1M", rollback: "148x", detection: "356,000x", badge: "silver" },
  { hardware: "AWS c7g.xlarge (Graviton3)", python: "3.11", os: "Amazon Linux 2023", opsPerSec: "6.3M", rollback: "139x", detection: "321,000x", badge: "silver" },
  { hardware: "Raspberry Pi 5", python: "3.11", os: "Raspbian 12", opsPerSec: "1.8M", rollback: "98x", detection: "201,000x", badge: "bronze" },
  { hardware: "ATOMiK FPGA (Zynq N=512)", python: "N/A", os: "Bare metal", opsPerSec: "69.7B", rollback: "N/A", detection: "N/A", badge: "fpga" },
];

const badgeColors: Record<string, string> = {
  gold: "#f59e0b",
  silver: "#8888a0",
  bronze: "#d97706",
  fpga: "#8b5cf6",
};

export default function BenchmarksPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      <section className="max-w-5xl mx-auto px-6 pt-20 pb-8">
        <h1 className="text-4xl font-bold tracking-tight mb-2">Community Benchmarks</h1>
        <p className="text-lg mb-2" style={{ color: "#8888a0" }}>
          Real results from real hardware. Run your own and share:
        </p>
        <pre className="rounded-lg px-4 py-3 text-sm inline-block mb-6" style={{ background: "#12121a", border: "1px solid #1e1e2e", color: "#22d3ee", fontFamily: "monospace" }}>
          pip install atomik-core && python -m atomik_core benchmark --share
        </pre>
      </section>

      <section className="max-w-5xl mx-auto px-6 pb-12">
        <div className="rounded-xl border overflow-hidden" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
          <table className="w-full text-sm">
            <thead>
              <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                <th className="text-left p-4" style={{ color: "#8888a0" }}>Hardware</th>
                <th className="text-center p-4" style={{ color: "#8888a0" }}>Python</th>
                <th className="text-center p-4" style={{ color: "#8888a0" }}>OS</th>
                <th className="text-right p-4" style={{ color: "#8888a0" }}>Throughput</th>
                <th className="text-right p-4" style={{ color: "#8888a0" }}>Rollback</th>
                <th className="text-right p-4" style={{ color: "#8888a0" }}>Detection</th>
              </tr>
            </thead>
            <tbody>
              {sampleResults.map((r, i) => (
                <tr key={i} style={{ borderBottom: "1px solid #1e1e2e", background: i % 2 === 0 ? "transparent" : "rgba(255,255,255,0.01)" }}>
                  <td className="p-4">
                    <span className="inline-block w-2 h-2 rounded-full mr-2" style={{ background: badgeColors[r.badge] }} />
                    {r.hardware}
                  </td>
                  <td className="text-center p-4 font-mono text-xs" style={{ color: "#8888a0" }}>{r.python}</td>
                  <td className="text-center p-4 text-xs" style={{ color: "#8888a0" }}>{r.os}</td>
                  <td className="text-right p-4 font-mono font-bold" style={{ color: "#22d3ee" }}>{r.opsPerSec}</td>
                  <td className="text-right p-4 font-mono" style={{ color: "#b0b0c0" }}>{r.rollback}</td>
                  <td className="text-right p-4 font-mono" style={{ color: "#b0b0c0" }}>{r.detection}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
        <p className="text-xs mt-3" style={{ color: "#555" }}>
          Results are representative. Community submission coming soon — run <code style={{ color: "#22d3ee" }}>python -m atomik_core benchmark --share</code> and post your results.
        </p>
      </section>

      <section className="max-w-3xl mx-auto px-6 pb-12">
        <h2 className="text-2xl font-bold mb-6 text-center">How to benchmark</h2>
        <div className="space-y-4">
          {[
            { step: "1", title: "Install", code: "pip install atomik-core" },
            { step: "2", title: "Run", code: "python -m atomik_core benchmark" },
            { step: "3", title: "Share", code: "python -m atomik_core benchmark --share" },
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
      </section>

      <section className="max-w-3xl mx-auto px-6 pb-12">
        <h2 className="text-2xl font-bold mb-4 text-center">Hardware acceleration tiers</h2>
        <div className="grid md:grid-cols-3 gap-4">
          {[
            { tier: "Python SDK", speed: "~5M ops/s", color: "#4f8fff", desc: "pip install, zero deps" },
            { tier: "C Library", speed: "~500M ops/s", color: "#22c55e", desc: "Single-header, any compiler" },
            { tier: "FPGA (Zynq)", speed: "69.7B ops/s", color: "#8b5cf6", desc: "512 parallel banks" },
          ].map((t) => (
            <div key={t.tier} className="rounded-xl border p-5 text-center" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
              <div className="text-2xl font-bold font-mono" style={{ color: t.color }}>{t.speed}</div>
              <div className="font-semibold mt-1">{t.tier}</div>
              <div className="text-xs mt-1" style={{ color: "#8888a0" }}>{t.desc}</div>
            </div>
          ))}
        </div>
      </section>

      <section className="max-w-3xl mx-auto px-6 pb-24">
        <div className="rounded-xl border p-8 text-center" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
          <h3 className="text-xl font-bold mb-3">Run the benchmark on your hardware</h3>
          <pre className="rounded-lg px-4 py-3 text-sm mb-6 inline-block" style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", color: "#22d3ee", fontFamily: "monospace" }}>
            pip install atomik-core && python -m atomik_core benchmark --share
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
