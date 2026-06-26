// Aggregation benchmark-proof logic for atomik.tech.
//
// This is a faithful mirror of the Python harness in
//   software/atomik_sdk/contract_bench/  (silicon.py + harness.py)
// which is the source of truth.  Every constant below was MEASURED on real
// hardware — the parallel-bank accumulator at 0xF0021000 on an ALINX/HamGeek
// AX7020 (XC7Z020) at 100 MHz, via abench.  Re-measure any time:
//   abench 65537 0xDEADBEEF0BADF00D
//
// The page computes the verdict client-side from these measured facts; it does
// not invent numbers.  A live, at-scale re-run on the board backs the "request
// a live run" CTA.

export const SILICON = {
  board: "ALINX/HamGeek AX7020 (XC7Z020-2CLG484)",
  engine: "parallel-bank delta-state accumulator @0xF0021000",
  clockHz: 100_000_000,
  nBanks: 8,
  perBankThroughput: 100_000_000, // ~1 accumulation / bank / cycle, measured
  peakThroughput: 800_000_000, // 8 banks @ 100 MHz, measured
  resultHex: "0x17e9fe29b5dc0aad", // byte-identical across 1/2/4/8 banks
  identicalAcrossBanks: true,
  // measured speedup vs 1 bank (the +0.01 is fixed per-run setup overhead)
  scaling: { 1: 1.0, 2: 1.99, 4: 3.99, 8: 7.99 } as Record<number, number>,
  measuredOn: "2026-06-21",
  reproduce: "abench 65537 0xDEADBEEF0BADF00D",
} as const;

// The RECORDED reference run — the silicon-measured ladder behind every verdict.
// Not a fresh run; the recorded 2026-06-21 measurement (mirrors silicon.py /
// live_run.reference_live_run).  Used to make "800 M/s measured" concrete and
// to back the LOI with witnessed evidence.
export const REFERENCE_RUN = {
  source: "recorded reference",
  runOn: SILICON.measuredOn,
  board: SILICON.board,
  count: 65537,
  seed: "0xDEADBEEF0BADF00D",
  cycles: { 1: 65537, 2: 32769, 4: 16385, 8: 8193 } as Record<number, number>,
  resultHex: SILICON.resultHex,
  identical: SILICON.identicalAcrossBanks,
  reproduce: SILICON.reproduce,
} as const;

export interface LadderRow {
  banks: number;
  cycles: number;
  speedup: number; // truncated (1.99/3.99/7.99), honest about per-run overhead
  throughput: number;
}

export function referenceLadder(): LadderRow[] {
  const base = REFERENCE_RUN.cycles[1];
  return Object.keys(REFERENCE_RUN.cycles)
    .map(Number)
    .sort((a, b) => a - b)
    .map((banks) => {
      const cycles = REFERENCE_RUN.cycles[banks];
      return {
        banks,
        cycles,
        speedup: Math.floor((base / cycles) * 100) / 100,
        throughput: (REFERENCE_RUN.count * SILICON.clockHz) / cycles,
      };
    });
}

export interface LiveRunEvidence {
  source: string;
  runOn: string;
  board: string;
  peak: number;
  perBank: number;
  banks: number;
  resultHex: string;
  identical: boolean;
  reproduce: string;
}

// The recorded reference, as LOI acceptance evidence.
export function referenceRunEvidence(): LiveRunEvidence {
  const base = REFERENCE_RUN.cycles[1];
  return {
    source: REFERENCE_RUN.source,
    runOn: REFERENCE_RUN.runOn,
    board: REFERENCE_RUN.board,
    peak: (REFERENCE_RUN.count * SILICON.clockHz) / REFERENCE_RUN.cycles[8],
    perBank: (REFERENCE_RUN.count * SILICON.clockHz) / base,
    banks: 8,
    resultHex: REFERENCE_RUN.resultHex,
    identical: REFERENCE_RUN.identical,
    reproduce: REFERENCE_RUN.reproduce,
  };
}

