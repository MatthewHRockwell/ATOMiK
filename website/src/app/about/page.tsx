import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "About ATOMiK — Delta-State Computing",
  description:
    "ATOMiK is building the next computing primitive: delta-state algebra that replaces store-and-retrieve with reconstruct-from-deltas.",
};

const timelineItems = [
  {
    year: "2025",
    title: "Mathematical Formalization",
    description:
      "108 Lean4 theorems proving Abelian group properties: commutativity, associativity, self-inverse, identity. The math that makes delta-state algebra work.",
    color: "text-purple-400",
    border: "border-purple-500/40",
    bg: "bg-purple-500/5",
  },
  {
    year: "2025",
    title: "Software SDK",
    description:
      "Python and C libraries with 218+ tests. Pipeline orchestration, delta generators, and agentic scheduling. Available via pip install atomik-core.",
    color: "text-blue-400",
    border: "border-blue-500/40",
    bg: "bg-blue-500/5",
  },
  {
    year: "2025",
    title: "FPGA v1 -- PicoRV32 + ATOMiK",
    description:
      "First silicon proof: ATOMiK core integrated with PicoRV32 RISC-V on a $13.50 Tang Nano 9K. Single-bank at 81 MHz, 94.5 Mops/s.",
    color: "text-green-400",
    border: "border-green-500/40",
    bg: "bg-green-500/5",
  },
  {
    year: "2025",
    title: "FPGA v2 -- Custom RV64I CPU",
    description:
      "Built a custom 64-bit RISC-V CPU with native ATOMiK ISA extensions. Delta operations became first-class instructions, not MMIO.",
    color: "text-green-400",
    border: "border-green-500/40",
    bg: "bg-green-500/5",
  },
  {
    year: "2025",
    title: "FPGA v3 -- HD Video + Multi-Node",
    description:
      "1280x720@60Hz HDMI output on a $13.50 FPGA. Delta-driven display pipeline. Multi-node convergence proven: two SoCs streaming deltas reach identical state.",
    color: "text-green-400",
    border: "border-green-500/40",
    bg: "bg-green-500/5",
  },
  {
    year: "2026",
    title: "Zynq Characterization",
    description:
      "69.7 Gops/s peak on Xilinx Zynq XC7Z020. 444 MHz single-bank. 512 parallel banks in 44% of the fabric. Sub-linear LUT scaling confirmed.",
    color: "text-yellow-400",
    border: "border-yellow-500/40",
    bg: "bg-yellow-500/5",
  },
  {
    year: "2026",
    title: "Software Licensing Launch",
    description:
      "atomik-core on PyPI. Commercial licensing for enterprise integration. Making delta-state algebra accessible to every developer.",
    color: "text-blue-400",
    border: "border-blue-500/40",
    bg: "bg-blue-500/5",
  },
  {
    year: "2026",
    title: "ASIC Path Initiated",
    description:
      "Sky130 shuttle evaluation underway. The goal: a standalone delta-state chip that any system can drop in.",
    color: "text-yellow-400",
    border: "border-yellow-500/40",
    bg: "bg-yellow-500/5",
  },
];

const proofPoints = [
  {
    value: "108",
    label: "Lean4 Theorems",
    sublabel: "Formally verified",
    gradient: "from-purple-400 to-purple-600",
  },
  {
    value: "500+",
    label: "Tests Passing",
    sublabel: "SDK + hardware",
    gradient: "from-green-400 to-green-600",
  },
  {
    value: "3",
    label: "FPGA Platforms",
    sublabel: "Tang Nano, Zynq, Custom",
    gradient: "from-blue-400 to-blue-600",
  },
  {
    value: "69.7",
    label: "Gops/s Peak",
    sublabel: "512 parallel banks",
    gradient: "from-yellow-400 to-yellow-600",
  },
];

