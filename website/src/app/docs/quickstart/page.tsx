import type { Metadata } from "next";
import {
  codeBlockStyle,
  kwColor,
  fnColor,
  numColor,
  cmtColor,
  typeColor,
  varColor,
} from "../shared";

export const metadata: Metadata = {
  title: "Quick Start — ATOMiK Docs",
  description:
    "Install the ATOMiK Python SDK and run your first delta-state operation in under a minute.",
};

export default function QuickStartPage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#8b5cf6" }}
      >
        Getting Started
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">Quick Start</h1>
      <p className="text-lg mb-10" style={{ color: "#8888a0" }}>
        Install the Python SDK and run your first delta-state operation in under a minute.
      </p>

      {/* Install */}
      <div className="mb-6">
        <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
          Install
        </p>
        <div style={codeBlockStyle}>
          <code>
            <span style={{ color: cmtColor }}>$</span>{" "}
            <span style={{ color: varColor }}>pip install atomik-core</span>
          </code>
        </div>
      </div>

      {/* 4-operation example */}
      <div className="mb-6">
        <p className="text-sm font-mono mb-2" style={{ color: "#8888a0" }}>
          The four operations
        </p>
        <div style={codeBlockStyle}>
          <pre style={{ margin: 0 }}>
            <code>
              <span style={{ color: kwColor }}>from</span>{" "}
              <span style={{ color: varColor }}>atomik_core</span>{" "}
              <span style={{ color: kwColor }}>import</span>{" "}
              <span style={{ color: typeColor }}>AtomikContext</span>
              {"\n\n"}
              <span style={{ color: cmtColor }}>
                # Create a single delta-state context
              </span>
              {"\n"}
              <span style={{ color: varColor }}>ctx</span>{" "}
              <span style={{ color: kwColor }}>=</span>{" "}
              <span style={{ color: typeColor }}>AtomikContext</span>
              <span style={{ color: varColor }}>()</span>
              {"\n\n"}
              <span style={{ color: cmtColor }}>
                # LOAD — set the initial reference state
              </span>
              {"\n"}
              <span style={{ color: varColor }}>ctx.</span>
              <span style={{ color: fnColor }}>load</span>
              <span style={{ color: varColor }}>(</span>
              <span style={{ color: numColor }}>0xDEADBEEF</span>
              <span style={{ color: varColor }}>)</span>
              {"\n\n"}
              <span style={{ color: cmtColor }}>
                # ACCUM — XOR a delta into the accumulator
              </span>
              {"\n"}
              <span style={{ color: varColor }}>ctx.</span>
              <span style={{ color: fnColor }}>accum</span>
              <span style={{ color: varColor }}>(</span>
              <span style={{ color: numColor }}>0x000000FF</span>
              <span style={{ color: varColor }}>)</span>
              {"\n\n"}
              <span style={{ color: cmtColor }}>
                # READ — reconstruct current state (reference XOR accumulator)
              </span>
              {"\n"}
              <span style={{ color: kwColor }}>assert</span>{" "}
              <span style={{ color: varColor }}>ctx.</span>
              <span style={{ color: fnColor }}>read</span>
              <span style={{ color: varColor }}>() == </span>
              <span style={{ color: numColor }}>0xDEADBE10</span>
              {"\n\n"}
              <span style={{ color: cmtColor }}>
                # SWAP — atomic snapshot + reset accumulator
              </span>
              {"\n"}
              <span style={{ color: varColor }}>snapshot</span>{" "}
              <span style={{ color: kwColor }}>=</span>{" "}
              <span style={{ color: varColor }}>ctx.</span>
              <span style={{ color: fnColor }}>swap</span>
              <span style={{ color: varColor }}>()</span>
              {"\n"}
              <span style={{ color: kwColor }}>assert</span>{" "}
              <span style={{ color: varColor }}>snapshot == </span>
              <span style={{ color: numColor }}>0xDEADBE10</span>
              {"  "}
              <span style={{ color: cmtColor }}># previous state</span>
              {"\n"}
              <span style={{ color: kwColor }}>assert</span>{" "}
              <span style={{ color: varColor }}>ctx.</span>
              <span style={{ color: fnColor }}>read</span>
              <span style={{ color: varColor }}>() == </span>
              <span style={{ color: numColor }}>0xDEADBE10</span>
              {"    "}
              <span style={{ color: cmtColor }}># accumulator reset, state preserved</span>
            </code>
          </pre>
        </div>
      </div>

      {/* Key insight callout */}
      <div
        className="rounded-xl p-6 border"
        style={{
          background: "linear-gradient(135deg, rgba(139,92,246,0.06), rgba(79,143,255,0.06))",
          borderColor: "#8b5cf630",
        }}
      >
        <p className="text-sm font-semibold mb-2" style={{ color: "#8b5cf6" }}>
          Key insight
        </p>
        <p style={{ color: "#b0b0c0" }}>
          State is never stored — it is reconstructed:{" "}
          <code
            className="text-sm font-mono px-2 py-0.5 rounded"
            style={{ background: "#1e1e2e", color: "#22c55e" }}
          >
            current_state = initial_state &oplus; accumulator
          </code>
          . Deltas are commutative, associative, and self-inverse (XOR Abelian group).
          Order does not matter. Duplicate deltas cancel. Zero dependencies, Python 3.9+.
        </p>
      </div>

    </div>
  );
}
