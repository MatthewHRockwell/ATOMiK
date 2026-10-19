"use client";

import { useEffect, useMemo, useState } from "react";
import type { CSSProperties } from "react";
import { ABELIAN, NON_ABELIAN } from "@/lib/benchmark-proof";
import {
  APPROACH_LABEL,
  computeValue,
  renderValueMemoMarkdown,
  type Approach,
  type ValueInputs,
} from "@/lib/cost-of-lock";

const c = {
  bg: "#070807",
  panel: "#10130f",
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
const label: CSSProperties = {
  color: c.faint,
  fontSize: 12,
  letterSpacing: 0.4,
  textTransform: "uppercase",
  marginBottom: 6,
};
const input: CSSProperties = {
  background: c.bg,
  border: `1px solid ${c.border}`,
  borderRadius: 8,
  color: c.text,
  padding: "10px 12px",
  fontSize: 15,
  width: "100%",
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

export default function CostOfLock() {
  const [agg, setAgg] = useState("sum");
  const [value, setValue] = useState(100);
  const [unit, setUnit] = useState("M/s");
  const [approach, setApproach] = useState<Approach>("shard");
  const [instances, setInstances] = useState("");
  const [instanceMonthly, setInstanceMonthly] = useState("");
  const [overhead, setOverhead] = useState("");
  const [p99, setP99] = useState("");
  const [p99Lock, setP99Lock] = useState(true);
  const [company, setCompany] = useState("");

  // preload operator / rate carried from the benchmark tool or catalog
  useEffect(() => {
    const p = new URLSearchParams(window.location.search);
    const a = p.get("agg") || p.get("bm_agg");
    const rate = p.get("rate") || p.get("bm_rate");
    if (!a && !rate) return;
    const t = window.setTimeout(() => {
      if (a) setAgg(a);
      if (rate && isFinite(Number(rate))) {
        setValue(Number(rate) / 1e6);
        setUnit("M/s");
      }
    }, 0);
    return () => window.clearTimeout(t);
  }, []);

  const required = value * (UNIT[unit] ?? 1e6);
  const num = (s: string) => (s.trim() === "" ? undefined : Math.max(0, Number(s)));

  const inputs: ValueInputs = useMemo(
    () => ({
      aggregation: agg,
      requiredThroughput: required,
      approach,
      instances: num(instances),
      instanceMonthly: num(instanceMonthly),
      coordinationOverheadPct: num(overhead),
      p99Ms: num(p99),
      p99LockInduced: p99Lock,
    }),
    [agg, required, approach, instances, instanceMonthly, overhead, p99, p99Lock],
  );

  const memo = useMemo(() => computeValue(inputs), [inputs]);
  const vc = memo.applicable ? c.green : c.amber;

  return (
    <div style={{ display: "grid", gap: 20, maxWidth: 900 }}>
      <div style={{ ...card, display: "grid", gap: 16 }}>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12 }}>
          <div>
            <div style={label}>Your aggregation</div>
            <select style={input} value={agg} onChange={(e) => setAgg(e.target.value)}>
              <optgroup label="Order-independent (ATOMiK-native)">
                {Object.keys(ABELIAN).map((k) => (
                  <option key={k} value={k}>{k}</option>
                ))}
              </optgroup>
              <optgroup label="Order-dependent (honest no)">
                {Object.keys(NON_ABELIAN).map((k) => (
                  <option key={k} value={k}>{k}</option>
                ))}
              </optgroup>
            </select>
          </div>
          <div>
            <div style={label}>How you keep it correct today</div>
            <select style={input} value={approach} onChange={(e) => setApproach(e.target.value as Approach)}>
              {(Object.keys(APPROACH_LABEL) as Approach[]).map((k) => (
                <option key={k} value={k}>{APPROACH_LABEL[k]}</option>
              ))}
            </select>
          </div>
        </div>

        <div style={{ display: "grid", gridTemplateColumns: "2fr 1fr", gap: 12 }}>
          <div>
            <div style={label}>Required throughput</div>
            <input style={input} type="number" min={0} value={value} onChange={(e) => setValue(Math.max(0, Number(e.target.value)))} />
          </div>
          <div>
            <div style={label}>Unit</div>
            <select style={input} value={unit} onChange={(e) => setUnit(e.target.value)}>
              {Object.keys(UNIT).map((u) => (<option key={u} value={u}>{u}</option>))}
            </select>
          </div>
        </div>

        <div style={{ borderTop: `1px solid ${c.border}`, paddingTop: 14 }}>
          <div style={{ ...label, color: c.amber }}>Your baseline today (optional — drives the $ / latency)</div>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 12 }}>
            <div>
              <div style={label}>Instances / shards</div>
              <input style={input} type="number" min={0} value={instances} placeholder="e.g. 12" onChange={(e) => setInstances(e.target.value)} />
            </div>
            <div>
              <div style={label}>$ / instance / mo</div>
              <input style={input} type="number" min={0} value={instanceMonthly} placeholder="e.g. 900" onChange={(e) => setInstanceMonthly(e.target.value)} />
            </div>
            <div>
              <div style={label}>% on coordination</div>
              <input style={input} type="number" min={0} max={100} value={overhead} placeholder="e.g. 35" onChange={(e) => setOverhead(e.target.value)} />
            </div>
          </div>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 2fr", gap: 12, marginTop: 12 }}>
            <div>
              <div style={label}>Current p99 (ms)</div>
              <input style={input} type="number" min={0} value={p99} placeholder="e.g. 40" onChange={(e) => setP99(e.target.value)} />
            </div>
            <label style={{ display: "flex", gap: 10, alignItems: "center", color: c.muted, fontSize: 14, alignSelf: "end", paddingBottom: 10 }}>
              <input type="checkbox" checked={p99Lock} onChange={(e) => setP99Lock(e.target.checked)} />
              That tail is coordination-induced (peak contention)
            </label>
          </div>
        </div>
      </div>

      {/* ---- memo ---- */}
      <div style={{ ...card, borderColor: vc }}>
        <div style={{ display: "flex", alignItems: "baseline", gap: 14, flexWrap: "wrap" }}>
          <span style={{ color: c.bg, background: vc, fontWeight: 800, fontSize: 16, padding: "4px 14px", borderRadius: 8, letterSpacing: 0.5 }}>
            {memo.applicable ? "VALUE" : "NO CLAIM"}
          </span>
          <span style={{ color: c.text, fontSize: 15 }}>{memo.headline}</span>
        </div>

        {memo.applicable ? (
          <div style={{ display: "grid", gap: 10, marginTop: 18 }}>
            {memo.lines.map((l, i) => (
              <div key={i} style={{ display: "flex", gap: 12, alignItems: "baseline", flexWrap: "wrap" }}>
                <span
                  style={{
                    fontSize: 10.5, fontWeight: 700, letterSpacing: 0.5, padding: "2px 7px", borderRadius: 5,
                    color: l.source === "MEASURED" ? c.bg : c.amber,
                    background: l.source === "MEASURED" ? c.cyan : "transparent",
                    border: l.source === "MEASURED" ? "none" : `1px solid ${c.amber}`,
                    whiteSpace: "nowrap",
                  }}
                >
                  {l.source}
                </span>
                <span style={{ color: c.text, fontSize: 14 }}>
                  <strong>{l.label}:</strong> <span style={{ color: c.muted }}>{l.value}</span>
                </span>
              </div>
            ))}
          </div>
        ) : (
          <p style={{ color: c.muted, fontSize: 14, marginTop: 14, lineHeight: 1.5 }}>{memo.reason}</p>
        )}

        <p style={{ color: c.faint, fontSize: 12, marginTop: 16, lineHeight: 1.5 }}>
          {memo.caveats[0]} {memo.caveats[1]}
        </p>

        {memo.applicable && (
          <div style={{ display: "flex", gap: 12, marginTop: 16, flexWrap: "wrap" }}>
            <input
              style={{ ...input, width: 200 }}
              value={company}
              placeholder="Company (for the memo)"
              onChange={(e) => setCompany(e.target.value)}
            />
            <button
              onClick={() => download(renderValueMemoMarkdown({ company, inputs, memo }), "atomik-cost-of-lock-memo.md")}
              style={{ background: vc, color: c.bg, fontWeight: 700, fontSize: 14, padding: "10px 18px", borderRadius: 8, border: "none", cursor: "pointer" }}
            >
              Download the value memo
            </button>
          </div>
        )}
      </div>
    </div>
  );
}