// ATOMiK's engine accelerates exactly the aggregations that form an abelian
// group — only then is the merged result provably order-independent.  The page
// will NOT claim reorder-stable correctness for anything outside this set.
export const ABELIAN: Record<string, string> = {
  sum: "modular integer sum — counters, totals, moments",
  xor: "bitwise XOR / parity — set symmetric-difference, sketch merge",
  count: "occurrence counting — histograms, frequency tables",
  or: "monotone OR / set union — presence, bloom-style membership",
  min: "min reduction (idempotent)",
  max: "max reduction (idempotent)",
  hll_merge: "HyperLogLog register merge — cardinality estimation",
  counter_merge: "CRDT counter / sketch merge — replica aggregation",
};

// A couple of deliberately-outside-the-class ops, so the page can show the
// honest FAIL rather than overselling.
export const NON_ABELIAN: Record<string, string> = {
  median: "median / percentile — order-dependent, not a group operation",
  "last-write-wins": "last-write-wins — depends on arrival order by definition",
};

export type Verdict = "PASS" | "SURPASS" | "FAIL";

export interface ProofResult {
  verdict: Verdict;
  headline: string;
  abelian: boolean;
  margin: number; // peak / required
  throughputPass: boolean;
  correctnessPass: boolean;
  minBanks: number;
  banksAvailable: number;
  requiredThroughput: number;
  peakThroughput: number;
  perBankThroughput: number;
}

const SURPASS_MARGIN = 2.0;

export function evaluate(
  aggregation: string,
  requiredThroughput: number,
  requiresReorderStability = true,
): ProofResult {
  const abelian = aggregation.toLowerCase() in ABELIAN;
  const peak = SILICON.peakThroughput;
  const margin = requiredThroughput > 0 ? peak / requiredThroughput : Infinity;
  const throughputPass = peak >= requiredThroughput && requiredThroughput > 0;
  const correctnessPass =
    !requiresReorderStability || (abelian && SILICON.identicalAcrossBanks);
  const minBanks = Math.max(
    1,
    Math.ceil(requiredThroughput / SILICON.perBankThroughput),
  );

  let verdict: Verdict = "FAIL";
  if (throughputPass && correctnessPass) {
    verdict = margin >= SURPASS_MARGIN ? "SURPASS" : "PASS";
  }

  let headline: string;
  if (verdict === "FAIL" && !correctnessPass) {
    headline =
      "this operator is outside ATOMiK's order-independent class — we won't claim reorder-stable correctness for it";
  } else if (verdict === "FAIL") {
    headline = `8 banks reach ${eng(peak)}; that's below the required rate`;
  } else if (verdict === "SURPASS") {
    headline = `meets the rate with ${margin.toFixed(1)}x headroom AND a byte-identical, lock-free correctness guarantee`;
  } else {
    headline =
      "meets the rate AND a byte-identical, lock-free correctness guarantee";
  }

  return {
    verdict,
    headline,
    abelian,
    margin,
    throughputPass,
    correctnessPass,
    minBanks,
    banksAvailable: SILICON.nBanks,
    requiredThroughput,
    peakThroughput: peak,
    perBankThroughput: SILICON.perBankThroughput,
  };
}

