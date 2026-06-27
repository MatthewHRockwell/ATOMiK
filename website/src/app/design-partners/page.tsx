import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import Footer from "@/components/Footer";

export const metadata: Metadata = {
  title: "Design Partner Program — Co-Validate ATOMiK on Your Workload",
  description:
    "Become a founding ATOMiK design partner. We run your aggregation or replica-sync workload live on silicon, you get early access and roadmap influence, and you only commit to a pilot if it hits your bar.",
  openGraph: {
    title: "ATOMiK — Design Partner Program",
    description:
      "Your workload, run live on the AX7020. A conditional LOI you only honor if we hit your bar. Early access, founder access, roadmap influence.",
    url: "https://atomik.tech/design-partners",
    type: "website",
  },
};

const c = {
  bg: "#070807",
  panel: "#10130f",
  panel2: "#0c0f0c",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  border: "#2d3a34",
  cyan: "#22d3ee",
  green: "#2dd36f",
  amber: "#f5a524",
};

const BENEFITS: { title: string; body: string }[] = [
  {
    title: "Your workload, run live on silicon",
    body: "We run your aggregation or replica-sync workload at your scale on the AX7020 and hand you the dated, witnessed transcript — not a slide, the actual abench output and the byte-identical result hash.",
  },
  {
    title: "The full proof packet",
    body: "A silicon-measured verdict, a downloadable proof report, a Cost-of-Lock value memo computed from your own baseline, and a conditional LOI — the leave-behind your team uses to decide.",
  },
  {
    title: "Direct founder access",
    body: "You work with the founder, not a sales funnel. Architecture questions get answered by the person who built the engine and proved the algebra.",
  },
  {
    title: "Influence the roadmap",
    body: "Your archetype gets prioritized in the workload catalog and the runtime. Design partners shape what ships next while it is still cheap to shape.",
  },
  {
    title: "Preferential early-partner terms",
    body: "Founding partners get early-partner commercial terms, locked before general availability. Specifics are set with you — this page makes no pricing claim.",
  },
  {
    title: "White-glove integration support",
    body: "We do the integration work for the evaluation — mapping your workload to the delta-state engine and standing up the live run — so the bar is tested on your real workload, fast.",
  },
];

const STAGES: { n: string; title: string; body: string; href?: string; hrefLabel?: string }[] = [
  {
    n: "1",
    title: "Scope your bar",
    body: "Bring one workload: the aggregation (or sync), the rate, and the throughput-at-correctness bar you gate commitment on. You get a silicon-measured verdict — or an honest no if your operator is order-dependent.",
    href: "/benchmark-proof",
    hrefLabel: "Test your bar",
  },
  {
    n: "2",
    title: "Conditional LOI",
    body: "We draft the bar; you counter-sign a Conditional LOI that commits you to the next step only if a live run meets it. Non-binding until executed — it scopes the deal, it doesn't lock you in.",
    href: "/workloads",
    hrefLabel: "See the archetypes",
  },
  {
    n: "3",
    title: "Live silicon run",
    body: "We run your workload at your scale on the AX7020 and return the dated transcript: sustained throughput plus the byte-identical result across 1/2/4/8 banks. The bar is met on your workload, witnessed.",
    href: "/formal-verification",
    hrefLabel: "And formally proven",
  },
  {
    n: "4",
    title: "Pilot",
    body: "On a PASS, we proceed to a scoped pilot on the terms in the LOI. If it doesn't pass, you owe nothing and we tell you straight — including \"no fit.\"",
  },
];

function Card({ children, style }: { children: React.ReactNode; style?: React.CSSProperties }) {
  return (
    <div style={{ background: c.panel, border: `1px solid ${c.border}`, borderRadius: 14, padding: 22, ...style }}>
      {children}
    </div>
  );
}

