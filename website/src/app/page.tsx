import Image from "next/image";
import Link from "next/link";
import type { Metadata } from "next";
import type { CSSProperties, ReactNode } from "react";
import Nav from "@/components/Nav";
import {
  fitSignals,
  intakeItems,
  metricMatrix,
  offerFormula,
  positioningStatement,
  primaryIcp,
  proofToday,
  successExamples,
} from "@/lib/messaging";

export const metadata: Metadata = {
  title: "ATOMiK - State-Aware Compute Evaluation",
  description:
    "ATOMiK helps edge and embedded teams evaluate whether state-aware execution can reduce redundant state movement in constrained workloads.",
  openGraph: {
    title: "ATOMiK - State-Aware Compute Evaluation",
    description:
      "Request an evidence-bound evaluation for workloads constrained by heat, battery, bandwidth, latency, or hardware footprint.",
    url: "https://atomik.tech",
    images: [{ url: "https://atomik.tech/09-current-live-atomik-desk-v039k.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK - State-Aware Compute Evaluation",
    description:
      "Bring one state-heavy workload, one baseline, and one painful constraint.",
    images: ["https://atomik.tech/09-current-live-atomik-desk-v039k.png"],
  },
};

const colors = {
  bg: "#070807",
  panel: "#10130f",
  panel2: "#17130f",
  panel3: "#0d1513",
  border: "#2d3a34",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  cyan: "#22d3ee",
  blue: "#2563eb",
  green: "#2dd36f",
  amber: "#f5a524",
  rose: "#fb7185",
  violet: "#c084fc",
};

const paidPains = [
  {
    title: "Battery and duty cycle",
    body:
      "If state scans, syncs, or radio wake-ups consume a limited power budget, ATOMiK can evaluate whether meaningful-change tracking may reduce active work.",
    accent: colors.green,
  },
  {
    title: "Heat and enclosure limits",
    body:
      "If redundant state work contributes to utilization or thermal pressure, ATOMiK can map the workload before any heat claim is made.",
    accent: colors.amber,
  },
  {
    title: "Bandwidth and link cost",
    body:
      "If full-state movement stresses expensive, remote, intermittent, or congested links, ATOMiK evaluates bytes moved and transfers avoided.",
    accent: colors.cyan,
  },
  {
    title: "Latency and local execution",
    body:
      "If replay, reconstruction, or sync sits on the response path, ATOMiK evaluates whether state-aware boundaries can lower update cost.",
    accent: colors.violet,
  },
  {
    title: "Footprint and weight",
    body:
      "If hardware is overbuilt around state movement, cooling, or bandwidth pressure, ATOMiK can test whether that pressure has a state-aware path.",
    accent: colors.rose,
  },
];

const mechanisms = [
  {
    title: "Track meaningful change",
    body:
      "ATOMiK starts from a known state and records the state transitions that matter to the workload.",
  },
  {
    title: "Coalesce repeated work",
    body:
      "When many logical operations hit fewer state regions, ATOMiK evaluates whether they can be collapsed safely.",
  },
  {
    title: "Avoid full-state movement",
    body:
      "The goal is not to move everything faster. The goal is to avoid moving, scanning, replaying, or rebuilding state that does not need it.",
  },
  {
    title: "Preserve correctness",
    body:
      "Every evaluation needs an expected state, invariant, or output so reduced movement does not become reduced correctness.",
  },
];

function Label({ children }: { children: ReactNode }) {
  return (
    <span
      className="inline-flex rounded px-2 py-1 text-[11px] font-semibold uppercase"
      style={{ color: colors.cyan, border: `1px solid ${colors.border}`, background: "#07110f" }}
    >
      {children}
    </span>
  );
}

function EvidenceLink({
  href,
  className,
  style,
  children,
}: {
  href: string;
  className?: string;
  style?: CSSProperties;
  children: ReactNode;
}) {
  if (href.startsWith("http")) {
    return (
      <a href={href} className={className} style={style}>
        {children}
      </a>
    );
  }
  return (
    <Link href={href} className={className} style={style}>
      {children}
    </Link>
  );
}

export default function Home() {
  const homepageMetrics = metricMatrix.filter((item) =>
    [
      "Bytes moved",
      "Full-state transfers avoided",
      "Operations coalesced",
      "Unique-region ratio",
      "Cycles per update",
      "Update latency",
      "Power proxy",
      "Thermal proxy",
      "Correctness preservation",
    ].includes(item.metric)
  );

  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Product" />

      <section className="px-6 pb-14 pt-16 md:pt-20">
        <div className="mx-auto grid max-w-6xl gap-10 md:grid-cols-[1.05fr_0.95fr] md:items-center">
          <div>
            <Label>State-aware compute evaluation</Label>
            <h1 className="mt-5 max-w-4xl text-4xl font-bold leading-[1.05] md:text-6xl">
              Stop wasting power, bandwidth, and time moving state your system already knows.
            </h1>
            <p className="mt-6 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
              ATOMiK helps edge and embedded teams evaluate whether state-aware execution can reduce redundant state movement in workloads constrained by heat, battery, bandwidth, latency, or hardware footprint.
            </p>
            <div className="mt-8 flex flex-col gap-3 sm:flex-row">
              <Link
                href="/contact?intent=evaluation&source=homepage-hero&cta=request-evaluation"
                className="rounded px-5 py-3 text-center text-sm font-semibold no-underline transition-opacity hover:opacity-90"
                style={{ background: colors.blue, color: "#fff" }}
              >
                Request Evaluation
              </Link>
              <Link
                href="/benchmarks"
                className="rounded px-5 py-3 text-center text-sm font-semibold no-underline transition-colors hover:text-white"
                style={{ color: colors.text, border: `1px solid ${colors.border}` }}
              >
                Review Proof
              </Link>
            </div>
            <div className="mt-8 grid gap-3 text-sm sm:grid-cols-3" style={{ color: colors.muted }}>
              <span>One workload</span>
              <span>One baseline</span>
              <span>One painful constraint</span>
            </div>
          </div>

          <figure className="overflow-hidden rounded" style={{ border: `1px solid ${colors.border}`, background: colors.panel }}>
            <Image
              src="/09-current-live-atomik-desk-v039k.png"
              width={1920}
              height={1080}
              priority
              sizes="(min-width: 768px) 46vw, 100vw"
              alt="ATOMiK Desk v0.39-K prototype UI running on live Zynq hardware"
              className="h-auto w-full"
            />
            <figcaption className="px-4 py-3 text-xs" style={{ color: colors.muted }}>
              <strong style={{ color: colors.text }}>Hardware-validated prototype image:</strong> ATOMiK Desk v0.39-K running on live Zynq hardware. This image is not a power, thermal, uptime, or production-readiness claim.
            </figcaption>
          </figure>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <Label>ATOMiK is X that helps Y by Z</Label>
          <div className="mt-5 grid gap-5 md:grid-cols-[1fr_1.05fr]">
            <div className="rounded p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
              <h2 className="text-3xl font-bold md:text-4xl">What ATOMiK is.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>{positioningStatement}</p>
              <p className="mt-5 text-sm leading-6" style={{ color: colors.faint }}>
                First ICP: {primaryIcp}
              </p>
            </div>
            <div className="grid gap-3">
              {[
                ["Give us X", offerFormula.give],
                ["We evaluate Y", offerFormula.evaluate],
                ["You receive Z", offerFormula.receive],
                ["Success looks like A", offerFormula.success],
              ].map(([title, body]) => (
                <article key={title} className="rounded p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                  <p className="text-[11px] font-semibold uppercase" style={{ color: colors.green }}>{title}</p>
                  <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{body}</p>
                </article>
              ))}
            </div>
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>Paid pain</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">Why wasted state movement becomes a buying conversation.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              The first question is not whether ATOMiK is interesting. It is whether redundant state movement is already costing the team power, heat, bandwidth, latency, size, reliability, or engineering time.
            </p>
          </div>
          <div className="mt-8 grid gap-4 md:grid-cols-5">
            {paidPains.map((item) => (
              <article key={item.title} className="rounded p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <div className="mb-4 h-1 w-14 rounded" style={{ background: item.accent }} />
                <h3 className="text-lg font-bold">{item.title}</h3>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
              </article>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto grid max-w-6xl gap-8 md:grid-cols-[0.92fr_1.08fr]">
          <div>
            <Label>Plain English mechanism</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">ATOMiK tracks change instead of repeatedly treating all state as new.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Many constrained systems spend too much work moving, scanning, syncing, replaying, or rebuilding state. ATOMiK maps those paths and evaluates whether a state-aware architecture can reduce the wasted work.
            </p>
            <Link href="/pricing" className="mt-6 inline-flex rounded px-4 py-2 text-sm font-semibold no-underline" style={{ color: colors.cyan, border: `1px solid ${colors.border}` }}>
              See evaluation process
            </Link>
          </div>
          <div className="grid gap-4 sm:grid-cols-2">
            {mechanisms.map((item) => (
              <article key={item.title} className="rounded p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <h3 className="font-semibold">{item.title}</h3>
                <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
              </article>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="grid gap-8 md:grid-cols-[0.85fr_1.15fr]">
            <div>
              <Label>Best fit</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">The first evaluation needs evidence, not a technology tour.</h2>
              <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
                ATOMiK is strongest where state movement, repeated scans, full-state sync, replay, or reconstruction dominates the cost.
              </p>
            </div>
            <div className="grid gap-3 sm:grid-cols-2">
              {fitSignals.map((signal) => (
                <div key={signal} className="rounded p-4 text-sm leading-6" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.muted }}>
                  <span style={{ color: colors.green }}>-</span> {signal}
                </div>
              ))}
            </div>
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>What to bring to an evaluation</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">Bring one real workload, one baseline, and one constraint.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Customers do not need to expose an entire product. They need to provide enough about one constrained state path to decide whether ATOMiK is relevant.
            </p>
          </div>
          <div className="mt-8 grid gap-3 md:grid-cols-3">
            {intakeItems.map((item) => (
              <div key={item} className="rounded p-4 text-sm leading-6" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.muted }}>
                {item}
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="flex flex-col gap-4 md:flex-row md:items-end md:justify-between">
            <div className="max-w-3xl">
              <Label>What we measure</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">Success is agreed before the benchmark starts.</h2>
              <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
                ATOMiK is successful when it improves the pre-agreed metric against the current baseline, preserves correctness, and connects that improvement to a business constraint worth pursuing.
              </p>
            </div>
            <Link href="/pricing#metrics" className="rounded px-4 py-2 text-sm font-semibold no-underline" style={{ color: colors.cyan, border: `1px solid ${colors.border}` }}>
              Full metrics matrix
            </Link>
          </div>
          <div className="mt-8 overflow-x-auto rounded" style={{ border: `1px solid ${colors.border}` }}>
            <table className="w-full min-w-[860px] border-collapse text-left text-sm">
              <thead style={{ background: colors.panel2 }}>
                <tr>
                  <th className="px-4 py-3 font-semibold">Metric</th>
                  <th className="px-4 py-3 font-semibold">When it matters</th>
                  <th className="px-4 py-3 font-semibold">Meaningful result</th>
                  <th className="px-4 py-3 font-semibold">Public safety</th>
                </tr>
              </thead>
              <tbody>
                {homepageMetrics.map((item) => (
                  <tr key={item.metric} style={{ borderTop: `1px solid ${colors.border}` }}>
                    <td className="px-4 py-3 font-semibold" style={{ color: colors.text }}>{item.metric}</td>
                    <td className="px-4 py-3" style={{ color: colors.muted }}>{item.matters}</td>
                    <td className="px-4 py-3" style={{ color: colors.muted }}>{item.meaningful}</td>
                    <td className="px-4 py-3" style={{ color: colors.faint }}>{item.whereSafe}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
          <div className="mt-6 grid gap-3 md:grid-cols-3">
            {successExamples.map((item) => (
              <div key={item} className="rounded p-4 text-sm leading-6" style={{ background: colors.panel3, border: `1px solid ${colors.border}`, color: colors.muted }}>
                {item}
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <Label>How we know ATOMiK works today</Label>
            <h2 className="mt-4 text-3xl font-bold md:text-4xl">The proof is real, specific, and workload-bound.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              ATOMiK is evaluated workload by workload. Current evidence shows the architecture can win in specific coalesced or batched scenarios and lose in others.
            </p>
          </div>
          <div className="mt-8 grid gap-4 md:grid-cols-3">
            {proofToday.map((item) => (
              <EvidenceLink key={item.title} href={item.href} className="rounded p-5 no-underline" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.text }}>
                <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{item.label}</p>
                <h3 className="mt-2 text-lg font-bold">{item.title}</h3>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
              </EvidenceLink>
            ))}
          </div>
          <p className="mt-6 max-w-4xl text-xs leading-6" style={{ color: colors.faint }}>
            Performance claims must be quoted only with their artifact, context, and caveat. Do not isolate the biggest number without the interpretation.
          </p>
        </div>
      </section>

      <section className="px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl rounded p-8" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
          <div className="grid gap-8 md:grid-cols-[1fr_0.9fr] md:items-center">
            <div>
              <Label>Next step</Label>
              <h2 className="mt-4 text-3xl font-bold md:text-4xl">Request an evaluation around one constrained state path.</h2>
              <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
                The right next conversation determines whether you need proof review, workload mapping, a benchmark exchange, technical evaluation, licensing diligence, investor diligence, or a no-fit answer.
              </p>
            </div>
            <div className="flex flex-col gap-3">
              <Link href="/contact?intent=evaluation&source=homepage-final&cta=request-evaluation" className="rounded px-5 py-3 text-center text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
                Request Evaluation
              </Link>
              <Link href="/benchmarks" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
                Review Proof
              </Link>
              <Link href="/contact?intent=licensing&source=homepage-final&cta=discuss-licensing" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
                Discuss Licensing
              </Link>
              <Link href="/investor-brief" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
                Investor Diligence
              </Link>
            </div>
          </div>
        </div>
      </section>
    </div>
  );
}
