import Image from "next/image";
import Link from "next/link";
import type { Metadata } from "next";
import type { ReactNode } from "react";
import Nav from "@/components/Nav";
import {
  buyerRoutes,
  metricMatrix,
  offerFormula,
  positioningStatement,
  proofToday,
  statusQuoAlternatives,
} from "@/lib/messaging";
import { contactHref } from "@/lib/tracking";

export const metadata: Metadata = {
  title: "ATOMiK Pitch Brief - Make Change the Unit of Compute",
  description:
    "A VC-ready ATOMiK pitch narrative: buyer pain, ROI pressure, evaluation offer, proof today, business model, roadmap, and next conversation.",
  openGraph: {
    title: "ATOMiK Pitch Brief",
    description:
      "ATOMiK makes change the unit of compute for constrained edge and embedded systems.",
    url: "https://atomik.tech/pitch",
    images: [{ url: "https://atomik.tech/SiteLogo.png", width: 1024, height: 1024 }],
    type: "website",
  },
};

const colors = {
  bg: "#0B0F17",
  charcoal: "#21181E",
  porcelain: "#DED2D7",
  warm: "#F6D3B0",
  cyan: "#5FB1DA",
  blue: "#3B82B5",
  violet: "#7D40B0",
  magenta: "#B45DD8",
  amber: "#F9CA67",
  orange: "#DF8D15",
  border: "rgba(222, 210, 215, 0.18)",
  muted: "rgba(222, 210, 215, 0.72)",
};

const painLanes = [
  "Battery drain from repeated scans, syncs, and wake-ups",
  "Bandwidth pressure from moving full state over constrained links",
  "Heat from redundant work before the system creates useful output",
  "Latency from replay, reconstruction, and unnecessary state checks",
];

const proofCards = [
  {
    title: "Zynq Desk v0.40-A UI artifact",
    label: "HARDWARE_VALIDATED",
    body:
      "The current UI proof image shows ATOMiK Desk v0.40-A captured live from /dev/fb0 on Zynq hardware, with on-screen metrics driven by real measured on-board data. It is not a customer workload benchmark or production-readiness claim.",
    href: "/10-current-live-atomik-desk-v040a.png",
  },
  {
    title: "Linux userspace to FPGA validation",
    label: "HARDWARE_VALIDATED",
    body:
      "The recorded proof exercises a user process through Linux, /dev/mem, Wishbone CSR bus, and the ATOMiK core. It records 16 algebraic checks passing with zero failures.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/LINUX_USERSPACE_PROOF.md",
  },
  {
    title: "AX7020 workload matrix",
    label: "LIVE_MEASURED",
    body:
      "The board-run matrix shows workload-specific wins and losses. The honest pitch is that coalescing and workload personality matter.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/perf/20260509_matrix_interpretation.md",
  },
  {
    title: "Hardware synthesis and bank scaling",
    label: "SYNTHESIS_VALIDATED",
    body:
      "The synthesis artifact reports toolchain-characterized scaling context. It is not live-board performance proof and not production silicon proof.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/docs/HARDWARE_SYNTHESIS.md",
  },
  {
    title: "Formal proof foundation",
    label: "FORMAL_PROOF",
    body:
      "Formal proof work supports directly audited algebraic statements. It should not be expanded into workload, customer, or production outcomes.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/tree/main/math/proofs",
  },
  {
    title: "Claims registry and labels",
    label: "CLAIM_CONTROL",
    body:
      "Public claims are separated by live measured, hardware validated, software validated, formal proof, synthesis validated, build artifact, projected, conceptual, and roadmap.",
    href: "https://github.com/MatthewHRockwell/ATOMiK/blob/main/results/claims_registry.yaml",
  },
];

const roadmap = [
  "Lock 2-3 design-partner evaluations",
  "Publish one sanitized workload evaluation",
  "Harden proof packet and claims registry",
  "Complete external ASIC/IP feasibility review",
  "Define licensing architecture",
  "Build repeatable evaluation tooling",
];

const mainMetrics = [
  "Bytes moved",
  "Full-state transfers avoided",
  "Operations coalesced",
  "Cycles per update",
  "Update latency",
  "Memory/state footprint",
  "Power proxy",
  "Thermal proxy",
  "Correctness preservation",
];

function Pill({ children, tone = colors.cyan }: { children: ReactNode; tone?: string }) {
  return (
    <span
      className="inline-flex rounded px-2.5 py-1 text-[11px] font-bold uppercase"
      style={{ border: `1px solid ${tone}`, color: tone, background: "rgba(11, 15, 23, 0.75)" }}
    >
      {children}
    </span>
  );
}

