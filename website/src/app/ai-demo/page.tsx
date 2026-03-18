"use client";

import { useState, useEffect, useRef, useCallback } from "react";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

/* ═══════════════════════════════════════════════════════════════════════
   CONSTANTS & HELPERS
   ═══════════════════════════════════════════════════════════════════════ */

const MODEL_SIZES = [
  { label: "3B", power: { gpu: 150, atomik: 8 }, cost: { gpu: 1.6, atomik: 0.09 }, tps: 120 },
  { label: "7B", power: { gpu: 220, atomik: 12 }, cost: { gpu: 2.4, atomik: 0.14 }, tps: 100 },
  { label: "13B", power: { gpu: 320, atomik: 18 }, cost: { gpu: 3.8, atomik: 0.22 }, tps: 75 },
];

const SAMPLE_RESPONSES = [
  "A black hole is a region of spacetime where gravity is so intense that nothing, not even light or other electromagnetic waves, has enough energy to escape the event horizon.",
  "Quantum computing leverages quantum mechanical phenomena such as superposition and entanglement to process information in fundamentally different ways than classical computers.",
  "Neural networks are computational systems inspired by biological neural networks that learn to perform tasks by considering examples, without being explicitly programmed with task-specific rules.",
  "The transformer architecture revolutionized natural language processing by using self-attention mechanisms to process all tokens in a sequence simultaneously rather than sequentially.",
  "Delta-state algebra replaces full-state replication with O(1) reconstruction, reducing memory traffic by orders of magnitude while maintaining mathematically proven correctness.",
];

const SAMPLE_PROMPTS = [
  "Explain black holes in one sentence",
  "What is quantum computing?",
  "How do neural networks learn?",
  "What makes transformers special?",
  "Describe delta-state algebra",
];

function lerp(a: number, b: number, t: number): number {
  return a + (b - a) * t;
}

function formatPower(w: number): string {
  return `${w.toFixed(0)} W`;
}

function formatCost(c: number): string {
  return `$${c.toFixed(2)}`;
}

/* ═══════════════════════════════════════════════════════════════════════
   PANEL 1 — Live Inference Simulation
   ═══════════════════════════════════════════════════════════════════════ */

function InferencePanel({
  running,
  gpuTokens,
  atomikTokens,
  gpuPower,
  atomikPower,
  gpuTps,
  atomikTps,
  gpuCost,
  atomikCost,
  concurrency,
}: {
  running: boolean;
  gpuTokens: string;
  atomikTokens: string;
  gpuPower: number;
  atomikPower: number;
  gpuTps: number;
  atomikTps: number;
  gpuCost: number;
  atomikCost: number;
  concurrency: number;
}) {
  const queueDepth = running ? Math.min(concurrency * 0.3, 50) : 0;

  return (
    <div className="grid md:grid-cols-2 gap-4">
      {/* GPU Column */}
      <div className="rounded-xl border p-5 relative overflow-hidden" style={{ background: "#12121a", borderColor: "rgba(249,115,22,0.2)" }}>
        <div className="absolute top-0 left-0 right-0 h-[2px]" style={{ background: "linear-gradient(90deg, #f97316, #ef4444)" }} />
        <h3 className="text-sm font-bold uppercase tracking-wider mb-3" style={{ color: "#f97316" }}>
          Traditional GPU Architecture
        </h3>
        <div className="rounded-lg p-3 mb-3 min-h-[80px] text-sm leading-relaxed" style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", color: "#d4d4d8", fontFamily: "var(--font-geist-mono), monospace" }}>
          {gpuTokens || <span style={{ color: "#555" }}>Waiting for prompt...</span>}
          {running && <span className="inline-block w-2 h-4 ml-0.5 animate-pulse" style={{ background: "#f97316" }} />}
        </div>
        <div className="grid grid-cols-3 gap-2 text-center">
          <MetricBox label="Power" value={formatPower(gpuPower)} color="#f97316" />
          <MetricBox label="Throughput" value={`${gpuTps.toFixed(0)} tok/s`} color="#f97316" />
          <MetricBox label="Cost/1M tok" value={formatCost(gpuCost)} color="#f97316" />
        </div>
        {queueDepth > 5 && (
          <div className="mt-2 text-xs px-2 py-1 rounded" style={{ background: "rgba(239,68,68,0.1)", color: "#ef4444", border: "1px solid rgba(239,68,68,0.2)" }}>
            Queue depth: {queueDepth.toFixed(0)} requests waiting
          </div>
        )}
      </div>

      {/* ATOMiK Column */}
      <div className="rounded-xl border p-5 relative overflow-hidden" style={{ background: "#12121a", borderColor: "rgba(79,143,255,0.2)" }}>
        <div className="absolute top-0 left-0 right-0 h-[2px]" style={{ background: "linear-gradient(90deg, #4f8fff, #8b5cf6)" }} />
        <h3 className="text-sm font-bold uppercase tracking-wider mb-3" style={{ color: "#4f8fff" }}>
          ATOMiK Architecture
        </h3>
        <div className="rounded-lg p-3 mb-3 min-h-[80px] text-sm leading-relaxed" style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", color: "#d4d4d8", fontFamily: "var(--font-geist-mono), monospace" }}>
          {atomikTokens || <span style={{ color: "#555" }}>Waiting for prompt...</span>}
          {running && <span className="inline-block w-2 h-4 ml-0.5 animate-pulse" style={{ background: "#4f8fff" }} />}
        </div>
        <div className="grid grid-cols-3 gap-2 text-center">
          <MetricBox label="Power" value={formatPower(atomikPower)} color="#4f8fff" />
          <MetricBox label="Throughput" value={`${atomikTps.toFixed(0)} tok/s`} color="#4f8fff" />
          <MetricBox label="Cost/1M tok" value={formatCost(atomikCost)} color="#4f8fff" />
        </div>
      </div>
    </div>
  );
}

