import type { Metadata } from "next";
import Link from "next/link";

export const metadata: Metadata = {
  title: "Documentation — ATOMiK Delta-State Computing",
  description:
    "Developer documentation for ATOMiK: Quick start, API reference, kernel module, architecture, examples, and FPGA deployment.",
};

/* ── Inline code-block styling ────────────────────────────────────── */

const codeBlockStyle: React.CSSProperties = {
  background: "#0d0d14",
  border: "1px solid #1e1e2e",
  borderRadius: "0.75rem",
  padding: "1.25rem",
  overflowX: "auto",
  fontSize: "0.8125rem",
  lineHeight: "1.7",
  fontFamily: "var(--font-mono), ui-monospace, monospace",
};

const kwColor = "#c084fc"; // purple — keywords
const fnColor = "#4f8fff"; // blue — functions/methods
const strColor = "#22c55e"; // green — strings
const numColor = "#d4a843"; // gold — numbers
const cmtColor = "#555570"; // muted — comments
const typeColor = "#38bdf8"; // cyan — types
const varColor = "#e0e0e8"; // default text — variables

/* ── Example cards data ───────────────────────────────────────────── */

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

/* ── Hardware platforms data ──────────────────────────────────────── */

const platforms = [
  {
    name: "Tang Nano 9K",
    chip: "Gowin GW1NR-9K",
    price: "$13.50",
    desc: "Production SoC with PicoRV32 @ 25.2 MHz + ATOMiK @ 81 MHz. Custom RV64I CPU with native ATOMiK ISA extensions. 1280x720 HDMI output.",
    stats: "94.5 Mops/s \u00b7 477 LUT \u00b7 81 MHz",
    color: "#22c55e",
    border: "border-green-500/30",
  },
  {
    name: "Xilinx Zynq XC7Z020",
    chip: "ALINX AX7020",
    price: "~$100",
    desc: "512 parallel banks at 135.6 MHz. 444 MHz single-bank. Full ceiling characterization across 6 configs with 4 Vivado strategies each.",
    stats: "69.7 Gops/s \u00b7 23,542 LUT \u00b7 512 banks",
    color: "#d4a843",
    border: "border-yellow-500/30",
  },
];

/* ── API reference links ──────────────────────────────────────────── */

const apiLinks = [
  {
    lang: "Python",
    pkg: "atomik-core",
    classes: ["AtomikContext", "AtomikTable", "DeltaStream", "Fingerprint"],
    install: "pip install atomik-core",
    color: "#4f8fff",
  },
  {
    lang: "C",
    pkg: "atomik_core.h",
    classes: ["atomik_ctx_t", "atomik_table_t", "atomik_fingerprint_t"],
    install: "#include \"atomik_core.h\"",
    color: "#22c55e",
  },
  {
    lang: "Kernel Module",
    pkg: "/dev/atomik",
    classes: [
      "ATOMIK_IOC_LOAD",
      "ATOMIK_IOC_ACCUM",
      "ATOMIK_IOC_READ",
      "ATOMIK_IOC_SWAP",
      "ATOMIK_IOC_BATCH",
    ],
    install: "sudo ./install.sh",
    color: "#d4a843",
  },
];

/* ── Page component ───────────────────────────────────────────────── */