export default function DesignPartnersPage() {
  return (
    <main style={{ background: c.bg, color: c.text, minHeight: "100vh" }}>
      <Nav active="Workloads" />
      <section style={{ maxWidth: 980, margin: "0 auto", padding: "56px 22px 80px" }}>
        <div
          style={{
            display: "inline-block", border: `1px solid ${c.border}`, borderRadius: 999,
            padding: "4px 12px", fontSize: 12, letterSpacing: 0.5, color: c.green, marginBottom: 18,
          }}
        >
          FOUNDING COHORT · OPEN
        </div>
        <h1 style={{ fontSize: 38, lineHeight: 1.1, margin: "0 0 14px", fontWeight: 800 }}>
          Become a founding design partner.
        </h1>
        <p style={{ color: c.muted, fontSize: 17, lineHeight: 1.55, maxWidth: 760 }}>
          A design partnership is the whole proof pipeline pointed at <em>your</em> workload: we run
          it live on silicon, quantify what it&apos;s worth to your stack, and put it behind a{" "}
          <strong style={{ color: c.text }}>conditional LOI you only honor if we hit your bar.</strong>{" "}
          You get early access, founder access, and roadmap influence. We get a real workload to prove
          ATOMiK on. The asymmetry is intentional — the risk sits with us.
        </p>

        {/* who it's for */}
        <Card style={{ marginTop: 26 }}>
          <p style={{ color: c.cyan, fontSize: 12, fontWeight: 600, textTransform: "uppercase", letterSpacing: 0.5, margin: "0 0 8px" }}>
            This is for you if
          </p>
          <p style={{ color: c.muted, fontSize: 14.5, lineHeight: 1.6, margin: 0 }}>
            You run an <strong style={{ color: c.text }}>aggregation/reduction or replica-sync
            workload</strong> that gates on throughput at correctness — counters, rollups, sketches,
            or convergent state that has to stay exactly correct under concurrent, out-of-order load,
            and that you keep correct today with a lock, shard fan-out, CRDT metadata, or consensus.
            If your operator is order-dependent, ATOMiK returns an honest no — this program is not for
            every workload, and we say so up front.
          </p>
        </Card>

        {/* benefits */}
        <h2 style={{ fontSize: 22, fontWeight: 800, margin: "40px 0 14px" }}>What partners get</h2>
        <div style={{ display: "grid", gap: 14, gridTemplateColumns: "repeat(auto-fit, minmax(280px, 1fr))" }}>
          {BENEFITS.map((b) => (
            <Card key={b.title}>
              <h3 style={{ fontSize: 15.5, fontWeight: 700, margin: "0 0 6px", color: c.text }}>{b.title}</h3>
              <p style={{ color: c.muted, fontSize: 14, lineHeight: 1.55, margin: 0 }}>{b.body}</p>
            </Card>
          ))}
        </div>

        {/* how it works */}
        <h2 style={{ fontSize: 22, fontWeight: 800, margin: "44px 0 14px" }}>How it works</h2>
        <div style={{ display: "grid", gap: 14 }}>
          {STAGES.map((s) => (
            <Card key={s.n} style={{ display: "flex", gap: 18, alignItems: "flex-start" }}>
              <div
                style={{
                  flexShrink: 0, width: 38, height: 38, borderRadius: 10, background: c.panel2,
                  border: `1px solid ${c.border}`, color: c.cyan, fontWeight: 800, fontSize: 17,
                  display: "flex", alignItems: "center", justifyContent: "center",
                }}
              >
                {s.n}
              </div>
              <div>
                <h3 style={{ fontSize: 16, fontWeight: 700, margin: "4px 0 6px" }}>{s.title}</h3>
                <p style={{ color: c.muted, fontSize: 14.5, lineHeight: 1.6, margin: 0 }}>{s.body}</p>
                {s.href && (
                  <Link href={s.href} style={{ color: c.cyan, fontSize: 13.5, textDecoration: "none", display: "inline-block", marginTop: 8 }}>
                    {s.hrefLabel} →
                  </Link>
                )}
              </div>
            </Card>
          ))}
        </div>

        {/* what we ask + honesty */}
        <Card style={{ marginTop: 28, borderColor: c.amber }}>
          <h2 style={{ fontSize: 17, fontWeight: 700, margin: "0 0 8px", color: c.amber }}>
            What we ask — and what we won&apos;t do
          </h2>
          <p style={{ color: c.muted, fontSize: 14.5, lineHeight: 1.6, margin: 0 }}>
            We ask for one real (sanitized) workload, your honest bar, and a good-faith, conditional
            intent to proceed to a pilot <em>if</em> the live run meets that bar. That&apos;s it — no
            binding commitment until terms are executed, and nothing proprietary changes hands without
            an NDA and scope.
          </p>
          <p style={{ color: c.faint, fontSize: 13.5, lineHeight: 1.6, margin: "12px 0 0" }}>
            What we won&apos;t do: quote a number we haven&apos;t measured, claim order-independence
            for an operator that isn&apos;t abelian, or promise an end-to-end speedup we can&apos;t
            back. Every figure you see is measured on the AX7020 or formally proven; the value math is
            arithmetic on your own baseline. We&apos;d rather tell you &ldquo;no fit&rdquo; than oversell.
          </p>
        </Card>

        {/* CTA */}
        <div style={{ marginTop: 30, display: "flex", gap: 12, flexWrap: "wrap", alignItems: "center" }}>
          <Link
            href="/contact?intent=design-partner&source=design-partners&cta=apply-design-partner"
            style={{ background: c.green, color: c.bg, fontWeight: 700, fontSize: 15, padding: "12px 22px", borderRadius: 10, textDecoration: "none" }}
          >
            Apply to the program →
          </Link>
          <Link
            href="/benchmark-proof"
            style={{ color: c.text, fontWeight: 600, fontSize: 15, padding: "12px 22px", borderRadius: 10, border: `1px solid ${c.border}`, textDecoration: "none" }}
          >
            Test your bar first
          </Link>
        </div>
        <p style={{ color: c.faint, fontSize: 13, lineHeight: 1.6, marginTop: 18, maxWidth: 760 }}>
          Representative archetypes, measured numbers, conditional commitments. The strongest
          application names one workload, one current baseline, one painful constraint, and the bar
          the evidence has to clear.
        </p>
      </section>
      <Footer />
    </main>
  );
}
