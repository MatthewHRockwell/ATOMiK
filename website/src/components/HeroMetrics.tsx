"use client";

import { AnimatedCounter } from "./AnimatedCounter";

const metrics: { value: string; label: string; subtitle?: string; suffix?: string; color: string }[] = [
  { value: "99.9%", label: "Bandwidth Reduction", subtitle: "via delta compression \u2014 only 8-byte deltas transmitted instead of full state", color: "#22c55e" },
  { value: "92", label: "Lean4 Formal Proofs", color: "#8b5cf6" },
  { value: "69.7", label: "Gops/s Peak (FPGA)", suffix: " Gops/s", color: "#4f8fff" },
  { value: "5", label: "M ops/s (Python)", suffix: "M+", color: "#22d3ee" },
];

export default function HeroMetrics() {
  return (
    <div className="grid grid-cols-2 md:grid-cols-4 gap-4 max-w-3xl mx-auto">
      {metrics.map((m) => (
        <div
          key={m.label}
          className="rounded-xl px-4 py-4 text-center"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid #1e1e2e",
          }}
          title={m.subtitle}
        >
          <div className="text-2xl font-bold font-mono" style={{ color: m.color }}>
            <AnimatedCounter value={m.value} suffix={m.suffix} />
          </div>
          <div className="text-xs mt-1" style={{ color: "#8888a0" }}>
            {m.label}
          </div>
          {m.subtitle && (
            <div className="text-[10px] mt-1.5 leading-snug" style={{ color: "#666680" }}>
              {m.subtitle}
            </div>
          )}
        </div>
      ))}
    </div>
  );
}
