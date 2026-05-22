import Image from "next/image";
import Link from "next/link";
import type { Metadata } from "next";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "ATOMiK - Cooler, Faster State-Aware Compute",
  description:
    "ATOMiK targets wasted state movement so devices and infrastructure can run cooler, move less data, and do more useful work per watt.",
  openGraph: {
    title: "ATOMiK - Cooler, Faster State-Aware Compute",
    description:
      "Less heat, less bandwidth, and more useful work per watt through state-aware compute.",
    url: "https://atomik.tech",
    images: [{ url: "https://atomik.tech/09-current-live-atomik-desk-v039k.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK - Cooler, Faster State-Aware Compute",
    description:
      "Less heat, less bandwidth, and more useful work per watt through state-aware compute.",
    images: ["https://atomik.tech/09-current-live-atomik-desk-v039k.png"],
  },
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
  blue: "#4f8fff",
  green: "#22c55e",
  violet: "#a78bfa",
  amber: "#f59e0b",
};

const outcomes = [
  {
    title: "Less heat",
    body: "Wasted memory movement and repeated state scans turn into heat. ATOMiK targets the upstream cause: moving and recomputing state that did not materially change.",
    accent: colors.green,
  },
  {
    title: "Longer battery life",
    body: "For edge and mobile-class devices, every avoided transfer and recompute cycle can preserve power budget for useful work instead of bookkeeping.",
    accent: colors.cyan,
  },
  {
    title: "Faster state paths",
    body: "ATOMiK treats change as the work unit, so systems can evaluate what changed instead of repeatedly reconstructing the same state through slow paths.",
    accent: colors.blue,
  },
  {
    title: "Smaller hardware profiles",
    body: "If a workload needs less bandwidth, less cooling headroom, and less redundant state machinery, the design space opens for smaller devices and denser deployments.",
    accent: colors.violet,
  },
];

const industries = [
  {
    name: "Data centers",
    value: "Reduce cooling pressure",
    body: "Data centers spend real money and water removing heat. ATOMiK is positioned for workloads where avoiding redundant state movement can reduce the heat created in the first place.",
  },
  {
    name: "Edge devices",
    value: "Do more inside tight power envelopes",
    body: "Edge systems are constrained by battery, thermal limits, radio bandwidth, and latency. ATOMiK evaluates whether state-aware execution can keep more work local.",
  },
  {
    name: "AI at the edge",
    value: "Keep context hot without moving everything",
    body: "Agent and AI systems constantly manage context. ATOMiK targets the state-management layer around those workloads, especially where moving full context is the bottleneck.",
  },
  {
    name: "Defense and remote operations",
    value: "Lower weight, lower power, higher reliability",
    body: "Remote systems care about every ounce, watt, and minute of uptime. ATOMiK is a fit to evaluate where deterministic state handling and reduced data movement matter operationally.",
  },
];

const mechanisms = [
  { title: "LOAD", body: "Start from a known reference state." },
  { title: "ACCUM", body: "Accumulate meaningful state changes as compact deltas." },
  { title: "READ", body: "Reconstruct current state from reference plus accumulated change." },
  { title: "SWAP", body: "Commit a state transition and reset the accumulator boundary." },
];

const proofItems = [
  ["HARDWARE_VALIDATED", "ATOMiK Desk v0.39-K is the current live hardware screenshot for the public site."],
  ["BUILD_ARTIFACT", "Minimal FSBL, generated fsbl.elf, SD-boot bitstream, and BOOT.bin exist in the Zynq boot path. Power-on validation is still pending."],
  ["HARDWARE_VALIDATED", "Linux userspace to FPGA validation remains a separate hardware proof path."],
  ["SYNTHESIS_VALIDATED", "Zynq scaling ceilings remain synthesis-characterized unless a matching board-run artifact is published."],
  ["SOFTWARE_VALIDATED", "Formal proof and SDK work are available for technical diligence in the repository."],
];

const tractionItems = [
  {
    label: "LIVE DESK PROOF",
    title: "Hardware-native UI at v0.39-K",
    body: "The current Desk surface runs as a single framebuffer process on Zynq hardware and presents the STATE, SYNC, and AGENT personalities visually.",
  },
  {
    label: "STANDALONE BOOT PATH",
    title: "BOOT.bin assembled",
    body: "The SD-boot bitstream, minimal FSBL build, and BOOT.bin artifact are present. Power-on standalone validation is the next gate.",
  },
  {
    label: "IP PATH",
    title: "Pre-seed goal: evaluate and de-risk silicon",
    body: "The near-term funding use is to convert proof into paid evaluations, stronger IP protection, and ASIC feasibility work before any tape-out commitment.",
  },
];

function Label({ children }: { children: React.ReactNode }) {
  return (
    <span
      className="inline-flex rounded px-2 py-1 text-[11px] font-semibold uppercase"
      style={{ color: colors.cyan, border: `1px solid ${colors.border}`, background: "#091522" }}
    >
      {children}
    </span>
  );
}

export default function Home() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Product" />

      <section className="px-6 pb-16 pt-16 md:pt-20">
        <div className="mx-auto grid max-w-6xl gap-10 md:grid-cols-[1.02fr_0.98fr] md:items-center">
          <div>
            <Label>Cooler, faster state-aware compute</Label>
            <h1 className="mt-5 max-w-3xl text-4xl font-bold leading-[1.06] md:text-6xl">
              Less heat. Less bandwidth. More useful work per watt.
            </h1>
            <p className="mt-6 max-w-2xl text-lg leading-8" style={{ color: colors.muted }}>
              ATOMiK attacks a simple systems problem: computers waste energy moving, scanning, and reconstructing state that barely changed. We make state change a first-class hardware/software primitive so devices and infrastructure can target cooler operation, faster state paths, longer battery life, and smaller hardware profiles.
            </p>
            <div className="mt-8 flex flex-wrap gap-3">
              <Link
                href="/contact?intent=evaluation"
                className="rounded-lg px-5 py-3 text-sm font-semibold no-underline transition-opacity hover:opacity-90"
                style={{ background: colors.blue, color: "#fff" }}
              >
                Request Technical Evaluation
              </Link>
              <Link
                href="/docs/hardware"
                className="rounded-lg px-5 py-3 text-sm font-semibold no-underline transition-colors hover:text-white"
                style={{ color: colors.text, border: `1px solid ${colors.border}` }}
              >
                View Evidence
              </Link>
            </div>
            <div className="mt-8 grid gap-3 text-sm sm:grid-cols-5" style={{ color: colors.muted }}>
              <span>Lower heat targets</span>
              <span>Less bandwidth</span>
              <span>Edge battery fit</span>
              <span>Live Zynq Desk</span>
              <span>Silicon IP path</span>
            </div>
          </div>

          <figure className="overflow-hidden rounded-lg" style={{ border: `1px solid ${colors.border}`, background: colors.panel }}>
            <Image
              src="/09-current-live-atomik-desk-v039k.png"
              width={1920}
              height={1080}
              priority
              sizes="(min-width: 768px) 48vw, 100vw"
              alt="ATOMiK Desk v0.39-K prototype UI running on live Zynq hardware"
              className="h-auto w-full"
            />
            <figcaption className="px-4 py-3 text-xs" style={{ color: colors.muted }}>
              <strong style={{ color: colors.text }}>HARDWARE_VALIDATED:</strong> ATOMiK Desk v0.39-K prototype UI running on live Zynq hardware. Not a commercial desktop product.
            </figcaption>
          </figure>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>Customer outcomes</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">The end game is not a new dashboard. It is cheaper, cooler compute.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              ATOMiK is valuable when the customer is constrained by heat, power, bandwidth, latency, or hardware footprint. The architecture is evaluated against those outcomes, not against abstract technical novelty.
            </p>
          </div>
          <div className="mt-8 grid gap-4 md:grid-cols-4">
            {outcomes.map((item) => (
              <div key={item.title} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <div className="mb-4 h-1 w-16 rounded" style={{ background: item.accent }} />
                <h3 className="text-lg font-bold">{item.title}</h3>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="grid gap-8 md:grid-cols-[0.9fr_1.1fr] md:items-start">
            <div>
              <Label>Industry use cases</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">Show customers their version of the benefit.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
                Different buyers feel the same state-waste problem through different budgets: cooling, battery, network, latency, weight, or compute density.
              </p>
            </div>
            <div className="grid gap-4 sm:grid-cols-2">
              {industries.map((item) => (
                <div key={item.name} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                  <div className="text-[11px] font-semibold uppercase" style={{ color: colors.green }}>{item.name}</div>
                  <h3 className="mt-2 text-lg font-bold">{item.value}</h3>
                  <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
                </div>
              ))}
            </div>
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="grid gap-8 md:grid-cols-[0.95fr_1.05fr] md:items-start">
            <div>
              <Label>How it works</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">Make change the unit of compute.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
                Traditional systems keep asking what changed. ATOMiK keeps a reference state, accumulates compact deltas, reconstructs on demand, and commits clean epoch boundaries.
              </p>
              <Link
                href="/docs/hardware"
                className="mt-6 inline-flex rounded-lg px-4 py-2 text-sm font-semibold no-underline"
                style={{ color: colors.cyan, border: `1px solid ${colors.border}` }}
              >
                View docs and examples
              </Link>
            </div>
            <div className="grid gap-4 sm:grid-cols-2">
              {mechanisms.map((item) => (
                <div key={item.title} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                  <h3 className="font-semibold" style={{ color: colors.text }}>{item.title}</h3>
                  <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
                </div>
              ))}
            </div>
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>Current traction</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">Built enough to evaluate, disciplined enough to trust.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              The current site separates what is live, what is a build artifact, and what is still roadmap. That keeps the investor story compelling without overstating the proof.
            </p>
          </div>
          <div className="mt-8 grid gap-4 md:grid-cols-3">
            {tractionItems.map((item) => (
              <div key={item.title} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <div className="text-[11px] font-semibold" style={{ color: colors.green }}>{item.label}</div>
                <h3 className="mt-2 text-lg font-bold">{item.title}</h3>
                <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="flex flex-col gap-4 md:flex-row md:items-end md:justify-between">
            <div className="max-w-3xl">
              <Label>Proof hierarchy</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">Live proof, build artifacts, and roadmap are separated.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
                ATOMiK is ambitious, but public claims stay bounded. The site distinguishes live hardware screenshots, boot-chain artifacts, software proof, synthesis output, and roadmap work.
              </p>
            </div>
            <Link
              href="/docs/hardware"
              className="rounded-lg px-4 py-2 text-sm font-semibold no-underline"
              style={{ color: colors.cyan, border: `1px solid ${colors.border}` }}
            >
              Open hardware proof
            </Link>
          </div>

          <div className="mt-8 grid gap-4 md:grid-cols-5">
            {proofItems.map(([label, text]) => (
              <div key={label} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <div className="text-[11px] font-semibold" style={{ color: colors.cyan }}>{label}</div>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{text}</p>
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>Evaluation and licensing</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">Bring one workload. We will map the wasted state.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              The right first customer is not buying a slogan. They are bringing a workload where heat, battery, latency, bandwidth, or hardware profile is already painful.
            </p>
          </div>

          <div className="mt-8 grid gap-5 md:grid-cols-2">
            <div className="rounded-lg p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
              <h3 className="text-xl font-bold">Design Partner Evaluation</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}><strong style={{ color: colors.text }}>For:</strong> Teams with edge, embedded, data-center, AI-context, defense, or distributed-state workloads.</p>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>Map a real workload to ATOMiK&apos;s evidence stack, define success criteria, and decide whether state-aware execution creates measurable value.</p>
              <Link href="/contact?intent=evaluation" className="mt-5 inline-flex rounded-lg px-4 py-2 text-sm font-semibold no-underline transition-opacity hover:opacity-90" style={{ background: colors.blue, color: "#fff" }}>
                Request Evaluation Access
              </Link>
            </div>
            <div className="rounded-lg p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
              <h3 className="text-xl font-bold">IP / ASIC Feasibility Path</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}><strong style={{ color: colors.text }}>For:</strong> Investors, chip partners, and teams evaluating ATOMiK as embeddable hardware IP.</p>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>Pre-seed funding de-risks IP protection, paid evaluations, ASIC mentorship, and feasibility work before any production tape-out decision.</p>
              <Link href="/contact?intent=licensing" className="mt-5 inline-flex rounded-lg px-4 py-2 text-sm font-semibold no-underline transition-opacity hover:opacity-90" style={{ background: colors.blue, color: "#fff" }}>
                Discuss Licensing
              </Link>
            </div>
          </div>

          <p className="mt-8 max-w-4xl text-xs leading-6" style={{ color: colors.faint }}>
            Live screenshots show current prototypes. Build artifacts show readiness for the next validation gate. Cooling, battery, speed, footprint, and bandwidth benefits are evaluation targets unless a linked measured artifact states a specific result. Concept visuals show product direction and are not represented as current commercial functionality.
          </p>
        </div>
      </section>
    </div>
  );
}
