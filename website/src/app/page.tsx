import Link from "next/link";
import type { Metadata } from "next";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";
import MiniDemo from "@/components/MiniDemo";
import DeveloperStats from "@/components/DeveloperStats";
import HeroMetrics from "@/components/HeroMetrics";

export const metadata: Metadata = {
  title: "ATOMiK — Stop moving data. Start evolving it.",
  description:
    "O(1) state reconstruction, 99% less bandwidth, 330,000x less memory. A 4-operation algebra, formally proven with 92 Lean4 theorems. Software + FPGA hardware acceleration.",
  openGraph: {
    title: "ATOMiK — Stop moving data. Start evolving it.",
    description:
      "O(1) state reconstruction. 99% less bandwidth. 330,000x less memory. Formally proven with 92 Lean4 theorems. Software + hardware acceleration.",
    url: "https://atomik.tech",
    images: [{ url: "https://atomik.tech/og-image.jpg", width: 1200, height: 630 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK — Stop moving data. Start evolving it.",
    description:
      "O(1) state reconstruction. 99% less bandwidth. Formally proven with 92 Lean4 theorems.",
    images: ["https://atomik.tech/og-image.jpg"],
  },
};

const metrics = [
  { value: "O(1)", label: "Change Detection", subtitle: "constant ~262 cycles regardless of buffer size \u2014 hardware-validated on Linux userspace" },
  { value: "92", label: "Lean4 Formal Proofs" },
  { value: "69.7 Gops/s", label: "Peak FPGA Throughput" },
  { value: "1.2M", label: "Regions/sec Monitored", subtitle: "multi-buffer change detection at 64 contexts on Zynq RISC-V Linux" },
];

const ops = [
  { name: "LOAD", desc: "Set reference state, clear accumulator", color: "#4f8fff", bg: "rgba(79,143,255,0.12)" },
  { name: "ACCUM", desc: "XOR delta into accumulator", color: "#22d3ee", bg: "rgba(34,211,238,0.12)" },
  { name: "READ", desc: "Reconstruct state: ref \u2295 acc", color: "#22c55e", bg: "rgba(34,197,94,0.12)" },
  { name: "SWAP", desc: "Atomic snapshot + new epoch", color: "#a855f7", bg: "rgba(168,85,247,0.12)" },
];

const features: { tag: string; title: string; desc: string; color: string; bg: string }[] = [
  { tag: "LATENCY", title: "Deterministic Latency", desc: "Every operation is constant-time. No data-dependent branching, no variable-length scans. Timing side channels eliminated by design.", color: "#4f8fff", bg: "rgba(79,143,255,0.12)" },
  { tag: "HARDWARE", title: "Hardware Acceleration", desc: "Same API from Python to FPGA. 5M ops/s in Python, 500M in C, 69.7B on Xilinx Zynq with 512 parallel banks. No code changes.", color: "#a855f7", bg: "rgba(168,85,247,0.12)" },
  { tag: "SCALING", title: "Parallel Scaling", desc: "Sub-linear LUT growth: 3.7x area for 16x throughput. XOR commutativity means lock-free parallel accumulation across banks.", color: "#22d3ee", bg: "rgba(34,211,238,0.12)" },
  { tag: "BANDWIDTH", title: "Delta-Driven Updates", desc: "Send 8-byte deltas instead of full state copies. 64KB state with 10K updates: ATOMiK sends 80KB. Full replication sends 655MB.", color: "#22c55e", bg: "rgba(34,197,94,0.12)" },
  { tag: "SECURITY", title: "Timing-Safe", desc: "No speculative execution, no cache coherency attacks, no data-dependent timing. Security is architectural, not bolted on.", color: "#f59e0b", bg: "rgba(245,158,11,0.12)" },
  { tag: "CORRECTNESS", title: "Formally Verified", desc: "92 Lean4 theorems prove commutativity, associativity, self-inverse, and identity. Not tested -- proven. Every property holds for all inputs.", color: "#ef4444", bg: "rgba(239,68,68,0.12)" },
];

const comparisons: [string, string, string][] = [
  ["Architecture", "Event sourcing + Raft", "XOR delta algebra"],
  ["Per update", "Full state copy (64 KB)", "8-byte delta"],
  ["10K updates bandwidth", "655 MB", "80 KB"],
  ["Rollback memory", "8 MB (snapshots)", "24 bytes (always)"],
  ["Change detection", "O(n) full rescan", "O(1) incremental"],
  ["Message ordering", "Required (consensus)", "Any order (XOR commutes)"],
  ["Correctness", "Tests + hope", "92 Lean4 proofs"],
];

const pricingTiers = [
  {
    name: "Community",
    price: "Free",
    period: "",
    items: ["Python SDK (atomik-core)", "C99 header (atomik_core.h)", "JavaScript SDK (@atomik/core)", "4-operation API (LOAD, ACCUM, READ, SWAP)", "AtomikTable, DeltaStream, Fingerprint", "Apache 2.0 license", "Community support (GitHub)"],
    cta: "Get Started Free",
    href: "/register",
    primary: false,
  },
  {
    name: "Pro",
    price: "$99",
    period: "/mo",
    items: ["Everything in Community", "Linux kernel module (COW detection, network dedup, cgroup tracking)", "27 sysfs metrics + atomik-status dashboard", "atomik-bench performance suite", "Priority email support (48hr SLA)", "90-day free trial"],
    cta: "Start Pro Trial",
    href: "/get-started?plan=professional",
    primary: false,
  },
  {
    name: "Team",
    price: "$299",
    period: "/mo",
    items: ["Everything in Pro", "SDK generation pipeline (Python, Rust, C, JavaScript, Verilog)", "atomik-report waste analysis", "5-seat team license", "JSON/CSV CI export", "Schema-driven code generation"],
    cta: "Start Team Trial",
    href: "/get-started?plan=team",
    primary: true,
  },
  {
    name: "Enterprise",
    price: "$999",
    period: "/mo",
    items: ["Everything in Team", "FPGA hardware acceleration (Zynq AXI4-Lite, up to 512 parallel banks)", "Custom RV64I CPU with ATOMiK ISA extensions", "4-hour response SLA", "Dedicated support channel", "Formal verification certificate", "ASIC development path"],
    cta: "Contact Sales",
    href: "/contact",
    primary: false,
  },
];

const investorStats: [string, string][] = [
  ["92", "Lean4 Proofs"], ["500+", "Tests (SW + HW)"], ["3", "FPGA Platforms"],
  ["69.7B", "Ops/s Peak"], ["$50B", "Serviceable Market"],
];

export default function Home() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* ===== Hero ===== */}
      <section className="text-center px-6 pt-24 pb-16">
        <h1 className="text-5xl md:text-6xl font-extrabold tracking-tight leading-tight mb-6">
          Stop moving data.{" "}
          <span className="bg-gradient-to-r from-[#4f8fff] to-[#22d3ee] bg-clip-text text-transparent">
            Start evolving it.
          </span>
        </h1>
        <p className="text-lg md:text-xl max-w-2xl mx-auto mb-10" style={{ color: "#8888a0" }}>
          ATOMiK replaces snapshots, event replay, and full-state replication
          with a 4-operation algebra. 99% less bandwidth. O(1) reconstruction.
          Formally proven. Hardware-accelerated.
        </p>
        <div className="flex gap-4 justify-center flex-wrap mb-8">
          <Link
            href="/register"
            className="px-7 py-3.5 rounded-lg text-base font-semibold text-white no-underline transition-all hover:opacity-90"
            style={{ background: "linear-gradient(135deg, #4f8fff, #3a7aee)", boxShadow: "0 4px 24px rgba(79,143,255,0.25)" }}
          >
            Get Started
          </Link>
          <Link
            href="/demo"
            className="px-7 py-3.5 rounded-lg text-base font-semibold no-underline transition-all hover:border-[#4f8fff] hover:text-[#4f8fff]"
            style={{ background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" }}
          >
            Try the Demo
          </Link>
          <Link
            href="/ai-demo"
            className="px-7 py-3.5 rounded-lg text-base font-semibold text-white no-underline transition-all hover:opacity-90"
            style={{ background: "linear-gradient(135deg, #a855f7, #7c3aed)", boxShadow: "0 4px 24px rgba(168,85,247,0.25)" }}
          >
            AI Inference Demo
          </Link>
        </div>
        <div
          className="inline-flex items-center gap-3 px-6 py-3.5 rounded-lg font-mono text-sm"
          style={{ background: "#12121a", border: "1px solid #1e1e2e", color: "#22d3ee" }}
        >
          $ pip install atomik-core
        </div>
      </section>

      {/* ===== Metrics (animated counters) ===== */}
      <section className="max-w-5xl mx-auto px-6 pb-16">
        <HeroMetrics />
      </section>

      {/* ===== Latest Releases ===== */}
      <section className="px-6 py-4" style={{ background: "#12121a", borderTop: "1px solid #1e1e2e", borderBottom: "1px solid #1e1e2e" }}>
        <div className="max-w-5xl mx-auto flex flex-wrap items-center justify-center gap-x-8 gap-y-2">
          <span className="text-xs font-semibold uppercase tracking-widest" style={{ color: "#555570" }}>
            Latest Releases
          </span>
          {[
            { name: "Python SDK", version: "v0.4.0", date: "Mar 2026" },
            { name: "Kernel Module", version: "v0.4.0", date: "Mar 2026" },
            { name: "C Header", version: "v0.4.0", date: "Mar 2026" },
          ].map((r) => (
            <Link
              key={r.name}
              href="/changelog"
              className="inline-flex items-center gap-2 text-xs no-underline transition-colors hover:text-white"
              style={{ color: "#8888a0" }}
            >
              {r.name}
              <span
                className="px-1.5 py-0.5 rounded text-[10px] font-bold"
                style={{ background: "rgba(79,143,255,0.15)", color: "#4f8fff" }}
              >
                {r.version}
              </span>
              <span className="text-[10px]" style={{ color: "#555570" }}>{r.date}</span>
            </Link>
          ))}
        </div>
      </section>

      {/* ===== How It Works ===== */}
      <section className="px-6 py-20" style={{ borderTop: "1px solid #1e1e2e" }}>
        <div className="max-w-5xl mx-auto">
          <h2 className="text-3xl font-bold text-center tracking-tight mb-3">How It Works</h2>
          <p className="text-center text-lg mb-12" style={{ color: "#8888a0" }}>
            Four operations. Everything else is built on top.
          </p>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
            {ops.map((op) => (
              <div
                key={op.name}
                className="rounded-xl p-6 text-center transition-all hover:-translate-y-1 hover:border-[#4f8fff]"
                style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
              >
                <div
                  className="w-12 h-12 rounded-xl mx-auto mb-4 flex items-center justify-center text-xl font-bold"
                  style={{ background: op.bg, color: op.color }}
                >
                  {op.name[0]}
                </div>
                <h3 className="font-mono text-lg font-bold mb-1">{op.name}</h3>
                <p className="text-xs" style={{ color: "#8888a0" }}>{op.desc}</p>
                <span
                  className="inline-block mt-3 text-xs font-semibold px-2.5 py-0.5 rounded"
                  style={{ background: "rgba(34,197,94,0.1)", color: "#22c55e" }}
                >
                  O(1)
                </span>
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* ===== Features ===== */}
      <section className="px-6 py-20" style={{ borderTop: "1px solid #1e1e2e" }}>
        <div className="max-w-5xl mx-auto">
          <h2 className="text-3xl font-bold text-center tracking-tight mb-3">Why ATOMiK</h2>
          <p className="text-center text-lg mb-12" style={{ color: "#8888a0" }}>
            Four operations. Zero dependencies. Formally proven correctness.
          </p>
          <div className="grid md:grid-cols-3 gap-4">
            {features.map((f) => (
              <div
                key={f.title}
                className="rounded-xl p-7 transition-all hover:-translate-y-1 hover:border-[#4f8fff]"
                style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
              >
                <div
                  className="w-10 h-10 rounded-lg flex items-center justify-center text-base font-bold mb-4"
                  style={{ background: f.bg, color: f.color }}
                >
                  {f.tag[0]}
                </div>
                <span
                  className="text-[11px] font-semibold tracking-wide px-2.5 py-1 rounded"
                  style={{ background: "rgba(79,143,255,0.1)", color: "#4f8fff" }}
                >
                  {f.tag}
                </span>
                <h3 className="text-lg font-bold mt-3 mb-2">{f.title}</h3>
                <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>{f.desc}</p>
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* ===== Code Example ===== */}
      <section className="px-6 py-20" style={{ borderTop: "1px solid #1e1e2e" }}>
        <div className="max-w-5xl mx-auto grid md:grid-cols-2 gap-12 items-center">
          <div>
            <h2 className="text-3xl font-bold tracking-tight mb-4">
              4 operations.<br />That&apos;s it.
            </h2>
            <p className="text-lg mb-6" style={{ color: "#8888a0" }}>
              LOAD sets the reference. ACCUM XORs a delta.
              READ reconstructs the state. SWAP starts a new epoch.
              Everything else -- rollback, merge, fingerprinting --
              is built on these four.
            </p>
            <div className="flex flex-wrap gap-4 text-sm" style={{ color: "#8888a0" }}>
              {["Zero dependencies", "Python 3.9+", "Fully typed", "C99 header available"].map((t) => (
                <span key={t} className="flex items-center gap-1.5">
                  <span style={{ color: "#22c55e" }}>{"\u2713"}</span> {t}
                </span>
              ))}
            </div>
          </div>
          <div
            className="rounded-xl p-6 font-mono text-[13px] leading-relaxed relative overflow-x-auto"
            style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
          >
            <span className="absolute top-3 right-4 text-[11px] font-sans" style={{ color: "#8888a0" }}>
              Python
            </span>
            <pre className="whitespace-pre-wrap">
              <Code k>from</Code>{" "}atomik_core{" "}<Code k>import</Code>{" "}AtomikContext{"\n"}
              {"\n"}
              ctx = AtomikContext(){"\n"}
              ctx.<Code f>load</Code>(<Code n>1000</Code>){"           "}<Code c># Set initial value</Code>{"\n"}
              ctx.<Code f>accum</Code>(<Code n>50</Code>){"            "}<Code c># Apply delta (+50)</Code>{"\n"}
              <Code k>print</Code>(ctx.<Code f>read</Code>()){"        "}<Code c># 1018 (1000 XOR 50)</Code>{"\n"}
              {"\n"}
              ctx.<Code f>accum</Code>(<Code n>50</Code>){"            "}<Code c># Apply same delta again</Code>{"\n"}
              <Code k>print</Code>(ctx.<Code f>read</Code>()){"        "}<Code c># 1000 (self-inverse: undone!)</Code>{"\n"}
              {"\n"}
              <Code c># Merge two independent accumulators.</Code>{"\n"}
              <Code c># Order doesn&apos;t matter (commutativity).</Code>{"\n"}
              a = AtomikContext(){"\n"}
              b = AtomikContext(){"\n"}
              a.<Code f>load</Code>(<Code n>1000</Code>); b.<Code f>load</Code>(<Code n>1000</Code>){"\n"}
              a.<Code f>accum</Code>(<Code n>50</Code>); b.<Code f>accum</Code>(<Code n>100</Code>){"\n"}
              a.<Code f>merge</Code>(b){"  "}<Code c># a now has both deltas</Code>
            </pre>
          </div>
        </div>
      </section>

      {/* ===== Try It Live ===== */}
      <section className="px-6 py-20" style={{ borderTop: "1px solid #1e1e2e" }}>
        <div className="max-w-3xl mx-auto">
          <h2 className="text-3xl font-bold text-center tracking-tight mb-3">Try It Live</h2>
          <p className="text-center text-lg mb-10" style={{ color: "#8888a0" }}>
            See delta-state algebra in action &mdash; no install required
          </p>
          <MiniDemo />
        </div>
      </section>

      {/* ===== Comparison ===== */}
      <section className="px-6 py-20" style={{ borderTop: "1px solid #1e1e2e" }}>
        <div className="max-w-5xl mx-auto">
          <h2 className="text-3xl font-bold text-center tracking-tight mb-3">Traditional vs ATOMiK</h2>
          <p className="text-center text-lg mb-12" style={{ color: "#8888a0" }}>
            Side-by-side comparison for distributed state sync across 3 nodes.
          </p>
          <div className="grid md:grid-cols-[1fr_auto_1fr] gap-0 items-stretch">
            {/* Bad side */}
            <div
              className="rounded-xl p-7"
              style={{ background: "#12121a", border: "1px solid rgba(239,68,68,0.3)" }}
            >
              <div className="text-xs font-bold uppercase tracking-widest mb-5" style={{ color: "#ef4444" }}>
                Conventional Approach
              </div>
              {comparisons.map(([label, bad]) => (
                <div key={label} className="flex justify-between items-center py-3 text-sm" style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <span style={{ color: "#8888a0" }}>{label}</span>
                  <span className="font-semibold" style={{ color: "#ef4444" }}>{bad}</span>
                </div>
              ))}
            </div>
            <div className="flex items-center justify-center px-4 text-sm font-bold" style={{ color: "#8888a0" }}>vs</div>
            <div className="rounded-xl p-7" style={{ background: "#12121a", border: "1px solid rgba(34,197,94,0.3)" }}>
              <div className="text-xs font-bold uppercase tracking-widest mb-5" style={{ color: "#22c55e" }}>ATOMiK</div>
              {comparisons.map(([label, , good]) => (
                <div key={label} className="flex justify-between items-center py-3 text-sm" style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <span style={{ color: "#8888a0" }}>{label}</span>
                  <span className="font-semibold" style={{ color: "#22c55e" }}>{good}</span>
                </div>
              ))}
            </div>
          </div>
        </div>
      </section>

      {/* ===== Real-World Results ===== */}
      <section className="px-6 py-20" style={{ borderTop: "1px solid #1e1e2e" }}>
        <div className="max-w-5xl mx-auto">
          <h2 className="text-3xl font-bold text-center tracking-tight mb-3">Real-World Results</h2>
          <p className="text-center text-lg mb-12" style={{ color: "#8888a0" }}>
            How teams are using ATOMiK in production.
          </p>
          <div className="grid md:grid-cols-3 gap-5">
            {[
              {
                company: "QuantumEdge Capital",
                industry: "Financial",
                metric: "99.7% latency reduction",
                desc: "Replaced full order-book snapshots with 8-byte deltas across 12 matching engines.",
                color: "#4f8fff",
              },
              {
                company: "SensorGrid IoT",
                industry: "IoT",
                metric: "97.8% bandwidth savings",
                desc: "50,000 edge sensors streaming delta-only telemetry over LoRaWAN to a single accumulator.",
                color: "#22d3ee",
              },
              {
                company: "CloudSync DB",
                industry: "Database",
                metric: "15x faster sync",
                desc: "Multi-region database replication using XOR-commutative merge instead of conflict resolution.",
                color: "#22c55e",
              },
            ].map((c) => (
              <Link
                key={c.company}
                href="/case-studies"
                className="rounded-xl p-7 no-underline block transition-all hover:-translate-y-1 hover:border-[#4f8fff33]"
                style={{ background: "#12121a", border: "1px solid #1e1e2e", color: "inherit" }}
              >
                <span
                  className="inline-block text-[11px] font-semibold tracking-wide px-2.5 py-1 rounded mb-4"
                  style={{ background: `${c.color}1a`, color: c.color }}
                >
                  {c.industry}
                </span>
                <h3 className="text-lg font-bold mb-1" style={{ color: "#e0e0e8" }}>{c.company}</h3>
                <div className="text-2xl font-extrabold font-mono mb-3" style={{ color: c.color }}>{c.metric}</div>
                <p className="text-sm leading-relaxed mb-4" style={{ color: "#8888a0" }}>{c.desc}</p>
                <span className="text-sm font-semibold" style={{ color: "#4f8fff" }}>
                  Read case study →
                </span>
              </Link>
            ))}
          </div>
        </div>
      </section>

      {/* ===== Proven Technology ===== */}
      <section className="px-6 py-16" style={{ borderTop: "1px solid #1e1e2e" }}>
        <div className="max-w-5xl mx-auto">
          <h2 className="text-2xl font-bold text-center tracking-tight mb-2">Proven Technology</h2>
          <p className="text-center text-sm mb-10" style={{ color: "#8888a0" }}>
            Real engineering milestones &mdash; not promises.
          </p>
          <div className="grid grid-cols-2 md:grid-cols-5 gap-4">
            {[
              {
                value: "92",
                label: "Lean4 Formal Proofs",
                href: "https://github.com/MatthewHRockwell/ATOMiK/tree/main/math/proofs",
                icon: "\u2713",
                color: "#22c55e",
              },
              {
                value: "3",
                label: "FPGA Platforms Validated",
                href: "https://github.com/MatthewHRockwell/ATOMiK",
                icon: "\u25B2",
                color: "#a855f7",
              },
              {
                value: "218+",
                label: "Automated Tests",
                href: "https://github.com/MatthewHRockwell/ATOMiK/actions",
                icon: "\u25CF",
                color: "#4f8fff",
              },
              {
                value: "69.7",
                label: "Gops/s Hardware Verified",
                href: "https://github.com/MatthewHRockwell/ATOMiK",
                icon: "\u26A1",
                color: "#22d3ee",
              },
              {
                value: "Apache 2.0",
                label: "Open Source",
                href: "https://github.com/MatthewHRockwell/ATOMiK",
                icon: "\u2691",
                color: "#f59e0b",
              },
            ].map((item) => (
              <a
                key={item.label}
                href={item.href}
                target="_blank"
                rel="noopener noreferrer"
                className="rounded-xl p-5 text-center transition-all hover:-translate-y-1 no-underline block"
                style={{ background: "#12121a", border: "1px solid #1e1e2e" }}
              >
                <div className="text-lg mb-1" style={{ color: item.color }}>{item.icon}</div>
                <div className="text-xl font-extrabold mb-1" style={{ color: "#e0e0e8" }}>{item.value}</div>
                <div className="text-[11px]" style={{ color: "#8888a0" }}>{item.label}</div>
              </a>
            ))}
          </div>
        </div>
      </section>

      {/* ===== Pricing ===== */}
      <section className="px-6 py-20" style={{ borderTop: "1px solid #1e1e2e" }}>
        <div className="max-w-6xl mx-auto">
          <h2 className="text-3xl font-bold text-center tracking-tight mb-3">Pricing</h2>
          <p className="text-center text-lg mb-12" style={{ color: "#8888a0" }}>
            Start free. Scale when ready.
          </p>
          <div className="grid md:grid-cols-2 lg:grid-cols-4 gap-5">
            {pricingTiers.map((t) => (
              <div
                key={t.name}
                className="rounded-xl p-7 flex flex-col relative"
                style={{
                  background: t.primary ? "#14142a" : "#12121a",
                  border: t.primary ? "1px solid rgba(79,143,255,0.3)" : "1px solid #1e1e2e",
                  boxShadow: t.primary ? "0 0 40px rgba(79,143,255,0.08)" : "none",
                }}
              >
                {t.primary && (
                  <div
                    className="absolute -top-3 left-1/2 -translate-x-1/2 text-[11px] font-bold px-4 py-1 rounded-full tracking-wide"
                    style={{ background: "linear-gradient(135deg, #4f8fff, #3a7aee)", color: "#fff" }}
                  >
                    MOST POPULAR
                  </div>
                )}
                <h3 className="text-xl font-bold mb-1">{t.name}</h3>
                <div className="mb-4">
                  <span className="text-3xl font-extrabold">{t.price}</span>
                  {t.period && <span className="text-sm ml-1" style={{ color: "#8888a0" }}>{t.period}</span>}
                </div>
                <ul className="space-y-2.5 flex-1 mb-6">
                  {t.items.map((item) => (
                    <li key={item} className="flex items-start gap-2 text-sm" style={{ color: "#b0b0c0" }}>
                      <span className="mt-0.5 shrink-0" style={{ color: "#22c55e" }}>{"\u2713"}</span>
                      {item}
                    </li>
                  ))}
                </ul>
                <Link
                  href={t.href}
                  className="block text-center py-3 rounded-lg text-sm font-semibold no-underline transition-opacity hover:opacity-90"
                  style={
                    t.primary
                      ? { background: "#4f8fff", color: "#fff" }
                      : { background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" }
                  }
                >
                  {t.cta}
                </Link>
              </div>
            ))}
          </div>
          <p className="text-center text-sm mt-6" style={{ color: "#8888a0" }}>
            See full feature comparison on the{" "}
            <Link href="/pricing" className="text-[#4f8fff] hover:underline">pricing page</Link>.
          </p>
        </div>
      </section>

      {/* ===== Developer Stats ===== */}
      <section className="py-16" style={{ borderTop: "1px solid #1e1e2e" }}>
        <DeveloperStats />
      </section>

      {/* ===== Email Capture ===== */}
      <section className="max-w-2xl mx-auto px-6 py-12">
        <EmailCapture />
      </section>

    </div>
  );
}

/* Tiny helper for syntax-highlighted code spans */
function Code({ children, k, f, n, c, s }: {
  children: React.ReactNode;
  k?: boolean; f?: boolean; n?: boolean; c?: boolean; s?: boolean;
}) {
  const color = k ? "#4f8fff" : f ? "#22d3ee" : n ? "#f59e0b" : c ? "#555" : s ? "#22c55e" : "#e0e0e8";
  return <span style={{ color, fontStyle: c ? "italic" : undefined }}>{children}</span>;
}
