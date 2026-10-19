"use client";

import type { CSSProperties } from "react";
import {
  eng,
  evaluate,
  loiHref,
  referenceRunEvidence,
  renderConditionalLOI,
  renderReportMarkdown,
  renderSyncReportMarkdown,
  SILICON,
} from "@/lib/benchmark-proof";
import { syncReductionPct, type Scenario } from "@/lib/workload-scenarios";

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

function download(md: string, name: string) {
  const blob = new Blob([md], { type: "text/markdown" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = name;
  a.click();
  URL.revokeObjectURL(url);
}

export default function ScenarioProof({ scenario }: { scenario: Scenario }) {
  return (scenario.kind ?? "aggregation") === "sync" ? (
    <SyncProof scenario={scenario} />
  ) : (
    <AggregationProof scenario={scenario} />
  );
}

function shell(vc: string): CSSProperties {
  return { background: c.panel, border: `1px solid ${vc}`, borderRadius: 14, padding: 22 };
}

function CTA({
  href,
  vc,
  onDownload,
  onDownloadLOI,
}: {
  href: string;
  vc: string;
  onDownload: () => void;
  onDownloadLOI: () => void;
}) {
  return (
    <div style={{ display: "flex", gap: 12, marginTop: 18, flexWrap: "wrap" }}>
      <a
        href={href}
        style={{ background: vc, color: c.bg, fontWeight: 700, fontSize: 14, padding: "11px 18px", borderRadius: 8, textDecoration: "none" }}
      >
        Run this on our silicon → sign conditional on your bar
      </a>
      <button
        onClick={onDownloadLOI}
        style={{ background: "transparent", color: c.text, fontWeight: 600, fontSize: 14, padding: "11px 18px", borderRadius: 8, border: `1px solid ${vc}`, cursor: "pointer" }}
      >
        Download conditional LOI (draft)
      </button>
      <button
        onClick={onDownload}
        style={{ background: "transparent", color: c.muted, fontWeight: 600, fontSize: 14, padding: "11px 18px", borderRadius: 8, border: `1px solid ${c.border}`, cursor: "pointer" }}
      >
        Download the proof report
      </button>
    </div>
  );
}

function AggregationProof({ scenario }: { scenario: Scenario }) {
  const r = evaluate(scenario.aggregation!, scenario.requiredThroughput!, scenario.requiresReorderStability ?? true);
  const vc = r.verdict === "SURPASS" ? c.green : r.verdict === "PASS" ? c.cyan : c.rose;
  const onDownload = () =>
    download(
      renderReportMarkdown({
        workload: scenario.name,
        customer: scenario.vertical,
        aggregation: scenario.aggregation!,
        requiredThroughput: scenario.requiredThroughput!,
        requiresReorder: scenario.requiresReorderStability ?? true,
        result: r,
      }),
      `atomik-benchmark-proof-${scenario.slug}.md`,
    );
  return (
    <div style={shell(vc)}>
      <Badge color={vc} label={r.verdict} text={r.headline} />
      <div style={{ display: "grid", gap: 14, marginTop: 18 }}>
        <Gate
          title="Throughput"
          pass={r.throughputPass}
          line={`${eng(scenario.requiredThroughput!)} required → ${eng(r.peakThroughput)} measured on silicon (${eng(r.perBankThroughput)}/bank)`}
          detail={r.throughputPass ? `${r.margin.toFixed(1)}× headroom — needs ${r.minBanks} of ${r.banksAvailable} banks` : `short by ${(1 / r.margin).toFixed(1)}× at ${r.banksAvailable} banks`}
        />
        <Gate
          title="Correctness under concurrency"
          pass={r.correctnessPass}
          line={r.correctnessPass ? `byte-identical across 1/2/4/8 banks (${SILICON.resultHex})` : "not order-independent for this operator"}
          detail={r.correctnessPass ? `'${scenario.aggregation}' is abelian → provably order-independent, lock-free` : "ATOMiK won't claim reorder-stable correctness here"}
        />
      </div>
      <Provenance>
        Measured {SILICON.measuredOn} on {SILICON.board}; reproduce with{" "}
        <code style={{ color: c.muted }}>{SILICON.reproduce}</code>. Throughput is the engine&apos;s
        accumulation rate (the aggregation step); end-to-end depends on your ingest path, which
        ATOMiK&apos;s order-independence frees from global ordering / locks.
      </Provenance>
      <CTA
        href={loiHref({
          kind: "aggregation",
          workload: scenario.name,
          agg: scenario.aggregation!,
          rate: scenario.requiredThroughput!,
          reorder: scenario.requiresReorderStability ?? true,
        })}
        vc={vc}
        onDownload={onDownload}
        onDownloadLOI={() =>
          download(
            renderConditionalLOI({
              kind: "aggregation",
              workload: scenario.name,
              aggregation: scenario.aggregation!,
              requiredThroughput: scenario.requiredThroughput!,
              requiresReorder: scenario.requiresReorderStability ?? true,
              liveRun: referenceRunEvidence(),
            }),
            `atomik-conditional-loi-${scenario.slug}.md`,
          )
        }
      />
    </div>
  );
}

function SyncProof({ scenario }: { scenario: Scenario }) {
  const s = scenario.sync!;
  const pct = syncReductionPct(s);
  const vc = c.green;
  const onDownload = () =>
    download(
      renderSyncReportMarkdown({
        workload: scenario.name,
        vertical: scenario.vertical,
        fullStateBytes: s.fullStateBytes,
        deltaBytes: s.deltaBytes,
        changeDensity: s.changeDensity,
        measuredNote: s.measuredNote,
      }),
      `atomik-sync-proof-${scenario.slug}.md`,
    );
  return (
    <div style={shell(vc)}>
      <Badge color={vc} label="CONVERGES" text={`coordination-free convergence + ~${pct}% less data moved`} />
      <div style={{ display: "grid", gap: 14, marginTop: 18 }}>
        <Gate
          title="Convergence (correctness)"
          pass
          line={`replicas reach identical state with no coordinator — byte-identical across 1/2/4/8 banks (${SILICON.resultHex})`}
          detail="any node, any order, after any partition → identical merged state. The same property, on silicon."
        />
        <Gate
          title="Data moved (bandwidth)"
          pass
          line={`${s.fullStateBytes} B/tick full-state → ${s.deltaBytes} B/tick delta (measured) = ~${pct}% less`}
          detail={`at ${s.changeDensity} — scales with change density, not a universal multiplier`}
        />
      </div>
      <Provenance>
        {s.measuredNote} Convergence is the same order-independence proven on the AX7020
        ({SILICON.measuredOn}); reproduce with <code style={{ color: c.muted }}>{SILICON.reproduce}</code>.
      </Provenance>
      <CTA
        href={loiHref({
          kind: "sync",
          workload: scenario.name,
          fullStateBytes: s.fullStateBytes,
          deltaBytes: s.deltaBytes,
        })}
        vc={vc}
        onDownload={onDownload}
        onDownloadLOI={() =>
          download(
            renderConditionalLOI({
              kind: "sync",
              workload: scenario.name,
              fullStateBytes: s.fullStateBytes,
              deltaBytes: s.deltaBytes,
              changeDensity: s.changeDensity,
              liveRun: referenceRunEvidence(),
            }),
            `atomik-conditional-loi-${scenario.slug}.md`,
          )
        }
      />
    </div>
  );
}

function Badge({ color, label, text }: { color: string; label: string; text: string }) {
  return (
    <div style={{ display: "flex", alignItems: "baseline", gap: 14, flexWrap: "wrap" }}>
      <span style={{ color: c.bg, background: color, fontWeight: 800, fontSize: 17, padding: "4px 14px", borderRadius: 8, letterSpacing: 0.5 }}>
        {label}
      </span>
      <span style={{ color: c.text, fontSize: 15 }}>{text}</span>
    </div>
  );
}

function Provenance({ children }: { children: React.ReactNode }) {
  return <p style={{ color: c.faint, fontSize: 12.5, marginTop: 16, lineHeight: 1.5 }}>{children}</p>;
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
