import { pageMetadata } from "@/lib/siteMetadata";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata = pageMetadata({
  title: 'ASIC Roadmap | ATOMiK',
  description: "ATOMiK's evidence-labeled path from FPGA validation toward silicon IP feasibility and future Resource Fabric hardware.",
  path: '/about/roadmap',
  openGraphTitle: 'ATOMiK ASIC Roadmap',
});

/* ------------------------------------------------------------------ */
/*  Color tokens (Tailwind arbitrary values matching the dark theme)   */
/* ------------------------------------------------------------------ */
const bg = "#0a0a0f";
const surface = "#12121a";
const surface2 = "#181824";
const border = "#1e1e2e";
const accent = "#8b5cf6";
const accent2 = "#4f8fff";
const green = "#22c55e";
const gold = "#d4a843";

const evidenceLinks = {
  technicalProof: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/technical-proof.md",
  hardwareValidation: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/hardware-validation.md",
  hardwareSynthesis: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/HARDWARE_SYNTHESIS.md",
  linuxUserspace: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/LINUX_USERSPACE_PROOF.md",
} as const;

/* ------------------------------------------------------------------ */
/*  Tiny helpers                                                       */
/* ------------------------------------------------------------------ */
type Status = "completed" | "in-progress" | "future";

function statusColor(s: Status) {
  if (s === "completed") return green;
  if (s === "in-progress") return gold;
  return "#555568";
}

function statusLabel(s: Status) {
  if (s === "completed") return "Validated";
  if (s === "in-progress") return "In Progress";
  return "Planned";
}

function StatusBadge({ status }: { status: Status }) {
  const color = statusColor(status);
  return (
    <span
      className="inline-flex items-center gap-1.5 rounded-full px-3 py-1 text-xs font-semibold uppercase tracking-wider"
      style={{ color, border: `1px solid ${color}33`, background: `${color}10` }}
    >
      <span
        className="inline-block h-2 w-2 rounded-full"
        style={{ background: color, boxShadow: `0 0 6px ${color}88` }}
      />
      {statusLabel(status)}
    </span>
  );
}

/* ------------------------------------------------------------------ */
/*  Data                                                               */
/* ------------------------------------------------------------------ */
interface EvidenceLink {
  label: string;
  href: string;
}

interface Milestone {
  phase: string;
  title: string;
  status: Status;
  bullets: string[];
  highlight?: string;
  proofLinks?: EvidenceLink[];
}

