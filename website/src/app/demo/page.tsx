"use client";

import { useState, useCallback, useRef, useEffect } from "react";
import Link from "next/link";
import Nav from "@/components/Nav";

/* ---------- 32-bit XOR helper ---------- */

function xor32(a: number, b: number): number {
  return (a ^ b) >>> 0;
}

/* ---------- Decimal display helpers ---------- */

function fmt(v: number): string {
  return v.toLocaleString("en-US");
}

function parseDec(s: string): number | null {
  const cleaned = s.replace(/,/g, "").trim();
  if (cleaned === "" || !/^\d+$/.test(cleaned)) return null;
  const v = parseInt(cleaned, 10);
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

/* ---------- Value display ---------- */

function ValueDisplay({
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
        {fmt(value)}
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

/* ---------- Decimal input ---------- */

function DecInput({
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

/* ---------- Preset button ---------- */

function PresetButton({
  label,
  value,
  onClick,
}: {
  label: string;
  value: string;
  onClick: (v: string) => void;
}) {
  return (
    <button
      onClick={() => onClick(value)}
      className="px-3 py-1.5 rounded-lg text-xs font-medium transition-all duration-150 hover:opacity-90 active:scale-95"
      style={{
        background: "rgba(79,143,255,0.1)",
        color: "#4f8fff",
        border: "1px solid rgba(79,143,255,0.2)",
      }}
    >
      {label}
    </button>
  );
}

/* ===================================================================
   Main Demo Page
   =================================================================== */

export default function DemoPage() {
  /* --- Guided tour state --- */
  const [tourStep, setTourStep] = useState(0); // 0 = inactive, 1-4 = steps

  /* --- Simulator state --- */
  const [reference, setReference] = useState<number>(0);
  const [accumulator, setAccumulator] = useState<number>(0);
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [nextId, setNextId] = useState(1);
  const [pulseField, setPulseField] = useState<"ref" | "acc" | "state" | null>(null);

  /* Input fields */
  const [loadInput, setLoadInput] = useState("1000");
  const [accumInput, setAccumInput] = useState("50");

  /* --- Live sync state --- */
  const [liveEnabled, setLiveEnabled] = useState(false);
  const [peerCount, setPeerCount] = useState(0);
  const wsRef = useRef<WebSocket | null>(null);

  const currentState = xor32(reference, accumulator);

  const historyEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    historyEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [history]);

  /* --- WebSocket connection for live sync --- */
  useEffect(() => {
    if (!liveEnabled) {
      if (wsRef.current) {
        wsRef.current.close();
        wsRef.current = null;
        setPeerCount(0);
      }
      return;
    }

    const ws = new WebSocket("wss://atomik-demo.matthewhrockwell.partykit.dev/party/lobby");
    wsRef.current = ws;

    ws.onmessage = (e) => {
      try {
        const msg = JSON.parse(e.data);
        if (msg.type === "peers") {
          setPeerCount(msg.count);
        } else if (msg.type === "delta") {
          setAccumulator((prev) => xor32(prev, msg.value));
          setHistory((prev) => [
            ...prev,
            {
              id: Date.now(),
              op: "ACCUM" as const,
              detail: `RECV from peer: applied delta ${fmt(msg.value)}`,
              resultRef: 0,
              resultAcc: 0,
            },
          ]);
        } else if (msg.type === "load") {
          setReference(msg.value >>> 0);
          setAccumulator(0);
          setHistory((prev) => [
            ...prev,
            {
              id: Date.now(),
              op: "LOAD" as const,
              detail: `RECV from peer: set reference to ${fmt(msg.value)}`,
              resultRef: msg.value >>> 0,
              resultAcc: 0,
            },
          ]);
        }
      } catch { /* ignore malformed */ }
    };

    ws.onclose = () => setPeerCount(0);

    return () => {
      ws.close();
      wsRef.current = null;
    };
  }, [liveEnabled]);

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
    const v = parseDec(loadInput);
    if (v === null) return;
    setReference(v);
    setAccumulator(0);
    record("LOAD", `Set reference to ${fmt(v)}`, v, 0);
    pulse("ref");
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ type: "load", value: v }));
    }
  }, [loadInput, record, pulse]);

  const doAccum = useCallback(() => {
    const v = parseDec(accumInput);
    if (v === null) return;
    const newAcc = xor32(accumulator, v);
    const newState = xor32(reference, newAcc);
    setAccumulator(newAcc);
    record("ACCUM", `Applied delta ${fmt(v)}, state now ${fmt(newState)}`, reference, newAcc);
    pulse("acc");
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ type: "delta", value: v }));
    }
  }, [accumInput, accumulator, reference, record, pulse]);

  const doRead = useCallback(() => {
    record("READ", `Current value: ${fmt(currentState)}`, reference, accumulator);
    pulse("state");
  }, [currentState, reference, accumulator, record, pulse]);

  const doSwap = useCallback(() => {
    const newRef = currentState;
    setReference(newRef);
    setAccumulator(0);
    record("SWAP", `Saved state ${fmt(newRef)} as new reference`, newRef, 0);
    pulse("ref");
  }, [currentState, record, pulse]);

  /* --- Preset handler --- */
  const applyPreset = useCallback((value: string) => {
    setLoadInput(value);
  }, []);

  /* ---- Key Properties Demo state ---- */

  const [commResult, setCommResult] = useState<{ a: number; b: number; base: number; ab: number; ba: number } | null>(null);
  const [selfInvResult, setSelfInvResult] = useState<{
    original: number;
    delta: number;
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
    setCommResult({ a, b, base, ab, ba });
  }, []);

  /* Self-inverse demo */
  const demoSelfInverse = useCallback(() => {
    const base = randomU32();
    const delta = randomU32();
    const afterOne = xor32(base, delta);
    const afterTwo = xor32(afterOne, delta);
    setSelfInvResult({ original: base, delta, afterOne, afterTwo });
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
          Imagine tracking a sensor reading, account balance, or game score. Try
          ATOMiK&apos;s four core operations below&nbsp;&mdash; same algebra, same
          rules, same results as the hardware.
        </p>
        {tourStep === 0 && (
          <button
            onClick={() => setTourStep(1)}
            className="mt-6 px-5 py-2.5 rounded-lg text-sm font-bold transition-all duration-150 hover:opacity-90 active:scale-95"
            style={{
              background: "rgba(79,143,255,0.15)",
              color: "#4f8fff",
              border: "1px solid rgba(79,143,255,0.3)",
            }}
          >
            Take the Tour
          </button>
        )}
      </section>

      {/* ======== Guided Tour Callout ======== */}
      {tourStep >= 1 && tourStep <= 4 && (
        <section className="max-w-3xl mx-auto px-6 pb-4">
          <div
            className="rounded-xl p-5"
            style={{
              background: "rgba(79,143,255,0.05)",
              border: "1px solid #4f8fff",
            }}
          >
            <div className="flex items-start justify-between gap-4 mb-3">
              <div className="flex items-center gap-2">
                <span
                  className="inline-flex items-center justify-center w-6 h-6 rounded-full text-xs font-bold"
                  style={{ background: "#4f8fff", color: "#fff" }}
                >
                  {tourStep}
                </span>
                <span className="text-xs font-medium uppercase tracking-wider" style={{ color: "#4f8fff" }}>
                  Step {tourStep} of 4
                </span>
              </div>
              <button
                onClick={() => setTourStep(0)}
                className="text-xs hover:underline shrink-0"
                style={{ color: "#8888a0" }}
              >
                Skip tour
              </button>
            </div>
            <p className="text-sm leading-relaxed mb-4" style={{ color: "#e0e0e8" }}>
              {tourStep === 1 &&
                "Enter a value (try 42) and click LOAD to set your initial state."}
              {tourStep === 2 &&
                "Now enter a delta value (try 7) and click ACCUM to XOR it with the accumulator."}
              {tourStep === 3 &&
                "Click READ to see the reconstructed state (initial XOR accumulator)."}
              {tourStep === 4 &&
                "Click SWAP to checkpoint \u2014 the state is saved and accumulator resets. That\u2019s delta-state algebra!"}
            </p>
            {tourStep < 4 ? (
              <button
                onClick={() => setTourStep((s) => s + 1)}
                className="px-4 py-1.5 rounded-lg text-xs font-semibold transition-all duration-150 hover:opacity-90"
                style={{
                  background: "#4f8fff",
                  color: "#fff",
                }}
              >
                Next &rarr;
              </button>
            ) : (
              <button
                onClick={() => setTourStep(5)}
                className="px-4 py-1.5 rounded-lg text-xs font-semibold transition-all duration-150 hover:opacity-90"
                style={{
                  background: "#22c55e",
                  color: "#fff",
                }}
              >
                Finish Tour
              </button>
            )}
          </div>
        </section>
      )}

      {/* ======== Tour Complete Message ======== */}
      {tourStep === 5 && (
        <section className="max-w-3xl mx-auto px-6 pb-4">
          <div
            className="rounded-xl p-5"
            style={{
              background: "rgba(34,197,94,0.05)",
              border: "1px solid rgba(34,197,94,0.3)",
            }}
          >
            <p className="text-sm leading-relaxed mb-2" style={{ color: "#22c55e", fontWeight: 600 }}>
              Tour complete!
            </p>
            <p className="text-sm leading-relaxed" style={{ color: "#b0b0c0" }}>
              You just performed the four fundamental ATOMiK operations. These same
              operations run on custom RISC-V hardware at 69.7 Gops/s.
            </p>
            <button
              onClick={() => setTourStep(0)}
              className="mt-3 text-xs hover:underline"
              style={{ color: "#8888a0" }}
            >
              Dismiss
            </button>
          </div>
        </section>
      )}

      {/* ======== Live Sync Toggle ======== */}
      <section className="max-w-3xl mx-auto px-6 pb-4">
        <div
          className="rounded-xl border p-4 flex items-center justify-between"
          style={{
            background: liveEnabled ? "rgba(34,197,94,0.05)" : "#12121a",
            borderColor: liveEnabled ? "rgba(34,197,94,0.3)" : "#1e1e2e",
          }}
        >
          <div className="flex items-center gap-3">
            <span
              className="inline-block w-2.5 h-2.5 rounded-full"
              style={{ background: liveEnabled && peerCount > 0 ? "#22c55e" : "#555" }}
            />
            <div>
              <span className="text-sm font-semibold">
                {liveEnabled ? "Live Sync Active" : "Live Sync"}
              </span>
              {liveEnabled && peerCount > 0 && (
                <span className="text-xs ml-2" style={{ color: "#22c55e" }}>
                  {peerCount} peer{peerCount !== 1 ? "s" : ""} connected
                </span>
              )}
              {!liveEnabled && (
                <span className="text-xs ml-2" style={{ color: "#8888a0" }}>
                  Open this page in two tabs to see convergence
                </span>
              )}
            </div>
          </div>
          <button
            onClick={() => setLiveEnabled((p) => !p)}
            className="px-4 py-1.5 rounded-lg text-xs font-semibold transition-colors"
            style={{
              background: liveEnabled ? "rgba(239,68,68,0.15)" : "rgba(34,197,94,0.15)",
              color: liveEnabled ? "#ef4444" : "#22c55e",
              border: `1px solid ${liveEnabled ? "rgba(239,68,68,0.3)" : "rgba(34,197,94,0.3)"}`,
            }}
          >
            {liveEnabled ? "Disconnect" : "Connect"}
          </button>
        </div>
      </section>

      {/* ======== Simulator ======== */}
      <section className="max-w-3xl mx-auto px-6 pb-16">
        <Card>
          {/* State display */}
          <div className="mb-6">
            <h2 className="text-sm font-semibold uppercase tracking-wider mb-4" style={{ color: "#8888a0" }}>
              ATOMiK Context State
            </h2>
            <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 text-center">
              <ValueDisplay
                label="reference"
                value={reference}
                color="#8b5cf6"
                pulse={pulseField === "ref"}
              />
              <ValueDisplay
                label="accumulator"
                value={accumulator}
                color="#4f8fff"
                pulse={pulseField === "acc"}
              />
              <ValueDisplay
                label="current state (ref XOR acc)"
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
              <span style={{ color: "#8b5cf6" }}>{fmt(reference)}</span>
              {" XOR "}
              <span style={{ color: "#4f8fff" }}>{fmt(accumulator)}</span>
              {" = "}
              <span style={{ color: "#22d3ee" }}>{fmt(currentState)}</span>
            </div>
          </div>

          {/* Preset buttons */}
          <div className="mb-4">
            <span className="text-xs font-medium uppercase tracking-wider mr-3" style={{ color: "#8888a0" }}>
              Presets
            </span>
            <div className="inline-flex flex-wrap gap-2">
              <PresetButton label="Sensor: 2,500" value="2500" onClick={applyPreset} />
              <PresetButton label="Balance: 10,000" value="10000" onClick={applyPreset} />
              <PresetButton label="Counter: 0" value="0" onClick={applyPreset} />
              <PresetButton label="Score: 500" value="500" onClick={applyPreset} />
            </div>
          </div>

          {/* Operations */}
          <div className="space-y-4">
            {/* LOAD */}
            <div className="flex flex-col sm:flex-row items-stretch sm:items-center gap-2">
              <OpButton label="LOAD" onClick={doLoad} color="#8b5cf6" />
              <DecInput value={loadInput} onChange={setLoadInput} placeholder="e.g. 1000 (starting balance)" />
              <span className="text-xs shrink-0" style={{ color: "#8888a0" }}>
                Set initial value, reset accumulator
              </span>
            </div>

            {/* ACCUM */}
            <div className="flex flex-col sm:flex-row items-stretch sm:items-center gap-2">
              <OpButton label="ACCUM" onClick={doAccum} color="#4f8fff" />
              <DecInput value={accumInput} onChange={setAccumInput} placeholder="e.g. 50 (a change/delta)" />
              <span className="text-xs shrink-0" style={{ color: "#8888a0" }}>
                Apply a change (XOR delta)
              </span>
            </div>

            {/* READ + SWAP */}
            <div className="flex flex-wrap gap-2">
              <OpButton label="READ" onClick={doRead} color="#22b8a6" />
              <OpButton label="SWAP" onClick={doSwap} color="#f59e0b" />
              <span className="text-xs self-center ml-2" style={{ color: "#8888a0" }}>
                READ shows current value &middot; SWAP checkpoints it as new reference
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
                      state={fmt(xor32(h.resultRef, h.resultAcc))}
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
                  A XOR B = B XOR A &mdash; order of accumulation doesn&apos;t matter.
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
                  <span style={{ color: "#22d3ee" }}>{fmt(commResult.ab)}</span>
                </div>
                <div>
                  <span style={{ color: "#8888a0" }}>B then A: </span>
                  <span style={{ color: "#22d3ee" }}>{fmt(commResult.ba)}</span>
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
                  A XOR A = 0 &mdash; every delta is its own undo.
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
                  <span style={{ color: "#8b5cf6" }}>{fmt(selfInvResult.original)}</span>
                </div>
                <div>
                  <span style={{ color: "#8888a0" }}>After XOR delta ({fmt(selfInvResult.delta)}): </span>
                  <span style={{ color: "#f59e0b" }}>{fmt(selfInvResult.afterOne)}</span>
                </div>
                <div>
                  <span style={{ color: "#8888a0" }}>After XOR same delta again: </span>
                  <span style={{ color: "#22d3ee" }}>{fmt(selfInvResult.afterTwo)}</span>
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
                    {orderResult.deltas.map(fmt).join(", ")}
                  </span>
                </div>
                <div>
                  <span style={{ color: "#8888a0" }}>Shuffled order: </span>
                  <span style={{ color: "#b0b0c0" }}>
                    {orderResult.shuffledOrder.map(fmt).join(", ")}
                  </span>
                </div>
                <div className="pt-1 flex flex-col sm:flex-row gap-4">
                  <div>
                    <span style={{ color: "#8888a0" }}>Result (ordered): </span>
                    <span style={{ color: "#22d3ee" }}>{fmt(orderResult.ordered)}</span>
                  </div>
                  <div>
                    <span style={{ color: "#8888a0" }}>Result (shuffled): </span>
                    <span style={{ color: "#22d3ee" }}>{fmt(orderResult.shuffled)}</span>
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
