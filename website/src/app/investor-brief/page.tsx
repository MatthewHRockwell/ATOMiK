import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import {
  buyerRoutes,
  offerFormula,
  positioningStatement,
  proofToday,
} from "@/lib/messaging";

export const metadata: Metadata = {
  title: "Investor Diligence - ATOMiK",
  description:
    "Investor diligence for ATOMiK: proof-bound claims, paid evaluation motion, IP path, ASIC feasibility, and customer evaluation strategy.",
  openGraph: {
    title: "ATOMiK Investor Diligence",
    description:
      "ATOMiK is moving from proof to evaluated customer and IP opportunities, with public claims bounded by evidence tier.",
    url: "https://atomik.tech/investor-brief",
    images: [{ url: "https://atomik.tech/09-current-live-atomik-desk-v039k.png", width: 1920, height: 1080 }],
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "ATOMiK Investor Diligence",
    description:
      "Proof-bound diligence for customer evaluations, IP protection, and ASIC feasibility.",
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
};

const diligenceFocus = [
  "Proof-bound customer evaluation motion",
  "Design-partner qualification around one workload and one painful constraint",
  "IP protection and licensing path",
  "ASIC feasibility review before any tape-out commitment",
  "Evidence hierarchy separating measured, hardware validated, synthesis validated, projected, conceptual, and roadmap claims",
];

function evidenceHref(href: string) {
  return href.startsWith("http") ? href : href;
}

export default function InvestorBriefPage() {
  return (
    <div className="min-h-screen" style={{ background: colors.bg, color: colors.text }}>
      <Nav active="Investors" />

      <section className="mx-auto max-w-6xl px-6 pb-14 pt-16">
        <div className="grid gap-10 md:grid-cols-[1.05fr_0.95fr] md:items-center">
          <div>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Investor diligence</p>
            <h1 className="mt-4 max-w-4xl text-4xl font-bold leading-tight md:text-6xl">
              ATOMiK is moving from proof to evaluated customer and IP opportunities.
            </h1>
            <p className="mt-6 max-w-3xl text-lg leading-8" style={{ color: colors.muted }}>
              The homepage is customer-first. This page routes investors into the diligence path: proof-bound claims, paid evaluations, IP protection, licensing potential, and ASIC feasibility review.
            </p>
            <div className="mt-8 flex flex-col gap-3 sm:flex-row">
              <Link href="/contact?intent=investor&source=investor-hero&cta=investor-diligence" className="rounded px-5 py-3 text-center text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
                Investor Diligence
              </Link>
              <Link href="/benchmarks" className="rounded px-5 py-3 text-center text-sm font-semibold no-underline" style={{ color: colors.text, border: `1px solid ${colors.border}` }}>
                Review Proof
              </Link>
            </div>
          </div>

          <div className="rounded p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
            <p className="text-[11px] font-semibold uppercase" style={{ color: colors.green }}>Positioning</p>
            <h2 className="mt-2 text-2xl font-bold">Customer wedge before investor narrative.</h2>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>{positioningStatement}</p>
            <div className="mt-5 grid gap-3 text-sm" style={{ color: colors.muted }}>
              <div><strong style={{ color: colors.text }}>Offer:</strong> {offerFormula.give}</div>
              <div><strong style={{ color: colors.text }}>Readout:</strong> {offerFormula.receive}</div>
              <div><strong style={{ color: colors.text }}>Success:</strong> {offerFormula.success}</div>
            </div>
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto grid max-w-6xl gap-8 md:grid-cols-[0.85fr_1.15fr]">
          <div>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Diligence focus</p>
            <h2 className="mt-3 text-3xl font-bold md:text-4xl">The investor question is whether proof can become evaluated demand and protectable IP.</h2>
            <p className="mt-4 text-sm leading-6" style={{ color: colors.muted }}>
              Investor diligence should use the same claim discipline as customer evaluation. Do not turn evaluation targets into proven universal outcomes.
            </p>
          </div>
          <div className="grid gap-3">
            {diligenceFocus.map((item) => (
              <div key={item} className="rounded p-4 text-sm leading-6" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.muted }}>
                {item}
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Buyer journey</p>
          <h2 className="mt-3 max-w-3xl text-3xl font-bold md:text-4xl">Investors are secondary, but routed clearly.</h2>
          <div className="mt-8 grid gap-4 md:grid-cols-5">
            {buyerRoutes.map((route) => (
              <article key={route.route} className="rounded p-5" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
                <h3 className="text-lg font-bold">{route.route}</h3>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
                  <strong style={{ color: colors.text }}>For:</strong> {route.for}
                </p>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>
                  <strong style={{ color: colors.text }}>Outcome:</strong> {route.outcome}
                </p>
              </article>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-6xl">
          <div className="max-w-3xl">
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>Proof packet</p>
            <h2 className="mt-3 text-3xl font-bold md:text-4xl">Compelling, but evidence-bounded.</h2>
            <p className="mt-4 text-lg leading-8" style={{ color: colors.muted }}>
              Benefits such as reduced heat, lower water burden, longer battery life, lower power, smaller footprint, or faster execution are evaluation targets unless a linked artifact measures that exact outcome.
            </p>
          </div>
          <div className="mt-8 grid gap-4 md:grid-cols-3">
            {proofToday.map((item) => (
              <Link key={item.title} href={evidenceHref(item.href)} className="rounded p-5 no-underline" style={{ background: colors.panel, border: `1px solid ${colors.border}`, color: colors.text }}>
                <p className="text-[11px] font-semibold uppercase" style={{ color: colors.cyan }}>{item.label}</p>
                <h3 className="mt-2 text-lg font-bold">{item.title}</h3>
                <p className="mt-3 text-sm leading-6" style={{ color: colors.muted }}>{item.body}</p>
              </Link>
            ))}
          </div>
        </div>
      </section>

      <section className="px-6 py-12" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto grid max-w-6xl gap-8 md:grid-cols-2">
          <article className="rounded p-6" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.cyan }}>What capital de-risks</p>
            <h2 className="mt-3 text-3xl font-bold">Proof to evaluated opportunity.</h2>
            <div className="mt-5 grid gap-3 text-sm leading-6" style={{ color: colors.muted }}>
              <p>Paid design-partner evaluations against real workload constraints.</p>
              <p>Stronger IP protection and diligence materials.</p>
              <p>ASIC feasibility review before any production tape-out decision.</p>
              <p>Proof packaging that keeps claims tied to artifacts.</p>
            </div>
          </article>
          <article className="rounded p-6" style={{ background: colors.panel, border: `1px solid ${colors.border}` }}>
            <p className="text-sm font-semibold uppercase" style={{ color: colors.amber }}>Do not over-read</p>
            <h2 className="mt-3 text-3xl font-bold">Current proof is not a universal market claim.</h2>
            <p className="mt-5 text-sm leading-6" style={{ color: colors.muted }}>
              The AX7020 matrix shows workload-dependent behavior. Formal proof work exists but public pages avoid unaudited proof counts. Concept and roadmap visuals show direction, not commercial readiness.
            </p>
          </article>
        </div>
      </section>

      <section className="px-6 py-16" style={{ borderTop: `1px solid ${colors.border}` }}>
        <div className="mx-auto max-w-4xl rounded p-8 text-center" style={{ background: colors.panel2, border: `1px solid ${colors.border}` }}>
          <h2 className="text-3xl font-bold">Ready for investor diligence?</h2>
          <p className="mx-auto mt-4 max-w-2xl text-sm leading-6" style={{ color: colors.muted }}>
            The right review starts with the proof packet, claims registry, customer evaluation motion, IP status, and ASIC feasibility path.
          </p>
          <Link href="/contact?intent=investor&source=investor-final&cta=investor-diligence" className="mt-6 inline-flex rounded px-5 py-3 text-sm font-semibold text-white no-underline" style={{ background: colors.blue }}>
            Investor Diligence
          </Link>
        </div>
      </section>
    </div>
  );
}
