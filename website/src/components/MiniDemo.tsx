"use client";

import { useState } from "react";
import Link from "next/link";

function fmt(n: number): string {
  return n.toLocaleString("en-US");
}

export default function MiniDemo() {
  const [reference, setReference] = useState<number | null>(null);
  const [accumulator, setAccumulator] = useState(0);
  const [log, setLog] = useState<string[]>([]);

  const currentState = reference !== null ? (reference ^ accumulator) : null;

  function pushLog(msg: string) {
    setLog((prev) => [...prev.slice(-4), msg]);
  }

  function handleLoad() {
    setReference(1000);
    setAccumulator(0);
    pushLog("LOAD 1,000 \u2192 reference set, accumulator cleared");
  }

  function handleAccum(delta: number) {
    setAccumulator((prev) => prev ^ delta);
    pushLog(`ACCUM \u2295 ${fmt(delta)} \u2192 accumulator updated`);
  }

  function handleSwap() {
    if (reference === null) return;
    const snapshotState = reference ^ accumulator;
    setReference(snapshotState);
    setAccumulator(0);
    pushLog(`SWAP \u2192 snapshot ${fmt(snapshotState)}, new epoch`);
  }

  const boxStyle = {
    background: "#12121a",
    border: "1px solid #1e1e2e",
  };

  return (
    <div>
      {/* State boxes */}
      <div className="grid grid-cols-3 gap-4 mb-6">
        <div className="rounded-xl p-5 text-center" style={boxStyle}>
          <div className="text-xs font-semibold uppercase tracking-wide mb-2" style={{ color: "#4f8fff" }}>
            Reference
          </div>
          <div className="text-2xl md:text-3xl font-extrabold font-mono" style={{ color: "#e0e0e8" }}>
            {reference !== null ? fmt(reference) : "\u2014"}
          </div>
        </div>
        <div className="rounded-xl p-5 text-center" style={boxStyle}>
          <div className="text-xs font-semibold uppercase tracking-wide mb-2" style={{ color: "#22d3ee" }}>
            Accumulator
          </div>
          <div className="text-2xl md:text-3xl font-extrabold font-mono" style={{ color: "#e0e0e8" }}>
            {fmt(accumulator)}
          </div>
        </div>
        <div className="rounded-xl p-5 text-center" style={{ ...boxStyle, border: "1px solid rgba(34,197,94,0.3)" }}>
          <div className="text-xs font-semibold uppercase tracking-wide mb-2" style={{ color: "#22c55e" }}>
            Current State
          </div>
          <div className="text-2xl md:text-3xl font-extrabold font-mono" style={{ color: "#22c55e" }}>
            {currentState !== null ? fmt(currentState) : "\u2014"}
          </div>
          <div className="text-[10px] mt-1 font-mono" style={{ color: "#555" }}>
            ref \u2295 acc
          </div>
        </div>
      </div>

      {/* Operation buttons */}
      <div className="flex flex-wrap gap-3 justify-center mb-6">
        <button
          onClick={handleLoad}
          className="px-5 py-2.5 rounded-lg text-sm font-semibold transition-all hover:opacity-90 cursor-pointer"
          style={{ background: "rgba(79,143,255,0.15)", color: "#4f8fff", border: "1px solid rgba(79,143,255,0.3)" }}
        >
          LOAD
        </button>
        <button
          onClick={() => handleAccum(50)}
          disabled={reference === null}
          className="px-5 py-2.5 rounded-lg text-sm font-semibold transition-all hover:opacity-90 cursor-pointer disabled:opacity-30 disabled:cursor-not-allowed"
          style={{ background: "rgba(34,211,238,0.15)", color: "#22d3ee", border: "1px solid rgba(34,211,238,0.3)" }}
        >
          ACCUM +50
        </button>
        <button
          onClick={() => handleAccum(100)}
          disabled={reference === null}
          className="px-5 py-2.5 rounded-lg text-sm font-semibold transition-all hover:opacity-90 cursor-pointer disabled:opacity-30 disabled:cursor-not-allowed"
          style={{ background: "rgba(34,211,238,0.15)", color: "#22d3ee", border: "1px solid rgba(34,211,238,0.3)" }}
        >
          ACCUM +100
        </button>
        <button
          onClick={handleSwap}
          disabled={reference === null}
          className="px-5 py-2.5 rounded-lg text-sm font-semibold transition-all hover:opacity-90 cursor-pointer disabled:opacity-30 disabled:cursor-not-allowed"
          style={{ background: "rgba(168,85,247,0.15)", color: "#a855f7", border: "1px solid rgba(168,85,247,0.3)" }}
        >
          SWAP
        </button>
      </div>

      {/* Operation log */}
      {log.length > 0 && (
        <div
          className="rounded-lg p-3 mb-6 font-mono text-xs space-y-1"
          style={{ background: "#0d0d14", border: "1px solid #1e1e2e" }}
        >
          {log.map((entry, i) => (
            <div key={i} style={{ color: i === log.length - 1 ? "#e0e0e8" : "#555" }}>
              {entry}
            </div>
          ))}
        </div>
      )}

      {/* Caption */}
      <p className="text-center text-sm mb-8" style={{ color: "#8888a0" }}>
        Same result regardless of operation order. Proven with 108 theorems.
      </p>

      {/* CTA buttons */}
      <div className="flex gap-4 justify-center flex-wrap">
        <Link
          href="/demo"
          className="px-7 py-3.5 rounded-lg text-sm font-semibold no-underline transition-all hover:opacity-90"
          style={{ background: "linear-gradient(135deg, #4f8fff, #3a7aee)", color: "#fff", boxShadow: "0 4px 24px rgba(79,143,255,0.2)" }}
        >
          Full Interactive Demo &rarr;
        </Link>
        <Link
          href="/ai-demo"
          className="px-7 py-3.5 rounded-lg text-sm font-semibold no-underline transition-all hover:opacity-90"
          style={{ background: "linear-gradient(135deg, #a855f7, #7c3aed)", color: "#fff", boxShadow: "0 4px 24px rgba(168,85,247,0.2)" }}
        >
          AI Inference Demo &rarr;
        </Link>
      </div>
    </div>
  );
}