// The downloadable proof report — the leave-behind that travels to the buyer's
// decision-maker.  Mirrors report.to_markdown() in the Python harness.
export function renderReportMarkdown(opts: {
  workload: string;
  customer?: string;
  aggregation: string;
  requiredThroughput: number;
  requiresReorder: boolean;
  result: ProofResult;
}): string {
  const r = opts.result;
  const who = opts.customer ? `${opts.customer} / ${opts.workload}` : opts.workload;
  const scale = Object.entries(SILICON.scaling)
    .map(([b, s]) => `${b}->${s.toFixed(2)}x`)
    .join("  ");
  const ok = (b: boolean) => (b ? "PASS" : "FAIL");
  const aggDesc = ABELIAN[opts.aggregation.toLowerCase()] ?? "order-dependent operator";
  const tput = r.throughputPass
    ? `${r.margin.toFixed(1)}x headroom — needs ${r.minBanks} of ${r.banksAvailable} banks`
    : `short by ${(1 / r.margin).toFixed(1)}x at ${r.banksAvailable} banks`;
  const corr = r.correctnessPass
    ? r.abelian
      ? `'${opts.aggregation}' (${aggDesc}) is abelian -> provably order-independent, lock-free`
      : "delivered even though not required"
    : `'${opts.aggregation}' is order-dependent — ATOMiK won't claim reorder-stable correctness here`;
  return [
    `# ATOMiK Benchmark Proof — ${who}`,
    ``,
    `**VERDICT: ${r.verdict}** — ${r.headline}`,
    ``,
    `## Throughput gate`,
    `- Required: **${eng(opts.requiredThroughput)}**`,
    `- ATOMiK (measured on silicon): **${eng(r.peakThroughput)} at ${r.banksAvailable} banks ` +
      `(${eng(r.perBankThroughput)} per bank)**`,
    `- ${ok(r.throughputPass)} — ${tput}`,
    ``,
    `## Correctness gate (the part a lock would cost you)`,
    `- Required: ${opts.requiresReorder ? "byte-identical regardless of arrival order / parallelism" : "exact result"}`,
    `- ATOMiK: ${r.correctnessPass ? `byte-identical across 1/2/4/8 banks (${SILICON.resultHex})` : "not order-independent for this operator"}`,
    `- ${ok(r.correctnessPass)} — ${corr}`,
    ``,
    `## Measured linear scaling (silicon)`,
    `- Speedup vs 1 bank: ${scale}`,
    ``,
    `## Provenance`,
    `- Measured ${SILICON.measuredOn} on ${SILICON.board}`,
    `- Engine: ${SILICON.engine}, ${(SILICON.clockHz / 1e6).toFixed(0)} MHz`,
    `- Reproduce: ${SILICON.reproduce}`,
    ``,
    `> Scope: throughput is the engine's measured accumulation rate (the aggregation step).`,
    `> End-to-end depends on the ingest path; ATOMiK's order-independence removes the global`,
    `> ordering/locks that cap a conventional aggregator.`,
  ].join("\n");
}

// The downloadable SYNC proof — for the replica-convergence archetype class.
// Two gates: convergence (the byte-identical order-independence, silicon-proven)
// and data-moved (a MEASURED ratio, caveated as change-density-dependent).
export function renderSyncReportMarkdown(opts: {
  workload: string;
  vertical: string;
  fullStateBytes: number;
  deltaBytes: number;
  changeDensity: string;
  measuredNote: string;
}): string {
  const pct = Math.round((1 - opts.deltaBytes / opts.fullStateBytes) * 100);
  return [
    `# ATOMiK Sync Proof — ${opts.vertical} / ${opts.workload}`,
    ``,
    `**VERDICT: CONVERGES** — coordination-free convergence + ~${pct}% less data moved`,
    ``,
    `## Convergence gate (correctness)`,
    `- Required: replicas reach identical state with no coordinator / consensus`,
    `- ATOMiK: order-, partition-, and merge-independent — byte-identical result across`,
    `  1/2/4/8 banks (${SILICON.resultHex})`,
    `- PASS — the same property that makes parallel banks byte-identical makes replicas`,
    `  converge: any order or partition of deltas merges to identical state, no coordination`,
    ``,
    `## Data-moved gate (bandwidth)`,
    `- Conventional full-state sync: ${opts.fullStateBytes} B/tick`,
    `- ATOMiK delta sync: ${opts.deltaBytes} B/tick (measured)`,
    `- ~${pct}% less data moved at ${opts.changeDensity}`,
    `- ${opts.measuredNote}`,
    ``,
    `## Provenance`,
    `- Convergence proof measured ${SILICON.measuredOn} on ${SILICON.board}`,
    `- Reproduce the order-independence: ${SILICON.reproduce}`,
    ``,
    `> Scope: the data-moved ratio is for the measured change density (${opts.changeDensity}).`,
    `> It scales with how much of the state changes per tick — lower change density means a`,
    `> larger reduction, higher means smaller. It is not a universal multiplier.`,
  ].join("\n");
}

