"use client";

import type { CSSProperties } from "react";
import { eng, evaluate, renderReportMarkdown, SILICON } from "@/lib/benchmark-proof";
import type { Scenario } from "@/lib/workload-scenarios";

const c = {
  bg: "#070807",
  panel: "#10130f",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  border: "#2d3a34",
  cyan: "#22d3ee",
  green: "#2dd36f",
  rose: "#fb7185",
};

export default function ScenarioProof({ scenario }: { scenario: Scenario }) {
  const r = evaluate(scenario.aggregation, scenario.requiredThroughput, scenario.requiresReorderStability);
  const vc = r.verdict === "SURPASS" ? c.green : r.verdict === "PASS" ? c.cyan : c.rose;

  function downloadReport() {
    const md = renderReportMarkdown({
      workload: scenario.name,
      customer: scenario.vertical,
      aggregation: scenario.aggregation,
      requiredThroughput: scenario.requiredThroughput,
      requiresReorder: scenario.requiresReorderStability,
      result: r,
    });
    const blob = new Blob([md], { type: "text/markdown" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `atomik-benchmark-proof-${scenario.slug}.md`;
    a.click();
    URL.revokeObjectURL(url);
  }

  const card: CSSProperties = {
    background: c.panel,
    border: `1px solid ${vc}`,
    borderRadius: 14,
    padding: 22,
  };

  return (
    <div style={card}>
      <div style={{ display: "flex", alignItems: "baseline", gap: 14, flexWrap: "wrap" }}>
        <span style={{ color: c.bg, background: vc, fontWeight: 800, fontSize: 17, padding: "4px 14px", borderRadius: 8, letterSpacing: 0.5 }}>
          {r.verdict}
        </span>
        <span style={{ color: c.text, fontSize: 15 }}>{r.headline}</span>
      </div>

      <div style={{ display: "grid", gap: 14, marginTop: 18 }}>
        <Gate
          title="Throughput"
          pass={r.throughputPass}
          line={`${eng(scenario.requiredThroughput)} required → ${eng(r.peakThroughput)} measured on silicon (${eng(r.perBankThroughput)}/bank)`}
          detail={
            r.throughputPass
              ? `${r.margin.toFixed(1)}× headroom — needs ${r.minBanks} of ${r.banksAvailable} banks`
              : `short by ${(1 / r.margin).toFixed(1)}× at ${r.banksAvailable} banks`
          }
        />
        <Gate
          title="Correctness under concurrency"
          pass={r.correctnessPass}
          line={
            r.correctnessPass
              ? `byte-identical across 1/2/4/8 banks (${SILICON.resultHex})`
              : "not order-independent for this operator"
          }
          detail={
            r.correctnessPass
              ? `'${scenario.aggregation}' is abelian → provably order-independent, lock-free`
              : "ATOMiK won't claim reorder-stable correctness here"
          }
        />
      </div>

      <p style={{ color: c.faint, fontSize: 12.5, marginTop: 16, lineHeight: 1.5 }}>
        Measured {SILICON.measuredOn} on {SILICON.board}; reproduce with{" "}
        <code style={{ color: c.muted }}>{SILICON.reproduce}</code>. Throughput is the engine&apos;s
        accumulation rate (the aggregation step); end-to-end depends on your ingest path, which
        ATOMiK&apos;s order-independence frees from global ordering / locks.
      </p>

      <div style={{ display: "flex", gap: 12, marginTop: 18, flexWrap: "wrap" }}>
        <a
          href={`/contact?topic=live-benchmark&workload=${scenario.slug}`}
          style={{ background: vc, color: c.bg, fontWeight: 700, fontSize: 14, padding: "11px 18px", borderRadius: 8, textDecoration: "none" }}
        >
          Run this on our silicon → sign conditional on your bar
        </a>
        <button
          onClick={downloadReport}
          style={{ background: "transparent", color: c.text, fontWeight: 600, fontSize: 14, padding: "11px 18px", borderRadius: 8, border: `1px solid ${c.border}`, cursor: "pointer" }}
        >
          Download the proof report
        </button>
      </div>
    </div>
  );
}

function Gate({ title, pass, line, detail }: { title: string; pass: boolean; line: string; detail: string }) {
  return (
    <div style={{ borderLeft: `3px solid ${pass ? c.green : c.rose}`, paddingLeft: 14 }}>
      <div style={{ display: "flex", gap: 10, alignItems: "center" }}>
        <span style={{ color: c.text, fontWeight: 600, fontSize: 14 }}>{title}</span>
        <span style={{ color: pass ? c.green : c.rose, fontWeight: 700, fontSize: 12, letterSpacing: 0.5 }}>
          {pass ? "PASS" : "FAIL"}
        </span>
      </div>
      <div style={{ color: c.muted, fontSize: 13, marginTop: 4 }}>{line}</div>
      <div style={{ color: c.faint, fontSize: 12.5, marginTop: 2 }}>{detail}</div>
    </div>
  );
}
