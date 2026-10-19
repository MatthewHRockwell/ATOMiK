"use client";

import { useMemo, useState } from "react";
import type { CSSProperties } from "react";
import {
  ABELIAN,
  NON_ABELIAN,
  SILICON,
  eng,
  evaluate,
  loiHref,
  referenceRunEvidence,
  renderConditionalLOI,
} from "@/lib/benchmark-proof";

function downloadMarkdown(md: string, name: string) {
  const blob = new Blob([md], { type: "text/markdown" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = name;
  a.click();
  URL.revokeObjectURL(url);
}

const c = {
  bg: "#070807",
  panel: "#10130f",
  panel2: "#17130f",
  border: "#2d3a34",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  cyan: "#22d3ee",
  green: "#2dd36f",
  amber: "#f5a524",
  rose: "#fb7185",
};

const UNIT: Record<string, number> = { "K/s": 1e3, "M/s": 1e6, "G/s": 1e9 };

const card: CSSProperties = {
  background: c.panel,
  border: `1px solid ${c.border}`,
  borderRadius: 12,
  padding: 20,
};

export default function BenchmarkProver() {
  const [agg, setAgg] = useState("sum");
  const [value, setValue] = useState(50);
  const [unit, setUnit] = useState("M/s");
  const [reorder, setReorder] = useState(true);

  const required = value * (UNIT[unit] ?? 1e6);
  const r = useMemo(() => evaluate(agg, required, reorder), [agg, required, reorder]);

  const verdictColor =
    r.verdict === "SURPASS" ? c.green : r.verdict === "PASS" ? c.cyan : c.rose;
  const aggDesc = ABELIAN[agg] ?? NON_ABELIAN[agg] ?? "";

  const labelStyle: CSSProperties = {
    color: c.faint,
    fontSize: 12,
    letterSpacing: 0.4,
    textTransform: "uppercase",
    marginBottom: 6,
  };
  const inputStyle: CSSProperties = {
    background: c.bg,
    border: `1px solid ${c.border}`,
    borderRadius: 8,
    color: c.text,
    padding: "10px 12px",
    fontSize: 15,
    width: "100%",
  };

  return (
    <div style={{ display: "grid", gap: 20, gridTemplateColumns: "1fr", maxWidth: 880 }}>
      {/* ---- inputs ---- */}
      <div style={{ ...card, display: "grid", gap: 16 }}>
        <div>
          <div style={labelStyle}>Your aggregation</div>
          <select style={inputStyle} value={agg} onChange={(e) => setAgg(e.target.value)}>
            <optgroup label="Order-independent (ATOMiK-native)">
              {Object.keys(ABELIAN).map((k) => (
                <option key={k} value={k}>
                  {k} — {ABELIAN[k]}
                </option>
              ))}
            </optgroup>
            <optgroup label="Order-dependent (honest no)">
              {Object.keys(NON_ABELIAN).map((k) => (
                <option key={k} value={k}>
                  {k} — {NON_ABELIAN[k]}
                </option>
              ))}
            </optgroup>
          </select>
        </div>

        <div style={{ display: "grid", gridTemplateColumns: "2fr 1fr", gap: 12 }}>
          <div>
            <div style={labelStyle}>Required throughput to commit</div>
            <input
              style={inputStyle}
              type="number"
              min={0}
              value={value}
              onChange={(e) => setValue(Math.max(0, Number(e.target.value)))}
            />
          </div>
          <div>
            <div style={labelStyle}>Unit</div>
            <select style={inputStyle} value={unit} onChange={(e) => setUnit(e.target.value)}>
              {Object.keys(UNIT).map((u) => (
                <option key={u} value={u}>
                  {u}
                </option>
              ))}
            </select>
          </div>
        </div>

        <label style={{ display: "flex", gap: 10, alignItems: "center", color: c.muted, fontSize: 14 }}>
          <input type="checkbox" checked={reorder} onChange={(e) => setReorder(e.target.checked)} />
          Result must be exactly correct regardless of arrival order / parallelism
          <span style={{ color: c.faint }}>(why you&apos;d otherwise need a lock)</span>
        </label>
      </div>

      {/* ---- verdict ---- */}
      <div style={{ ...card, borderColor: verdictColor }}>
        <div style={{ display: "flex", alignItems: "baseline", gap: 14, flexWrap: "wrap" }}>
          <span
            style={{
              color: c.bg,
              background: verdictColor,
              fontWeight: 800,
              fontSize: 18,
              padding: "4px 14px",
              borderRadius: 8,
              letterSpacing: 0.5,
            }}
          >
            {r.verdict}
          </span>
          <span style={{ color: c.text, fontSize: 15 }}>{r.headline}</span>
        </div>

        <div style={{ display: "grid", gap: 14, marginTop: 18 }}>
          <Gate
            title="Throughput"
            pass={r.throughputPass}
            required={`${eng(required)} required`}
            measured={`${eng(r.peakThroughput)} measured on silicon · ${eng(r.perBankThroughput)}/bank`}
            detail={
              r.throughputPass
                ? `${r.margin.toFixed(1)}× headroom — needs ${r.minBanks} of ${r.banksAvailable} banks`
                : `short by ${(1 / r.margin).toFixed(1)}× at ${r.banksAvailable} banks`
            }
          />
          <Gate
            title="Correctness under concurrency"
            pass={r.correctnessPass}
            required={reorder ? "byte-identical regardless of order" : "exact result"}
            measured={
              r.correctnessPass
                ? `byte-identical across 1/2/4/8 banks (${SILICON.resultHex})`
                : "not order-independent for this operator"
            }
            detail={
              r.correctnessPass
                ? r.abelian
                  ? `'${agg}' (${aggDesc}) is abelian → provably order-independent, lock-free`
                  : "delivered even though not required"
                : `'${agg}' is order-dependent — ATOMiK won't claim reorder-stable correctness here`
            }
          />
        </div>

        <p style={{ color: c.faint, fontSize: 12.5, marginTop: 16, lineHeight: 1.5 }}>
          Scope: throughput is the engine&apos;s measured accumulation rate (the
          aggregation step). End-to-end depends on your ingest path — but
          ATOMiK&apos;s order-independence is exactly what removes the global
          ordering / locks that cap a conventional aggregator. Measured{" "}
          {SILICON.measuredOn} on {SILICON.board}; reproduce with{" "}
          <code style={{ color: c.muted }}>{SILICON.reproduce}</code>.
        </p>

        <div style={{ display: "flex", gap: 12, marginTop: 16, flexWrap: "wrap" }}>
          <a
            href={loiHref({ kind: "aggregation", agg, rate: required, reorder })}
            style={{
              display: "inline-block",
              background: verdictColor,
              color: c.bg,
              fontWeight: 700,
              fontSize: 14,
              padding: "10px 18px",
              borderRadius: 8,
              textDecoration: "none",
            }}
          >
            Request a live on-silicon run + conditional LOI on this bar →
          </a>
          {r.verdict !== "FAIL" && (
            <button
              onClick={() =>
                downloadMarkdown(
                  renderConditionalLOI({
                    kind: "aggregation",
                    workload: "My aggregation workload",
                    aggregation: agg,
                    requiredThroughput: required,
                    requiresReorder: reorder,
                    liveRun: referenceRunEvidence(),
                  }),
                  "atomik-conditional-loi-draft.md",
                )
              }
              style={{
                background: "transparent",
                color: c.text,
                fontWeight: 600,
                fontSize: 14,
                padding: "10px 18px",
                borderRadius: 8,
                border: `1px solid ${c.border}`,
                cursor: "pointer",
              }}
            >
              Download conditional LOI (draft)
            </button>
          )}
        </div>
        <p style={{ color: c.faint, fontSize: 12, marginTop: 8, lineHeight: 1.5 }}>
          Carries this exact spec — {agg} at {eng(required)}
          {reorder ? ", reorder-stable" : ""} — into the request, with the verdict
          re-computed from the measured engine on the next page.
        </p>
        {r.verdict !== "FAIL" && (
          <p style={{ marginTop: 10, fontSize: 13.5 }}>
            <a
              href={`/cost-of-lock?agg=${encodeURIComponent(agg)}&rate=${Math.round(required)}&reorder=${reorder ? 1 : 0}`}
              style={{ color: c.cyan, textDecoration: "none" }}
            >
              What is hitting this bar worth? → quantify the coordination tax it removes
            </a>
          </p>
        )}
      </div>
    </div>
  );
}

function Gate({
  title,
  pass,
  required,
  measured,
  detail,
}: {
  title: string;
  pass: boolean;
  required: string;
  measured: string;
  detail: string;
}) {
  return (
    <div
      style={{
        borderLeft: `3px solid ${pass ? c.green : c.rose}`,
        paddingLeft: 14,
      }}
    >
      <div style={{ display: "flex", gap: 10, alignItems: "center" }}>
        <span style={{ color: c.text, fontWeight: 600, fontSize: 14 }}>{title}</span>
        <span
          style={{
            color: pass ? c.green : c.rose,
            fontWeight: 700,
            fontSize: 12,
            letterSpacing: 0.5,
          }}
        >
          {pass ? "PASS" : "FAIL"}
        </span>
      </div>
      <div style={{ color: c.muted, fontSize: 13, marginTop: 4 }}>
        {required} → {measured}
      </div>
      <div style={{ color: c.faint, fontSize: 12.5, marginTop: 2 }}>{detail}</div>
    </div>
  );
}
