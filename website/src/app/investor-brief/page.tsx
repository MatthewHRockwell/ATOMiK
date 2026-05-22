import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Investor Brief - ATOMiK",
  description:
    "Investor-oriented brief for ATOMiK: customer value, current proof, funding use, and diligence path.",
};

const colors = {
  bg: "#070b12",
  panel: "#0d1420",
  panel2: "#101a29",
  border: "#1d324a",
  text: "#f4f8ff",
  muted: "#9fb1c7",
  faint: "#6f8097",
  cyan: "#22d3ee",
  green: "#22c55e",
  violet: "#a78bfa",
  amber: "#f59e0b",
  blue: "#4f8fff",
};

const outcomes = [
  {
    title: "Less heat",
    body: "ATOMiK targets wasted state movement and repeated reconstruction before that work becomes heat, cooling cost, and density pressure.",
    color: colors.green,
  },
  {
    title: "Longer battery life",
    body: "For edge and mobile-class devices, avoided transfers and state scans can preserve limited power budget for useful work.",
    color: colors.cyan,
  },
  {
    title: "Faster state paths",
    body: "Change becomes the work unit, so eligible systems can evaluate deltas instead of repeatedly rebuilding the same state.",
    color: colors.blue,
  },
  {
    title: "Smaller hardware profiles",
    body: "Less bandwidth pressure and less cooling headroom can open room for smaller devices, denser racks, and simpler deployments.",
    color: colors.violet,
  },
];

const customerSegments = [
  ["Data centers", "Cooling energy, water pressure, rack density, and utilization."],
  ["Edge devices", "Battery life, thermal envelopes, intermittent links, and local latency."],
  ["AI at the edge", "Context movement, state retention, memory pressure, and response time."],
  ["Defense / remote", "Weight, wattage, packet budget, reliability, and field service constraints."],
];

const proofRows = [
  ["LIVE UI PROOF", "ATOMiK Desk v0.39-K runs as a framebuffer-native shell on Zynq hardware and demonstrates the STATE, SYNC, and AGENT personalities."],
  ["HARDWARE VALIDATION", "Linux userspace validation and AX7020 board-run artifacts are separated from interpretation notes and synthesis-only claims."],
  ["FORMAL FOUNDATION", "The core algebra has 108 Lean4 theorems proving the properties the architecture relies on."],
  ["NEXT GATE", "Standalone SD boot artifacts are assembled locally; power-on validation is the next hardware milestone before promoting that link as public proof."],
];

const fundingUses = [
  "Convert provisional IP protection into stronger patent coverage and diligence materials.",
  "Run paid design-partner evaluations against real heat, power, bandwidth, latency, or footprint constraints.",
  "Bring in fractional CFO support for valuation and financing structure.",
  "Add ASIC mentorship to review the feasibility path before any tape-out commitment.",
  "Package the Zynq demo into a lower-friction investor and partner proof system.",
];

const diligenceLinks = [
  ["Hardware proof map", "/docs/hardware"],
  ["Evidence and benchmarks", "/benchmarks"],
  ["Use cases", "/solutions"],
  ["Evaluation structure", "/pricing"],
];

function Label({ children }: { children: React.ReactNode }) {
  return <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>{children}</p>;
}

