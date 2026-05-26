import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import CheckoutButton from "@/components/CheckoutButton";
import { paidOffers } from "@/lib/offers";
import { contactHref } from "@/lib/tracking";
import {
  evaluationProcess,
  fitSignals,
  intakeItems,
  metricMatrix,
  noFitSignals,
  notSuccessClaims,
  offerFormula,
  primaryIcp,
  successExamples,
} from "@/lib/messaging";

export const metadata: Metadata = {
  title: "Evaluation Access - ATOMiK",
  description:
    "Request proof review or a scoped technical evaluation for one state-heavy workload, one baseline, and one measurable constraint.",
  openGraph: {
    title: "ATOMiK Evaluation Access",
    description:
      "Operational evaluation process, intake package, metrics matrix, fit criteria, and proof boundaries.",
    url: "https://atomik.tech/pricing",
    images: [{ url: "https://atomik.tech/09-current-live-atomik-desk-v039k.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK Evaluation Access",
    description:
      "Bring one workload, one baseline, and one constraint that already hurts.",
    images: ["https://atomik.tech/09-current-live-atomik-desk-v039k.png"],
  },
};

const colors = {
  bg: "#070807",
  panel: "#10130f",
  panel2: "#17130f",
  border: "#2d3a34",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  cyan: "#22d3ee",
  blue: "#2563eb",
  green: "#2dd36f",
  amber: "#f5a524",
  rose: "#fb7185",
};

const requestOffers = [
  {
    name: "Licensing / IP Diligence",
    cta: "Discuss Licensing",
    href: contactHref("licensing", "evaluation-strategic-card", "discuss-licensing"),
    price: "Request-based",
    for: "Chip, embedded, infrastructure, and strategic partners evaluating ATOMiK as architecture or embeddable IP.",
    includes: [
      "Proof-bound IP and artifact review",
      "Hardware and synthesis evidence map",
      "ASIC feasibility and integration-risk discussion",
      "Licensing or strategic partnership next-step plan",
    ],
  },
  {
    name: "Investor Diligence",
    cta: "Investor Diligence",
    href: contactHref("investor", "evaluation-strategic-card", "investor-diligence"),
    price: "Request-based",
    for: "Investors reviewing the transition from proof to evaluated customer/IP opportunities.",
    includes: [
      "Investor brief and evidence map",
      "Claim boundaries and proof packet",
      "Paid-evaluation and design-partner strategy",
      "IP protection and ASIC feasibility review",
    ],
  },
];

const faqs = [
  {
    q: "Who is this evaluation for?",
    a: `The best fit is ${primaryIcp}`,
  },
  {
    q: "What happens after checkout or contact?",
    a:
      "ATOMiK follows up to anchor the conversation on one workload, one current baseline, one painful constraint, one primary metric, and the evidence required for a decision. Larger engagements are scoped in writing.",
  },
  {
    q: "Do we need an NDA?",
    a:
      "Not always. The first pass can use pseudocode, synthetic traces, sanitized logs, or representative state behavior. NDA is appropriate when the constrained state path cannot be evaluated responsibly without sensitive details.",
  },
  {
    q: "What does not happen yet?",
    a:
      "This is not generic self-serve product access, a guaranteed production integration, or a promise of universal speedup, battery extension, heat reduction, water savings, or smaller hardware.",
  },
  {
    q: "How are claims handled?",
    a:
      "Measured, hardware-validated, software-validated, synthesis-validated, build-artifact, projected, conceptual, and roadmap claims are labeled separately and linked to artifacts when used publicly.",
  },
];

export default function PricingPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Evaluation" />

      <section className="mx-auto max-w-6xl px-6 pb-10 pt-16">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Evaluation access
        </p>
        <h1 className="mt-4 max-w-4xl text-4xl font-bold leading-tight md:text-6xl">
          Bring one workload. We will map whether state movement is the expensive part.
        </h1>
        <p className="mt-5 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
          The evaluation page is for edge, embedded, AI-at-the-edge, remote, robotics, industrial, IoT, defense-adjacent, and hardware-constrained teams that can name a baseline and a constraint worth solving.
        </p>
        <div className="mt-7 flex flex-col gap-3 sm:flex-row">
          <Link href={contactHref("evaluation", "evaluation-hero", "request-evaluation")} className="rounded px-5 py-3 text-center text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Request Evaluation
          </Link>
          <Link href="/benchmarks" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
            Review Proof
          </Link>
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 pb-12">
        <div className="grid gap-4 md:grid-cols-4">
          {[
            ["Give us", offerFormula.give],
            ["We evaluate", offerFormula.evaluate],
            ["You receive", offerFormula.receive],
            ["Success looks like", offerFormula.success],
          ].map(([title, body]) => (
            <article key={title} className="rounded p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
              <p className="text-[11px] font-semibold uppercase" style={{ color: colors.green }}>{title}</p>
              <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{body}</p>
            </article>
          ))}
        </div>
      </section>

      <section id="what-to-bring" className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="grid gap-8 md:grid-cols-[0.8fr_1.2fr]">
          <div>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>What to bring</p>
            <h2 className="mt-3 text-3xl font-bold">The best first evaluation starts with one real workload.</h2>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              You do not need to expose your entire product. Provide enough about one constrained state path to evaluate whether ATOMiK is relevant.
            </p>
          </div>
          <div className="grid gap-3 sm:grid-cols-2">
            {intakeItems.map((item) => (
              <div key={item} className="rounded p-4 text-sm leading-6" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.muted }}>
                {item}
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>What ATOMiK evaluates</p>
        <h2 className="mt-3 max-w-3xl text-3xl font-bold md:text-4xl">Where state movement creates waste.</h2>
        <div className="mt-6 grid gap-4 md:grid-cols-3">
          {[
            "Full-state movement",
            "Repeated state scans",
            "Repeated replay or reconstruction",
            "Avoidable sync work",
            "Deltas and coalescing opportunities",
            "Cleaner state transition boundaries",
          ].map((item) => (
            <div key={item} className="rounded p-5 text-sm leading-6" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.muted }}>
              {item}
            </div>
          ))}
        </div>
      </section>

      <section id="metrics" className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="max-w-3xl">
          <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Metrics we can measure</p>
          <h2 className="mt-3 text-3xl font-bold md:text-4xl">Every claim needs a metric, baseline, method, and evidence tier.</h2>
          <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
            Power, thermal, battery, footprint, and business-outcome claims stay diligence-only unless a responsible artifact supports the exact result.
          </p>
        </div>
        <div className="mt-8 overflow-x-auto rounded" style={{ border: `1px solid ${colors.border}` }}>
          <table className="w-full min-w-[1180px] border-collapse text-left text-sm">
            <thead style={{ background: colors.panel2 }}>
              <tr>
                <th className="px-4 py-3 font-semibold">Metric</th>
                <th className="px-4 py-3 font-semibold">When it matters</th>
                <th className="px-4 py-3 font-semibold">Customer provides</th>
                <th className="px-4 py-3 font-semibold">ATOMiK evaluates</th>
                <th className="px-4 py-3 font-semibold">Meaningful result</th>
                <th className="px-4 py-3 font-semibold">Public safety</th>
              </tr>
            </thead>
            <tbody>
              {metricMatrix.map((item) => (
                <tr key={item.metric} style={{ borderTop: `1px solid ${colors.border}` }}>
                  <td className="px-4 py-3 font-semibold" style={{ color: colors.text }}>{item.metric}</td>
                  <td className="px-4 py-3" style={{ color: colors.muted }}>{item.matters}</td>
                  <td className="px-4 py-3" style={{ color: colors.muted }}>{item.customerProvides}</td>
                  <td className="px-4 py-3" style={{ color: colors.muted }}>{item.atomikEvaluates}</td>
                  <td className="px-4 py-3" style={{ color: colors.muted }}>{item.meaningful}</td>
                  <td className="px-4 py-3" style={{ color: colors.faint }}>{item.whereSafe}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Step-by-step process</p>
        <h2 className="mt-3 max-w-3xl text-3xl font-bold md:text-4xl">From first response to final outcome.</h2>
        <div className="mt-8 grid gap-4 md:grid-cols-2">
          {evaluationProcess.map((item) => (
            <article key={item.step} className="rounded p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
              <p className="text-[11px] font-semibold uppercase" style={{ color: colors.green }}>Step {item.step}</p>
              <h3 className="mt-2 text-lg font-bold">{item.title}</h3>
              <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
            </article>
          ))}
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="grid gap-5 md:grid-cols-2">
          <article className="rounded p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>What you receive</p>
            <ul className="mt-5 space-y-3">
              {[
                "Workload map",
                "Baseline comparison",
                "Evidence/proof map",
                "Results or evaluation plan",
                "Fit/no-fit recommendation",
                "Risks and claim boundaries",
                "Next-step recommendation",
              ].map((item) => (
                <li key={item} className="text-sm leading-6" style={{ color: colors.muted }}>
                  <span style={{ color: colors.green }}>-</span> {item}
                </li>
              ))}
            </ul>
          </article>
          <article className="rounded p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>What success looks like</p>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              ATOMiK is successful for a customer evaluation when it improves the pre-agreed metric against the current baseline, preserves correctness, and connects that improvement to a business constraint worth pursuing.
            </p>
            <div className="mt-5 grid gap-2">
              {successExamples.map((item) => (
                <div key={item} className="text-sm leading-6" style={{ color: colors.muted }}>
                  <span style={{ color: colors.green }}>-</span> {item}
                </div>
              ))}
            </div>
          </article>
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="grid gap-5 md:grid-cols-2">
          <article className="rounded p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.green }}>Fit signals</p>
            <div className="mt-5 grid gap-3">
              {fitSignals.map((item) => (
                <p key={item} className="text-sm leading-6" style={{ color: colors.muted }}>{item}</p>
              ))}
            </div>
          </article>
          <article className="rounded p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.rose }}>No-fit signals</p>
            <div className="mt-5 grid gap-3">
              {noFitSignals.map((item) => (
                <p key={item} className="text-sm leading-6" style={{ color: colors.muted }}>{item}</p>
              ))}
            </div>
          </article>
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Reserve or request</p>
        <h2 className="mt-3 max-w-3xl text-3xl font-bold md:text-4xl">Pick the next serious step.</h2>
        <div className="mt-8 grid gap-5 md:grid-cols-2">
          {paidOffers.map((offer) => (
            <article key={offer.id} className="rounded p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
              <div className="flex flex-col gap-3 sm:flex-row sm:items-start sm:justify-between">
                <div>
                  <p className="text-xs font-semibold uppercase" style={{ color: colors.green }}>Reservation</p>
                  <h3 className="mt-2 text-2xl font-bold">{offer.name}</h3>
                </div>
                <div className="text-left sm:text-right">
                  <div className="text-3xl font-bold">{offer.priceLabel}</div>
                  <div className="text-xs" style={{ color: colors.muted }}>one-time reservation</div>
                </div>
              </div>
              <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
                <strong style={{ color: colors.text }}>For:</strong> {offer.for}
              </p>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{offer.body}</p>
              <ul className="mt-5 space-y-3">
                {offer.includes.map((item) => (
                  <li key={item} className="text-sm leading-6" style={{ color: colors.muted }}>
                    <span style={{ color: colors.cyan }}>-</span> {item}
                  </li>
                ))}
              </ul>
              <div className="mt-6 flex flex-col gap-3 sm:flex-row">
                <CheckoutButton
                  offerId={offer.id}
                  source={`evaluation-card-${offer.id}`}
                  cta={offer.cta}
                  wrapperClassName="w-full sm:w-auto"
                  className="min-h-11 w-full rounded px-4 py-2 text-sm font-semibold text-white transition-opacity hover:opacity-90 disabled:opacity-60"
                  style={{ background: colors.blue }}
                >
                  {offer.cta}
                </CheckoutButton>
                <Link
                  href={`${offer.contactHref}&source=evaluation-card-${offer.id}&cta=ask-first`}
                  className="min-h-11 rounded px-4 py-2 text-center text-sm font-semibold no-underline transition-opacity hover:opacity-90"
                  style={{ color: colors.text, border: `1px solid ${colors.border}` }}
                >
                  Ask first
                </Link>
              </div>
            </article>
          ))}
        </div>

        <div className="mt-5 grid gap-5 md:grid-cols-2">
          {requestOffers.map((offer) => (
            <article key={offer.name} className="rounded p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
              <p className="text-xs font-semibold uppercase" style={{ color: colors.cyan }}>Strategic track</p>
              <div className="mt-2 flex flex-col gap-2 sm:flex-row sm:items-start sm:justify-between">
                <h3 className="text-2xl font-bold">{offer.name}</h3>
                <span className="text-sm font-semibold" style={{ color: colors.green }}>{offer.price}</span>
              </div>
              <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
                <strong style={{ color: colors.text }}>For:</strong> {offer.for}
              </p>
              <div className="mt-5 grid gap-2">
                {offer.includes.map((item) => (
                  <p key={item} className="text-sm leading-6" style={{ color: colors.muted }}>
                    <span style={{ color: colors.cyan }}>-</span> {item}
                  </p>
                ))}
              </div>
              <Link href={offer.href} className="mt-6 inline-flex rounded px-4 py-2 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
                {offer.cta}
              </Link>
            </article>
          ))}
        </div>
      </section>

      <section className="mx-auto max-w-6xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="rounded p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
          <p className="text-sm font-semibold uppercase" style={{ color: colors.amber }}>Proof boundary</p>
          <h2 className="mt-3 text-2xl font-bold">Do not define success as a universal claim.</h2>
          <div className="mt-5 grid gap-3 md:grid-cols-2">
            {notSuccessClaims.map((item) => (
              <p key={item} className="text-sm leading-6" style={{ color: colors.muted }}>
                <span style={{ color: colors.rose }}>-</span> {item}
              </p>
            ))}
          </div>
        </div>
      </section>

      <section className="mx-auto max-w-3xl px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <h2 className="text-2xl font-bold">FAQ</h2>
        <div className="mt-5 space-y-3">
          {faqs.map((faq) => (
            <article key={faq.q} className="rounded p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
              <h3 className="font-semibold">{faq.q}</h3>
              <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{faq.a}</p>
            </article>
          ))}
        </div>
        <div className="mt-8 flex flex-col gap-3 sm:flex-row">
          <Link href={contactHref("evaluation", "evaluation-final", "request-evaluation")} className="rounded px-5 py-3 text-center text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Request Evaluation
          </Link>
          <Link href="/benchmarks" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
            Review Proof
          </Link>
        </div>
      </section>
    </div>
  );
}