// ---------------------------------------------------------------------------
// The Conditional LOI — the instrument the buyer counter-signs.  It embeds the
// proof and the bar, states the acceptance test, and leaves blanks for the
// legal specifics.  It is offered ONLY for a passing verdict; on a FAIL the bar
// section states honestly that ATOMiK does not commit.
// ---------------------------------------------------------------------------
export function loiPasses(spec: CarriedSpec): boolean {
  if (spec.kind === "aggregation") {
    return evaluate(spec.agg ?? "", spec.rate ?? 0, spec.reorder ?? true).verdict !== "FAIL";
  }
  const full = spec.fullStateBytes ?? 0;
  const delta = spec.deltaBytes ?? 0;
  return delta > 0 && delta < full;
}

export function renderConditionalLOI(opts: {
  kind: "aggregation" | "sync";
  workload: string;
  company?: string;
  draftedOn?: string; // ISO date (browser-supplied); blank placeholder otherwise
  liveRun?: LiveRunEvidence; // witnessed silicon backing (recorded reference or live)
  // aggregation
  aggregation?: string;
  requiredThroughput?: number;
  requiresReorder?: boolean;
  // sync
  fullStateBytes?: number;
  deltaBytes?: number;
  changeDensity?: string;
}): string {
  const drafted = opts.draftedOn || "____-__-__";
  const counterparty = opts.company?.trim() || "________________________";
  const head = [
    "# ATOMiK — Conditional Letter of Intent (DRAFT)",
    "",
    "**This is a non-binding draft for discussion.** Binding terms require a mutually executed agreement, and any proprietary workload data requires an NDA and an agreed evaluation scope.",
    "",
    `- **Drafted:** ${drafted}`,
    `- **Between:** Rockwell Intelligence (\"ATOMiK\") and **${counterparty}** (\"Counterparty\")`,
    `- **Workload:** ${opts.workload}`,
    "",
    "## 1. Purpose",
    "This letter records a conditional commitment: if ATOMiK demonstrates the bar in Section 3 on a live run of the Counterparty's workload on ATOMiK silicon (Section 4), the Counterparty intends to proceed to the next step in Section 5.",
    "",
  ];

  let body: string[];
  if (opts.kind === "aggregation") {
    const r = evaluate(opts.aggregation ?? "sum", opts.requiredThroughput ?? 0, opts.requiresReorder ?? true);
    const commits = r.verdict !== "FAIL";
    body = [
      "## 2. The workload",
      `- Aggregation operator: **${opts.aggregation}**${r.abelian ? " (abelian — order-independent)" : " (order-dependent)"}`,
      `- Required throughput to commit: **${eng(opts.requiredThroughput ?? 0)}**`,
      `- Correctness requirement: **${opts.requiresReorder ? "byte-identical regardless of arrival order / parallelism" : "exact result"}**`,
      "",
      "## 3. ATOMiK's commitment (the bar)",
      commits
        ? `ATOMiK commits that the parallel-bank delta-state accumulator will sustain **>= ${eng(opts.requiredThroughput ?? 0)}** at **byte-identical correctness across 1 / 2 / 4 / 8 banks** for the **${opts.aggregation}** aggregation, on the ${SILICON.board} at ${(SILICON.clockHz / 1e6).toFixed(0)} MHz.`
        : `On the current spec ATOMiK does **NOT** commit: '${opts.aggregation}' is outside the order-independent (abelian) class, so ATOMiK will not claim reorder-stable correctness for it. Re-scope the operator or drop the reorder requirement to proceed.`,
      "",
      `- Site-computed verdict vs the measured engine: **${r.verdict}**${r.throughputPass ? ` — ${r.margin.toFixed(1)}x headroom (${r.minBanks} of ${r.banksAvailable} banks).` : "."}`,
      `- Measured engine ceiling: **${eng(r.peakThroughput)}** across ${r.banksAvailable} banks (${eng(r.perBankThroughput)} per bank).`,
      "",
      "## 4. Acceptance test",
      "A PASS is a live on-silicon run at the Counterparty's scale where BOTH hold:",
      "1. **Throughput:** sustained accumulation rate >= the required throughput in Section 3.",
      "2. **Correctness:** the merged result is byte-identical across 1 / 2 / 4 / 8 banks (the order-independence proof).",
      "",
      `Witness / reproduce the reference run: \`${SILICON.reproduce}\`. The Counterparty's run uses their operator and scale.`,
      "",
    ];
  } else {
    const full = opts.fullStateBytes ?? 0;
    const delta = opts.deltaBytes ?? 0;
    const pct = full > 0 ? Math.round((1 - delta / full) * 100) : 0;
    const commits = delta > 0 && delta < full;
    body = [
      "## 2. The workload",
      "- Pattern: replica / state convergence by delta exchange",
      `- Conventional full-state sync: **${full} B/tick**`,
      `- ATOMiK delta sync: **${delta} B/tick**${opts.changeDensity ? ` at ${opts.changeDensity}` : ""}`,
      "",
      "## 3. ATOMiK's commitment (the bar)",
      commits
        ? `ATOMiK commits that replicas exchanging delta-state **converge to byte-identical state with no coordinator or consensus** — any node, any order, after any partition (the same order-independence proven on silicon) — and that a sync tick moves **${delta} B vs ${full} B** of full state (**~${pct}% less**)${opts.changeDensity ? ` at ${opts.changeDensity}` : ""}.`
        : `On the current spec ATOMiK does **NOT** commit a bandwidth reduction: the stated delta (${delta} B) is not smaller than full state (${full} B). Re-scope the change density to proceed.`,
      "",
      "## 4. Acceptance test",
      "A PASS is a live on-silicon run where BOTH hold:",
      "1. **Convergence:** replicas merge to a byte-identical result regardless of delta order / partition.",
      "2. **Data moved:** the delta tick is materially smaller than full-state sync at the Counterparty's change density.",
      "",
      `Witness / reproduce the order-independence: \`${SILICON.reproduce}\`.`,
      "",
    ];
  }

  const lr = opts.liveRun;
  const witnessed = lr
    ? [
        "## 4a. Silicon backing — witnessed run",
        `- Source: **${lr.source}** (${lr.runOn}) on ${lr.board}`,
        `- Engine sustained **${eng(lr.peak)}** (${eng(lr.perBank)} per bank) at ${lr.banks} banks.`,
        `- Merged result **${lr.resultHex}**, ${lr.identical ? "byte-identical" : "NOT identical"} across 1 / 2 / 4 / 8 banks — the order-independence proof.`,
        `- Reproduce: \`${lr.reproduce}\``,
        "",
        "This run witnesses the engine ceiling and order-independence that back the bar. The acceptance test in Section 4 re-runs at the Counterparty's operator and scale.",
        "",
      ]
    : [];

  const tail = [
    ...witnessed,
    "## 5. Counterparty's conditional commitment",
    "Upon a PASS, the Counterparty intends in good faith to proceed to (select one):",
    "- [ ] Paid pilot / evaluation",
    "- [ ] Design-partner LOI",
    "- [ ] Purchase order / commercial terms",
    "",
    "within **____ days** of the acceptance test.",
    "",
    "## 6. Scope & limitations (read before signing)",
    "- The throughput commitment covers the **engine's accumulation / merge step measured on silicon**, not end-to-end system throughput, which depends on the Counterparty's ingest path.",
    "- The correctness / order-independence guarantee applies **only to abelian operators**. For order-dependent operators ATOMiK makes no reorder-stable claim.",
    "- Any sync bandwidth figure **scales with change density** and is not a universal multiplier.",
    "- This draft is **non-binding**; binding terms require a mutually executed agreement.",
    "",
    "## 7. Provenance",
    `- Engine: ${SILICON.engine}`,
    `- Measured: ${SILICON.measuredOn} on ${SILICON.board}`,
    `- Byte-identical result hash across banks: ${SILICON.resultHex}`,
    `- Reproduce: ${SILICON.reproduce}`,
    "",
    "## 8. Signatures",
    "",
    "ATOMiK / Rockwell Intelligence — Matthew Rockwell   ____________________   Date ________",
    "",
    `${counterparty} — ____________________   Date ________`,
  ];

  return [...head, ...body, ...tail].join("\n");
}

