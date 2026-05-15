"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

/* ── Release data ────────────────────────────────────────────────── */

type Release = {
  version: string;
  title: string;
  date: string;
  category: "software" | "hardware" | "tools";
  items: string[];
  blogLink?: string;
};

const releases: Release[] = [
  {
    version: "",
    title: "Phase 10: Investor Demo System",
    date: "2026-04-28",
    category: "hardware",
    items: [
      "15+ interactive demo screens with attract mode, State Storm, Break It challenge, Dollar-Per-Second Race",
      "90-second scripted investor demo with Claude auto-narration (demo_claude_drive.py)",
      "Freeze Frame closing with three proof cards: 456x benchmark, hardware validation, formal verification",
      "State Storm visualization: software drowns in O(n) scans while ATOMiK stays sparse at O(1)",
      "Dollar-Per-Second Race: real-time cost counters at 100K acceleration, ATOMiK vs traditional",
      "Two-tier Claude integration: laptop-side operator + board-side command executor (13/13 commands pass)",
      "Slide sync (slide_sync.py + slide_advance.sh) for PowerPoint presentation coordination",
      "Framebuffer screenshot capture (board_selfie.py) for proof artifacts",
      "Browser control plane with live metrics streaming",
    ],
  },
  {
    version: "",
    title: "Multi-Buffer Change Detection Benchmark",
    date: "2026-04-06",
    category: "hardware",
    items: [
      "End-to-end workload: track N memory regions, detect changes via ATOMiK vs memcmp",
      "ATOMiK detection: O(1) per region (~262 cycles), independent of buffer size",
      "Software memcmp: O(N \u00d7 size), degrades above 4 KB D-cache boundary",
      "8\u00d74 KB regions: 5,983\u00d7 speedup | 64\u00d71 KB regions: 251\u00d7 speedup",
      "ATOMiK monitoring rate: 1.2 million regions/sec at 64 contexts",
      "libatomik C runtime with /dev/mem backend and MMIO ordering fences",
    ],
  },
  {
    version: "",
    title: "Linux Userspace Validation on Zynq",
    date: "2026-04-06",
    category: "hardware",
    items: [
      "ATOMiK 16/16 PASS from Linux 6.9 userspace (S-mode, MMU enabled, /dev/mem mmap)",
      "Full stack validated: user process \u2192 kernel \u2192 Wishbone CSR \u2192 ATOMiK core",
      "Linux 6.9 + OpenSBI 1.3.1 booting on VexRiscv SMP (Zynq XC7Z020)",
      "MMIO ordering: fence iorw,iorw + STATUS readback required for Wishbone CSR correctness",
      "Frozen baseline with SHA-256 manifest (tag: zynq-linux-v1)",
    ],
  },
  {
    version: "v0.5.0",
    title: "Kernel Module — Enterprise Features",
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
      "License key expiry (YYMM encoding) for subscription enforcement",
    ],
  },
  {
    version: "v0.4.0",
    title: "Python SDK — Production Ready",
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
    blogLink: "/blog/announcing-atomik-kernel-module",
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
      "Sub-linear LUT scaling confirmed (~34 LUT per additional bank)",
    ],
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
    ],
  },
  {
    version: "v2.0.0",
    title: "PicoRV32 Prototype SoC",
    date: "2025-09",
    category: "hardware",
    items: [
      "Single-bank ATOMiK @ 81 MHz, 94.5 Mops/s",
      "PicoRV32 integration, SPI XIP boot",
      "11/11 hardware tests, +23% Fmax margin",
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
            Every release, every improvement.
          </p>
          <div className="mt-4">
            <a
              href="/feeds/changelog.xml"
              className="inline-flex items-center gap-2 text-sm font-mono px-4 py-2 rounded-lg border transition-colors hover:bg-white/5"
              style={{ borderColor: "#1e1e2e", color: "#d4a843" }}
            >
              <svg
                xmlns="http://www.w3.org/2000/svg"
                width="14"
                height="14"
                viewBox="0 0 24 24"
                fill="currentColor"
              >
                <circle cx="6.18" cy="17.82" r="2.18" />
                <path d="M4 4.44v2.83c7.03 0 12.73 5.7 12.73 12.73h2.83c0-8.59-6.97-15.56-15.56-15.56zm0 5.66v2.83c3.9 0 7.07 3.17 7.07 7.07h2.83c0-5.47-4.43-9.9-9.9-9.9z" />
              </svg>
              Subscribe via RSS
            </a>
          </div>
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

                    {/* Blog link */}
                    {release.blogLink && (
                      <div className="mt-5 pt-4" style={{ borderTop: "1px solid #1e1e2e" }}>
                        <Link
                          href={release.blogLink}
                          className="inline-flex items-center gap-2 text-sm font-medium transition-colors hover:opacity-80"
                          style={{ color: cfg.color }}
                        >
                          Read the announcement
                          <span>&rarr;</span>
                        </Link>
                      </div>
                    )}
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </section>

      {/* Subscribe for release updates */}
      <section className="max-w-5xl mx-auto px-6 pb-24">
        <div
          className="rounded-2xl p-8 sm:p-12 border text-center"
          style={{
            background: "linear-gradient(135deg, rgba(139,92,246,0.08), rgba(79,143,255,0.08))",
            borderColor: "#8b5cf630",
          }}
        >
          <h2 className="text-2xl font-bold mb-3">Subscribe for Release Updates</h2>
          <p className="mb-6" style={{ color: "#8888a0" }}>
            Get notified when we ship new versions. No spam, just releases.
          </p>
          <div className="max-w-md mx-auto">
            <EmailCapture />
          </div>
        </div>
      </section>

    </div>
  );
}
