import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import Footer from "@/components/Footer";
import { SCENARIOS, syncReductionPct } from "@/lib/workload-scenarios";
import { evaluate, eng } from "@/lib/benchmark-proof";

export const metadata: Metadata = {
  title: "Workload Catalog — Aggregation Workloads Proven on Silicon",
  description:
    "Find your aggregation workload. Each archetype shows the silicon-measured throughput-at-correctness verdict, the lock-elimination value, and a path to run yours live on the AX7020.",
  openGraph: {
    title: "ATOMiK Workload Catalog",
    description:
      "Order-independent, lock-free, byte-identical aggregation — proven on silicon, per workload.",
    url: "https://atomik.tech/workloads",
    type: "website",
  },
};

const c = {
  bg: "#070807",
  panel: "#10130f",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  border: "#2d3a34",
  cyan: "#22d3ee",
  green: "#2dd36f",
};

export default function WorkloadsPage() {
  return (
    <main style={{ background: c.bg, color: c.text, minHeight: "100vh" }}>
      <Nav active="Workloads" />
      <section style={{ maxWidth: 1040, margin: "0 auto", padding: "56px 22px 28px" }}>
        <p style={{ color: c.cyan, fontSize: 13, fontWeight: 600, textTransform: "uppercase", letterSpacing: 0.5 }}>
          Workload Catalog
        </p>
        <h1 style={{ fontSize: 38, lineHeight: 1.1, margin: "10px 0 14px", fontWeight: 800 }}>
          Find your workload. See it proven.
        </h1>
        <p style={{ color: c.muted, fontSize: 17, lineHeight: 1.55, maxWidth: 760 }}>
          Aggregation and reduction workloads that gate on{" "}
          <strong style={{ color: c.text }}>throughput at correctness</strong> — hitting your rate
          while the result stays exactly correct no matter the arrival order or how many cores feed
          it. Each archetype below is checked against the silicon-measured parallel-bank engine. Pick
          the one that&apos;s yours.
        </p>
        <p style={{ color: c.faint, fontSize: 12.5, marginTop: 10 }}>
          Representative archetypes — not named customers. Verdicts use measured engine facts (AX7020,
          100 MHz, byte-identical across 1/2/4/8 banks).
        </p>
        <p style={{ color: c.muted, fontSize: 14, marginTop: 14 }}>
          Recognize yours?{" "}
          <Link href="/design-partners" style={{ color: c.cyan }}>
            Become a founding design partner →
          </Link>{" "}
          — we run it live on silicon behind a conditional LOI you only honor if we hit your bar.
        </p>
      </section>

      <section style={{ maxWidth: 1040, margin: "0 auto", padding: "0 22px 64px", display: "grid", gap: 18 }}>
        {SCENARIOS.map((s) => {
          const isSync = (s.kind ?? "aggregation") === "sync";
          const r = isSync
            ? null
            : evaluate(s.aggregation!, s.requiredThroughput!, s.requiresReorderStability ?? true);
          const badge = isSync
            ? `CONVERGES · ${syncReductionPct(s.sync!)}% less data`
            : `${r!.verdict} · ${r!.margin.toFixed(0)}× headroom`;
          const subline = isSync
            ? `${s.sync!.deltaBytes} B vs ${s.sync!.fullStateBytes} B/tick · converges with no coordinator`
            : `${eng(s.requiredThroughput!)} required → ${eng(r!.peakThroughput)} measured, byte-identical`;
          const vc = isSync || r!.verdict === "SURPASS" ? c.green : r!.verdict === "PASS" ? c.cyan : "#fb7185";
          return (
            <Link
              key={s.slug}
              href={`/workloads/${s.slug}`}
              className="block no-underline"
              style={{ border: `1px solid ${c.border}`, borderRadius: 14, padding: 22, background: c.panel }}
            >
              <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline", gap: 16, flexWrap: "wrap" }}>
                <div>
                  <span style={{ color: c.faint, fontSize: 12, textTransform: "uppercase", letterSpacing: 0.5 }}>
                    {s.vertical}
                  </span>
                  <h2 style={{ color: c.text, fontSize: 22, fontWeight: 700, margin: "4px 0 0" }}>{s.name}</h2>
                </div>
                <span
                  style={{
                    color: c.bg, background: vc, fontWeight: 800, fontSize: 13,
                    padding: "4px 12px", borderRadius: 8, whiteSpace: "nowrap",
                  }}
                >
                  {badge}
                </span>
              </div>
              <p style={{ color: c.muted, fontSize: 14.5, lineHeight: 1.5, margin: "10px 0 0", maxWidth: 720 }}>
                {s.tagline}
              </p>
              <p style={{ color: c.faint, fontSize: 13, margin: "10px 0 0" }}>
                {subline} · <span style={{ color: c.cyan }}>see the proof →</span>
              </p>
            </Link>
          );
        })}
      </section>
      <Footer />
    </main>
  );
}
