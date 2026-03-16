"use client";

import { useState, useCallback, useRef, useEffect } from "react";
import Link from "next/link";
import Nav from "@/components/Nav";

/* ---------- 32-bit XOR helpers ---------- */

function xor32(a: number, b: number): number {
  return (a ^ b) >>> 0;
}

function toHex(v: number): string {
  return "0x" + v.toString(16).toUpperCase().padStart(8, "0");
}

function parseHex(s: string): number | null {
  const cleaned = s.replace(/^0x/i, "").trim();
  if (cleaned === "" || !/^[0-9a-fA-F]+$/.test(cleaned)) return null;
  const v = parseInt(cleaned, 16);
  if (isNaN(v) || v < 0 || v > 0xffffffff) return null;
  return v >>> 0;
}

function randomU32(): number {
  return (Math.random() * 0xffffffff) >>> 0;
}

/* ---------- History entry ---------- */

interface HistoryEntry {
  id: number;
  op: "LOAD" | "ACCUM" | "READ" | "SWAP";
  detail: string;
  resultRef: number;
  resultAcc: number;
}

/* ---------- Mono style constant ---------- */

const mono = "var(--font-geist-mono), 'SF Mono', 'Fira Code', 'Consolas', monospace";

/* ---------- Card wrapper ---------- */

function Card({
  children,
  className = "",
}: {
  children: React.ReactNode;
  className?: string;
}) {
  return (
    <div
      className={`rounded-xl border p-6 ${className}`}
      style={{ background: "#12121a", borderColor: "#1e1e2e" }}
    >
      {children}
    </div>
  );
}

/* ---------- Hex value display ---------- */

function HexValue({
  label,
  value,
  color = "#22d3ee",
  pulse = false,
}: {
  label: string;
  value: number;
  color?: string;
  pulse?: boolean;
}) {
  return (
    <div className="flex flex-col items-center gap-1">
      <span className="text-xs font-medium uppercase tracking-wider" style={{ color: "#8888a0" }}>
        {label}
      </span>
      <span
        className="text-lg sm:text-xl font-bold transition-all duration-300"
        style={{
          fontFamily: mono,
          color,
          textShadow: pulse ? `0 0 12px ${color}60` : "none",
        }}
      >
        {toHex(value)}
      </span>
    </div>
  );
}

/* ---------- Operation button ---------- */

function OpButton({
  label,
  onClick,
  color,
  disabled = false,
}: {
  label: string;
  onClick: () => void;
  color: string;
  disabled?: boolean;
}) {
  return (
    <button
      onClick={onClick}
      disabled={disabled}
      className="px-4 py-2 rounded-lg text-sm font-bold transition-all duration-150 hover:opacity-90 active:scale-95 disabled:opacity-40 disabled:cursor-not-allowed"
      style={{
        background: color,
        color: "#fff",
        border: `1px solid ${color}`,
      }}
    >
      {label}
    </button>
  );
}

/* ---------- Hex input ---------- */

function HexInput({
  value,
  onChange,
  placeholder,
}: {
  value: string;
  onChange: (v: string) => void;
  placeholder: string;
}) {
  return (
    <input
      type="text"
      value={value}
      onChange={(e) => onChange(e.target.value)}
      placeholder={placeholder}
      className="rounded-lg px-3 py-2 text-sm w-full transition-colors duration-150 focus:outline-none"
      style={{
        background: "#0a0a0f",
        border: "1px solid #1e1e2e",
        color: "#22d3ee",
        fontFamily: mono,
      }}
      onFocus={(e) => {
        e.currentTarget.style.borderColor = "#4f8fff";
      }}
      onBlur={(e) => {
        e.currentTarget.style.borderColor = "#1e1e2e";
      }}
    />
  );
}

/* ===================================================================
   Main Demo Page
   =================================================================== */