function Section({
  eyebrow,
  title,
  children,
}: {
  eyebrow: string;
  title: string;
  children: ReactNode;
}) {
  return (
    <section className="px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
      <div className="mx-auto max-w-6xl">
        <Pill>{eyebrow}</Pill>
        <h2 className="mt-5 max-w-4xl text-3xl font-bold leading-tight md:text-5xl">{title}</h2>
        <div className="mt-8">{children}</div>
      </div>
    </section>
  );
}

function ExternalLink({
  href,
  children,
}: {
  href: string;
  children: ReactNode;
}) {
  if (href.startsWith("http")) {
    return (
      <a href={href} className="font-semibold no-underline hover:underline" style={{ color: colors.cyan }}>
        {children}
      </a>
    );
  }

  return (
    <Link href={href} className="font-semibold no-underline hover:underline" style={{ color: colors.cyan }}>
      {children}
    </Link>
  );
}

function StateStormVisual() {
  return (
    <div className="relative min-h-[520px] overflow-hidden rounded md:min-h-[360px]" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
      <div className="absolute inset-x-6 top-8 grid grid-cols-2 gap-3 md:grid-cols-4">
        {painLanes.map((lane, index) => (
          <div
            key={lane}
            className="h-28 rounded p-3 text-xs leading-5"
            style={{
              border: `1px solid ${colors.border}`,
              background: index % 2 === 0 ? "rgba(223, 141, 21, 0.16)" : "rgba(180, 93, 216, 0.14)",
              color: colors.porcelain,
            }}
          >
            {lane}
          </div>
        ))}
      </div>
      <div className="absolute left-8 right-8 top-72 h-14 rounded md:top-48 md:h-16" style={{ background: "rgba(223, 141, 21, 0.22)", border: `1px solid ${colors.orange}` }} />
      <div className="absolute left-12 right-12 top-80 h-10 rounded md:left-16 md:right-16 md:top-56" style={{ background: "rgba(95, 177, 218, 0.18)", border: `1px solid ${colors.cyan}` }} />
      <div className="absolute bottom-8 left-8 right-8 grid grid-cols-3 gap-3">
        {["Reference state", "Meaningful deltas", "Correct reconstruction"].map((item) => (
          <div key={item} className="rounded p-3 text-sm font-semibold" style={{ background: colors.bg, border: `1px solid ${colors.border}` }}>
            {item}
          </div>
        ))}
      </div>
    </div>
  );
}

