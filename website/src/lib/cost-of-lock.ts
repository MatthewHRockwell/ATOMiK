// Cost-of-Lock value translator for atomik.tech.
//
// Makes the LOI's bar economically legible WITHOUT inventing anything:
//   * ATOMiK's capability is MEASURED (the 800 M/s engine, byte-identical) —
//     pulled from benchmark-proof.ts, never a fabricated speedup multiplier.
//   * The "savings" are arithmetic on the BUYER'S OWN stated baseline (their
//     instances, their $/mo, the overhead THEY attribute to coordination).
//   * What ATOMiK removes is the coordination tax for correctness-under-
//     concurrency — an architectural fact for abelian merges (no lock needed),
//     not a "we're faster" claim.  Every output line is tagged MEASURED or
//     YOUR BASELINE so the memo survives scrutiny.
//
// Mirror of software/atomik_sdk/contract_bench/value.py (source of truth).

import { SILICON, eng, evaluate } from "./benchmark-proof";

export type Approach = "lock" | "shard" | "crdt" | "consensus";

export const APPROACH_LABEL: Record<Approach, string> = {
  lock: "lock contention / serialized hot path",
  shard: "shard fan-out maintained to avoid contention",
  crdt: "CRDT metadata + reconciliation overhead",
  consensus: "consensus / coordinator round-trip overhead",
};

export interface ValueInputs {
  aggregation: string;
  requiredThroughput: number; // events/s
  approach: Approach;
  reorder?: boolean; // default true
  // optional baseline — ALL buyer-stated; drive the $ / latency lines
  instances?: number; // instances / replicas / shards provisioned today
  instanceMonthly?: number; // $/instance/month
  coordinationOverheadPct?: number; // % of capacity/cost the buyer attributes to coordination
  p99Ms?: number; // current p99 latency
  p99LockInduced?: boolean; // is that tail coordination-induced?
}

export type Source = "MEASURED" | "YOUR BASELINE";

export interface MemoLine {
  label: string;
  value: string;
  source: Source;
}

export interface ValueMemo {
  applicable: boolean;
  reason: string;
  headline: string;
  // measured ATOMiK facts
  minBanks: number;
  banksAvailable: number;
  margin: number;
  peakThroughput: number;
  // buyer-baseline-derived (undefined unless the inputs were given)
  approachLabel: string;
  monthlyBaseline?: number;
  coordinationTaxMonthly?: number;
  reclaimedPct?: number;
  latencyNote?: string;
  lines: MemoLine[];
  caveats: string[];
}

export function usd(n: number): string {
  return "$" + Math.round(n).toLocaleString();
}

