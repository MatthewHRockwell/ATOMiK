import Image from "next/image";
import Link from "next/link";
import type { Metadata } from "next";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "ATOMiK - State-Aware Compute",
  description:
    "ATOMiK makes state change, delta application, and adaptive execution first-class compute primitives, starting with live hardware prototypes.",
  openGraph: {
    title: "ATOMiK - State-Aware Compute",
    description:
      "State-aware compute architecture for systems that waste work rediscovering change.",
    url: "https://atomik.tech",
    images: [{ url: "https://atomik.tech/08-current-live-atomik-desk-v038i-og.jpg", width: 1200, height: 630 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK - State-Aware Compute",
    description:
      "State-aware compute architecture for systems that waste work rediscovering change.",
    images: ["https://atomik.tech/08-current-live-atomik-desk-v038i-og.jpg"],
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
};

const operations = [
  {
    title: "LOAD",
    body: "Start from a known reference state.",
  },
  {
    title: "ACCUM",
    body: "Accumulate meaningful state changes as compact deltas.",
  },
  {
    title: "READ",
    body: "Reconstruct the current state from reference plus accumulated change.",
  },
  {
    title: "SWAP",
    body: "Commit a state transition and reset the accumulator boundary.",
  },
];

const valuePoints = [
  "Avoid repeated full-state rescans.",
  "Coalesce redundant updates before they become wasted work.",
  "Propagate meaningful deltas instead of whole context.",
  "Make state transitions observable and evidence-labeled.",
  "Map from software SDK exploration toward hardware acceleration when justified.",
];

const proofItems = [
  ["SOFTWARE_VALIDATED", "Formal proof work and public SDK paths are present in the repository."],
  ["SOFTWARE_VALIDATED", "Software SDK and evaluation tooling support developer inspection."],
  ["SYNTHESIS_VALIDATED", "Zynq ceiling results are synthesis-characterized and kept separate from live-board claims."],
  ["HARDWARE_VALIDATED", "ATOMiK Desk v0.38-I is a live prototype screenshot from hardware."],
  ["ROADMAP", "First-silicon evaluation chip work is a preparation path, not a completed silicon artifact."],
];

const offers = [
  {
    name: "Design Partner Evaluation",
    cta: "Request Evaluation Access",
    href: "/contact?intent=evaluation",
    for: "Edge, embedded, distributed, AI/agent infrastructure, or state-heavy system teams.",
    body: "Map one real workload to ATOMiK's evidence stack, success criteria, and prototype/evaluation path. Limited evaluation access may be available for qualified teams.",
  },
  {
    name: "Enterprise / IP / SDK Licensing",
    cta: "Discuss Licensing",
    href: "/contact?intent=licensing",
    for: "Teams evaluating ATOMiK IP, SDK integration, hardware acceleration, or first-silicon partnerships.",
    body: "Discuss SDK, hardware integration, IP licensing, or first-silicon evaluation requirements with evidence boundaries defined up front.",
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
      <Nav />

      <section className="px-6 pb-16 pt-16 md:pt-20">
        <div className="mx-auto grid max-w-6xl gap-10 md:grid-cols-[1.02fr_0.98fr] md:items-center">
          <div>
            <Label>State-aware compute architecture</Label>
            <h1 className="mt-5 max-w-3xl text-4xl font-bold leading-[1.06] md:text-6xl">
              Stop rediscovering state. Compute what changed.
            </h1>
            <p className="mt-6 max-w-2xl text-lg leading-8" style={{ color: colors.muted }}>
              ATOMiK is a state-aware compute architecture that turns change tracking and delta application into a hardware/software primitive - reducing wasted scans, sync, and state reconstruction in edge, embedded, and distributed systems.
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
                href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/technical-proof.md"
                className="rounded-lg px-5 py-3 text-sm font-semibold no-underline transition-colors hover:text-white"
                style={{ color: colors.text, border: `1px solid ${colors.border}` }}
              >
                View Hardware Proof
              </Link>
            </div>
            <div className="mt-8 grid gap-3 text-sm sm:grid-cols-5" style={{ color: colors.muted }}>
              <span>Formal proofs</span>
              <span>Software SDK</span>
              <span>FPGA validation</span>
              <span>Live Zynq prototype</span>
              <span>First-silicon path</span>
            </div>
          </div>

          <figure
            className="overflow-hidden rounded-lg"
            style={{ border: `1px solid ${colors.border}`, background: colors.panel }}
          >
            <Image
              src="/08-current-live-atomik-desk-v038i.png"
              width={1920}
              height={1080}
              priority
              sizes="(min-width: 768px) 48vw, 100vw"
              alt="ATOMiK Desk v0.38-I prototype UI running on live hardware"
              className="h-auto w-full"
            />
            <figcaption className="px-4 py-3 text-xs" style={{ color: colors.muted }}>
              <strong style={{ color: colors.text }}>HARDWARE_VALIDATED:</strong> ATOMiK Desk v0.38-I prototype UI running on live hardware. Not a commercial desktop product.
            </figcaption>
          </figure>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="grid gap-8 md:grid-cols-[0.95fr_1.05fr] md:items-start">
            <div>
              <Label>How ATOMiK works</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">Four operations. A new state model.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
                ATOMiK keeps a reference state, accumulates compact deltas, reconstructs state on demand, and commits clean transition boundaries. The deeper architecture lives in the technical docs.
              </p>
              <Link
                href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/technical-proof.md"
                className="mt-6 inline-flex rounded-lg px-4 py-2 text-sm font-semibold no-underline"
                style={{ color: colors.cyan, border: `1px solid ${colors.border}` }}
              >
                View docs and examples
              </Link>
            </div>
            <div className="grid gap-4 sm:grid-cols-2">
              {operations.map((item) => (
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
            <Label>Why it matters</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">State-heavy systems keep asking what changed.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              At the edge, inside embedded systems, and across distributed infrastructure, repeated recomputation creates latency, power draw, synchronization overhead, and operational complexity.
            </p>
          </div>
          <div className="mt-8 grid gap-4 md:grid-cols-5">
            {valuePoints.map((point) => (
              <div key={point} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <p className="text-sm leading-6" style={{ color: colors.muted }}>{point}</p>
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
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">From formal proof and FPGA validation to first silicon.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
                Current proof, measured artifacts, synthesis output, and roadmap items are labeled separately. The public story is live prototype progress and a first-silicon path, not a deployment claim.
              </p>
            </div>
            <Link
              href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml"
              className="rounded-lg px-4 py-2 text-sm font-semibold no-underline"
              style={{ color: colors.cyan, border: `1px solid ${colors.border}` }}
            >
              Open proof registry
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

          <p className="mt-6 text-sm leading-6" style={{ color: colors.faint }}>
            Live AX7020/Zynq hardware demo results are being consolidated into reproducible artifacts. Until then, quote the linked proof registry and artifact notes rather than screenshot-visible numbers.
          </p>
        </div>
      </section>

      <section className="px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>Evaluation and licensing</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">Start with a scoped evaluation, not a leap of faith.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Bring one workload, one sync path, or one state-heavy bottleneck. ATOMiK will map it to the evidence stack, define success criteria, and determine whether there is real fit. Limited evaluation access may be available for qualified teams.
            </p>
          </div>

          <div className="mt-8 grid gap-5 md:grid-cols-2">
            {offers.map((offer) => (
              <div key={offer.name} className="rounded-lg p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
                <h3 className="text-xl font-bold">{offer.name}</h3>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}><strong style={{ color: colors.text }}>For:</strong> {offer.for}</p>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{offer.body}</p>
                <Link
                  href={offer.href}
                  className="mt-5 inline-flex rounded-lg px-4 py-2 text-sm font-semibold no-underline transition-opacity hover:opacity-90"
                  style={{ background: colors.blue, color: "#fff" }}
                >
                  {offer.cta}
                </Link>
              </div>
            ))}
          </div>

          <p className="mt-8 max-w-4xl text-xs leading-6" style={{ color: colors.faint }}>
            Live screenshots show current prototypes. Concept visuals show product direction and are not represented as current commercial functionality. Performance claims are only stated when backed by measured artifacts.
          </p>
        </div>
      </section>
    </div>
  );
}