export default function DocsPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      {/* Navigation */}
      <nav
        className="sticky top-0 z-50 backdrop-blur-md border-b"
        style={{ background: "rgba(10, 10, 15, 0.85)", borderColor: "#1e1e2e" }}
      >
        <div className="max-w-5xl mx-auto px-6 py-4 flex items-center justify-between">
          <Link
            href="/"
            className="text-lg font-bold tracking-tight hover:opacity-80 transition-opacity"
          >
            <span style={{ color: "#8b5cf6" }}>ATOM</span>
            <span style={{ color: "#4f8fff" }}>i</span>
            <span style={{ color: "#8b5cf6" }}>K</span>
          </Link>
          <div className="flex items-center gap-6 text-sm" style={{ color: "#8888a0" }}>
            <Link href="/" className="hover:text-white transition-colors">
              Home
            </Link>
            <Link href="/about" className="hover:text-white transition-colors">
              About
            </Link>
            <span className="text-white font-medium">Docs</span>
          </div>
        </div>
      </nav>

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
            Developer Documentation
          </p>
          <h1 className="text-5xl sm:text-6xl font-bold tracking-tight mb-6">
            <span
              className="bg-clip-text text-transparent"
              style={{
                backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff, #22c55e)",
              }}
            >
              Docs
            </span>
          </h1>
          <p className="text-xl leading-relaxed max-w-3xl" style={{ color: "#8888a0" }}>
            Everything you need to integrate delta-state algebra into your stack
            &mdash; from a single Python import to a Linux kernel module to custom FPGA silicon.
          </p>
        </div>
      </section>

      {/* ─────────────────────────── Quick Start ─────────────────────────── */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Quick Start</h2>
        <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
          Install the Python SDK and run your first delta-state operation in under a minute.
        </p>

        {/* Install */}
        <div className="mb-6">
          <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
            Install
          </p>
          <div style={codeBlockStyle}>
            <code>
              <span style={{ color: cmtColor }}>$</span>{" "}
              <span style={{ color: varColor }}>pip install atomik-core</span>
            </code>
          </div>
        </div>

        {/* 4-operation example */}
        <div className="mb-6">
          <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
            The four operations
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: kwColor }}>from</span>{" "}
                <span style={{ color: varColor }}>atomik_core</span>{" "}
                <span style={{ color: kwColor }}>import</span>{" "}
                <span style={{ color: typeColor }}>AtomikContext</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>
                  # Create a single delta-state context
                </span>
                {"\n"}
                <span style={{ color: varColor }}>ctx</span>{" "}
                <span style={{ color: kwColor }}>=</span>{" "}
                <span style={{ color: typeColor }}>AtomikContext</span>
                <span style={{ color: varColor }}>()</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>
                  # LOAD — set the initial reference state
                </span>
                {"\n"}
                <span style={{ color: varColor }}>ctx.</span>
                <span style={{ color: fnColor }}>load</span>
                <span style={{ color: varColor }}>(</span>
                <span style={{ color: numColor }}>0xDEADBEEF</span>
                <span style={{ color: varColor }}>)</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>
                  # ACCUM — XOR a delta into the accumulator
                </span>
                {"\n"}
                <span style={{ color: varColor }}>ctx.</span>
                <span style={{ color: fnColor }}>accum</span>
                <span style={{ color: varColor }}>(</span>
                <span style={{ color: numColor }}>0x000000FF</span>
                <span style={{ color: varColor }}>)</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>
                  # READ — reconstruct current state (reference XOR accumulator)
                </span>
                {"\n"}
                <span style={{ color: kwColor }}>assert</span>{" "}
                <span style={{ color: varColor }}>ctx.</span>
                <span style={{ color: fnColor }}>read</span>
                <span style={{ color: varColor }}>() == </span>
                <span style={{ color: numColor }}>0xDEADBE10</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>
                  # SWAP — atomic snapshot + reset accumulator
                </span>
                {"\n"}
                <span style={{ color: varColor }}>snapshot</span>{" "}
                <span style={{ color: kwColor }}>=</span>{" "}
                <span style={{ color: varColor }}>ctx.</span>
                <span style={{ color: fnColor }}>swap</span>
                <span style={{ color: varColor }}>()</span>
                {"\n"}
                <span style={{ color: kwColor }}>assert</span>{" "}
                <span style={{ color: varColor }}>snapshot == </span>
                <span style={{ color: numColor }}>0xDEADBE10</span>
                {"  "}
                <span style={{ color: cmtColor }}># previous state</span>
                {"\n"}
                <span style={{ color: kwColor }}>assert</span>{" "}
                <span style={{ color: varColor }}>ctx.</span>
                <span style={{ color: fnColor }}>read</span>
                <span style={{ color: varColor }}>() == </span>
                <span style={{ color: numColor }}>0xDEADBE10</span>
                {"    "}
                <span style={{ color: cmtColor }}># accumulator reset, state preserved</span>
              </code>
            </pre>
          </div>
        </div>

        {/* Key insight callout */}
        <div
          className="rounded-xl p-6 border"
          style={{
            background: "linear-gradient(135deg, rgba(139,92,246,0.06), rgba(79,143,255,0.06))",
            borderColor: "#8b5cf630",
          }}
        >
          <p className="text-sm font-semibold mb-2" style={{ color: "#8b5cf6" }}>
            Key insight
          </p>
          <p style={{ color: "#b0b0c0" }}>
            State is never stored — it is reconstructed:{" "}
            <code
              className="text-sm font-mono px-2 py-0.5 rounded"
              style={{ background: "#1e1e2e", color: "#22c55e" }}
            >
              current_state = initial_state &oplus; accumulator
            </code>
            . Deltas are commutative, associative, and self-inverse (XOR Abelian group).
            Order does not matter. Duplicate deltas cancel. Zero dependencies, Python 3.9+.
          </p>
        </div>
      </section>

      {/* ─────────────────────── Kernel Module ───────────────────────────── */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Kernel Module</h2>
        <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
          The ATOMiK Linux kernel module exposes delta-state operations via{" "}
          <code
            className="text-sm font-mono px-2 py-0.5 rounded"
            style={{ background: "#1e1e2e", color: "#d4a843" }}
          >
            /dev/atomik
          </code>{" "}
          and sysfs. DKMS-managed, supports kernels 5.15+.
        </p>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-8">
          {/* Install + Load */}
          <div>
            <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
              Install &amp; load
            </p>
            <div style={codeBlockStyle}>
              <pre style={{ margin: 0 }}>
                <code>
                  <span style={{ color: cmtColor }}># Clone and install via DKMS</span>
                  {"\n"}
                  <span style={{ color: cmtColor }}>$</span>{" "}
                  <span style={{ color: varColor }}>
                    git clone https://github.com/MatthewHRockwell/ATOMiK
                  </span>
                  {"\n"}
                  <span style={{ color: cmtColor }}>$</span>{" "}
                  <span style={{ color: kwColor }}>cd</span>{" "}
                  <span style={{ color: varColor }}>ATOMiK/software/atomik_kmod</span>
                  {"\n"}
                  <span style={{ color: cmtColor }}>$</span>{" "}
                  <span style={{ color: kwColor }}>sudo</span>{" "}
                  <span style={{ color: varColor }}>./install.sh</span>
                  {"\n\n"}
                  <span style={{ color: cmtColor }}># Verify it loaded</span>
                  {"\n"}
                  <span style={{ color: cmtColor }}>$</span>{" "}
                  <span style={{ color: varColor }}>
                    cat /sys/class/atomik/atomik0/version
                  </span>
                  {"\n"}
                  <span style={{ color: strColor }}>0.4.0</span>
                </code>
              </pre>
            </div>
          </div>

          {/* Sysfs interface */}
          <div>
            <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
              sysfs interface
            </p>
            <div style={codeBlockStyle}>
              <pre style={{ margin: 0 }}>
                <code>
                  <span style={{ color: cmtColor }}># Runtime status</span>
                  {"\n"}
                  <span style={{ color: cmtColor }}>$</span>{" "}
                  <span style={{ color: varColor }}>
                    cat /sys/class/atomik/atomik0/backend
                  </span>
                  {"\n"}
                  <span style={{ color: strColor }}>software</span>
                  {"\n\n"}
                  <span style={{ color: cmtColor }}># Operation counters</span>
                  {"\n"}
                  <span style={{ color: cmtColor }}>$</span>{" "}
                  <span style={{ color: varColor }}>
                    cat /sys/class/atomik/atomik0/ops_total
                  </span>
                  {"\n"}
                  <span style={{ color: numColor }}>0</span>
                  {"\n\n"}
                  <span style={{ color: cmtColor }}># Per-operation: ops_load, ops_accum,</span>
                  {"\n"}
                  <span style={{ color: cmtColor }}># ops_read, ops_swap</span>
                </code>
              </pre>
            </div>
          </div>
        </div>

        {/* ioctl example */}
        <div className="mb-6">
          <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
            ioctl usage (C)
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: kwColor }}>#include</span>{" "}
                <span style={{ color: strColor }}>&lt;uapi/atomik.h&gt;</span>
                {"\n\n"}
                <span style={{ color: typeColor }}>int</span>{" "}
                <span style={{ color: varColor }}>fd</span>{" "}
                <span style={{ color: kwColor }}>=</span>{" "}
                <span style={{ color: fnColor }}>open</span>
                <span style={{ color: varColor }}>(</span>
                <span style={{ color: strColor }}>"/dev/atomik"</span>
                <span style={{ color: varColor }}>, O_RDWR);</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>// Create a table with 256 contexts</span>
                {"\n"}
                <span style={{ color: kwColor }}>struct</span>{" "}
                <span style={{ color: typeColor }}>atomik_create_table_args</span>{" "}
                <span style={{ color: varColor }}>ct</span>{" "}
                <span style={{ color: kwColor }}>=</span>{" "}
                <span style={{ color: varColor }}>{"{"}</span>{" "}
                <span style={{ color: varColor }}>.num_contexts</span>{" "}
                <span style={{ color: kwColor }}>=</span>{" "}
                <span style={{ color: numColor }}>256</span>{" "}
                <span style={{ color: varColor }}>{"}"}</span>
                <span style={{ color: varColor }}>;</span>
                {"\n"}
                <span style={{ color: fnColor }}>ioctl</span>
                <span style={{ color: varColor }}>(fd, ATOMIK_IOC_CREATE_TABLE, &amp;ct);</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>// Core operations via ioctl</span>
                {"\n"}
                <span style={{ color: kwColor }}>struct</span>{" "}
                <span style={{ color: typeColor }}>atomik_load_args</span>{" "}
                <span style={{ color: varColor }}>la</span>{" "}
                <span style={{ color: kwColor }}>=</span>{" "}
                <span style={{ color: varColor }}>
                  {"{"} .table_id = ct.table_id,
                </span>
                {"\n"}
                <span style={{ color: varColor }}>
                  {"    "}.addr = <span style={{ color: numColor }}>0</span>, .initial_state ={" "}
                  <span style={{ color: numColor }}>0xDEADBEEF</span> {"}"};
                </span>
                {"\n"}
                <span style={{ color: fnColor }}>ioctl</span>
                <span style={{ color: varColor }}>(fd, ATOMIK_IOC_LOAD, &amp;la);</span>
              </code>
            </pre>
          </div>
        </div>
      </section>

      {/* ────────────────────── API Reference ────────────────────────────── */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">API Reference</h2>
        <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
          Three interfaces, one algebra. Every API exposes the same four operations:
          LOAD, ACCUM, READ, SWAP.
        </p>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          {apiLinks.map((api) => (
            <div
              key={api.lang}
              className="rounded-xl p-6 border transition-all duration-300 hover:-translate-y-1"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex items-center gap-3 mb-4">
                <div
                  className="w-2 h-2 rounded-full"
                  style={{ background: api.color }}
                />
                <h3 className="text-lg font-bold" style={{ color: api.color }}>
                  {api.lang}
                </h3>
              </div>
              <code
                className="block text-xs font-mono px-3 py-2 rounded-lg mb-4"
                style={{ background: "#0d0d14", color: "#8888a0" }}
              >
                {api.install}
              </code>
              <ul className="space-y-1.5">
                {api.classes.map((cls) => (
                  <li key={cls} className="text-sm font-mono" style={{ color: "#b0b0c0" }}>
                    <span style={{ color: api.color, opacity: 0.6 }}>&#x25b8;</span>{" "}
                    {cls}
                  </li>
                ))}
              </ul>
            </div>
          ))}
        </div>
      </section>

      {/* ─────────────────────── Architecture ────────────────────────────── */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Architecture</h2>
        <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
          ATOMiK is not Von Neumann. State is reconstructed, not stored.
        </p>

        <div
          className="rounded-2xl p-8 sm:p-12 border mb-8"
          style={{
            background: "linear-gradient(135deg, #12121a, #181824)",
            borderColor: "#1e1e2e",
          }}
        >
          {/* Core equation */}
          <div className="text-center mb-10">
            <p
              className="text-sm font-mono tracking-widest uppercase mb-4"
              style={{ color: "#8888a0" }}
            >
              The fundamental equation
            </p>
            <div
              className="inline-block px-8 py-4 rounded-xl border"
              style={{ background: "#0d0d14", borderColor: "#8b5cf630" }}
            >
              <code className="text-xl sm:text-2xl font-mono font-bold">
                <span style={{ color: fnColor }}>current_state</span>{" "}
                <span style={{ color: "#8888a0" }}>=</span>{" "}
                <span style={{ color: kwColor }}>initial_state</span>{" "}
                <span style={{ color: "#22c55e" }}>&oplus;</span>{" "}
                <span style={{ color: numColor }}>accumulator</span>
              </code>
            </div>
          </div>

          {/* Four properties */}
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-6">
            {[
              {
                property: "Commutativity",
                formula: "a \u2295 b = b \u2295 a",
                desc: "Deltas can arrive in any order. No coordination needed between producers.",
                color: "#8b5cf6",
              },
              {
                property: "Associativity",
                formula: "(a \u2295 b) \u2295 c = a \u2295 (b \u2295 c)",
                desc: "Deltas can be grouped and merged in any tree structure. Enables parallel reduction.",
                color: "#4f8fff",
              },
              {
                property: "Self-inverse",
                formula: "a \u2295 a = 0",
                desc: "Any delta applied twice cancels itself. Duplicate packets are harmless. Undo is free.",
                color: "#22c55e",
              },
              {
                property: "Identity",
                formula: "a \u2295 0 = a",
                desc: "Accumulating zero changes nothing. The accumulator starts empty and grows only with real deltas.",
                color: "#d4a843",
              },
            ].map((prop) => (
              <div
                key={prop.property}
                className="rounded-xl p-5 border"
                style={{ background: "#0d0d14", borderColor: "#1e1e2e" }}
              >
                <h4 className="font-bold mb-1" style={{ color: prop.color }}>
                  {prop.property}
                </h4>
                <code
                  className="block text-sm font-mono mb-3 px-3 py-1.5 rounded-lg"
                  style={{ background: "#12121a", color: "#e0e0e8" }}
                >
                  {prop.formula}
                </code>
                <p className="text-sm" style={{ color: "#8888a0" }}>
                  {prop.desc}
                </p>
              </div>
            ))}
          </div>
        </div>

        {/* Four operations */}
        <div className="grid grid-cols-2 lg:grid-cols-4 gap-4">
          {[
            {
              op: "LOAD",
              desc: "Set initial reference state",
              detail: "reference = value; accumulator = 0",
              color: "#8b5cf6",
            },
            {
              op: "ACCUM",
              desc: "XOR delta into accumulator",
              detail: "accumulator ^= delta",
              color: "#4f8fff",
            },
            {
              op: "READ",
              desc: "Reconstruct current state",
              detail: "return reference ^ accumulator",
              color: "#22c55e",
            },
            {
              op: "SWAP",
              desc: "Atomic snapshot + reset",
              detail: "old = read(); reference = old; accumulator = 0; return old",
              color: "#d4a843",
            },
          ].map((item) => (
            <div
              key={item.op}
              className="rounded-xl p-5 border"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div
                className="text-sm font-mono font-bold mb-2 px-2 py-0.5 rounded inline-block"
                style={{ background: "#0d0d14", color: item.color }}
              >
                {item.op}
              </div>
              <p className="text-sm font-semibold text-white mb-1">{item.desc}</p>
              <p className="text-xs font-mono" style={{ color: "#8888a0" }}>
                {item.detail}
              </p>
            </div>
          ))}
        </div>
      </section>

      {/* ─────────────────────── Examples ────────────────────────────────── */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Examples</h2>
        <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
          Production-ready patterns showing delta-state algebra in real workloads.
        </p>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
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
      </section>

      {/* ─────────────────────── Hardware ────────────────────────────────── */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Hardware</h2>
        <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
          ATOMiK runs on real silicon. FPGA-validated across two platforms, from $13.50 to 69.7 Gops/s.
        </p>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-8">
          {platforms.map((p) => (
            <div
              key={p.name}
              className={`rounded-xl p-6 border ${p.border} transition-all duration-300 hover:scale-[1.01]`}
              style={{ background: "#12121a" }}
            >
              <div className="flex items-center justify-between mb-3">
                <h3 className="text-lg font-bold" style={{ color: p.color }}>
                  {p.name}
                </h3>
                <span
                  className="text-xs font-mono px-2 py-0.5 rounded"
                  style={{ background: "#0d0d14", color: "#8888a0" }}
                >
                  {p.price}
                </span>
              </div>
              <p className="text-xs font-mono mb-3" style={{ color: "#555570" }}>
                {p.chip}
              </p>
              <p className="text-sm mb-4 leading-relaxed" style={{ color: "#b0b0c0" }}>
                {p.desc}
              </p>
              <div
                className="text-xs font-mono px-3 py-2 rounded-lg"
                style={{ background: "#0d0d14", color: p.color }}
              >
                {p.stats}
              </div>
            </div>
          ))}
        </div>

        {/* Scaling note */}
        <div
          className="rounded-xl p-6 border"
          style={{
            background: "linear-gradient(135deg, rgba(34,197,94,0.06), rgba(212,168,67,0.06))",
            borderColor: "#22c55e30",
          }}
        >
          <p className="text-sm font-semibold mb-2" style={{ color: "#22c55e" }}>
            Sub-linear LUT scaling
          </p>
          <p className="text-sm" style={{ color: "#b0b0c0" }}>
            Each additional bank costs ~34 LUTs (at N=512). 16x throughput requires only 3.7x more
            logic. All configurations share a single BRAM. The architecture is designed for parallel
            scaling — the accumulator is a shared resource by design because XOR commutativity means
            multiple producers can feed deltas in any order and the result is identical.
          </p>
        </div>
      </section>

      {/* ─────────────────── Multi-context table ─────────────────────────── */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold mb-4">Multi-Context Tables</h2>
        <p className="mb-8 text-lg" style={{ color: "#8888a0" }}>
          Track thousands of independent state channels in a single table.
        </p>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Python */}
          <div>
            <p className="text-sm font-mono mb-2" style={{ color: "#4f8fff" }}>
              Python
            </p>
            <div style={codeBlockStyle}>
              <pre style={{ margin: 0 }}>
                <code>
                  <span style={{ color: kwColor }}>from</span>{" "}
                  <span style={{ color: varColor }}>atomik_core</span>{" "}
                  <span style={{ color: kwColor }}>import</span>{" "}
                  <span style={{ color: typeColor }}>AtomikTable</span>
                  {"\n\n"}
                  <span style={{ color: varColor }}>table</span>{" "}
                  <span style={{ color: kwColor }}>=</span>{" "}
                  <span style={{ color: typeColor }}>AtomikTable</span>
                  <span style={{ color: varColor }}>(num_contexts=</span>
                  <span style={{ color: numColor }}>256</span>
                  <span style={{ color: varColor }}>)</span>
                  {"\n"}
                  <span style={{ color: varColor }}>table.</span>
                  <span style={{ color: fnColor }}>load</span>
                  <span style={{ color: varColor }}>(addr=</span>
                  <span style={{ color: numColor }}>0</span>
                  <span style={{ color: varColor }}>, initial_state=</span>
                  <span style={{ color: numColor }}>0xCAFEBABE</span>
                  <span style={{ color: varColor }}>)</span>
                  {"\n"}
                  <span style={{ color: varColor }}>table.</span>
                  <span style={{ color: fnColor }}>accum</span>
                  <span style={{ color: varColor }}>(addr=</span>
                  <span style={{ color: numColor }}>0</span>
                  <span style={{ color: varColor }}>, delta=</span>
                  <span style={{ color: numColor }}>0x00000001</span>
                  <span style={{ color: varColor }}>)</span>
                  {"\n"}
                  <span style={{ color: kwColor }}>assert</span>{" "}
                  <span style={{ color: varColor }}>table.</span>
                  <span style={{ color: fnColor }}>read</span>
                  <span style={{ color: varColor }}>(addr=</span>
                  <span style={{ color: numColor }}>0</span>
                  <span style={{ color: varColor }}>) == </span>
                  <span style={{ color: numColor }}>0xCAFEBABF</span>
                </code>
              </pre>
            </div>
          </div>

          {/* C */}
          <div>
            <p className="text-sm font-mono mb-2" style={{ color: "#22c55e" }}>
              C (single-header library)
            </p>
            <div style={codeBlockStyle}>
              <pre style={{ margin: 0 }}>
                <code>
                  <span style={{ color: kwColor }}>#define</span>{" "}
                  <span style={{ color: varColor }}>ATOMIK_IMPLEMENTATION</span>
                  {"\n"}
                  <span style={{ color: kwColor }}>#include</span>{" "}
                  <span style={{ color: strColor }}>"atomik_core.h"</span>
                  {"\n\n"}
                  <span style={{ color: typeColor }}>atomik_table_t</span>{" "}
                  <span style={{ color: varColor }}>table;</span>
                  {"\n"}
                  <span style={{ color: fnColor }}>atomik_table_init</span>
                  <span style={{ color: varColor }}>(&amp;table, </span>
                  <span style={{ color: numColor }}>256</span>
                  <span style={{ color: varColor }}>);</span>
                  {"\n"}
                  <span style={{ color: fnColor }}>atomik_table_load</span>
                  <span style={{ color: varColor }}>(&amp;table, </span>
                  <span style={{ color: numColor }}>0</span>
                  <span style={{ color: varColor }}>, </span>
                  <span style={{ color: numColor }}>0xCAFEBABE</span>
                  <span style={{ color: varColor }}>);</span>
                  {"\n"}
                  <span style={{ color: fnColor }}>atomik_table_accum</span>
                  <span style={{ color: varColor }}>(&amp;table, </span>
                  <span style={{ color: numColor }}>0</span>
                  <span style={{ color: varColor }}>, </span>
                  <span style={{ color: numColor }}>0x00000001</span>
                  <span style={{ color: varColor }}>);</span>
                  {"\n"}
                  <span style={{ color: typeColor }}>uint64_t</span>{" "}
                  <span style={{ color: varColor }}>s = </span>
                  <span style={{ color: fnColor }}>atomik_table_read</span>
                  <span style={{ color: varColor }}>(&amp;table, </span>
                  <span style={{ color: numColor }}>0</span>
                  <span style={{ color: varColor }}>);</span>
                  {"\n"}
                  <span style={{ color: cmtColor }}>// s == 0xCAFEBABF</span>
                </code>
              </pre>
            </div>
          </div>
        </div>
      </section>

      {/* ─────────────────── Bottom CTA ──────────────────────────────────── */}
      <section className="max-w-5xl mx-auto px-6 pb-24">
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
      </section>

      {/* Footer */}
      <footer
        className="border-t py-8"
        style={{ borderColor: "#1e1e2e", background: "#0a0a0f" }}
      >
        <div
          className="max-w-5xl mx-auto px-6 flex flex-col sm:flex-row items-center justify-between gap-4 text-sm"
          style={{ color: "#8888a0" }}
        >
          <div>
            <span style={{ color: "#8b5cf6" }}>ATOM</span>
            <span style={{ color: "#4f8fff" }}>i</span>
            <span style={{ color: "#8b5cf6" }}>K</span>
            <span className="ml-2">Delta-State Computing</span>
          </div>
          <div className="flex gap-6">
            <Link href="/" className="hover:text-white transition-colors">
              Home
            </Link>
            <Link href="/about" className="hover:text-white transition-colors">
              About
            </Link>
            <Link href="/about/roadmap" className="hover:text-white transition-colors">
              Roadmap
            </Link>
            <a
              href="mailto:mrockwell@atomik.tech"
              className="hover:text-white transition-colors"
            >
              Contact
            </a>
          </div>
        </div>
      </footer>
    </div>
  );
}