export default function PitchPage() {
  const selectedMetrics = metricMatrix.filter((metric) => mainMetrics.includes(metric.metric));
  const selectedProof = proofToday.slice(0, 6);

  return (
    <main className="min-h-screen" style={{ background: colors.bg, color: colors.porcelain }}>
      <Nav />

      <section className="px-6 pb-16 pt-14 md:pt-20" style={{ background: `linear-gradient(135deg, ${colors.bg} 0%, ${colors.charcoal} 100%)` }}>
        <div className="mx-auto grid max-w-6xl gap-10 md:grid-cols-[1.05fr_0.95fr] md:items-center">
          <div>
            <Pill tone={colors.amber}>Pitch brief</Pill>
            <h1 className="mt-6 max-w-4xl text-4xl font-bold leading-none sm:text-5xl md:text-7xl">
              Make change the unit of compute.
            </h1>
            <p className="mt-6 max-w-3xl text-xl leading-8" style={{ color: colors.muted }}>
              ATOMiK makes change the unit of compute. We help constrained edge and embedded teams find wasted state movement, measure it against a real baseline, and decide whether a state-aware architecture belongs in their system.
            </p>
            <div className="mt-8 flex flex-col gap-3 sm:flex-row">
              <Link
                href={contactHref("evaluation", "pitch-hero", "request-evaluation")}
                className="rounded px-5 py-3 text-center text-sm font-bold text-black no-underline transition-opacity hover:opacity-90"
                style={{ background: colors.amber }}
              >
                Request Evaluation
              </Link>
              <Link
                href="/benchmarks"
                className="rounded px-5 py-3 text-center text-sm font-bold no-underline transition-colors hover:text-white"
                style={{ color: colors.porcelain, border: `1px solid ${colors.border}` }}
              >
                Review Proof
              </Link>
            </div>
          </div>

          <figure className="rounded p-6" style={{ background: "rgba(11, 15, 23, 0.68)", border: `1px solid ${colors.border}` }}>
            <Image
              src="/SiteLogo.png"
              width={1024}
              height={1024}
              priority
              alt="ATOMiK assistant concept visual holding a chip"
              className="h-auto w-full"
            />
            <figcaption className="mt-4 text-xs leading-5" style={{ color: colors.muted }}>
              Concept visual used as a guided evaluation motif. It is not a claim that Atom AI is a current commercial assistant product.
            </figcaption>
          </figure>
        </div>
      </section>

      <Section eyebrow="Pain" title="The hidden tax in constrained compute is state movement.">
        <div className="grid gap-4 md:grid-cols-4">
          {painLanes.map((lane) => (
            <article key={lane} className="rounded p-5" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              <div className="mb-4 h-1 w-14 rounded" style={{ background: colors.orange }} />
              <p className="text-lg font-semibold leading-7">{lane}</p>
            </article>
          ))}
        </div>
        <p className="mt-7 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          The commercial question is not whether less movement sounds good. It is whether one state path is expensive enough to evaluate against a real baseline.
        </p>
      </Section>

      <Section eyebrow="First wedge" title="The first customer has one workload, one baseline, and one painful constraint.">
        <div className="grid gap-5 md:grid-cols-[0.95fr_1.05fr]">
          <StateStormVisual />
          <div className="rounded p-6" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
            <h3 className="text-2xl font-bold">Best first fit</h3>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Edge and embedded teams, especially AI-at-the-edge, remote systems, industrial control, robotics, IoT, defense-adjacent, and hardware-constrained devices where battery, heat, bandwidth, latency, footprint, weight, or reliability is already painful.
            </p>
            <div className="mt-6 grid gap-3 sm:grid-cols-3">
              {["State-heavy", "Change-heavy", "Constraint-heavy"].map((item) => (
                <div key={item} className="rounded p-4 text-sm font-bold" style={{ border: `1px solid ${colors.border}` }}>
                  {item}
                </div>
              ))}
            </div>
          </div>
        </div>
      </Section>

      <Section eyebrow="Insight" title="State does not need to move as if everything changed.">
        <div className="grid gap-5 md:grid-cols-2">
          <div className="rounded p-6" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
            <h3 className="text-xl font-bold" style={{ color: colors.orange }}>Traditional path</h3>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Scan, move, replay, reconstruct, and repeat. This can be correct, but in constrained systems it often buys reliability with power, bandwidth, heat, and latency.
            </p>
          </div>
          <div className="rounded p-6" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
            <h3 className="text-xl font-bold" style={{ color: colors.cyan }}>ATOMiK path</h3>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Start from reference state, track meaningful deltas, coalesce repeated work, and reconstruct only what the workload actually needs.
            </p>
          </div>
        </div>
      </Section>

      <Section eyebrow="What ATOMiK is" title="A state-aware compute architecture for evidence-bound evaluation.">
        <div className="rounded p-7" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
          <p className="max-w-4xl text-2xl font-semibold leading-10">{positioningStatement}</p>
          <div className="mt-7 grid gap-4 md:grid-cols-3">
            {["Track meaningful change", "Coalesce repeated work", "Preserve correctness"].map((item) => (
              <div key={item} className="rounded p-4 text-sm font-bold" style={{ background: colors.bg, border: `1px solid ${colors.border}` }}>
                {item}
              </div>
            ))}
          </div>
        </div>
      </Section>

      <Section eyebrow="Offer" title="Give us one workload. We will tell you if ATOMiK fits.">
        <div className="grid gap-4 md:grid-cols-4">
          {[
            ["Give us", offerFormula.give],
            ["We evaluate", offerFormula.evaluate],
            ["You receive", offerFormula.receive],
            ["Success looks like", offerFormula.success],
          ].map(([title, body]) => (
            <article key={title} className="rounded p-5" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              <Pill tone={colors.amber}>{title}</Pill>
              <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>{body}</p>
            </article>
          ))}
        </div>
      </Section>

      <Section eyebrow="ROI path" title="ATOMiK turns waste into an investment decision.">
        <div className="grid gap-5 md:grid-cols-3">
          {[
            ["Find paid waste", "Identify where state movement creates power, bandwidth, heat, latency, footprint, or engineering-time pressure."],
            ["Measure against baseline", "Compare one constrained path against the current implementation, with correctness preserved and caveats attached."],
            ["Convert to next step", "Use the result to decide whether the right commercial path is no-fit, proof review, design partner, licensing, or investor diligence."],
          ].map(([title, body]) => (
            <article key={title} className="rounded p-6" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              <h3 className="text-xl font-bold">{title}</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{body}</p>
            </article>
          ))}
        </div>
      </Section>

      <Section eyebrow="Measurement" title="Every evaluation starts with a metric, baseline, and decision threshold.">
        <div className="grid gap-4 md:grid-cols-3">
          {selectedMetrics.map((metric) => (
            <article key={metric.metric} className="rounded p-5" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              <h3 className="text-lg font-bold">{metric.metric}</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{metric.matters}</p>
              <p className="mt-4 text-xs font-semibold uppercase" style={{ color: colors.cyan }}>{metric.whereSafe}</p>
            </article>
          ))}
        </div>
      </Section>

      <Section eyebrow="Proof today" title="The proof is real, specific, and evidence-labeled.">
        <div className="grid gap-5 md:grid-cols-2">
          {proofCards.map((proof) => (
            <article key={proof.title} className="rounded p-6" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              <Pill tone={proof.label === "LIVE_MEASURED" ? colors.amber : colors.cyan}>{proof.label}</Pill>
              <h3 className="mt-4 text-2xl font-bold">{proof.title}</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{proof.body}</p>
              <div className="mt-5">
                <ExternalLink href={proof.href}>Review artifact</ExternalLink>
              </div>
            </article>
          ))}
        </div>
        <div className="mt-6 rounded p-5" style={{ background: "rgba(249, 202, 103, 0.1)", border: `1px solid ${colors.amber}` }}>
          <p className="text-sm leading-6" style={{ color: colors.warm }}>
            Benchmark rule: quote the artifact, context, and caveat. Do not isolate the biggest number. The proof story is artifact-bound, not a universal performance claim.
          </p>
        </div>
      </Section>

      <Section eyebrow="Honest benchmark story" title="ATOMiK wins when the workload lets architecture compound.">
        <div className="grid gap-4 md:grid-cols-4">
          {["Software baseline", "Direct hardware", "Batched ATOMiK", "Profiled and coalesced"].map((stage, index) => (
            <article key={stage} className="rounded p-5" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              <p className="text-[11px] font-bold uppercase" style={{ color: colors.amber }}>Step {index + 1}</p>
              <h3 className="mt-3 text-lg font-bold">{stage}</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
                {index === 0 && "Start with the customer's current path."}
                {index === 1 && "Naive hardware access can lose when MMIO overhead dominates."}
                {index === 2 && "Batching reduces control overhead when the path allows it."}
                {index === 3 && "Coalescing can create the major win when repeated updates hit fewer regions."}
              </p>
            </article>
          ))}
        </div>
      </Section>

      <Section eyebrow="Business" title="Start with evaluations. Expand to design partnerships and licensing.">
        <div className="grid gap-4 md:grid-cols-5">
          {buyerRoutes.map((route) => (
            <article key={route.route} className="rounded p-5" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              <h3 className="text-lg font-bold">{route.route}</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{route.outcome}</p>
            </article>
          ))}
        </div>
      </Section>

      <Section eyebrow="Status quo" title="The status quo buys margin instead of removing waste.">
        <div className="grid gap-4 md:grid-cols-3">
          {statusQuoAlternatives.slice(0, 6).map((item) => (
            <article key={item.name} className="rounded p-5" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              <h3 className="text-lg font-bold">{item.name}</h3>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.notSolve}</p>
              <p className="mt-4 text-xs font-semibold uppercase" style={{ color: colors.cyan }}>ATOMiK differs: {item.atomikDiffers}</p>
            </article>
          ))}
        </div>
      </Section>

      <Section eyebrow="Roadmap" title="The next milestone is evaluated customer proof.">
        <div className="grid gap-4 md:grid-cols-3">
          {roadmap.map((item) => (
            <article key={item} className="rounded p-5 text-lg font-semibold leading-7" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
              {item}
            </article>
          ))}
        </div>
      </Section>

      <Section eyebrow="Ask" title="Turn proof into evaluated commercial opportunity.">
        <div className="grid gap-5 md:grid-cols-[1fr_0.85fr]">
          <div className="rounded p-7" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
            <p className="text-2xl font-semibold leading-10">
              ATOMiK is not asking the market to accept a broad compute claim. We are asking the right customers to bring one constrained state path, measure the waste, and decide with evidence whether state-aware compute belongs in their architecture.
            </p>
          </div>
          <div className="rounded p-7" style={{ background: colors.charcoal, border: `1px solid ${colors.border}` }}>
            <h3 className="text-2xl font-bold">Next conversation</h3>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              Request an evaluation, review the proof packet, discuss licensing/IP diligence, or route investor diligence through the claims and evidence map.
            </p>
            <div className="mt-6 grid gap-3">
              <Link href={contactHref("evaluation", "pitch-close", "request-evaluation")} className="rounded px-4 py-3 text-center text-sm font-bold text-black no-underline" style={{ background: colors.amber }}>
                Request Evaluation
              </Link>
              <Link href="/benchmarks" className="rounded px-4 py-3 text-center text-sm font-bold no-underline" style={{ border: `1px solid ${colors.border}`, color: colors.porcelain }}>
                Review Proof
              </Link>
            </div>
          </div>
        </div>
        <div className="mt-8 grid gap-4 md:grid-cols-3">
          {selectedProof.map((proof) => (
            <ExternalLink key={proof.title} href={proof.href}>
              {proof.label}: {proof.title}
            </ExternalLink>
          ))}
        </div>
      </Section>
    </main>
  );
}
