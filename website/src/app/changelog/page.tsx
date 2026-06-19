import Link from "next/link";
import Nav from "@/components/Nav";

/* ── Release data ────────────────────────────────────────────────── */

type Release = {
  version: string;
  title: string;
  date: string;
  category: "software" | "hardware" | "tools";
  items: string[];
  links?: { label: string; href: string }[];
};

const evidenceLinks = {
  deskProof: "/10-current-live-atomik-desk-v040a.png",
  perfMatrix: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/perf_matrix_ax7020_20260509.txt",
  perfInterpretation: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/perf/20260509_matrix_interpretation.md",
  linuxUserspace: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/LINUX_USERSPACE_PROOF.md",
  hardwareSynthesis: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/HARDWARE_SYNTHESIS.md",
  kernelEvaluation: "/docs/kernel-module",
  hardwareProof: "/docs/hardware",
} as const;

const releases: Release[] = [
  {
    version: "bring-up",
    title: "Standalone SD Boot Reached Minimal FSBL",
    date: "2026-05-24",
    category: "hardware",
    items: [
      "Strict-FAT32 SD boot now reaches the minimal FSBL from BootROM",
      "PL bitstream programming completes; PCFG_DONE is observed high",
      "Remaining gate is FSBL exception handling and final autonomous handoff",
      "Classified as BUILD_ARTIFACT until public power-on logs support a stronger claim",
    ],
    links: [{ label: "Open hardware proof map", href: evidenceLinks.hardwareProof }],
  },
  {
    version: "parallel-banks",
    title: "Parallel-Bank Throughput Measured on AX7020",
    date: "2026-05-30",
    category: "hardware",
    items: [
      "An 8-bank ATOMiK XOR accumulator scaled linearly with allocated banks on the AX7020, measured with an on-chip cycle counter",
      "1/2/4/8 banks complete the same accumulation in 1/2/4/8x fewer cycles (about 100 to 800 Mdeltas/s at the 100 MHz sys clock)",
      "The result was byte-identical across every bank count and matched a software recompute: order-independent and lock-free by construction",
      "Classified as LIVE_MEASURED for the throughput substrate (bench engine), not a customer-workload speedup, power, or thermal claim",
      "Same effort confirmed 1080p60 is physically impossible on this board (OSERDES2 pulse-width limit); the UI renders at 1080p30",
    ],
    links: [{ label: "Open parallel-bank proof", href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/hardware/zynq/results/PARALLEL_BANKS_HARDWARE_VALIDATED.md" }],
  },
  {
    version: "v0.40-A",
    title: "ATOMiK Desk Current Live Prototype UI Proof",
    date: "2026-05-29",
    category: "hardware",
    items: [
      "Current live-hardware framebuffer capture (fb2png of /dev/fb0) promoted as the public ATOMiK Desk proof artifact, superseding v0.39-K",
      "Concept-aligned hero, Pulse Bar, Cap Rail, and Resource Fabric surfaces visible in the captured prototype UI, with on-screen metrics driven by real measured on-board data",
      "Classified as HARDWARE_VALIDATED for the UI surface, not as a commercial desktop product or performance benchmark",
      "Public proof asset: 10-current-live-atomik-desk-v040a.png",
    ],
    links: [{ label: "Open proof image", href: evidenceLinks.deskProof }],
  },
  {
    version: "",
    title: "Phase 10: Internal Investor Demo System",
    date: "2026-04-28",
    category: "hardware",
    items: [
      "Internal scripted demo screens for explaining state waste, delta accumulation, and hardware proof boundaries",
      "Scripted operator demo flow with proof cards for hardware validation, formal verification, and benchmark review",
      "State Storm visualization: software scans are contrasted with delta-state accumulation as a narrative model",
      "Cost and race visualizations are treated as demo surfaces unless linked to measured artifacts",
      "Slide sync, framebuffer screenshot capture, and browser control plane built for live presentation operations",
    ],
  },
  {
    version: "",
    title: "Multi-Buffer Change Detection Benchmark",
    date: "2026-04-06",
    category: "hardware",
    items: [
      "End-to-end workload: track N memory regions, detect changes via ATOMiK vs memcmp",
      "ATOMiK accumulator-path detection: ~262 cycles per region in the linked AX7020 benchmark artifact",
      "Software memcmp comparison retained in the linked benchmark artifact and interpretation note",
      "Measured results retained in benchmark artifacts; quote exact speedups only from the linked evidence files",
      "Monitoring-rate claims require the matching board-run artifact and interpretation note",
      "libatomik C runtime with /dev/mem backend and MMIO ordering fences",
    ],
    links: [
      { label: "Benchmark artifact", href: evidenceLinks.perfMatrix },
      { label: "Interpretation note", href: evidenceLinks.perfInterpretation },
    ],
  },
  {
    version: "",
    title: "Linux Userspace Validation on Zynq",
    date: "2026-04-06",
    category: "hardware",
    items: [
      "ATOMiK 16/16 PASS from Linux 6.9 userspace (S-mode, MMU enabled, /dev/mem mmap)",
      "HARDWARE_VALIDATED stack path: user process -> kernel -> Wishbone CSR -> ATOMiK core",
      "Linux 6.9 + OpenSBI 1.3.1 booting on VexRiscv SMP (Zynq XC7Z020)",
      "MMIO ordering: fence iorw,iorw + STATUS readback required for Wishbone CSR correctness",
      "Frozen baseline with SHA-256 manifest (tag: zynq-linux-v1)",
    ],
    links: [{ label: "Linux userspace proof", href: evidenceLinks.linuxUserspace }],
  },
  {
    version: "v0.5.0",
    title: "Kernel Module — Archived Evaluation Experiments",
    date: "2026-03-16",
    category: "tools",
    items: [
      "/proc/atomik/audit ring-buffer audit log (last 1024 operations)",
      "perf/ftrace tracepoints wired to all 4 ioctl handlers",
      "Memory shrinkers for COW and network hash tables",
      "atomik-report waste analysis tool (--json/--csv/--brief)",
      "Prometheus exporter with health endpoints (/healthz, /ready)",
      "Grafana dashboard (3-row pre-built JSON)",
      "Kubernetes: DaemonSet, Helm chart with RBAC + NetworkPolicy + probes",
      "License-key experiment archived; public evaluation is now request-based",
    ],
  },
  {
    version: "v0.4.0",
    title: "Python SDK - Historical Release Notes",
    date: "2026-03-16",
    category: "software",
    items: [
      "PersistentSyncTable: crash-safe delta persistence with WAL and compaction",
      "AsyncSyncTable + AsyncCallbackTransport for asyncio/FastAPI/aiohttp",
      "Benchmark --share flag for shareable result summaries",
      "DeviceContext: Python bindings for /dev/atomik kernel module ioctls",
      "SyncTable + MemoryTransport + CallbackTransport (full transport layer)",
      "C accelerator wired into Fingerprint hot path",
      "200+ tests passing",
      "pip install atomik-core==0.4.0",
    ],
  },
  {
    version: "v0.3.0",
    title: "Python SDK — Serialization & Tracing",
    date: "2026-03-16",
    category: "software",
    items: [
      "DeltaMessage serialization: to_dict/from_dict + to_bytes/from_bytes (16-byte wire format)",
      "Fingerprint encapsulation fix: no more direct _accumulator access",
      "Benchmark --json flag for CI pipeline integration",
      "All bench_* functions return structured result dicts",
      "67 tests passing (up from 60)",
      "pip install atomik-core==0.3.0",
    ],
  },
  {
    version: "v0.4.0-kmod",
    title: "Kernel Module — Initial Release",
    date: "2026-03-15",
    category: "software",
    items: [
      "COW redundancy detection via kretprobe on wp_page_copy()",
      "TCP send deduplication with CRC32C fingerprinting (SSE4.2)",
      "Per-container/cgroup waste attribution for Kubernetes",
      "/proc/atomik machine-readable metrics interface",
      "27 sysfs attributes for real-time monitoring",
      "DKMS packaging, systemd service, udev rules",
      "Request-based evaluation access and evidence review",
    ],
    links: [{ label: "Kernel evaluation notes", href: evidenceLinks.kernelEvaluation }],
  },
  {
    version: "v0.2.0",
    title: "Python SDK — Benchmarks & PyPI",
    date: "2026-03-15",
    category: "software",
    items: [
      "Benchmark module: python -m atomik_core.benchmark",
      "5 public benchmark functions exported",
      "Project URLs on PyPI (atomik.tech, docs, changelog)",
      "60 tests passing, zero dependencies",
      "pip install atomik-core==0.2.0",
    ],
  },
  {
    version: "v0.1.0",
    title: "Initial Release",
    date: "2026-03-14",
    category: "software",
    items: [
      "AtomikContext: LOAD, ACCUM, READ, SWAP operations",
      "AtomikTable: 256-context state table",
      "DeltaStream: multi-context streaming with message serialization",
      "Fingerprint: XOR-based change detection",
      "Python 3.9\u20133.13, Apache 2.0, zero dependencies",
      "50+ tests",
    ],
  },
  {
    version: "",
    title: "Zynq Characterization",
    date: "2026-02",
    category: "hardware",
    items: [
      "6 configs (N=1 to N=512), 4 synthesis strategies",
      "N=512 peak synthesis configuration documented for XC7Z020",
      "SYNTHESIS_VALIDATED LUT scaling is documented in the linked synthesis artifact",
    ],
    links: [{ label: "Synthesis artifact", href: evidenceLinks.hardwareSynthesis }],
  },
  {
    version: "v3.1.0",
    title: "HD HDMI SoC",
    date: "2025-12",
    category: "hardware",
    items: [
      "1280\u00d7720@60Hz on $13.50 Tang Nano 9K",
      "Custom RV64I CPU with native ATOMiK ISA extensions",
      "Delta-driven display pipeline",
      "6,287 LUT (73%), zero TNS",
      "Archived Tang Nano prototype path; figures are historical hardware artifacts, not current Zynq proof. Use the hardware proof map for public claims.",
    ],
  },
  {
    version: "v2.0.0",
    title: "PicoRV32 Prototype SoC",
    date: "2025-09",
    category: "hardware",
    items: [
      "Single-bank ATOMiK timing and throughput documented in historical hardware artifacts",
      "PicoRV32 integration, SPI XIP boot",
      "11/11 hardware tests, +23% Fmax margin",
      "Archived prototype; quote LUT, Fmax, and test figures only from the linked historical hardware artifacts, not as current claims.",
    ],
  },
];

/* ── Category styling ────────────────────────────────────────────── */

const categoryConfig = {
  software: {
    label: "Software",
    color: "#22c55e",
    bg: "rgba(34, 197, 94, 0.08)",
    border: "rgba(34, 197, 94, 0.25)",
  },
  hardware: {
    label: "Hardware",
    color: "#a855f7",
    bg: "rgba(168, 85, 247, 0.08)",
    border: "rgba(168, 85, 247, 0.25)",
  },
  tools: {
    label: "Tools",
    color: "#4f8fff",
    bg: "rgba(79, 143, 255, 0.08)",
    border: "rgba(79, 143, 255, 0.25)",
  },
};

/* ── Page component ──────────────────────────────────────────────── */

export default function ChangelogPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Hero */}
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
            Release History
          </p>
          <h1 className="text-5xl sm:text-6xl font-bold tracking-tight mb-6">
            <span
              className="bg-clip-text text-transparent"
              style={{
                backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff, #22c55e)",
              }}
            >
              Changelog
            </span>
          </h1>
          <p className="text-xl leading-relaxed max-w-3xl" style={{ color: "#8888a0" }}>
            Evidence-labeled updates across software, hardware, and roadmap work.
          </p>
        </div>
      </section>

      {/* Category legend */}
      <section className="max-w-5xl mx-auto px-6 pb-10">
        <div className="flex flex-wrap gap-4">
          {(Object.keys(categoryConfig) as Array<keyof typeof categoryConfig>).map((key) => {
            const cfg = categoryConfig[key];
            return (
              <div key={key} className="flex items-center gap-2 text-sm">
                <div
                  className="w-2.5 h-2.5 rounded-full"
                  style={{ background: cfg.color }}
                />
                <span style={{ color: cfg.color }}>{cfg.label}</span>
              </div>
            );
          })}
        </div>
      </section>

      {/* Timeline */}
      <section className="max-w-5xl mx-auto px-6 pb-24">
        <div className="relative">
          {/* Vertical line */}
          <div
            className="absolute left-[15px] top-0 bottom-0 w-px hidden sm:block"
            style={{
              background:
                "linear-gradient(to bottom, #22c55e, #8b5cf6, #a855f7, transparent)",
            }}
          />

          <div className="space-y-6">
            {releases.map((release, i) => {
              const cfg = categoryConfig[release.category];
              return (
                <div key={i} className="relative flex gap-6 group">
                  {/* Dot */}
                  <div className="hidden sm:flex items-start pt-7 shrink-0">
                    <div
                      className="w-[11px] h-[11px] rounded-full border-2 relative z-10"
                      style={{
                        borderColor: cfg.color,
                        background: cfg.bg,
                        marginLeft: "5px",
                      }}
                    />
                  </div>

                  {/* Card */}
                  <div
                    className="flex-1 rounded-xl p-6 border transition-all duration-300 hover:scale-[1.005]"
                    style={{
                      background: "#12121a",
                      borderColor: "#1e1e2e",
                    }}
                  >
                    {/* Card header */}
                    <div className="flex flex-wrap items-center gap-3 mb-4">
                      {/* Version badge */}
                      {release.version && (
                        <span
                          className="text-xs font-mono font-bold px-2.5 py-1 rounded-md"
                          style={{
                            background: cfg.bg,
                            color: cfg.color,
                            border: `1px solid ${cfg.border}`,
                          }}
                        >
                          {release.version}
                        </span>
                      )}
                      {/* Category badge */}
                      <span
                        className="text-xs font-mono px-2 py-0.5 rounded"
                        style={{
                          background: "#1e1e2e",
                          color: cfg.color,
                        }}
                      >
                        {cfg.label}
                      </span>
                      {/* Date */}
                      <span
                        className="text-xs font-mono"
                        style={{ color: "#555570" }}
                      >
                        {release.date}
                      </span>
                    </div>

                    {/* Title */}
                    <h3 className="text-lg font-bold text-white mb-4">
                      {release.version ? `${release.version} \u2014 ` : ""}
                      {release.title}
                    </h3>

                    {/* Items */}
                    <ul className="space-y-2">
                      {release.items.map((item, j) => (
                        <li
                          key={j}
                          className="flex items-start gap-3 text-sm leading-relaxed"
                          style={{ color: "#b0b0c0" }}
                        >
                          <span
                            className="mt-1.5 w-1.5 h-1.5 rounded-full shrink-0"
                            style={{ background: cfg.color, opacity: 0.6 }}
                          />
                          {item.startsWith("pip install") ? (
                            <code
                              className="text-xs font-mono px-2 py-0.5 rounded"
                              style={{ background: "#1e1e2e", color: "#22c55e" }}
                            >
                              {item}
                            </code>
                          ) : item.startsWith("python -m") ? (
                            <span>
                              Benchmark module:{" "}
                              <code
                                className="text-xs font-mono px-2 py-0.5 rounded"
                                style={{ background: "#1e1e2e", color: "#22c55e" }}
                              >
                                python -m atomik_core.benchmark
                              </code>
                            </span>
                          ) : (
                            item
                          )}
                        </li>
                      ))}
                    </ul>

                    {/* Evidence links */}
                    {release.links && (
                      <div className="mt-5 flex flex-wrap gap-2 pt-4" style={{ borderTop: "1px solid #1e1e2e" }}>
                        {release.links.map((link) => (
                          <Link
                            key={link.href}
                            href={link.href}
                            className="inline-flex items-center gap-2 rounded px-2.5 py-1 text-xs font-semibold no-underline transition-colors hover:opacity-80"
                            style={{ color: cfg.color, border: `1px solid ${cfg.border}`, background: cfg.bg }}
                          >
                            {link.label}
                            <span>&rarr;</span>
                          </Link>
                        ))}
                      </div>
                    )}
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </section>

      {/* Diligence CTA */}
      <section className="max-w-5xl mx-auto px-6 pb-24">
        <div
          className="rounded-2xl p-8 sm:p-12 border text-center"
          style={{
            background: "linear-gradient(135deg, rgba(139,92,246,0.08), rgba(79,143,255,0.08))",
            borderColor: "#8b5cf630",
          }}
        >
          <h2 className="text-2xl font-bold mb-3">Turn the release trail into a proof review.</h2>
          <p className="mx-auto mb-6 max-w-2xl" style={{ color: "#8888a0" }}>
            The useful next step is not a generic release list. Bring one workload or diligence question and map the relevant hardware, software, synthesis, and roadmap artifacts to an evidence boundary.
          </p>
          <div className="flex flex-col justify-center gap-3 sm:flex-row">
            <Link
              href="/contact?intent=investor"
              className="rounded-lg px-5 py-3 text-sm font-semibold text-white no-underline transition-opacity hover:opacity-90"
              style={{ background: "#2563eb" }}
            >
              Request Investor Diligence
            </Link>
            <Link
              href="/docs/hardware"
              className="rounded-lg px-5 py-3 text-sm font-semibold no-underline transition-colors hover:text-white"
              style={{ color: "#e0e0e8", border: "1px solid #1e1e2e" }}
            >
              Review Hardware Proof
            </Link>
          </div>
        </div>
      </section>

    </div>
  );
}
