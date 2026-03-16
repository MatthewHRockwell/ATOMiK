import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

export const metadata: Metadata = {
  title: "Get Started — ATOMiK",
  description:
    "Install ATOMiK in 30 seconds. Python SDK, Linux kernel module, or FPGA hardware — choose your path.",
};

const steps = [
  {
    icon: "01",
    title: "Install the SDK",
    code: "pip install atomik-core",
    description: "Zero dependencies. Python 3.9+. Works on Linux, macOS, Windows.",
  },
  {
    icon: "02",
    title: "Create a context",
    code: `from atomik_core import AtomikContext

ctx = AtomikContext()
ctx.load(0xDEADBEEF)`,
    description: "Set your initial reference state with LOAD.",
  },
  {
    icon: "03",
    title: "Accumulate deltas",
    code: `ctx.accum(0x0000FFFF)  # XOR delta
ctx.accum(0xFF000000)  # Another delta

state = ctx.read()  # Reconstructed in O(1)`,
    description:
      "Deltas are commutative — order doesn't matter. READ reconstructs instantly.",
  },
  {
    icon: "04",
    title: "Detect changes",
    code: `from atomik_core import Fingerprint

fp = Fingerprint()
fp.update(data_block)
changed = fp.check(data_block)  # O(1) per page`,
    description:
      "Page-level XOR fingerprinting detects changes without comparing bytes.",
  },
];

const paths = [
  {
    title: "Python Developer",
    subtitle: "Software-only",
    badge: "Free",
    badgeColor: "#22c55e",
    items: [
      "pip install atomik-core",
      "Delta-state algebra in pure Python",
      "Fingerprinting & change detection",
      "Multi-context streaming",
      "353+ tests, fully typed",
    ],
    cta: "View on PyPI",
    ctaHref: "https://pypi.org/project/atomik-core/",
  },
  {
    title: "Linux Kernel Module",
    subtitle: "System-level optimization",
    badge: "$99/mo",
    badgeColor: "#4f8fff",
    items: [
      "COW redundancy detection (kretprobe)",
      "TCP send deduplication (CRC32C)",
      "Per-container waste tracking",
      "27 sysfs metrics in real-time",
      "DKMS packaging + systemd service",
    ],
    cta: "Get Professional License",
    ctaHref: "/#pricing",
  },
  {
    title: "FPGA Hardware",
    subtitle: "Maximum throughput",
    badge: "$499/mo",
    badgeColor: "#8b5cf6",
    items: [
      "AXI4-Lite MMIO on Zynq",
      "Up to 69.7 Gops/s (512 banks)",
      "Custom RV64I CPU with ATOMiK ISA",
      "Hardware-accelerated context tables",
      "Dedicated engineering support",
    ],
    cta: "Get Enterprise License",
    ctaHref: "/#pricing",
  },
];

export default function GetStartedPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-16">
        <h1 className="text-4xl font-bold tracking-tight mb-4">
          Get started in{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{
              backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            30 seconds
          </span>
        </h1>
        <p className="text-lg max-w-2xl mx-auto" style={{ color: "#8888a0" }}>
          One pip install. Zero dependencies. Start eliminating redundant memory
          operations immediately.
        </p>
      </section>

      {/* Steps */}
      <section className="max-w-3xl mx-auto px-6 pb-20">
        <div className="space-y-6">
          {steps.map((step) => (
            <div
              key={step.icon}
              className="rounded-xl border p-6"
              style={{
                background: "#12121a",
                borderColor: "#1e1e2e",
              }}
            >
              <div className="flex items-start gap-4">
                <span
                  className="shrink-0 w-10 h-10 rounded-lg flex items-center justify-center text-sm font-bold"
                  style={{
                    background: "rgba(79, 143, 255, 0.1)",
                    color: "#4f8fff",
                    border: "1px solid rgba(79, 143, 255, 0.2)",
                  }}
                >
                  {step.icon}
                </span>
                <div className="flex-1 min-w-0">
                  <h3 className="text-lg font-semibold mb-1">{step.title}</h3>
                  <p className="text-sm mb-3" style={{ color: "#8888a0" }}>
                    {step.description}
                  </p>
                  <pre
                    className="rounded-lg p-4 text-sm overflow-x-auto"
                    style={{
                      background: "#0a0a0f",
                      border: "1px solid #1e1e2e",
                      color: "#22d3ee",
                      fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
                    }}
                  >
                    <code>{step.code}</code>
                  </pre>
                </div>
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Choose your path */}
      <section className="px-6 pb-24">
        <h2 className="text-3xl font-bold text-center mb-4">Choose your path</h2>
        <p className="text-center mb-12" style={{ color: "#8888a0" }}>
          From Python SDK to FPGA silicon — scale when you&apos;re ready.
        </p>

        <div className="max-w-5xl mx-auto grid md:grid-cols-3 gap-6">
          {paths.map((path) => (
            <div
              key={path.title}
              className="rounded-xl border p-6 flex flex-col"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex items-center justify-between mb-2">
                <h3 className="text-lg font-semibold">{path.title}</h3>
                <span
                  className="text-xs font-bold px-2 py-1 rounded-full"
                  style={{
                    color: path.badgeColor,
                    background: `${path.badgeColor}15`,
                    border: `1px solid ${path.badgeColor}33`,
                  }}
                >
                  {path.badge}
                </span>
              </div>
              <p className="text-sm mb-4" style={{ color: "#8888a0" }}>
                {path.subtitle}
              </p>
              <ul className="space-y-2 mb-6 flex-1">
                {path.items.map((item) => (
                  <li
                    key={item}
                    className="text-sm flex items-start gap-2"
                    style={{ color: "#b0b0c0" }}
                  >
                    <span style={{ color: "#22c55e" }}>&#10003;</span>
                    {item}
                  </li>
                ))}
              </ul>
              <Link
                href={path.ctaHref}
                className="block text-center py-2.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
                style={{
                  background: path.badgeColor,
                  color: "#fff",
                }}
              >
                {path.cta}
              </Link>
            </div>
          ))}
        </div>
      </section>

      {/* Newsletter */}
      <section className="max-w-xl mx-auto px-6 pb-12">
        <EmailCapture />
      </section>

      {/* Links */}
      <section className="text-center px-6 pb-20">
        <div className="flex flex-wrap justify-center gap-6 text-sm">
          <Link href="/docs" className="hover:underline" style={{ color: "#4f8fff" }}>
            Full Documentation
          </Link>
          <Link
            href="https://github.com/MatthewHRockwell/ATOMiK"
            className="hover:underline"
            style={{ color: "#4f8fff" }}
          >
            GitHub Repository
          </Link>
          <Link href="/blog" className="hover:underline" style={{ color: "#4f8fff" }}>
            Read the Blog
          </Link>
        </div>
      </section>
    </div>
  );
}
