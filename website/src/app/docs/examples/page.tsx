import type { Metadata } from "next";
import { DocsNav } from "../shared";

export const metadata: Metadata = {
  title: "Examples — ATOMiK Docs",
  description:
    "Production-ready patterns showing delta-state algebra in real workloads: distributed cache, IoT sensor fusion, real-time analytics.",
};

const examples = [
  {
    title: "Distributed Cache",
    file: "distributed_cache.py",
    description:
      "Multi-node cache synchronization without consensus. Nodes broadcast XOR deltas — no leader election, no write-ahead log, no ordering constraints.",
    color: "#8b5cf6",
    icon: "\u21c4",
  },
  {
    title: "IoT Sensor Fusion",
    file: "iot_sensor_fusion.py",
    description:
      "Hundreds of sensors report 8-byte XOR deltas instead of full state. Gateway merges in any order. Lost packets self-cancel via self-inverse property.",
    color: "#4f8fff",
    icon: "\u2b21",
  },
  {
    title: "Real-Time Analytics",
    file: "realtime_analytics.py",
    description:
      "Track metrics across hundreds of dimensions. XOR-accumulate deltas per dimension, read any time. No database writes, no event logs.",
    color: "#22c55e",
    icon: "\u2237",
  },
];

export default function ExamplesPage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#c084fc" }}
      >
        Patterns
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">Examples</h1>
      <p className="text-lg mb-10" style={{ color: "#8888a0" }}>
        Production-ready patterns showing delta-state algebra in real workloads.
      </p>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-16">
        {examples.map((ex) => (
          <a
            key={ex.file}
            href={`https://github.com/MatthewHRockwell/ATOMiK/blob/main/software/atomik_core/examples/${ex.file}`}
            target="_blank"
            rel="noopener noreferrer"
            className="group rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1 no-underline"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div className="flex items-center gap-3 mb-3">
              <span className="text-2xl" style={{ color: ex.color }}>
                {ex.icon}
              </span>
              <h3
                className="font-bold group-hover:underline"
                style={{ color: ex.color }}
              >
                {ex.title}
              </h3>
            </div>
            <p className="text-sm leading-relaxed mb-4" style={{ color: "#8888a0" }}>
              {ex.description}
            </p>
            <code
              className="text-xs font-mono px-3 py-1.5 rounded-lg block"
              style={{ background: "#0d0d14", color: "#555570" }}
            >
              examples/{ex.file}
            </code>
          </a>
        ))}
      </div>

      {/* Bottom CTA */}
      <div
        className="rounded-2xl p-8 sm:p-12 border text-center"
        style={{
          background: "linear-gradient(135deg, rgba(139,92,246,0.08), rgba(79,143,255,0.08))",
          borderColor: "#8b5cf630",
        }}
      >
        <h2 className="text-2xl font-bold mb-3">Ready to Build?</h2>
        <p className="mb-6" style={{ color: "#8888a0" }}>
          Start with the Python SDK. Scale to kernel-level. Deploy on FPGA.
        </p>
        <div className="flex flex-col sm:flex-row items-center justify-center gap-4">
          <a
            href="https://github.com/MatthewHRockwell/ATOMiK"
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 px-6 py-3 rounded-lg font-semibold text-white transition-all duration-300 hover:scale-105 no-underline"
            style={{
              background: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            View on GitHub
            <span className="text-lg">&rarr;</span>
          </a>
          <a
            href="https://pypi.org/project/atomik-core/"
            target="_blank"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 px-6 py-3 rounded-lg font-semibold transition-all duration-300 hover:scale-105 border no-underline"
            style={{ borderColor: "#1e1e2e", color: "#e0e0e8" }}
          >
            PyPI Package
            <span className="text-lg">&rarr;</span>
          </a>
        </div>
      </div>

      <DocsNav
        prev={{ href: "/docs/architecture", label: "Architecture" }}
        next={{ href: "/docs/hardware", label: "Hardware" }}
      />
    </div>
  );
}
