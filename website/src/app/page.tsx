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
      "State-aware compute for systems that cannot afford to recompute everything.",
    url: "https://atomik.tech",
    images: [{ url: "https://atomik.tech/07-current-live-atomik-desk-v038g.png", width: 1200, height: 630 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK - State-Aware Compute",
    description:
      "State-aware compute for systems that cannot afford to recompute everything.",
    images: ["https://atomik.tech/07-current-live-atomik-desk-v038g.png"],
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

const problemPoints = [
  "Full-state copies move bytes that did not change.",
  "Replay paths reconstruct current state by walking old history.",
  "Change detection often rescans memory instead of consuming deltas.",
  "Rollback and synchronization logic leak into every layer of the system.",
];

const mechanisms = [
  {
    title: "State-aware execution",
    body: "Start from a reference state, apply compact deltas, and reconstruct current state on demand.",
  },
  {
    title: "Delta propagation",
    body: "Treat meaningful state change as the unit of movement instead of repeatedly shipping full context.",
  },
  {
    title: "Resource Fabric direction",
    body: "Model workload personalities such as STATE, SYNC, AGENT, EVENT, and VISUAL without making the homepage a systems manual.",
  },
  {
    title: "Adoption path",
    body: "Explore in software, evaluate against real workloads, then move toward standard-C, compiler, and hardware-backed paths where justified.",
  },
];

const proofItems = [
  ["HARDWARE_VALIDATED", "Current ATOMiK Desk prototype and v0.38-G UI proof screenshots are live-hardware artifacts."],
  ["LIVE_MEASURED", "AX7020 board-run artifacts are tracked separately from interpretation notes."],
  ["SOFTWARE_VALIDATED", "Public software and formal proof work are linked from the repository."],
  ["SYNTHESIS_VALIDATED", "Toolchain ceilings are labeled separately from live-board measurements."],
];

const conceptCards = [
  {
    src: "/05-replica-flow-sync-concept.png",
    label: "Roadmap visual",
    title: "Replica Flow",
    caption: "Concept visualization of Replica Flow and delta propagation.",
  },
  {
    src: "/03-adaptive-mode-mixed-workload-concept.png",
    label: "Roadmap visual",
    title: "Resource Fabric",
    caption: "Concept visualization of Resource Fabric adaptive workload reallocation.",
  },
  {
    src: "/02-atomik-desk-hero-concept.png",
    label: "Design target",
    title: "ATOMiK Desk",
    caption: "ATOMiK Desk concept visual - target product direction, not current live UI.",
  },
];

const offers = [
  {
    name: "Evaluation Access",
    cta: "Request Evaluation Access",
    href: "/contact?intent=evaluation",
    for: "Technical founders, engineers, researchers, infrastructure teams, and early evaluators.",
    body: "Request limited evaluation access and receive technical updates, proof artifacts, and availability for early demos.",
  },
  {
    name: "Design Partner / Paid Technical Evaluation",
    cta: "Discuss Design Partnership",
    href: "/contact?intent=design-partner",
    for: "Teams with a real state-heavy workload, edge or embedded deployment, distributed sync problem, or compute-efficiency requirement.",
    body: "Work with ATOMiK to evaluate whether state-aware execution can improve a real workload. Scope includes technical discovery, success criteria, prototype mapping, and evaluation deliverables.",
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
            <Label>Live hardware prototype screenshot</Label>
            <h1 className="mt-5 max-w-3xl text-4xl font-bold leading-[1.06] md:text-6xl">
              State-aware compute for systems that cannot afford to recompute everything.
            </h1>
            <p className="mt-6 max-w-2xl text-lg leading-8" style={{ color: colors.muted }}>
              ATOMiK makes state change, delta application, and adaptive execution first-class compute primitives - starting with live hardware prototypes on Zynq / NaxRiscv / Linux.
            </p>
            <div className="mt-8 flex flex-wrap gap-3">
              <Link
                href="/contact?intent=evaluation"
                className="rounded-lg px-5 py-3 text-sm font-semibold no-underline transition-opacity hover:opacity-90"
                style={{ background: colors.blue, color: "#fff" }}
              >
                Request Evaluation Access
              </Link>
              <Link
                href="https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/technical-proof.md"
                className="rounded-lg px-5 py-3 text-sm font-semibold no-underline transition-colors hover:text-white"
                style={{ color: colors.text, border: `1px solid ${colors.border}` }}
              >
                View Technical Proof
              </Link>
            </div>
            <div className="mt-8 grid gap-3 text-sm sm:grid-cols-3" style={{ color: colors.muted }}>
              <span>Live board demos</span>
              <span>Public proof artifacts</span>
              <span>Concepts labeled separately</span>
            </div>
          </div>

          <figure
            className="overflow-hidden rounded-lg"
            style={{ border: `1px solid ${colors.border}`, background: colors.panel }}
          >
            <Image
              src="/07-current-live-atomik-desk-v038g.png"
              width={1920}
              height={1080}
              priority
              sizes="(min-width: 768px) 48vw, 100vw"
              alt="ATOMiK Desk v0.38-G prototype UI running on live hardware"
              className="h-auto w-full"
            />
            <figcaption className="px-4 py-3 text-xs" style={{ color: colors.muted }}>
              <strong style={{ color: colors.text }}>HARDWARE_VALIDATED:</strong> ATOMiK Desk v0.38-G prototype UI running on live hardware. Not a shipped desktop product.
            </figcaption>
          </figure>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>The problem</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">State-heavy systems keep asking what changed.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              State-heavy systems spend too much work asking the same question again: what changed? At the edge, inside embedded systems, and across distributed infrastructure, repeated recomputation creates latency, power draw, synchronization overhead, and operational complexity.
            </p>
          </div>
          <div className="mt-8 grid gap-4 md:grid-cols-4">
            {problemPoints.map((point) => (
              <div key={point} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <p className="text-sm leading-6" style={{ color: colors.muted }}>{point}</p>
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-14" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="grid gap-8 md:grid-cols-[0.95fr_1.05fr] md:items-start">
            <div>
              <Label>How ATOMiK works</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">One state-change primitive, from SDK to hardware direction.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
                ATOMiK keeps the homepage simple: reference state, compact deltas, reconstruction on demand, and explicit epoch transitions. The deeper architecture lives in the technical docs.
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
          <div className="flex flex-col gap-4 md:flex-row md:items-end md:justify-between">
            <div className="max-w-3xl">
              <Label>Proof today, roadmap next</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">What is live today and where it goes next.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
                Current proof, measured artifacts, synthesis output, and concept visuals are labeled separately. That keeps ATOMiK ambitious without blurring what has already been demonstrated.
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

          <div className="mt-8 grid gap-4 md:grid-cols-4">
            {proofItems.map(([label, text]) => (
              <div key={label} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <div className="text-[11px] font-semibold" style={{ color: colors.cyan }}>{label}</div>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{text}</p>
              </div>
            ))}
          </div>

          <div className="mt-6 grid gap-4 md:grid-cols-3">
            {conceptCards.map((card) => (
              <figure key={card.src} className="overflow-hidden rounded-lg" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <Image
                  src={card.src}
                  width={1672}
                  height={941}
                  sizes="(min-width: 768px) 31vw, 100vw"
                  alt={card.caption}
                  className="h-auto w-full"
                />
                <figcaption className="px-4 py-4">
                  <div className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{card.label}</div>
                  <h3 className="mt-1 font-semibold">{card.title}</h3>
                  <p className="mt-1 text-xs leading-5" style={{ color: colors.muted }}>{card.caption}</p>
                </figcaption>
              </figure>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>Evaluation and design partners</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">Start with a scoped evaluation, not a leap of faith.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Bring one workload, one sync path, or one state-heavy bottleneck. ATOMiK will map it to the evidence stack, define success criteria, and determine whether there is real fit.
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
            Live screenshots show current prototypes. Concept visuals show product direction and are not represented as current shipped functionality. Performance claims are only stated when backed by measured artifacts.
          </p>
        </div>
      </section>
    </div>
  );
}