// ---------------------------------------------------------------------------
// Carry layer — moves a computed proof from the benchmark tool / catalog into
// the contact intake so the lead arrives spec'd, not as free-text prose.  The
// contact page RE-COMPUTES the verdict from these facts (it never trusts a
// verdict string from the URL), so the carried link is tamper-evident.
// ---------------------------------------------------------------------------
export interface CarriedSpec {
  kind: "aggregation" | "sync";
  workload?: string; // human label for the workload, for context
  // aggregation
  agg?: string;
  rate?: number; // required throughput, events/s
  reorder?: boolean;
  // sync
  fullStateBytes?: number;
  deltaBytes?: number;
}

// Build the /contact link that carries a spec into the conditional-LOI intake.
export function loiHref(spec: CarriedSpec): string {
  const p = new URLSearchParams();
  p.set("intent", "live-benchmark");
  p.set("bm_kind", spec.kind);
  if (spec.workload) p.set("bm_workload", spec.workload);
  if (spec.kind === "aggregation") {
    if (spec.agg) p.set("bm_agg", spec.agg);
    if (spec.rate != null && isFinite(spec.rate)) p.set("bm_rate", String(Math.round(spec.rate)));
    p.set("bm_reorder", spec.reorder === false ? "0" : "1");
  } else {
    if (spec.fullStateBytes != null) p.set("bm_full", String(spec.fullStateBytes));
    if (spec.deltaBytes != null) p.set("bm_delta", String(spec.deltaBytes));
  }
  return `/contact?${p.toString()}`;
}