function MetricBox({ label, value, color }: { label: string; value: string; color: string }) {
  return (
    <div className="rounded-lg p-2" style={{ background: "rgba(255,255,255,0.02)", border: "1px solid #1e1e2e" }}>
      <div className="text-[10px] uppercase tracking-wider mb-0.5" style={{ color: "#666" }}>{label}</div>
      <div className="text-sm font-bold font-mono" style={{ color }}>{value}</div>
    </div>
  );
}

/* ═══════════════════════════════════════════════════════════════════════
   PANEL 2 — Real-Time Power Visualization
   ═══════════════════════════════════════════════════════════════════════ */

function PowerPanel({ gpuPower, atomikPower, running }: { gpuPower: number; atomikPower: number; running: boolean }) {
  const maxPower = 400;
  const gpuPct = (gpuPower / maxPower) * 100;
  const atomikPct = (atomikPower / maxPower) * 100;
  const ratio = gpuPower / Math.max(atomikPower, 1);

  return (
    <div className="rounded-xl border p-6" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
      <h3 className="text-center text-lg font-bold mb-1">
        Same AI Output.{" "}
        <span className="bg-clip-text text-transparent" style={{ backgroundImage: "linear-gradient(135deg, #4f8fff, #22d3ee)" }}>
          Fraction of the Energy.
        </span>
      </h3>
      <p className="text-center text-xs mb-6" style={{ color: "#666" }}>
        {ratio.toFixed(0)}x power reduction — same throughput, same output quality
      </p>

      <div className="space-y-4">
        {/* GPU bar */}
        <div>
          <div className="flex justify-between text-xs mb-1">
            <span style={{ color: "#f97316" }}>GPU Inference Power Draw</span>
            <span className="font-mono font-bold" style={{ color: "#f97316" }}>{gpuPower.toFixed(0)} W</span>
          </div>
          <div className="h-8 rounded-full overflow-hidden" style={{ background: "#1a1a24" }}>
            <div
              className="h-full rounded-full transition-all duration-700 ease-out relative"
              style={{
                width: `${gpuPct}%`,
                background: "linear-gradient(90deg, #f97316, #ef4444)",
                boxShadow: running ? "0 0 20px rgba(249,115,22,0.4)" : "none",
              }}
            >
              {running && (
                <div className="absolute inset-0 rounded-full animate-pulse" style={{ background: "linear-gradient(90deg, transparent, rgba(255,255,255,0.1), transparent)" }} />
              )}
            </div>
          </div>
        </div>

        {/* ATOMiK bar */}
        <div>
          <div className="flex justify-between text-xs mb-1">
            <span style={{ color: "#4f8fff" }}>ATOMiK Inference Power Draw</span>
            <span className="font-mono font-bold" style={{ color: "#4f8fff" }}>{atomikPower.toFixed(0)} W</span>
          </div>
          <div className="h-8 rounded-full overflow-hidden" style={{ background: "#1a1a24" }}>
            <div
              className="h-full rounded-full transition-all duration-700 ease-out relative"
              style={{
                width: `${Math.max(atomikPct, 2)}%`,
                background: "linear-gradient(90deg, #4f8fff, #8b5cf6)",
                boxShadow: running ? "0 0 20px rgba(79,143,255,0.4)" : "none",
              }}
            >
              {running && (
                <div className="absolute inset-0 rounded-full animate-pulse" style={{ background: "linear-gradient(90deg, transparent, rgba(255,255,255,0.15), transparent)" }} />
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

/* ═══════════════════════════════════════════════════════════════════════
   PANEL 3 — Architecture Visualization
   ═══════════════════════════════════════════════════════════════════════ */

function ArchitecturePanel({ running }: { running: boolean }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const frameRef = useRef(0);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    let animId: number;
    const W = canvas.width;
    const H = canvas.height;
    const midX = W / 2;

    function draw() {
      if (!ctx) return;
      const t = frameRef.current;
      frameRef.current += 1;

      ctx.clearRect(0, 0, W, H);

      // ── Left: Traditional Architecture ──
      ctx.fillStyle = "#888";
      ctx.font = "11px system-ui";
      ctx.textAlign = "center";
      ctx.fillText("Traditional Compute", midX / 2, 18);

      const blocks = ["Memory", "Cache", "Compute", "Memory"];
      const blockY = [40, 90, 140, 190];
      const bw = 90, bh = 32;

      for (let i = 0; i < blocks.length; i++) {
        const x = midX / 2 - bw / 2;
        ctx.fillStyle = "#1e1e2e";
        ctx.strokeStyle = "#f97316";
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.roundRect(x, blockY[i], bw, bh, 4);
        ctx.fill();
        ctx.stroke();
        ctx.fillStyle = "#ccc";
        ctx.font = "10px system-ui";
        ctx.fillText(blocks[i], midX / 2, blockY[i] + 20);
      }

      // Animate data blocks moving between layers
      if (running) {
        for (let i = 0; i < 3; i++) {
          const progress = ((t * 2 + i * 40) % 120) / 120;
          const fromY = blockY[i] + bh;
          const toY = blockY[i + 1];
          const y = fromY + (toY - fromY) * progress;
          const size = 12 + Math.sin(progress * Math.PI) * 4;
          ctx.fillStyle = `rgba(249, 115, 22, ${0.3 + 0.7 * Math.sin(progress * Math.PI)})`;
          ctx.fillRect(midX / 2 - size / 2, y, size, size * 0.6);
        }
      }

      // ── Divider ──
      ctx.strokeStyle = "#333";
      ctx.setLineDash([4, 4]);
      ctx.beginPath();
      ctx.moveTo(midX, 0);
      ctx.lineTo(midX, H);
      ctx.stroke();
      ctx.setLineDash([]);

      // ── Right: ATOMiK Architecture ──
      ctx.fillStyle = "#888";
      ctx.font = "11px system-ui";
      ctx.textAlign = "center";
      ctx.fillText("ATOMiK Architecture", midX + midX / 2, 18);

      // Draw register grid
      const gridX = midX + midX / 2 - 60;
      const gridY = 40;
      const cellSize = 15;
      const cols = 8, rows = 10;

      for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
          const cx = gridX + c * cellSize;
          const cy = gridY + r * cellSize;

          // Delta propagation animation
          let brightness = 0.1;
          if (running) {
            const wave = Math.sin((t * 0.05) - (r + c) * 0.5);
            const ripple = Math.sin((t * 0.03) + (r * c) * 0.2);
            brightness = 0.1 + Math.max(0, wave * 0.3 + ripple * 0.2);
          }

          ctx.fillStyle = `rgba(79, 143, 255, ${brightness})`;
          ctx.strokeStyle = "rgba(79, 143, 255, 0.15)";
          ctx.lineWidth = 0.5;
          ctx.beginPath();
          ctx.roundRect(cx, cy, cellSize - 2, cellSize - 2, 2);
          ctx.fill();
          ctx.stroke();

          // Delta pulse
          if (running) {
            const pulsePhase = ((t + r * 7 + c * 11) % 60) / 60;
            if (pulsePhase < 0.3) {
              const alpha = (0.3 - pulsePhase) / 0.3;
              ctx.fillStyle = `rgba(139, 92, 246, ${alpha * 0.8})`;
              ctx.fillRect(cx + 2, cy + 2, cellSize - 6, cellSize - 6);
            }
          }
        }
      }

      // Labels
      if (running) {
        ctx.fillStyle = "rgba(139, 92, 246, 0.8)";
        ctx.font = "9px system-ui";
        ctx.fillText("Δ State Updates", midX + midX / 2, gridY + rows * cellSize + 16);
      }

      animId = requestAnimationFrame(draw);
    }

    draw();
    return () => cancelAnimationFrame(animId);
  }, [running]);

  return (
    <div className="rounded-xl border p-6" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
      <canvas
        ref={canvasRef}
        width={640}
        height={230}
        className="w-full"
        style={{ maxWidth: 640, margin: "0 auto", display: "block" }}
      />
      <p className="text-center text-xs mt-4 max-w-lg mx-auto" style={{ color: "#8888a0" }}>
        Instead of moving massive tensors through memory hierarchies, ATOMiK evolves
        system state locally using delta propagation.
      </p>
    </div>
  );
}

