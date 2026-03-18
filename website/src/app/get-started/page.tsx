import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";
import { CopyCodeBlock, CopyInstallButton } from "@/components/CopyCodeBlock";

export const metadata: Metadata = {
  title: "Get Started — ATOMiK",
  description:
    "Install ATOMiK in 30 seconds. Python SDK, Linux kernel module, or FPGA hardware — choose your path.",
};

const steps = [
  {
    icon: "01",
    title: "Install and benchmark",
    code: `pip install atomik-core
python -m atomik_core benchmark --share`,
    description: "Zero dependencies. Run the benchmark, get a shareable results card with your hardware specs.",
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
fp.load(original_data)          # Set reference fingerprint
fp.update(possibly_changed)     # Compare against reference
if fp.changed:                  # True if any bit differs
    print(f"Delta: 0x{fp.delta:x}")`,
    description:
      "Page-level XOR fingerprinting detects changes without comparing bytes.",
  },
];

const paths = [
  {
    title: "Community",
    subtitle: "Software-only",
    badge: "Free",
    badgeColor: "#22c55e",
    items: [
      "Python SDK (atomik-core)",
      "C99 header (atomik_core.h)",
      "JavaScript SDK (@atomik/core)",
      "4-operation API (LOAD, ACCUM, READ, SWAP)",
      "AtomikTable, DeltaStream, Fingerprint",
      "Apache 2.0 license",
      "Community support (GitHub)",
    ],
    ctaType: "copy-install" as const,
    ctaHref: "",
  },
  {
    title: "Pro",
    subtitle: "System-level optimization",
    badge: "$99/mo",
    badgeColor: "#4f8fff",
    items: [
      "Everything in Community",
      "Linux kernel module (COW detection, network dedup, cgroup tracking)",
      "27 sysfs metrics + atomik-status dashboard",
      "atomik-bench performance suite",
      "Priority email support (48hr SLA)",
      "90-day free trial",
    ],
    cta: "Start Pro Trial",
    ctaType: "link" as const,
    ctaHref: "/register?plan=professional",
  },
  {
    title: "Team",
    subtitle: "Multi-language generation",
    badge: "$299/mo",
    badgeColor: "#22d3ee",
    featured: true,
    items: [
      "Everything in Pro",
      "SDK generation pipeline (Python, Rust, C, JavaScript, Verilog)",
      "atomik-report waste analysis",
      "5-seat team license",
      "JSON/CSV CI export",
      "Schema-driven code generation",
    ],
    cta: "Start Team Trial",
    ctaType: "link" as const,
    ctaHref: "/register?plan=team",
  },
  {
    title: "Enterprise",
    subtitle: "Maximum throughput",
    badge: "$999/mo",
    badgeColor: "#8b5cf6",
    items: [
      "Everything in Team",
      "FPGA hardware acceleration (Zynq AXI4-Lite, up to 512 parallel banks)",
      "Custom RV64I CPU with ATOMiK ISA extensions",
      "4-hour response SLA",
      "Dedicated support channel",
      "Formal verification certificate",
      "ASIC development path",
    ],
    cta: "Contact Sales",
    ctaType: "link" as const,
    ctaHref: "/contact",
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
                  <CopyCodeBlock code={step.code} />
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

        <div className="max-w-6xl mx-auto grid md:grid-cols-2 lg:grid-cols-4 gap-6">
          {paths.map((path) => (
            <div
              key={path.title}
              className="rounded-xl border p-6 flex flex-col relative"
              style={{
                background: (path as { featured?: boolean }).featured ? "#14142a" : "#12121a",
                borderColor: (path as { featured?: boolean }).featured ? "rgba(79,143,255,0.3)" : "#1e1e2e",
                boxShadow: (path as { featured?: boolean }).featured ? "0 0 40px rgba(79,143,255,0.08)" : "none",
              }}
            >
              {(path as { featured?: boolean }).featured && (
                <div
                  className="absolute -top-3 left-1/2 -translate-x-1/2 text-[11px] font-bold px-4 py-1 rounded-full tracking-wide"
                  style={{ background: "linear-gradient(135deg, #4f8fff, #3a7aee)", color: "#fff" }}
                >
                  RECOMMENDED
                </div>
              )}
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
              {path.ctaType === "copy-install" ? (
                <CopyInstallButton command="pip install atomik-core" />
              ) : (
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
              )}
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