const milestones: Milestone[] = [
  {
    phase: "Phase 0",
    title: "Mathematical Foundation",
    status: "completed",
    bullets: [
      "Formal proof work covering delta-state algebra properties; public counts require current proof-packet audit",
      "Abelian group: commutative, associative, self-inverse, identity",
      "Reduced cache/speculation exposure is a design target to validate",
    ],
    highlight: "Proof-labeled algebra foundation",
    proofLinks: [{ label: "SOFTWARE_VALIDATED proof map", href: evidenceLinks.technicalProof }],
  },
  {
    phase: "Phase 1",
    title: "Gowin FPGA \u2014 Tang Nano 9K",
    status: "completed",
    bullets: [
      "Custom RV64I CPU + ATOMiK ISA extensions on GW1NR-9K",
      "1280\u00d7720 @60 Hz HDMI output from a $13.50 FPGA",
      "Multi-node delta streaming: dual-SoC convergence demonstrated in historical hardware artifacts",
      "Historical validation matrix: 53/54 compliance, 9/9 ATOMiK, 10/10 integration, 6/6 display tests",
    ],
    highlight: "Prototype firmware running today",
    proofLinks: [{ label: "HARDWARE_VALIDATED proof boundary", href: evidenceLinks.hardwareValidation }],
  },
  {
    phase: "Phase 2",
    title: "Xilinx Zynq XC7Z020 \u2014 Parallel Scaling + Linux",
    status: "completed",
    bullets: [
      "SYNTHESIS_VALIDATED N=512 config: 23,542 LUT at 136 MHz",
      "SYNTHESIS_VALIDATED scaling: 3.7x LUT growth for 16x throughput",
      "Linux 6.9 userspace validation: 16/16 PASS via /dev/mem mmap (S-mode, MMU)",
      "HARDWARE_VALIDATED Linux userspace path: user process -> kernel -> Wishbone CSR -> ATOMiK core",
    ],
    highlight: "Synthesis scaling + Linux userspace validated",
    proofLinks: [
      { label: "SYNTHESIS_VALIDATED artifact", href: evidenceLinks.hardwareSynthesis },
      { label: "HARDWARE_VALIDATED Linux proof", href: evidenceLinks.linuxUserspace },
    ],
  },
  {
    phase: "Phase 3",
    title: "ASIC Feasibility Review",
    status: "in-progress",
    bullets: [
      "Open-source PDK review path before any tape-out commitment",
      "Gate-count, timing, and power estimates require external review",
      "Review whether delta-state algebra maps cleanly into silicon IP",
      "Toolchain plan: OpenLane 2, Magic, KLayout, and mentor review",
    ],
    highlight: "First silicon target",
  },
  {
    phase: "Phase 4",
    title: "Strategic Silicon Partner Path",
    status: "future",
    bullets: [
      "Strategic foundry or chip-company diligence when proof gates justify it",
      "SRAM compiler integration for on-die state tables",
      "Multi-bank ASIC with dedicated on-die interconnect",
      "High-throughput custom-silicon direction; numbers remain roadmap until measured",
    ],
  },
  {
    phase: "Phase 5",
    title: "Volume ASIC \u2014 Edge + Data-Center SKUs",
    status: "future",
    bullets: [
      "Edge SKU: ultra-low-power, sub-1 mm\u00b2 die for IoT / embedded",
      "Data-center SKU: thousands of parallel banks, PCIe / CXL attach",
      "Hardware root-of-trust direction with reduced data-dependent timing surfaces",
      "Power and throughput targets require post-review projections and measured silicon before public quoting",
    ],
  },
];

/* ------------------------------------------------------------------ */
/*  Metrics bar                                                        */
/* ------------------------------------------------------------------ */
interface MetricCardProps {
  label: string;
  value: string;
  sub: string;
  color: string;
}