/* ═══════════════════════════════════════════════════════════════════════
   MAIN PAGE
   ═══════════════════════════════════════════════════════════════════════ */

export default function AIDemoPage() {
  /* ── Controls ── */
  const [modelIdx, setModelIdx] = useState(1); // 0=3B, 1=7B, 2=13B
  const [concurrency, setConcurrency] = useState(100);
  const [prompt, setPrompt] = useState(SAMPLE_PROMPTS[0]);
  const [running, setRunning] = useState(false);

  /* ── Inference state ── */
  const [gpuTokens, setGpuTokens] = useState("");
  const [atomikTokens, setAtomikTokens] = useState("");
  const [tokenIdx, setTokenIdx] = useState(0);
  const responseRef = useRef("");
  const intervalRef = useRef<ReturnType<typeof setInterval> | null>(null);

  /* ── Animated metrics ── */
  const [gpuPower, setGpuPower] = useState(0);
  const [atomikPower, setAtomikPower] = useState(0);
  const [gpuTps, setGpuTps] = useState(0);
  const [atomikTps, setAtomikTps] = useState(0);
  const [gpuCost, setGpuCost] = useState(0);
  const [atomikCost, setAtomikCost] = useState(0);

  const model = MODEL_SIZES[modelIdx];

  /* ── Concurrency scaling ── */
  const concurrencyScale = Math.log10(Math.max(concurrency, 1)) / Math.log10(1000);
  const gpuPowerTarget = model.power.gpu * (1 + concurrencyScale * 0.5);
  const atomikPowerTarget = model.power.atomik * (1 + concurrencyScale * 0.15);
  const gpuCostTarget = model.cost.gpu * (1 + concurrencyScale * 0.8);
  const atomikCostTarget = model.cost.atomik * (1 + concurrencyScale * 0.1);
  const gpuTpsTarget = running ? model.tps * (1 - concurrencyScale * 0.3) : 0;
  const atomikTpsTarget = running ? model.tps : 0;

  /* ── Smooth metric animation ── */
  useEffect(() => {
    const id = setInterval(() => {
      setGpuPower((p) => lerp(p, running ? gpuPowerTarget : 0, 0.08));
      setAtomikPower((p) => lerp(p, running ? atomikPowerTarget : 0, 0.08));
      setGpuTps((p) => lerp(p, gpuTpsTarget, 0.1));
      setAtomikTps((p) => lerp(p, atomikTpsTarget, 0.1));
      setGpuCost((p) => lerp(p, running ? gpuCostTarget : 0, 0.08));
      setAtomikCost((p) => lerp(p, running ? atomikCostTarget : 0, 0.08));
    }, 50);
    return () => clearInterval(id);
  }, [running, gpuPowerTarget, atomikPowerTarget, gpuTpsTarget, atomikTpsTarget, gpuCostTarget, atomikCostTarget]);

  /* ── Token streaming ── */
  const startInference = useCallback(() => {
    if (intervalRef.current) clearInterval(intervalRef.current);

    const response = SAMPLE_RESPONSES[Math.floor(Math.random() * SAMPLE_RESPONSES.length)];
    responseRef.current = response;
    const words = response.split(" ");

    setGpuTokens("");
    setAtomikTokens("");
    setTokenIdx(0);
    setRunning(true);

    let idx = 0;
    intervalRef.current = setInterval(() => {
      if (idx >= words.length) {
        if (intervalRef.current) clearInterval(intervalRef.current);
        intervalRef.current = null;
        // Keep running state for a moment to show final metrics
        setTimeout(() => setRunning(false), 2000);
        return;
      }
      const word = words[idx] + " ";
      setGpuTokens((prev) => prev + word);
      // ATOMiK produces identical output (slight delay for visual effect)
      setTimeout(() => {
        setAtomikTokens((prev) => prev + word);
      }, 20);
      idx++;
      setTokenIdx(idx);
    }, 80 + Math.random() * 40); // ~100 tokens/sec feel
  }, []);

  const stopInference = useCallback(() => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
    setRunning(false);
  }, []);

  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      {/* ── Hero ── */}
      <section className="text-center px-6 pt-16 pb-8">
        <div className="text-xs uppercase tracking-widest mb-3 font-mono" style={{ color: "#8b5cf6" }}>
          Interactive Demo
        </div>
        <h1 className="text-3xl md:text-4xl font-bold tracking-tight mb-3">
          AI Inference:{" "}
          <span className="bg-clip-text text-transparent" style={{ backgroundImage: "linear-gradient(135deg, #f97316, #ef4444)" }}>
            Conventional
          </span>
          {" "}vs{" "}
          <span className="bg-clip-text text-transparent" style={{ backgroundImage: "linear-gradient(135deg, #4f8fff, #8b5cf6)" }}>
            ATOMiK
          </span>
          {" "}
          <span
            className="inline-block align-middle rounded-full px-2.5 py-0.5 text-xs font-semibold"
            style={{ background: "rgba(245,158,11,0.15)", color: "#f59e0b" }}
          >
            Research Preview
          </span>
        </h1>
        <p style={{ color: "#8888a0" }}>
          Same AI output. Fraction of the energy. Explore the architectural advantage.
        </p>
      </section>

      {/* ── Disclaimer Banner ── */}
      <section className="max-w-4xl mx-auto px-6 pb-6">
        <div
          className="flex items-start gap-3 rounded-xl px-5 py-4 text-xs leading-relaxed"
          style={{ background: "rgba(79,143,255,0.06)", border: "1px solid rgba(79,143,255,0.12)", color: "#8888a0" }}
        >
          <span className="shrink-0 mt-0.5 text-sm" style={{ color: "#4f8fff" }}>&#9432;</span>
          <span>
            This demonstration illustrates ATOMiK&apos;s architectural advantages for AI inference
            workloads. Performance projections are based on validated hardware benchmarks
            (69.7 Gops/s on Zynq XC7Z020) extrapolated to inference scenarios. Production AI
            inference benchmarks are in active development.{" "}
            <Link href="/about/roadmap" className="underline hover:text-white transition-colors" style={{ color: "#4f8fff" }}>
              Track progress on our roadmap
            </Link>.
          </span>
        </div>
      </section>

      {/* ── Controls ── */}
      <section className="max-w-4xl mx-auto px-6 pb-6">
        <div className="rounded-xl border p-5 grid md:grid-cols-2 gap-6" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
          {/* Model Size */}
          <div>
            <label className="text-xs uppercase tracking-wider block mb-2" style={{ color: "#8888a0" }}>
              Model Size
            </label>
            <div className="flex gap-2">
              {MODEL_SIZES.map((m, i) => (
                <button
                  key={m.label}
                  onClick={() => setModelIdx(i)}
                  className="flex-1 py-2 rounded-lg text-sm font-semibold transition-all"
                  style={{
                    background: modelIdx === i ? "rgba(79,143,255,0.15)" : "rgba(255,255,255,0.03)",
                    color: modelIdx === i ? "#4f8fff" : "#666",
                    border: `1px solid ${modelIdx === i ? "rgba(79,143,255,0.3)" : "#1e1e2e"}`,
                  }}
                >
                  {m.label}
                </button>
              ))}
            </div>
          </div>

          {/* Concurrency */}
          <div>
            <label className="text-xs uppercase tracking-wider block mb-2" style={{ color: "#8888a0" }}>
              Concurrency: {concurrency} req/s
            </label>
            <input
              type="range"
              min={10}
              max={1000}
              step={10}
              value={concurrency}
              onChange={(e) => setConcurrency(Number(e.target.value))}
              className="w-full h-2 rounded-full appearance-none cursor-pointer"
              style={{ background: `linear-gradient(90deg, #4f8fff ${((concurrency - 10) / 990) * 100}%, #1e1e2e ${((concurrency - 10) / 990) * 100}%)` }}
            />
            <div className="flex justify-between text-[10px] mt-1" style={{ color: "#555" }}>
              <span>10</span><span>100</span><span>1000</span>
            </div>
          </div>
        </div>
      </section>

      {/* ── Prompt Input ── */}
      <section className="max-w-4xl mx-auto px-6 pb-6">
        <div className="flex gap-3">
          <input
            type="text"
            value={prompt}
            onChange={(e) => setPrompt(e.target.value)}
            placeholder="Enter a prompt..."
            className="flex-1 px-4 py-3 rounded-xl text-sm outline-none focus:ring-2 focus:ring-[#4f8fff]"
            style={{ background: "#12121a", border: "1px solid #1e1e2e", color: "#e0e0e8" }}
            onKeyDown={(e) => e.key === "Enter" && !running && startInference()}
          />
          <button
            onClick={running ? stopInference : startInference}
            className="px-8 py-3 rounded-xl text-sm font-bold transition-all"
            style={{
              background: running
                ? "rgba(239,68,68,0.15)"
                : "linear-gradient(135deg, #4f8fff, #8b5cf6)",
              color: running ? "#ef4444" : "#fff",
              border: running ? "1px solid rgba(239,68,68,0.3)" : "none",
            }}
          >
            {running ? "Stop" : "Run Inference"}
          </button>
        </div>
        <div className="flex gap-2 mt-2 overflow-x-auto">
          {SAMPLE_PROMPTS.map((p) => (
            <button
              key={p}
              onClick={() => setPrompt(p)}
              className="shrink-0 px-3 py-1 rounded-full text-xs transition-colors"
              style={{
                background: prompt === p ? "rgba(79,143,255,0.1)" : "transparent",
                color: prompt === p ? "#4f8fff" : "#555",
                border: `1px solid ${prompt === p ? "rgba(79,143,255,0.2)" : "#1e1e2e"}`,
              }}
            >
              {p}
            </button>
          ))}
        </div>
      </section>

      {/* ── Panel 1: Inference Simulation ── */}
      <section className="max-w-4xl mx-auto px-6 pb-6">
        <InferencePanel
          running={running}
          gpuTokens={gpuTokens}
          atomikTokens={atomikTokens}
          gpuPower={gpuPower}
          atomikPower={atomikPower}
          gpuTps={gpuTps}
          atomikTps={atomikTps}
          gpuCost={gpuCost}
          atomikCost={atomikCost}
          concurrency={concurrency}
        />
      </section>

      {/* ── Panel 2: Power Visualization ── */}
      <section className="max-w-4xl mx-auto px-6 pb-6">
        <PowerPanel gpuPower={gpuPower} atomikPower={atomikPower} running={running} />
      </section>

      {/* ── Panel 3: Architecture Visualization ── */}
      <section className="max-w-4xl mx-auto px-6 pb-12">
        <ArchitecturePanel running={running} />
      </section>

      {/* ── Methodology ── */}
      <section className="max-w-4xl mx-auto px-6 pb-8">
        <div className="rounded-xl border p-6" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
          <h3 className="text-sm font-bold uppercase tracking-wider mb-4" style={{ color: "#8888a0" }}>
            Methodology
          </h3>
          <ul className="space-y-3 text-xs leading-relaxed" style={{ color: "#8888a0" }}>
            <li className="flex items-start gap-2">
              <span className="shrink-0 mt-0.5" style={{ color: "#f97316" }}>&#9632;</span>
              <span>
                <strong style={{ color: "#b0b0c0" }}>Traditional GPU numbers</strong> are based on
                published TDP specifications for NVIDIA A100 (300W) and H100 (350W) in inference
                configurations. Actual power draw varies by batch size, model, and cooling.
              </span>
            </li>
            <li className="flex items-start gap-2">
              <span className="shrink-0 mt-0.5" style={{ color: "#4f8fff" }}>&#9632;</span>
              <span>
                <strong style={{ color: "#b0b0c0" }}>ATOMiK numbers</strong> are based on measured
                FPGA power consumption (Tang Nano 9K: 0.5W, Zynq XC7Z020: estimated 5-20W for
                inference-class workloads) extrapolated to inference scenarios using architectural
                analysis of data movement reduction.
              </span>
            </li>
            <li className="flex items-start gap-2">
              <span className="shrink-0 mt-0.5" style={{ color: "#8b5cf6" }}>&#9632;</span>
              <span>
                <strong style={{ color: "#b0b0c0" }}>These are architectural projections, not production benchmarks.</strong>{" "}
                ATOMiK&apos;s delta-state algebra is proven for state management workloads.
                Application to full AI inference pipelines is under active development and has not
                been independently validated.
              </span>
            </li>
          </ul>
        </div>
      </section>

      {/* ── Key Takeaways ── */}
      <section className="max-w-4xl mx-auto px-6 pb-12">
        <div className="grid md:grid-cols-3 gap-4">
          {[
            { icon: "=", title: "Same Output", desc: "Identical AI responses, token by token. No quality compromise.", color: "#22c55e" },
            { icon: `${(gpuPowerTarget / Math.max(atomikPowerTarget, 1)).toFixed(0)}x`, title: "Less Power", desc: "Delta propagation eliminates redundant memory movement.", color: "#4f8fff" },
            { icon: "Δ", title: "Delta Architecture", desc: "State evolves locally instead of being copied globally.", color: "#8b5cf6" },
          ].map((t) => (
            <div key={t.title} className="rounded-xl border p-5 text-center" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
              <div className="text-3xl font-bold mb-2 font-mono" style={{ color: t.color }}>{t.icon}</div>
              <div className="font-semibold mb-1">{t.title}</div>
              <div className="text-xs" style={{ color: "#8888a0" }}>{t.desc}</div>
            </div>
          ))}
        </div>
      </section>

      {/* ── Notify Me ── */}
      <section className="max-w-xl mx-auto px-6 pb-12">
        <p className="text-center text-sm font-semibold mb-3" style={{ color: "#8888a0" }}>
          Get notified when AI inference benchmarks are available
        </p>
        <EmailCapture />
      </section>

      {/* ── CTA ── */}
      <section className="max-w-3xl mx-auto px-6 pb-20 text-center">
        <div className="flex flex-wrap justify-center gap-4">
          <Link href="/get-started" className="px-8 py-3 rounded-xl text-sm font-bold" style={{ background: "linear-gradient(135deg, #4f8fff, #8b5cf6)", color: "#fff" }}>
            Get Started
          </Link>
          <Link href="/whitepaper" className="px-8 py-3 rounded-xl text-sm font-bold" style={{ background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" }}>
            Read the White Paper
          </Link>
          <Link href="/pricing" className="px-8 py-3 rounded-xl text-sm font-bold" style={{ background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" }}>
            View Pricing
          </Link>
        </div>
      </section>
    </div>
  );
}