export default function DemoPage() {
  /* --- Simulator state --- */
  const [reference, setReference] = useState<number>(0x00000000 >>> 0);
  const [accumulator, setAccumulator] = useState<number>(0x00000000 >>> 0);
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [nextId, setNextId] = useState(1);
  const [pulseField, setPulseField] = useState<"ref" | "acc" | "state" | null>(null);

  /* Input fields */
  const [loadInput, setLoadInput] = useState("DEADBEEF");
  const [accumInput, setAccumInput] = useState("0000FFFF");

  const currentState = xor32(reference, accumulator);

  const historyEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    historyEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [history]);

  /* Pulse animation helper */
  const pulse = useCallback(
    (field: "ref" | "acc" | "state") => {
      setPulseField(field);
      setTimeout(() => setPulseField(null), 500);
    },
    [],
  );

  /* Record history */
  const record = useCallback(
    (op: HistoryEntry["op"], detail: string, ref: number, acc: number) => {
      setHistory((prev) => [
        ...prev,
        { id: nextId, op, detail, resultRef: ref, resultAcc: acc },
      ]);
      setNextId((n) => n + 1);
    },
    [nextId],
  );

  /* --- Operations --- */

  const doLoad = useCallback(() => {
    const v = parseHex(loadInput);
    if (v === null) return;
    setReference(v);
    setAccumulator(0);
    record("LOAD", `reference = ${toHex(v)}`, v, 0);
    pulse("ref");
  }, [loadInput, record, pulse]);

  const doAccum = useCallback(() => {
    const v = parseHex(accumInput);
    if (v === null) return;
    const newAcc = xor32(accumulator, v);
    setAccumulator(newAcc);
    record("ACCUM", `acc ^= ${toHex(v)}`, reference, newAcc);
    pulse("acc");
  }, [accumInput, accumulator, reference, record, pulse]);

  const doRead = useCallback(() => {
    record("READ", `state = ref ^ acc = ${toHex(currentState)}`, reference, accumulator);
    pulse("state");
  }, [currentState, reference, accumulator, record, pulse]);

  const doSwap = useCallback(() => {
    const newRef = currentState;
    setReference(newRef);
    setAccumulator(0);
    record("SWAP", `new ref = ${toHex(newRef)}, acc reset`, newRef, 0);
    pulse("ref");
  }, [currentState, record, pulse]);

  /* ---- Key Properties Demo state ---- */

  const [commResult, setCommResult] = useState<{ ab: number; ba: number } | null>(null);
  const [selfInvResult, setSelfInvResult] = useState<{
    original: number;
    afterOne: number;
    afterTwo: number;
  } | null>(null);
  const [orderResult, setOrderResult] = useState<{
    deltas: number[];
    ordered: number;
    shuffled: number;
    shuffledOrder: number[];
  } | null>(null);

  /* Commutativity demo */
  const demoCommutativity = useCallback(() => {
    const a = randomU32();
    const b = randomU32();
    const base = randomU32();
    const ab = xor32(xor32(base, a), b);
    const ba = xor32(xor32(base, b), a);
    setCommResult({ ab, ba });
  }, []);

  /* Self-inverse demo */
  const demoSelfInverse = useCallback(() => {
    const base = randomU32();
    const delta = randomU32();
    const afterOne = xor32(base, delta);
    const afterTwo = xor32(afterOne, delta);
    setSelfInvResult({ original: base, afterOne, afterTwo });
  }, []);

  /* Order independence demo */
  const demoOrderIndependence = useCallback(() => {
    const deltas = Array.from({ length: 5 }, () => randomU32());
    const ordered = deltas.reduce((acc, d) => xor32(acc, d), 0 >>> 0);
    const shuffled = [...deltas];
    for (let i = shuffled.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [shuffled[i], shuffled[j]] = [shuffled[j], shuffled[i]];
    }
    const shuffledResult = shuffled.reduce((acc, d) => xor32(acc, d), 0 >>> 0);
    setOrderResult({
      deltas,
      ordered,
      shuffled: shuffledResult,
      shuffledOrder: shuffled,
    });
  }, []);

  /* --- Render --- */

  const opColors = {
    LOAD: "#8b5cf6",
    ACCUM: "#4f8fff",
    READ: "#22d3ee",
    SWAP: "#f59e0b",
  };

  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-12">
        <h1 className="text-4xl font-bold tracking-tight mb-4">
          Interactive{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{ backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)" }}
          >
            Delta-State
          </span>{" "}
          Demo
        </h1>
        <p className="text-lg max-w-2xl mx-auto" style={{ color: "#8888a0" }}>
          Try ATOMiK&apos;s four core operations in your browser. Everything runs in
          JavaScript&nbsp;&mdash; same algebra, same rules, same results as the hardware.
        </p>
      </section>

      {/* ======== Simulator ======== */}
      <section className="max-w-3xl mx-auto px-6 pb-16">
        <Card>
          {/* State display */}
          <div className="mb-6">
            <h2 className="text-sm font-semibold uppercase tracking-wider mb-4" style={{ color: "#8888a0" }}>
              AtomikContext State
            </h2>
            <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 text-center">
              <HexValue
                label="reference"
                value={reference}
                color="#8b5cf6"
                pulse={pulseField === "ref"}
              />
              <HexValue
                label="accumulator"
                value={accumulator}
                color="#4f8fff"
                pulse={pulseField === "acc"}
              />
              <HexValue
                label="current_state (ref XOR acc)"
                value={currentState}
                color="#22d3ee"
                pulse={pulseField === "state"}
              />
            </div>

            {/* Equation bar */}
            <div
              className="mt-4 rounded-lg px-4 py-2 text-center text-sm"
              style={{
                background: "#0a0a0f",
                border: "1px solid #1e1e2e",
                fontFamily: mono,
                color: "#8888a0",
              }}
            >
              <span style={{ color: "#8b5cf6" }}>{toHex(reference)}</span>
              {" ^ "}
              <span style={{ color: "#4f8fff" }}>{toHex(accumulator)}</span>
              {" = "}
              <span style={{ color: "#22d3ee" }}>{toHex(currentState)}</span>
            </div>
          </div>

          {/* Operations */}
          <div className="space-y-4">
            {/* LOAD */}
            <div className="flex flex-col sm:flex-row items-stretch sm:items-center gap-2">
              <OpButton label="LOAD" onClick={doLoad} color="#8b5cf6" />
              <HexInput value={loadInput} onChange={setLoadInput} placeholder="hex value, e.g. DEADBEEF" />
              <span className="text-xs shrink-0" style={{ color: "#8888a0" }}>
                Set reference, reset accumulator
              </span>
            </div>

            {/* ACCUM */}
            <div className="flex flex-col sm:flex-row items-stretch sm:items-center gap-2">
              <OpButton label="ACCUM" onClick={doAccum} color="#4f8fff" />
              <HexInput value={accumInput} onChange={setAccumInput} placeholder="hex delta, e.g. 0000FFFF" />
              <span className="text-xs shrink-0" style={{ color: "#8888a0" }}>
                XOR delta into accumulator
              </span>
            </div>

            {/* READ + SWAP */}
            <div className="flex flex-wrap gap-2">
              <OpButton label="READ" onClick={doRead} color="#22b8a6" />
              <OpButton label="SWAP" onClick={doSwap} color="#f59e0b" />
              <span className="text-xs self-center ml-2" style={{ color: "#8888a0" }}>
                READ highlights current state &middot; SWAP promotes it to new reference
              </span>
            </div>
          </div>

          {/* History log */}
          {history.length > 0 && (
            <div className="mt-6">
              <h3
                className="text-xs font-semibold uppercase tracking-wider mb-2 flex items-center justify-between"
                style={{ color: "#8888a0" }}
              >
                Operation Log
                <button
                  onClick={() => {
                    setHistory([]);
                    setNextId(1);
                  }}
                  className="text-xs font-normal hover:underline"
                  style={{ color: "#8888a0" }}
                >
                  Clear
                </button>
              </h3>
              <div
                className="rounded-lg overflow-y-auto space-y-1 p-3"
                style={{
                  background: "#0a0a0f",
                  border: "1px solid #1e1e2e",
                  maxHeight: "200px",
                }}
              >
                {history.map((h) => (
                  <div key={h.id} className="flex items-baseline gap-2 text-xs" style={{ fontFamily: mono }}>
                    <span
                      className="shrink-0 font-bold w-14 text-right"
                      style={{ color: opColors[h.op] }}
                    >
                      {h.op}
                    </span>
                    <span style={{ color: "#b0b0c0" }}>{h.detail}</span>
                    <span className="ml-auto shrink-0" style={{ color: "#555" }}>
                      state={toHex(xor32(h.resultRef, h.resultAcc))}
                    </span>
                  </div>
                ))}
                <div ref={historyEndRef} />
              </div>
            </div>
          )}
        </Card>
      </section>

      {/* ======== Key Properties ======== */}
      <section className="max-w-3xl mx-auto px-6 pb-16">
        <h2 className="text-2xl font-bold tracking-tight mb-2 text-center">
          Key Properties
        </h2>
        <p className="text-center text-sm mb-8" style={{ color: "#8888a0" }}>
          These aren&apos;t just nice-to-haves &mdash; they&apos;re what makes lock-free parallel
          accumulation correct.
        </p>

        <div className="space-y-6">
          {/* Commutativity */}
          <Card>
            <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-3 mb-4">
              <div>
                <h3 className="text-lg font-semibold">Commutativity</h3>
                <p className="text-sm" style={{ color: "#8888a0" }}>
                  A ^ B = B ^ A &mdash; order of accumulation doesn&apos;t matter.
                </p>
              </div>
              <OpButton label="Try it" onClick={demoCommutativity} color="#8b5cf6" />
            </div>
            {commResult && (
              <div
                className="rounded-lg p-4 space-y-1 text-sm"
                style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", fontFamily: mono }}
              >
                <div>
                  <span style={{ color: "#8888a0" }}>A then B: </span>
                  <span style={{ color: "#22d3ee" }}>{toHex(commResult.ab)}</span>
                </div>
                <div>
                  <span style={{ color: "#8888a0" }}>B then A: </span>
                  <span style={{ color: "#22d3ee" }}>{toHex(commResult.ba)}</span>
                </div>
                <div className="pt-1">
                  {commResult.ab === commResult.ba ? (
                    <span style={{ color: "#22c55e" }}>Identical. Order doesn&apos;t matter.</span>
                  ) : (
                    <span style={{ color: "#ef4444" }}>Mismatch (bug!)</span>
                  )}
                </div>
              </div>
            )}
          </Card>

          {/* Self-inverse */}
          <Card>
            <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-3 mb-4">
              <div>
                <h3 className="text-lg font-semibold">Self-Inverse</h3>
                <p className="text-sm" style={{ color: "#8888a0" }}>
                  A ^ A = 0 &mdash; every delta is its own undo.
                </p>
              </div>
              <OpButton label="Try it" onClick={demoSelfInverse} color="#4f8fff" />
            </div>
            {selfInvResult && (
              <div
                className="rounded-lg p-4 space-y-1 text-sm"
                style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", fontFamily: mono }}
              >
                <div>
                  <span style={{ color: "#8888a0" }}>Original state: </span>
                  <span style={{ color: "#8b5cf6" }}>{toHex(selfInvResult.original)}</span>
                </div>
                <div>
                  <span style={{ color: "#8888a0" }}>After XOR delta: </span>
                  <span style={{ color: "#f59e0b" }}>{toHex(selfInvResult.afterOne)}</span>
                </div>
                <div>
                  <span style={{ color: "#8888a0" }}>After XOR same delta again: </span>
                  <span style={{ color: "#22d3ee" }}>{toHex(selfInvResult.afterTwo)}</span>
                </div>
                <div className="pt-1">
                  {selfInvResult.original === selfInvResult.afterTwo ? (
                    <span style={{ color: "#22c55e" }}>
                      Back to original. Delta cancelled itself.
                    </span>
                  ) : (
                    <span style={{ color: "#ef4444" }}>Mismatch (bug!)</span>
                  )}
                </div>
              </div>
            )}
          </Card>

          {/* Order independence */}
          <Card>
            <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-3 mb-4">
              <div>
                <h3 className="text-lg font-semibold">Order Independence</h3>
                <p className="text-sm" style={{ color: "#8888a0" }}>
                  5 random deltas, applied in two different orders &mdash; same result.
                </p>
              </div>
              <OpButton label="Try it" onClick={demoOrderIndependence} color="#22b8a6" />
            </div>
            {orderResult && (
              <div
                className="rounded-lg p-4 space-y-2 text-sm"
                style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", fontFamily: mono }}
              >
                <div>
                  <span style={{ color: "#8888a0" }}>Original order: </span>
                  <span style={{ color: "#b0b0c0" }}>
                    {orderResult.deltas.map(toHex).join(", ")}
                  </span>
                </div>
                <div>
                  <span style={{ color: "#8888a0" }}>Shuffled order: </span>
                  <span style={{ color: "#b0b0c0" }}>
                    {orderResult.shuffledOrder.map(toHex).join(", ")}
                  </span>
                </div>
                <div className="pt-1 flex flex-col sm:flex-row gap-4">
                  <div>
                    <span style={{ color: "#8888a0" }}>Result (ordered): </span>
                    <span style={{ color: "#22d3ee" }}>{toHex(orderResult.ordered)}</span>
                  </div>
                  <div>
                    <span style={{ color: "#8888a0" }}>Result (shuffled): </span>
                    <span style={{ color: "#22d3ee" }}>{toHex(orderResult.shuffled)}</span>
                  </div>
                </div>
                <div className="pt-1">
                  {orderResult.ordered === orderResult.shuffled ? (
                    <span style={{ color: "#22c55e" }}>
                      Identical. This is why lock-free parallel accumulation works.
                    </span>
                  ) : (
                    <span style={{ color: "#ef4444" }}>Mismatch (bug!)</span>
                  )}
                </div>
              </div>
            )}
          </Card>
        </div>
      </section>

      {/* ======== CTA ======== */}
      <section className="text-center px-6 pb-24">
        <h2 className="text-2xl font-bold mb-3">Ready to build?</h2>
        <p className="mb-8" style={{ color: "#8888a0" }}>
          From pip install to FPGA silicon &mdash; start with the SDK and scale when you&apos;re ready.
        </p>
        <div className="flex flex-wrap justify-center gap-4">
          <Link
            href="/get-started"
            className="inline-block px-6 py-3 rounded-lg text-sm font-bold transition-opacity hover:opacity-90"
            style={{ background: "#4f8fff", color: "#fff" }}
          >
            Get Started
          </Link>
          <Link
            href="/docs"
            className="inline-block px-6 py-3 rounded-lg text-sm font-bold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#4f8fff",
              border: "1px solid #4f8fff",
            }}
          >
            Read the Docs
          </Link>
        </div>
      </section>
    </div>
  );
}
