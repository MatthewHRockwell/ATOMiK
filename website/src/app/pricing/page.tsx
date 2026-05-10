import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Evaluation Access - ATOMiK",
  description:
    "Two public paths for evaluating ATOMiK: request evaluation access or discuss a design partner engagement.",
};

const colors = {
  bg: "#070b12",
  panel: "#0d1420",
  border: "#1d324a",
  text: "#f4f8ff",
  muted: "#9fb1c7",
  cyan: "#22d3ee",
  blue: "#4f8fff",
};

const offers = [
  {
    name: "Evaluation Access",
    cta: "Request Evaluation Access",
    href: "/contact?intent=evaluation",
    for: "Technical founders, engineers, researchers, infrastructure teams, and early evaluators.",
    includes: [
      "Proof artifact review",
      "Workload-fit conversation",
      "Technical updates and early demo availability",
      "Path to a scoped benchmark exchange when fit is strong",
    ],
    body: "Request limited evaluation access and receive technical updates, proof artifacts, and availability for early demos.",
  },
  {
    name: "Design Partner / Paid Technical Evaluation",
    cta: "Discuss Design Partnership",
    href: "/contact?intent=design-partner",
    for: "Teams with a real state-heavy workload, edge or embedded deployment, distributed sync problem, or compute-efficiency requirement.",
    includes: [
      "Technical discovery and workload mapping",
      "Success criteria and evaluation plan",
      "Prototype mapping where appropriate",
      "Evaluation deliverables and continue / refine / stop recommendation",
    ],
    body: "Work with ATOMiK to evaluate whether state-aware execution can improve a real workload.",
  },
];

const faqs = [
  {
    q: "Is there a free tier?",
    a: "No conventional public free tier is listed. ATOMiK is currently request-based so evaluation time is focused on real workloads and evidence review.",
  },
  {
    q: "Do I need custom hardware to evaluate ATOMiK?",
    a: "Not necessarily. The right starting point may be software exploration, a proof review, a benchmark exchange, or a hardware-backed demo depending on the workload.",
  },
  {
    q: "Is ATOMiK Desk the product?",
    a: "ATOMiK Desk is a live demonstration surface and product direction signal. The near-term commercial wedge is state-aware execution for real state-heavy workloads.",
  },
  {
    q: "How are performance claims handled?",
    a: "Measured, hardware-validated, synthesis-validated, projected, conceptual, and roadmap claims are labeled separately and linked to artifacts when used publicly.",
  },
];

export default function PricingPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Evaluation" />

      <section className="mx-auto max-w-5xl px-6 pb-10 pt-16 text-center">
        <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>
          Evaluation access
        </p>
        <h1 className="mt-4 text-4xl font-bold md:text-5xl">
          Start with a scoped evaluation, not a subscription plan.
        </h1>
        <p className="mx-auto mt-5 max-w-2xl text-lg leading-8" style={{ color: colors.muted }}>
          ATOMiK has two public paths: request evaluation access or discuss a design partner / paid technical evaluation. Bring one real workload and define success criteria before making larger commitments.
        </p>
      </section>

      <section className="mx-auto grid max-w-6xl gap-5 px-6 pb-14 md:grid-cols-2">
        {offers.map((offer) => (
          <article key={offer.name} className="rounded-lg p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <h2 className="text-2xl font-bold">{offer.name}</h2>
            <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
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
            <Link
              href={offer.href}
              className="mt-6 inline-flex rounded-lg px-4 py-2 text-sm font-semibold text-white no-underline transition-opacity hover:opacity-90"
              style={{ background: colors.blue }}
            >
              {offer.cta}
            </Link>
          </article>
        ))}
      </section>

      <section className="mx-auto max-w-3xl px-6 pb-16">
        <h2 className="text-2xl font-bold">FAQ</h2>
        <div className="mt-5 space-y-3">
          {faqs.map((faq) => (
            <article key={faq.q} className="rounded-lg p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
              <h3 className="font-semibold">{faq.q}</h3>
              <p className="mt-2 text-sm leading-6" style={{ color: colors.muted }}>{faq.a}</p>
            </article>
          ))}
        </div>
      </section>
    </div>
  );
}
