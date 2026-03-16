import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Changelog — ATOMiK Delta-State Computing",
  description:
    "Every ATOMiK release, from Python SDK to FPGA silicon. Detailed changelog with version history, features, and improvements.",
};

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
    version: "v0.4.0",
    title: "Kernel Module",
    date: "March 15, 2026",
    category: "software",
    items: [
      "COW redundancy detection via kretprobe on wp_page_copy()",
      "TCP send deduplication with CRC32C fingerprinting (SSE4.2)",
      "Per-container/cgroup waste attribution for Kubernetes",
      "/proc/atomik machine-readable metrics interface",
      "27 sysfs attributes for real-time monitoring",
      "DKMS packaging, systemd service, udev rules",
      "90-day free trial, graceful degradation on expiry",
    ],
    blogLink: "/blog/announcing-atomik-kernel-module",
  },
  {
    version: "v0.2.0",
    title: "Python SDK",
    date: "March 15, 2026",
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
    date: "March 2026",
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
    date: "February 2026",
    category: "hardware",
    items: [
      "6 configs (N=1 to N=512), 4 synthesis strategies",
      "Peak: 69.7 Gops/s at N=512 on XC7Z020",
      "Sub-linear LUT scaling confirmed (~34 LUT per additional bank)",
    ],
  },
  {
    version: "v3.1.0",
    title: "HD HDMI SoC",
    date: "2025",
    category: "hardware",
    items: [
      "1280\u00d7720@60Hz on $13.50 Tang Nano 9K",
      "Custom RV64I CPU with native ATOMiK ISA extensions",
      "Delta-driven display pipeline",
      "6,287 LUT (73%), zero TNS",
    ],
  },
  {
    version: "v2",
    title: "PicoRV32 Production",
    date: "2025",
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
            <Link href="/docs" className="hover:text-white transition-colors">
              Docs
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