export default function InvestorBriefPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Investors" />

      <section className="mx-auto max-w-6xl px-6 pb-14 pt-16">
        <div className="grid gap-10 md:grid-cols-[1.05fr_0.95fr] md:items-center">
          <div>
            <Label>Investor brief</Label>
            <h1 className="mt-4 max-w-4xl text-4xl font-bold leading-tight md:text-6xl">
              ATOMiK turns wasted state movement into a customer-value story.
            </h1>
            <p className="mt-6 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
              The pitch is simple: less redundant state work can mean less heat, lower cooling burden, longer battery life, faster local execution, lower bandwidth pressure, and smaller hardware profiles. The current opportunity is to convert working proof into focused customer evaluations and de-risked silicon IP.
            </p>
            <div className="mt-8 flex flex-wrap gap-3">
              <Link href="/contact?intent=licensing" className="rounded-lg px-5 py-3 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
                Request Investor Diligence
              </Link>
              <Link href="/docs/hardware" className="rounded-lg px-5 py-3 text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
                Review Proof Map
              </Link>
            </div>
          </div>

          <div className="rounded-lg p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
            <p className="text-[11px] font-semibold uppercase" style={{ color: colors.green }}>Pre-seed focus</p>
            <h2 className="mt-2 text-2xl font-bold">Fund the transition from proof to evaluated IP.</h2>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              ATOMiK is not trying to outspend chip incumbents. The near-term path is to build credible evidence, protect the IP, run sharp customer evaluations, and position the architecture for strategic licensing or acquisition conversations.
            </p>
            <div className="mt-5 grid gap-3 text-sm" style={{ color: colors.muted }}>
              <div><strong style={{ color: colors.text }}>Today:</strong> formal proof, software, FPGA paths, Zynq Desk demo, evidence-labeled artifacts.</div>
              <div><strong style={{ color: colors.text }}>Next:</strong> standalone boot validation, paid workload evaluations, IP conversion, ASIC feasibility review.</div>
              <div><strong style={{ color: colors.text }}>Commercial wedge:</strong> state-heavy workloads where heat, power, bandwidth, latency, or footprint is already painful.</div>
            </div>
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <Label>Customer value</Label>
          <h2 className="mt-4 max-w-3xl text-3xl font-bold md:text-4xl">What the customer gets out of ATOMiK.</h2>
          <div className="mt-8 grid gap-4 md:grid-cols-4">
            {outcomes.map((item) => (
              <article key={item.title} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <div className="mb-4 h-1 w-16 rounded" style={{ background: item.color }} />
                <h3 className="text-lg font-bold">{item.title}</h3>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
              </article>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto grid max-w-6xl gap-8 md:grid-cols-[0.9fr_1.1fr]">
          <div>
            <Label>Who feels it</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">The same technical primitive maps to different buyer pain.</h2>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              The website and pitch should keep the architecture in the background until the buyer understands their benefit.
            </p>
          </div>
          <div className="grid gap-4 sm:grid-cols-2">
            {customerSegments.map(([name, body]) => (
              <article key={name} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <h3 className="font-bold">{name}</h3>
                <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{body}</p>
              </article>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>Proof boundary</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">Compelling, but evidence-bounded.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Public claims stay separated by proof level. Benefits such as reduced heat, lower water burden, or longer battery life are evaluation targets unless a linked artifact measures that exact outcome.
            </p>
          </div>
          <div className="mt-8 grid gap-4 md:grid-cols-2">
            {proofRows.map(([label, body]) => (
              <article key={label} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{label}</p>
                <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{body}</p>
              </article>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto grid max-w-6xl gap-8 md:grid-cols-[1fr_1fr]">
          <div className="rounded-lg p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
            <Label>Use of funds</Label>
            <h2 className="mt-4 text-3xl font-bold">What pre-seed capital unlocks.</h2>
            <ul className="mt-6 space-y-3">
              {fundingUses.map((item) => (
                <li key={item} className="text-sm leading-6" style={{ color: colors.muted }}>
                  <span style={{ color: colors.green }}>-</span> {item}
                </li>
              ))}
            </ul>
          </div>
          <div className="rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <Label>Diligence links</Label>
            <h2 className="mt-4 text-3xl font-bold">Start with the public proof packet.</h2>
            <div className="mt-6 grid gap-3">
              {diligenceLinks.map(([label, href]) => (
                <Link key={label} href={href} className="rounded-lg px-4 py-3 text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
                  {label}
                </Link>
              ))}
            </div>
          </div>
        </div>
      </section>

      <section className="px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-4xl rounded-lg p-8 text-center" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
          <h2 className="text-3xl font-bold">Ready for diligence or a strategic IP conversation?</h2>
          <p className="mx-auto mt-4 max-w-2xl text-sm leading-6" style={{ color: colors.muted }}>
            The right next conversation anchors on one workload, one constraint, and one evidence path: customer evaluation, investor diligence, or licensing review.
          </p>
          <Link href="/contact?intent=licensing" className="mt-6 inline-flex rounded-lg px-5 py-3 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Open Diligence Request
          </Link>
        </div>
      </section>
    </div>
  );
}
