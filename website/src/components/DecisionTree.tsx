"use client";

import { useState, useCallback } from "react";

/* ------------------------------------------------------------------ */
/*  Types                                                              */
/* ------------------------------------------------------------------ */

type Challenge =
  | "sync"    // Synchronizing state across distributed nodes
  | "stream"  // Streaming and processing events
  | "change"; // Detecting changes in data

interface Verdict {
  recommendation: string;
  reason: string;
  alternative: string | null;
  alternativeReason: string | null;
  scrollTarget: string;
}

/* ------------------------------------------------------------------ */
/*  Verdict logic — honest about trade-offs                            */
/* ------------------------------------------------------------------ */

const verdicts: Record<Challenge, Verdict> = {
  sync: {
    recommendation: "ATOMiK",
    reason:
      "ATOMiK models state updates as XOR deltas rather than repeated full-state transfers. It can reduce coordination pressure for workloads that fit the algebra, but deployment claims still require workload-specific validation.",
    alternative: "Redis, etcd, or ZooKeeper",
    alternativeReason:
      "you need a full-featured data store with queries, transactions, pub/sub, or strong consistency (linearizable reads). ATOMiK is not a database.",
    scrollTarget: "compare-state-sync",
  },
  stream: {
    recommendation: "ATOMiK",
    reason:
      "ATOMiK accumulates deltas into constant-size modeled state with self-inverse undo. It is most relevant when current-state reconstruction matters more than retaining every event.",
    alternative: "Kafka or Event Sourcing",
    alternativeReason:
      "you need ordered event delivery, consumer groups, replayable audit trails, or dead-letter queues. ATOMiK does not store individual events.",
    scrollTarget: "compare-stream",
  },
  change: {
    recommendation: "ATOMiK",
    reason:
      "Change detection can be modeled as an accumulator comparison. Hardware timing, side-channel, and deployment claims need measured artifacts for the specific implementation.",
    alternative: "Merkle Trees",
    alternativeReason:
      "you need to efficiently identify which specific subtree changed in a large hierarchical dataset. Merkle Trees give O(log n) localization; ATOMiK tells you that something changed, not where.",
    scrollTarget: "compare-change",
  },
};

const challenges: { key: Challenge; label: string; description: string }[] = [
  {
    key: "sync",
    label: "Synchronizing state across distributed nodes",
    description: "ATOMiK vs Redis, etcd, ZooKeeper",
  },
  {
    key: "stream",
    label: "Streaming and processing events",
    description: "ATOMiK vs Kafka, Event Sourcing, CRDTs",
  },
  {
    key: "change",
    label: "Detecting changes in data",
    description: "ATOMiK vs Checksums, Diff, Merkle Trees",
  },
];

/* ------------------------------------------------------------------ */
/*  Component                                                          */
/* ------------------------------------------------------------------ */

export default function DecisionTree() {
  const [selected, setSelected] = useState<Challenge | null>(null);

  const handleSelect = useCallback((key: Challenge) => {
    setSelected(key);
    // Scroll to the verdict first, then the comparison table
    setTimeout(() => {
      const verdictEl = document.getElementById("decision-verdict");
      if (verdictEl) {
        verdictEl.scrollIntoView({ behavior: "smooth", block: "start" });
      }
    }, 100);
  }, []);

  const handleViewComparison = useCallback(() => {
    if (!selected) return;
    const target = verdicts[selected].scrollTarget;
    const el = document.getElementById(target);
    if (el) {
      el.scrollIntoView({ behavior: "smooth", block: "start" });
      // Briefly highlight the section
      el.classList.add("ring-2", "ring-purple-500/50");
      setTimeout(() => el.classList.remove("ring-2", "ring-purple-500/50"), 2000);
    }
  }, [selected]);

  const verdict = selected ? verdicts[selected] : null;

  return (
    <section className="max-w-5xl mx-auto px-6 py-12">
      <h2 className="text-2xl font-bold mb-2">Which Solution Is Right for You?</h2>
      <p className="text-sm mb-8 max-w-3xl leading-relaxed" style={{ color: "#8888a0" }}>
        Answer one question and we will point you to the right comparison — and
        be honest about when ATOMiK is not the best fit.
      </p>

      {/* Step 1: Challenge selection */}
      <div className="mb-6">
        <p className="text-sm font-semibold mb-4" style={{ color: "#b0b0c0" }}>
          What is your primary challenge?
        </p>
        <div className="grid gap-3 sm:grid-cols-3">
          {challenges.map((c) => {
            const isActive = selected === c.key;
            return (
              <button
                key={c.key}
                onClick={() => handleSelect(c.key)}
                className="text-left rounded-xl border p-5 transition-all duration-200"
                style={{
                  background: isActive ? "rgba(139,92,246,0.08)" : "#12121a",
                  borderColor: isActive ? "#8b5cf6" : "#1e1e2e",
                  cursor: "pointer",
                }}
              >
                <p
                  className="text-sm font-semibold mb-1"
                  style={{ color: isActive ? "#c4b5fd" : "#d0d0e0" }}
                >
                  {c.label}
                </p>
                <p className="text-xs" style={{ color: "#8888a0" }}>
                  {c.description}
                </p>
              </button>
            );
          })}
        </div>
      </div>

      {/* Step 2 + 3: Verdict */}
      {verdict && (
        <div
          id="decision-verdict"
          className="rounded-xl border p-6 transition-all duration-300"
          style={{
            background: "rgba(139,92,246,0.04)",
            borderColor: "#2d2d4a",
          }}
        >
          {/* Recommended */}
          <div className="flex items-start gap-3 mb-4">
            <span className="mt-0.5 shrink-0 text-lg" style={{ color: "#22c55e" }}>
              {"\u2713"}
            </span>
            <div>
              <p className="text-sm font-semibold mb-1" style={{ color: "#c4b5fd" }}>
                Recommended: {verdict.recommendation}
              </p>
              <p className="text-sm leading-relaxed" style={{ color: "#b0b0c0" }}>
                {verdict.reason}
              </p>
            </div>
          </div>

          {/* Honest alternative */}
          {verdict.alternative && (
            <div className="flex items-start gap-3 mb-5">
              <span className="mt-0.5 shrink-0 text-lg" style={{ color: "#f59e0b" }}>
                {"\u26A0"}
              </span>
              <div>
                <p className="text-sm font-semibold mb-1" style={{ color: "#fbbf24" }}>
                  Consider {verdict.alternative} instead if...
                </p>
                <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
                  ...{verdict.alternativeReason}
                </p>
              </div>
            </div>
          )}

          {/* Scroll to comparison */}
          <button
            onClick={handleViewComparison}
            className="inline-flex items-center gap-2 px-4 py-2 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{
              background: "#4f8fff",
              color: "#fff",
              cursor: "pointer",
            }}
          >
            View detailed comparison
            <span aria-hidden="true">{"\u2193"}</span>
          </button>
        </div>
      )}
    </section>
  );
}
