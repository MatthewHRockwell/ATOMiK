import type { Metadata } from "next";
import Link from "next/link";

export const metadata: Metadata = {
  title: "Documentation — ATOMiK Delta-State Computing",
  description:
    "Developer documentation for ATOMiK: Quick start, API reference, kernel module, architecture, examples, and FPGA deployment.",
};

const cards = [
  {
    href: "/docs/quickstart",
    title: "Quick Start",
    desc: "Install the Python SDK and run your first delta-state operation in under a minute.",
    color: "#8b5cf6",
    icon: "\u26A1",
  },
  {
    href: "/docs/kernel-module",
    title: "Kernel Module",
    desc: "Linux kernel module exposing delta-state operations via /dev/atomik and sysfs. DKMS-managed.",
    color: "#d4a843",
    icon: "\u2699",
  },
  {
    href: "/docs/api",
    title: "API Reference",
    desc: "Three interfaces, one algebra. Python, C, and kernel module APIs with multi-context tables.",
    color: "#4f8fff",
    icon: "\u2630",
  },
  {
    href: "/docs/architecture",
    title: "Architecture",
    desc: "The fundamental equation, four algebraic properties, and four operations that define ATOMiK.",
    color: "#22c55e",
    icon: "\u2227",
  },
  {
    href: "/docs/examples",
    title: "Examples",
    desc: "Production-ready patterns: distributed cache, IoT sensor fusion, and real-time analytics.",
    color: "#c084fc",
    icon: "\u2237",
  },
  {
    href: "/docs/hardware",
    title: "Hardware",
    desc: "FPGA-validated across two platforms, from a $13.50 Tang Nano 9K to 69.7 Gops/s on Zynq.",
    color: "#38bdf8",
    icon: "\u2B22",
  },
];

export default function DocsIndexPage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#8b5cf6" }}
      >
        Developer Documentation
      </p>
      <h1 className="text-4xl sm:text-5xl font-bold tracking-tight mb-4">
        <span
          className="bg-clip-text text-transparent"
          style={{ backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff, #22c55e)" }}
        >
          Docs
        </span>
      </h1>
      <p className="text-lg mb-12" style={{ color: "#8888a0" }}>
        Everything you need to integrate delta-state algebra into your stack
        &mdash; from a single Python import to a Linux kernel module to custom FPGA silicon.
      </p>

      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-5">
        {cards.map((c) => (
          <Link
            key={c.href}
            href={c.href}
            className="group rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1 no-underline"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div className="flex items-center gap-3 mb-3">
              <span className="text-2xl" style={{ color: c.color }}>{c.icon}</span>
              <h2 className="text-lg font-bold group-hover:underline" style={{ color: c.color }}>
                {c.title}
              </h2>
            </div>
            <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
              {c.desc}
            </p>
          </Link>
        ))}
      </div>
    </div>
  );
}