export default function AboutPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="About" />

      {/* Hero Section */}
      <section className="relative overflow-hidden">
        <div
          className="absolute inset-0 opacity-20"
          style={{
            background:
              "radial-gradient(ellipse 60% 40% at 50% 0%, #8b5cf640, transparent), radial-gradient(ellipse 40% 30% at 80% 20%, #4f8fff30, transparent)",
          }}
        />
        <div className="max-w-5xl mx-auto px-6 pt-24 pb-16 relative">
          <p
            className="text-sm font-mono tracking-widest uppercase mb-4"
            style={{ color: "#8b5cf6" }}
          >
            About the Project
          </p>
          <h1 className="text-5xl sm:text-6xl font-bold tracking-tight mb-6">
            About{" "}
            <span
              className="bg-clip-text text-transparent"
              style={{
                backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff, #22c55e)",
              }}
            >
              ATOMiK
            </span>
          </h1>
          <p className="text-xl leading-relaxed max-w-3xl" style={{ color: "#8888a0" }}>
            Building the next computing primitive — from mathematical proof to custom silicon.
          </p>
        </div>
      </section>

      {/* Mission Statement */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div
          className="rounded-2xl p-8 sm:p-12 border"
          style={{
            background: "linear-gradient(135deg, #12121a, #181824)",
            borderColor: "#1e1e2e",
          }}
        >
          <h2 className="text-2xl font-bold mb-6" style={{ color: "#e0e0e8" }}>
            The Mission
          </h2>
          <div className="space-y-4 text-lg leading-relaxed" style={{ color: "#b0b0c0" }}>
            <p>
              Every computer built in the last 80 years stores state and retrieves it. ATOMiK
              replaces that model entirely.{" "}
              <span className="font-semibold text-white">
                Delta-state algebra reconstructs state from deltas
              </span>
              : small, commutative, self-inverse operations that compose in any order to produce
              the same result.
            </p>
            <p>
              This is not an optimization. It is a new primitive — as fundamental as the
              transistor or the cache line. The math is proven (108 Lean4 theorems). The hardware
              is built (3 FPGA platforms, 69.7 Gops/s). The software is shipping (
              <code
                className="text-sm font-mono px-2 py-0.5 rounded"
                style={{ background: "#1e1e2e", color: "#22c55e" }}
              >
                pip install atomik-core
              </code>
              ).
            </p>
            <p>
              The goal is to make delta-state algebra available at{" "}
              <span className="font-semibold text-white">every layer of the stack</span> —
              from a Python import to a custom ASIC — so that any engineer can stop moving data
              and start evolving it.
            </p>
          </div>
        </div>
      </section>

      {/* The Invention Story */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">The Invention</h2>
        <p className="mb-10 text-lg" style={{ color: "#8888a0" }}>
          From a mathematical insight to working silicon in under a year.
        </p>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
          {[
            {
              step: "01",
              title: "The Math",
              desc: "Delta-state algebra forms an Abelian group under XOR. 108 theorems formalized and machine-verified in Lean4.",
              color: "#8b5cf6",
            },
            {
              step: "02",
              title: "The Proofs",
              desc: "Commutativity, associativity, self-inverse, identity. Every property formally proven — no assumptions, no hand-waving.",
              color: "#4f8fff",
            },
            {
              step: "03",
              title: "The Software",
              desc: "Python and C SDKs with pipeline orchestration, delta generators, and 218+ passing tests. Available on PyPI.",
              color: "#22c55e",
            },
            {
              step: "04",
              title: "The Silicon",
              desc: "Three FPGA platforms validated. 444 MHz single-bank, 69.7 Gops/s parallel. ASIC evaluation underway.",
              color: "#d4a843",
            },
          ].map((item) => (
            <div
              key={item.step}
              className="rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div
                className="text-4xl font-bold font-mono mb-3 opacity-30"
                style={{ color: item.color }}
              >
                {item.step}
              </div>
              <h3 className="text-lg font-bold mb-2" style={{ color: item.color }}>
                {item.title}
              </h3>
              <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
                {item.desc}
              </p>
            </div>
          ))}
        </div>
      </section>

      {/* Technology Timeline */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Technology Timeline</h2>
        <p className="mb-10 text-lg" style={{ color: "#8888a0" }}>
          Every milestone built on the one before it.
        </p>

        <div className="relative">
          {/* Vertical line */}
          <div
            className="absolute left-4 top-0 bottom-0 w-px hidden sm:block"
            style={{ background: "linear-gradient(to bottom, #8b5cf6, #4f8fff, #22c55e, #d4a843)" }}
          />

          <div className="space-y-4">
            {timelineItems.map((item, i) => (
              <div key={i} className="relative flex gap-6 group">
                {/* Dot */}
                <div className="hidden sm:flex items-start pt-6">
                  <div
                    className={`w-3 h-3 rounded-full border-2 ${item.border} ${item.bg} relative z-10`}
                    style={{ marginLeft: "6.5px" }}
                  />
                </div>

                {/* Card */}
                <div
                  className={`flex-1 rounded-xl p-6 border ${item.border} ${item.bg} transition-all duration-300 hover:scale-[1.01]`}
                  style={{ background: "#12121a", borderColor: undefined }}
                >
                  <div className="flex items-center gap-3 mb-2">
                    <span
                      className={`text-xs font-mono font-bold px-2 py-0.5 rounded ${item.color}`}
                      style={{ background: "#1e1e2e" }}
                    >
                      {item.year}
                    </span>
                    <h3 className="font-bold text-white">{item.title}</h3>
                  </div>
                  <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
                    {item.description}
                  </p>
                </div>
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* Key Proof Points */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-10 text-center">By the Numbers</h2>
        <div className="grid grid-cols-2 lg:grid-cols-4 gap-6">
          {proofPoints.map((point) => (
            <div
              key={point.label}
              className="rounded-xl p-6 border text-center"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div
                className="text-4xl sm:text-5xl font-bold mb-2 bg-clip-text text-transparent"
                style={{ backgroundImage: `linear-gradient(135deg, ${point.gradient.includes("purple") ? "#a855f7, #7c3aed" : point.gradient.includes("green") ? "#22c55e, #16a34a" : point.gradient.includes("blue") ? "#4f8fff, #3b82f6" : "#d4a843, #b8941e"})` }}
              >
                {point.value}
              </div>
              <div className="font-semibold text-white text-sm mb-1">{point.label}</div>
              <div className="text-xs" style={{ color: "#8888a0" }}>
                {point.sublabel}
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Founder Section */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div
          className="rounded-2xl p-8 sm:p-12 border"
          style={{
            background: "linear-gradient(135deg, #12121a, #181824)",
            borderColor: "#1e1e2e",
          }}
        >
          <h2 className="text-2xl font-bold mb-6">Founder</h2>
          <div className="flex flex-col sm:flex-row gap-8 items-start">
            {/* Avatar placeholder */}
            <div
              className="w-24 h-24 rounded-full flex items-center justify-center text-3xl font-bold shrink-0"
              style={{
                background: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
              }}
            >
              MR
            </div>
            <div>
              <h3 className="text-xl font-bold text-white mb-1">Matt Rockwell</h3>
              <p className="text-sm font-mono mb-4" style={{ color: "#8b5cf6" }}>
                Inventor &amp; Founder
              </p>
              <p className="leading-relaxed mb-4" style={{ color: "#b0b0c0" }}>
                Matt Rockwell is a systems engineer and founder of ATOMiK, with deep expertise
                in embedded systems, FPGA development, and formal verification. He created the
                delta-state algebra as a fundamental rethinking of how computers handle state
                — proving it correct with 108 Lean4 theorems before writing the first line of
                production code. ATOMiK grew from a mathematical insight into a
                hardware-validated computing architecture running on custom RISC-V silicon.
              </p>
              <p className="leading-relaxed mb-6" style={{ color: "#b0b0c0" }}>
                Designed the delta-state algebra, wrote the formal proofs, built the hardware,
                and shipped the software. ATOMiK is a solo-founder deep-tech company —
                every theorem, every RTL module, and every line of SDK code traces back to one
                engineer with a conviction that computing has a better primitive waiting to be
                found.
              </p>
              <div className="flex flex-wrap items-center gap-3">
                <a
                  href="mailto:mrockwell@atomik.tech"
                  className="inline-flex items-center gap-2 text-sm font-mono px-4 py-2 rounded-lg border transition-colors hover:bg-white/5"
                  style={{ borderColor: "#1e1e2e", color: "#4f8fff" }}
                >
                  mrockwell@atomik.tech
                </a>
                <a
                  href="https://www.linkedin.com/in/matthewhrockwell/"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="inline-flex items-center gap-2 text-sm font-mono px-4 py-2 rounded-lg border transition-colors hover:bg-white/5"
                  style={{ borderColor: "#1e1e2e", color: "#4f8fff" }}
                >
                  <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
                    <path d="M20.447 20.452h-3.554v-5.569c0-1.328-.027-3.037-1.852-3.037-1.853 0-2.136 1.445-2.136 2.939v5.667H9.351V9h3.414v1.561h.046c.477-.9 1.637-1.85 3.37-1.85 3.601 0 4.267 2.37 4.267 5.455v6.286zM5.337 7.433c-1.144 0-2.063-.926-2.063-2.065 0-1.138.92-2.063 2.063-2.063 1.14 0 2.064.925 2.064 2.063 0 1.139-.925 2.065-2.064 2.065zm1.782 13.019H3.555V9h3.564v11.452zM22.225 0H1.771C.792 0 0 .774 0 1.729v20.542C0 23.227.792 24 1.771 24h20.451C23.2 24 24 23.227 24 22.271V1.729C24 .774 23.2 0 22.222 0h.003z"/>
                  </svg>
                  LinkedIn
                </a>
                <a
                  href="https://github.com/MatthewHRockwell"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="inline-flex items-center gap-2 text-sm font-mono px-4 py-2 rounded-lg border transition-colors hover:bg-white/5"
                  style={{ borderColor: "#1e1e2e", color: "#4f8fff" }}
                >
                  <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
                    <path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/>
                  </svg>
                  GitHub
                </a>
              </div>
            </div>
          </div>
        </div>
      </section>

      {/* Why ATOMiK? Origin Story */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div
          className="rounded-2xl p-8 sm:p-12 border"
          style={{
            background: "linear-gradient(135deg, #12121a, #181824)",
            borderColor: "#1e1e2e",
          }}
        >
          <h2 className="text-2xl font-bold mb-6" style={{ color: "#e0e0e8" }}>
            Why ATOMiK?
          </h2>
          <div className="space-y-4 text-lg leading-relaxed" style={{ color: "#b0b0c0" }}>
            <p>
              ATOMiK began with a simple question:{" "}
              <span className="font-semibold text-white">
                what if we never needed to copy state at all?
              </span>
            </p>
            <p>
              Traditional computing wastes enormous resources moving data that hasn&apos;t
              changed. Delta-state algebra eliminates this by design — tracking only what
              changes, with mathematical guarantees that the approach is correct.
            </p>
            <p>
              From 108 formal proofs to a custom RISC-V CPU running on a $13.50 FPGA, ATOMiK
              is the result of building from first principles.
            </p>
          </div>
        </div>
      </section>

      {/* ASIC Roadmap CTA */}
      <section className="max-w-5xl mx-auto px-6 pb-24">
        <div
          className="rounded-2xl p-8 sm:p-12 border text-center"
          style={{
            background: "linear-gradient(135deg, rgba(139,92,246,0.08), rgba(79,143,255,0.08))",
            borderColor: "#8b5cf630",
          }}
        >
          <h2 className="text-2xl font-bold mb-3">What Comes Next</h2>
          <p className="mb-6" style={{ color: "#8888a0" }}>
            From FPGA validation to custom silicon. See the path to a dedicated delta-state ASIC.
          </p>
          <Link
            href="/about/roadmap"
            className="inline-flex items-center gap-2 px-6 py-3 rounded-lg font-semibold text-white transition-all duration-300 hover:scale-105"
            style={{
              background: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            View ASIC Roadmap
            <span className="text-lg">&rarr;</span>
          </Link>
        </div>
      </section>

    </div>
  );
}
