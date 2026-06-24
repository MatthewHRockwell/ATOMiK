import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import Nav from "@/components/Nav";
import Footer from "@/components/Footer";
import { SCENARIOS, getScenario } from "@/lib/workload-scenarios";
import ScenarioProof from "./ScenarioProof";

const c = {
  bg: "#070807",
  panel: "#10130f",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  border: "#2d3a34",
  cyan: "#22d3ee",
  green: "#2dd36f",
  amber: "#f5a524",
};

export function generateStaticParams() {
  return SCENARIOS.map((s) => ({ slug: s.slug }));
}

export async function generateMetadata({
  params,
}: {
  params: Promise<{ slug: string }>;
}): Promise<Metadata> {
  const { slug } = await params;
  const s = getScenario(slug);
  if (!s) return { title: "Workload — ATOMiK" };
  return {
    title: `${s.name} — ATOMiK Workload Proof`,
    description: s.tagline,
    openGraph: { title: `ATOMiK · ${s.name}`, description: s.tagline, type: "article" },
  };
}

function Block({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <div>
      <p style={{ color: c.cyan, fontSize: 12, fontWeight: 600, textTransform: "uppercase", letterSpacing: 0.5, margin: "0 0 8px" }}>
        {label}
      </p>
      {children}
    </div>
  );
}

function Bullets({ items }: { items: string[] }) {
  return (
    <ul style={{ margin: 0, paddingLeft: 18, color: c.muted, fontSize: 14.5, lineHeight: 1.7 }}>
      {items.map((it, i) => (
        <li key={i}>{it}</li>
      ))}
    </ul>
  );
}

export default async function ScenarioPage({ params }: { params: Promise<{ slug: string }> }) {
  const { slug } = await params;
  const s = getScenario(slug);
  if (!s) notFound();

  return (
    <main style={{ background: c.bg, color: c.text, minHeight: "100vh" }}>
      <Nav active="Workloads" />
      <section style={{ maxWidth: 960, margin: "0 auto", padding: "48px 22px 80px" }}>
        <Link href="/workloads" style={{ color: c.faint, fontSize: 13, textDecoration: "none" }}>
          ← Workload Catalog
        </Link>
        <p style={{ color: c.faint, fontSize: 12, textTransform: "uppercase", letterSpacing: 0.5, margin: "18px 0 4px" }}>
          {s.vertical}
        </p>
        <h1 style={{ fontSize: 34, lineHeight: 1.12, margin: "0 0 12px", fontWeight: 800 }}>{s.name}</h1>
        <p style={{ color: c.muted, fontSize: 17, lineHeight: 1.5, maxWidth: 740 }}>{s.tagline}</p>

        {/* the silicon-backed verdict + report download + LOI CTA */}
        <div style={{ margin: "26px 0 8px" }}>
          <ScenarioProof scenario={s} />
        </div>

        {/* the LOI argument */}
        <div style={{ display: "grid", gap: 26, marginTop: 34, gridTemplateColumns: "1fr" }}>
          <Block label="This is you if">
            <Bullets items={s.whoThisIs} />
          </Block>
          <Block label="The workload">
            <Bullets items={s.pattern} />
          </Block>

          <div
            style={{
              display: "grid",
              gap: 18,
              gridTemplateColumns: "1fr",
              border: `1px solid ${c.border}`,
              borderRadius: 14,
              padding: 22,
              background: c.panel,
            }}
          >
            <div>
              <p style={{ color: c.amber, fontSize: 12, fontWeight: 600, textTransform: "uppercase", letterSpacing: 0.5, margin: "0 0 8px" }}>
                Today — serialized behind a lock
              </p>
              <p style={{ color: c.muted, fontSize: 14.5, lineHeight: 1.6, margin: 0 }}>{s.todayPain}</p>
            </div>
            <div style={{ borderTop: `1px solid ${c.border}`, paddingTop: 18 }}>
              <p style={{ color: c.green, fontSize: 12, fontWeight: 600, textTransform: "uppercase", letterSpacing: 0.5, margin: "0 0 8px" }}>
                With ATOMiK — order-independent, lock-free
              </p>
              <p style={{ color: c.text, fontSize: 14.5, lineHeight: 1.6, margin: "0 0 12px" }}>{s.withAtomik}</p>
              <Bullets items={s.valuePoints} />
            </div>
          </div>
        </div>

        <div style={{ marginTop: 30, paddingTop: 22, borderTop: `1px solid ${c.border}`, color: c.faint, fontSize: 13, lineHeight: 1.6 }}>
          Want to test a different rate or operator?{" "}
          <Link href="/benchmark-proof" style={{ color: c.cyan }}>
            Open the interactive benchmark tool
          </Link>{" "}
          · Representative archetype, not a named customer. Numbers are measured engine facts; the
          value is lock-elimination + the byte-identical guarantee.
        </div>
      </section>
      <Footer />
    </main>
  );
}