// Read a carried spec back out of the contact URL.  Returns null if absent or
// malformed (the contact page then just renders the normal form).
export function parseCarriedSpec(params: URLSearchParams): CarriedSpec | null {
  const kind = params.get("bm_kind");
  const workload = params.get("bm_workload") || undefined;
  if (kind === "aggregation") {
    const agg = params.get("bm_agg") || undefined;
    const rate = params.get("bm_rate");
    if (!agg || rate == null || !isFinite(Number(rate))) return null;
    return { kind, workload, agg, rate: Number(rate), reorder: params.get("bm_reorder") !== "0" };
  }
  if (kind === "sync") {
    const full = params.get("bm_full");
    const delta = params.get("bm_delta");
    if (full == null || delta == null) return null;
    return { kind, workload, fullStateBytes: Number(full), deltaBytes: Number(delta) };
  }
  return null;
}

// Human throughput, e.g. 800_000_000 -> "800 M/s".
export function eng(n: number): string {
  if (!isFinite(n)) return "—";
  for (const [div, suf] of [
    [1e9, "G"],
    [1e6, "M"],
    [1e3, "K"],
  ] as const) {
    if (n >= div) {
      const v = n / div;
      return v >= 100 || v === Math.floor(v)
        ? `${v.toFixed(0)} ${suf}/s`
        : `${v.toFixed(2)} ${suf}/s`;
    }
  }
  return `${n.toFixed(0)}/s`;
}