function MetricCard({ label, value, sub, color }: MetricCardProps) {
  return (
    <div
      className="rounded-xl p-5 text-center"
      style={{ background: surface2, border: `1px solid ${border}` }}
    >
      <p className="mb-1 text-xs font-medium uppercase tracking-wider text-gray-500">
        {label}
      </p>
      <p className="text-2xl font-bold" style={{ color }}>
        {value}
      </p>
      <p className="mt-1 text-xs text-gray-500">{sub}</p>
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  Milestone card                                                     */
/* ------------------------------------------------------------------ */
function MilestoneCard({ m, index }: { m: Milestone; index: number }) {
  const color = statusColor(m.status);
  const isCompleted = m.status === "completed";
  const isCurrent = m.status === "in-progress";

  return (
    <div className="relative flex gap-6 pb-12 last:pb-0">
      {/* Timeline spine */}
      <div className="flex flex-col items-center">
        {/* Node */}
        <div
          className="relative z-10 flex h-10 w-10 shrink-0 items-center justify-center rounded-full text-sm font-bold"
          style={{
            background: isCompleted
              ? `${green}20`
              : isCurrent
                ? `${gold}20`
                : `${surface2}`,
            border: `2px solid ${color}`,
            color,
            boxShadow: isCurrent ? `0 0 16px ${gold}44` : undefined,
          }}
        >
          {isCompleted ? "\u2713" : index + 1}
        </div>
        {/* Connector line */}
        {index < milestones.length - 1 && (
          <div
            className="w-px grow"
            style={{
              background: `linear-gradient(to bottom, ${color}66, ${border})`,
            }}
          />
        )}
      </div>

      {/* Card */}
      <div
        className="grow rounded-xl p-6"
        style={{
          background: isCurrent
            ? `linear-gradient(135deg, ${surface}, ${surface2})`
            : surface,
          border: `1px solid ${isCurrent ? `${gold}44` : border}`,
          boxShadow: isCurrent ? `0 0 32px ${gold}11` : undefined,
        }}
      >
        <div className="mb-3 flex flex-wrap items-center gap-3">
          <span
            className="text-xs font-bold uppercase tracking-widest"
            style={{ color: accent2 }}
          >
            {m.phase}
          </span>
          <StatusBadge status={m.status} />
        </div>

        <h3 className="mb-3 text-xl font-bold text-white">{m.title}</h3>

        <ul className="mb-4 space-y-2">
          {m.bullets.map((b, i) => (
            <li key={i} className="flex items-start gap-2 text-sm text-gray-400">
              <span style={{ color: accent }} className="mt-1 shrink-0">
                &#x25B8;
              </span>
              {b}
            </li>
          ))}
        </ul>

        {m.highlight && (
          <div
            className="inline-block rounded-lg px-3 py-1.5 text-xs font-semibold"
            style={{ background: `${color}15`, color }}
          >
            {m.highlight}
          </div>
        )}

        {m.proofLinks && (
          <div className="mt-4 flex flex-wrap gap-2">
            {m.proofLinks.map((link) => (
              <Link
                key={link.href}
                href={link.href}
                className="rounded px-2.5 py-1 text-xs font-semibold no-underline"
                style={{ color: accent2, border: `1px solid ${accent2}33`, background: `${accent2}10` }}
              >
                {link.label}
              </Link>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  Scaling chart (simple CSS bars)                                    */
/* ------------------------------------------------------------------ */
interface ScalingRow {
  label: string;
  lut: string;
  freq: string;
  throughput: string;
  barPct: number;
  color: string;
  evidenceHref?: string;
}

const scalingData: ScalingRow[] = [
  { label: "N=1", lut: "302", freq: "444 MHz", throughput: "Synthesis row", barPct: 0.6, color: accent2, evidenceHref: evidenceLinks.hardwareSynthesis },
  { label: "N=16", lut: "941", freq: "267 MHz", throughput: "Synthesis row", barPct: 6.3, color: accent, evidenceHref: evidenceLinks.hardwareSynthesis },
  { label: "N=512", lut: "23,542", freq: "136 MHz", throughput: "Synthesis ceiling", barPct: 100, color: green, evidenceHref: evidenceLinks.hardwareSynthesis },
  { label: "ASIC path", lut: "Not quoted", freq: "Not quoted", throughput: "Feasibility review", barPct: 100, color: gold },
];

function ScalingChart() {
  return (
    <div className="space-y-4">
      {scalingData.map((row) => (
        <div key={row.label} className="grid grid-cols-12 items-center gap-4">
          <div className="col-span-2 text-right">
            <span className="text-sm font-bold text-white">{row.label}</span>
          </div>
          <div className="col-span-6">
            <div
              className="h-6 rounded"
              style={{
                width: `${Math.max(row.barPct, 2)}%`,
                background: `linear-gradient(90deg, ${row.color}cc, ${row.color}44)`,
                boxShadow: `0 0 12px ${row.color}22`,
              }}
            />
          </div>
          <div className="col-span-4 text-xs text-gray-400">
            <span className="font-semibold text-white">{row.throughput}</span>{" "}
            &middot; {row.freq} &middot; {row.lut} LUT
            {row.evidenceHref && (
              <Link
                href={row.evidenceHref}
                className="ml-2 text-xs font-semibold no-underline"
                style={{ color: accent2 }}
              >
                Evidence
              </Link>
            )}
          </div>
        </div>
      ))}
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  Page                                                               */
/* ------------------------------------------------------------------ */
export default function ASICRoadmapPage() {
  return (
    <div className="min-h-screen" style={{ background: bg, color: "#e4e4eb" }}>
      <Nav active="ASIC Roadmap" />
      {/* ---- Hero ---- */}
      <section className="relative overflow-hidden px-6 pb-16 pt-24 text-center">
        {/* Gradient glow */}
        <div
          className="pointer-events-none absolute inset-0"
          style={{
            background: `radial-gradient(ellipse 60% 40% at 50% 0%, ${accent}18, transparent)`,
          }}
        />

        <p
          className="relative mb-3 text-sm font-semibold uppercase tracking-widest"
          style={{ color: accent }}
        >
          ASIC Roadmap
        </p>
        <h1 className="relative mx-auto mb-4 max-w-3xl text-4xl font-extrabold leading-tight text-white sm:text-5xl">
          From FPGA to{" "}
          <span
            style={{
              background: `linear-gradient(90deg, ${accent}, ${accent2})`,
              WebkitBackgroundClip: "text",
              WebkitTextFillColor: "transparent",
            }}
          >
            Silicon IP
          </span>
        </h1>
        <p className="relative mx-auto max-w-2xl text-lg text-gray-400">
          ATOMiK&apos;s delta-state algebra is formally verified, FPGA-backed, and
          mapped to a synthesis-characterized path toward silicon IP evaluation.
        </p>
      </section>

      {/* ---- Key metrics ---- */}
      <section className="mx-auto max-w-5xl px-6 pb-20">
        <div className="grid grid-cols-2 gap-4 sm:grid-cols-4">
          <MetricCard label="Lean 4 Proofs" value="108" sub="Theorems verified" color={green} />
          <MetricCard label="Peak FPGA" value="Synthesis" sub="Zynq XC7Z020, N=512" color={accent} />
          <MetricCard label="Single Core" value="Synthesis" sub="See evidence labels" color={accent2} />
          <MetricCard label="ASIC Path" value="Feasibility" sub="Roadmap, not measured" color={gold} />
        </div>
      </section>

      {/* ---- Scaling chart ---- */}
      <section className="mx-auto max-w-5xl px-6 pb-20">
        <h2 className="mb-2 text-2xl font-bold text-white">
          FPGA Scaling Evidence
        </h2>
        <p className="mb-8 text-sm text-gray-500">
          Zynq rows summarize synthesis-characterized FPGA scaling. The ASIC row is a feasibility review target, not a measured or promised result.
        </p>
        <div
          className="rounded-xl p-6"
          style={{ background: surface, border: `1px solid ${border}` }}
        >
          <ScalingChart />
        </div>
      </section>

      {/* ---- Timeline ---- */}
      <section className="mx-auto max-w-3xl px-6 pb-24">
        <h2 className="mb-2 text-2xl font-bold text-white">
          Development Timeline
        </h2>
        <p className="mb-10 text-sm text-gray-500">
          Each phase keeps measured hardware, synthesis output, and roadmap work separated.
        </p>

        {milestones.map((m, i) => (
          <MilestoneCard key={m.phase} m={m} index={i} />
        ))}
      </section>

      {/* ---- CTA ---- */}
      <section className="px-6 pb-24 text-center">
        <div
          className="mx-auto max-w-2xl rounded-2xl p-10"
          style={{
            background: `linear-gradient(135deg, ${surface}, ${surface2})`,
            border: `1px solid ${border}`,
          }}
        >
          <h2 className="mb-3 text-2xl font-bold text-white">
            Interested in ATOMiK IP?
          </h2>
          <p className="mb-6 text-gray-400">
            ATOMiK is preparing investor, chip-partner, and design-partner diligence around state-aware compute. Get in
            touch to discuss proof review, licensing, or integration.
          </p>
          <a
            href="/contact?intent=licensing"
            className="inline-flex items-center gap-2 rounded-lg px-8 py-3 text-sm font-semibold text-white transition-shadow hover:shadow-lg"
            style={{
              background: `linear-gradient(135deg, ${accent}, ${accent2})`,
              boxShadow: `0 4px 24px ${accent}33`,
            }}
          >
            Discuss Licensing / Evaluation
            <span aria-hidden="true">&rarr;</span>
          </a>
          <p className="mt-4 text-xs text-gray-600">
            Use the evaluation form to anchor on workload, proof boundary, and timeline.
          </p>
          <p className="mt-6 text-sm text-gray-400">
            Questions about our timeline?{" "}
            <a
              href="/contact"
              className="font-medium underline transition-colors hover:text-white"
              style={{ color: accent2 }}
            >
              Contact us
            </a>
          </p>
        </div>
      </section>
    </div>
  );
}