export function computeValue(inp: ValueInputs): ValueMemo {
  const reorder = inp.reorder ?? true;
  const r = evaluate(inp.aggregation, inp.requiredThroughput, reorder);
  const approachLabel = APPROACH_LABEL[inp.approach];
  const caveats = [
    "ATOMiK figures are MEASURED on silicon (the aggregation/merge step); savings are arithmetic on YOUR stated baseline.",
    "What is removed is the coordination tax for correctness under concurrency — an architectural property of abelian merges, not a throughput multiplier.",
    "End-to-end impact additionally depends on your ingest path; this is not a universal saving.",
  ];

  // Not applicable -> no value claim (honest).
  if (!r.correctnessPass) {
    return {
      applicable: false,
      reason: `'${inp.aggregation}' is order-dependent — ATOMiK won't claim it removes your coordination here.`,
      headline: "No order-independent value claim for this operator.",
      minBanks: r.minBanks,
      banksAvailable: r.banksAvailable,
      margin: r.margin,
      peakThroughput: r.peakThroughput,
      approachLabel,
      lines: [],
      caveats,
    };
  }
  if (!r.throughputPass) {
    return {
      applicable: false,
      reason: `Your rate exceeds the measured ${eng(r.peakThroughput)} engine ceiling — no headroom to translate.`,
      headline: "Required rate is above the measured engine ceiling.",
      minBanks: r.minBanks,
      banksAvailable: r.banksAvailable,
      margin: r.margin,
      peakThroughput: r.peakThroughput,
      approachLabel,
      lines: [],
      caveats,
    };
  }

  const lines: MemoLine[] = [
    {
      label: "ATOMiK sustains your rate",
      value: `${eng(inp.requiredThroughput)} on ${r.minBanks} of ${r.banksAvailable} banks (${r.margin.toFixed(1)}× headroom), lock-free, byte-identical`,
      source: "MEASURED",
    },
  ];

  let monthlyBaseline: number | undefined;
  let coordinationTaxMonthly: number | undefined;
  let reclaimedPct: number | undefined;

  if (inp.instances && inp.instanceMonthly) {
    monthlyBaseline = inp.instances * inp.instanceMonthly;
    lines.push({
      label: "Provisioned baseline",
      value: `${usd(monthlyBaseline)}/mo (${inp.instances} × ${usd(inp.instanceMonthly)})`,
      source: "YOUR BASELINE",
    });
  }

  if (inp.coordinationOverheadPct && inp.coordinationOverheadPct > 0) {
    reclaimedPct = inp.coordinationOverheadPct;
    if (monthlyBaseline != null) {
      coordinationTaxMonthly = (monthlyBaseline * reclaimedPct) / 100;
      lines.push({
        label: "Coordination tax (removed)",
        value: `~${usd(coordinationTaxMonthly)}/mo — ${reclaimedPct}% of baseline you attribute to ${approachLabel}`,
        source: "YOUR BASELINE",
      });
    } else {
      lines.push({
        label: "Coordination overhead (removed)",
        value: `${reclaimedPct}% of capacity you attribute to ${approachLabel}`,
        source: "YOUR BASELINE",
      });
    }
  }

  let latencyNote: string | undefined;
  if (inp.p99Ms && inp.p99Ms > 0 && inp.p99LockInduced) {
    latencyNote = `The ${approachLabel} component of your ${inp.p99Ms} ms p99 at peak is removed — ATOMiK's latency is deterministic (no lock or cache-coherency stall).`;
    lines.push({
      label: "Tail latency at peak",
      value: latencyNote,
      source: "YOUR BASELINE",
    });
  }

  const headline =
    coordinationTaxMonthly != null
      ? `~${usd(coordinationTaxMonthly)}/mo of ${approachLabel} is what order-independence removes`
      : reclaimedPct != null
        ? `${reclaimedPct}% of capacity spent on ${approachLabel} is what order-independence removes`
        : `the ${approachLabel} you maintain for correctness is what order-independence removes`;

  return {
    applicable: true,
    reason: `'${inp.aggregation}' is abelian and ${eng(inp.requiredThroughput)} fits the measured engine — the coordination tax is removable.`,
    headline,
    minBanks: r.minBanks,
    banksAvailable: r.banksAvailable,
    margin: r.margin,
    peakThroughput: r.peakThroughput,
    approachLabel,
    monthlyBaseline,
    coordinationTaxMonthly,
    reclaimedPct,
    latencyNote,
    lines,
    caveats,
  };
}

export function renderValueMemoMarkdown(opts: {
  workload?: string;
  company?: string;
  inputs: ValueInputs;
  memo: ValueMemo;
}): string {
  const { inputs: i, memo: m } = opts;
  const who = opts.company?.trim() || "________________________";
  const tag = (s: Source) => (s === "MEASURED" ? "[MEASURED]" : "[YOUR BASELINE]");
  const lines = [
    "# ATOMiK — Cost-of-Lock Value Memo (DRAFT)",
    "",
    `- **Prepared for:** ${who}`,
    `- **Workload:** ${opts.workload || "Aggregation workload"}`,
    `- **Operator / rate:** ${i.aggregation} at ${eng(i.requiredThroughput)}`,
    `- **Today:** ${m.approachLabel}`,
    "",
    `**${m.applicable ? "VALUE" : "NO CLAIM"}** — ${m.headline}`,
    "",
  ];
  if (!m.applicable) {
    lines.push(`> ${m.reason}`, "");
  } else {
    lines.push("## What the numbers say");
    for (const l of m.lines) {
      lines.push(`- ${tag(l.source)} **${l.label}:** ${l.value}`);
    }
    lines.push("");
    lines.push("## The argument");
    lines.push(
      `For an abelian aggregation like \`${i.aggregation}\`, the merged result is ` +
        `byte-identical regardless of arrival order or parallelism (measured on the ` +
        `${SILICON.board}, result ${SILICON.resultHex} across 1/2/4/8 banks). That is ` +
        `exactly the property a ${m.approachLabel} exists to guarantee — so it is no ` +
        `longer required. The tax above, from your stated baseline, is what that removes.`,
    );
    lines.push("");
  }
  lines.push("## Scope & honesty");
  for (const c of m.caveats) lines.push(`- ${c}`);
  lines.push("");
  lines.push(`> Reproduce the silicon facts: \`${SILICON.reproduce}\`.`);
  return lines.join("\n");
}
