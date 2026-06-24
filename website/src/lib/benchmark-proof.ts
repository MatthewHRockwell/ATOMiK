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
