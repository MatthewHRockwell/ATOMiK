"use client";

import { useState } from "react";

const codeBlockStyle: React.CSSProperties = {
  background: "#0d0d14",
  border: "1px solid #1e1e2e",
  borderRadius: "0.75rem",
  padding: "1.25rem",
  overflowX: "auto",
  fontSize: "0.8125rem",
  lineHeight: "1.7",
  fontFamily: "var(--font-mono), ui-monospace, monospace",
};

const cmtColor = "#555570";
const kwColor = "#c084fc";
const fnColor = "#4f8fff";
const strColor = "#22c55e";
const numColor = "#d4a843";
const typeColor = "#38bdf8";
const varColor = "#e0e0e8";

/* ── Method data types ─────────────────────────────────────────────── */

interface MethodDoc {
  name: string;
  signatures: Record<string, React.ReactNode>;
  description: string;
  returnValue: string;
  examples: Record<string, React.ReactNode>;
}

/* ── Method definitions ────────────────────────────────────────────── */

const methods: MethodDoc[] = [
  {
    name: "Context / Create",
    description:
      "Create a new ATOMiK context. A context holds a reference state and an accumulator, providing the four core delta-state operations.",
    returnValue: "A new context instance with reference = 0 and accumulator = 0.",
    signatures: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx</span>
          <span style={{ color: kwColor }}> = </span>
          <span style={{ color: typeColor }}>AtomikContext</span>
          <span style={{ color: varColor }}>()</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: typeColor }}>atomik_ctx_t</span>
          <span style={{ color: varColor }}> *ctx = </span>
          <span style={{ color: fnColor }}>atomik_context_create</span>
          <span style={{ color: varColor }}>();</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: kwColor }}>const</span>
          <span style={{ color: varColor }}> ctx = </span>
          <span style={{ color: kwColor }}>new</span>{" "}
          <span style={{ color: typeColor }}>AtomikContext</span>
          <span style={{ color: varColor }}>();</span>
        </code>
      ),
    },
    examples: {
      Python: (
        <code>
          <span style={{ color: kwColor }}>from</span>
          <span style={{ color: varColor }}> atomik_core </span>
          <span style={{ color: kwColor }}>import</span>
          <span style={{ color: typeColor }}> AtomikContext</span>
          {"\n\n"}
          <span style={{ color: varColor }}>ctx = </span>
          <span style={{ color: typeColor }}>AtomikContext</span>
          <span style={{ color: varColor }}>()</span>
          {"\n"}
          <span style={{ color: cmtColor }}># ctx.reference == 0, ctx.accumulator == 0</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: kwColor }}>#include</span>{" "}
          <span style={{ color: strColor }}>&quot;atomik_core.h&quot;</span>
          {"\n\n"}
          <span style={{ color: typeColor }}>atomik_ctx_t</span>
          <span style={{ color: varColor }}> *ctx = </span>
          <span style={{ color: fnColor }}>atomik_context_create</span>
          <span style={{ color: varColor }}>();</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// ctx-&gt;reference == 0, ctx-&gt;accumulator == 0</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: kwColor }}>import</span>
          <span style={{ color: varColor }}> {"{"} </span>
          <span style={{ color: typeColor }}>AtomikContext</span>
          <span style={{ color: varColor }}> {"}"} </span>
          <span style={{ color: kwColor }}>from</span>
          <span style={{ color: strColor }}> &quot;atomik-core&quot;</span>
          <span style={{ color: varColor }}>;</span>
          {"\n\n"}
          <span style={{ color: kwColor }}>const</span>
          <span style={{ color: varColor }}> ctx = </span>
          <span style={{ color: kwColor }}>new</span>{" "}
          <span style={{ color: typeColor }}>AtomikContext</span>
          <span style={{ color: varColor }}>();</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// ctx.reference === 0, ctx.accumulator === 0</span>
        </code>
      ),
    },
  },
  {
    name: "load",
    description:
      "Set the reference state to the given value and reset the accumulator to zero. This establishes the baseline from which all subsequent deltas are measured.",
    returnValue: "None. The context is mutated in place.",
    signatures: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: varColor }}>value</span>
          <span style={{ color: varColor }}>: </span>
          <span style={{ color: typeColor }}>int</span>
          <span style={{ color: varColor }}>) -&gt; </span>
          <span style={{ color: typeColor }}>None</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: typeColor }}>void</span>{" "}
          <span style={{ color: fnColor }}>atomik_load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: typeColor }}>atomik_ctx_t</span>
          <span style={{ color: varColor }}> *ctx, </span>
          <span style={{ color: typeColor }}>uint64_t</span>
          <span style={{ color: varColor }}> value);</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(value: </span>
          <span style={{ color: typeColor }}>number</span>
          <span style={{ color: varColor }}>): </span>
          <span style={{ color: typeColor }}>void</span>
        </code>
      ),
    },
    examples: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>0xCAFEBABE</span>
          <span style={{ color: varColor }}>)</span>
          {"\n"}
          <span style={{ color: cmtColor }}># reference = 0xCAFEBABE, accumulator = 0</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: fnColor }}>atomik_load</span>
          <span style={{ color: varColor }}>(ctx, </span>
          <span style={{ color: numColor }}>0xCAFEBABE</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// ctx-&gt;reference == 0xCAFEBABE, ctx-&gt;accumulator == 0</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>0xCAFEBABE</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// ctx.reference === 0xCAFEBABE, ctx.accumulator === 0</span>
        </code>
      ),
    },
  },
  {
    name: "accum",
    description:
      "XOR a delta value into the accumulator. This records a state change without modifying the reference. Multiple accum calls compose: accum(a); accum(b) is equivalent to accum(a XOR b).",
    returnValue: "None. The accumulator is updated in place via XOR.",
    signatures: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: varColor }}>delta</span>
          <span style={{ color: varColor }}>: </span>
          <span style={{ color: typeColor }}>int</span>
          <span style={{ color: varColor }}>) -&gt; </span>
          <span style={{ color: typeColor }}>None</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: typeColor }}>void</span>{" "}
          <span style={{ color: fnColor }}>atomik_accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: typeColor }}>atomik_ctx_t</span>
          <span style={{ color: varColor }}> *ctx, </span>
          <span style={{ color: typeColor }}>uint64_t</span>
          <span style={{ color: varColor }}> delta);</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(delta: </span>
          <span style={{ color: typeColor }}>number</span>
          <span style={{ color: varColor }}>): </span>
          <span style={{ color: typeColor }}>void</span>
        </code>
      ),
    },
    examples: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>100</span>
          <span style={{ color: varColor }}>)</span>
          {"\n"}
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>7</span>
          <span style={{ color: varColor }}>)</span>
          {"   "}
          <span style={{ color: cmtColor }}># accumulator ^= 7</span>
          {"\n"}
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>3</span>
          <span style={{ color: varColor }}>)</span>
          {"   "}
          <span style={{ color: cmtColor }}># accumulator ^= 3 (same as single accum(7^3))</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: fnColor }}>atomik_load</span>
          <span style={{ color: varColor }}>(ctx, </span>
          <span style={{ color: numColor }}>100</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: fnColor }}>atomik_accum</span>
          <span style={{ color: varColor }}>(ctx, </span>
          <span style={{ color: numColor }}>7</span>
          <span style={{ color: varColor }}>);</span>
          {"  "}
          <span style={{ color: cmtColor }}>// accumulator ^= 7</span>
          {"\n"}
          <span style={{ color: fnColor }}>atomik_accum</span>
          <span style={{ color: varColor }}>(ctx, </span>
          <span style={{ color: numColor }}>3</span>
          <span style={{ color: varColor }}>);</span>
          {"  "}
          <span style={{ color: cmtColor }}>// accumulator ^= 3</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>100</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>7</span>
          <span style={{ color: varColor }}>);</span>
          {"  "}
          <span style={{ color: cmtColor }}>// accumulator ^= 7</span>
          {"\n"}
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>3</span>
          <span style={{ color: varColor }}>);</span>
          {"  "}
          <span style={{ color: cmtColor }}>// accumulator ^= 3</span>
        </code>
      ),
    },
  },
  {
    name: "read",
    description:
      "Reconstruct the current state by XOR-ing the reference with the accumulator. This is a pure computation: current_state = reference XOR accumulator. The context is not modified.",
    returnValue: "The reconstructed state value (reference XOR accumulator).",
    signatures: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>read</span>
          <span style={{ color: varColor }}>() -&gt; </span>
          <span style={{ color: typeColor }}>int</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: typeColor }}>uint64_t</span>{" "}
          <span style={{ color: fnColor }}>atomik_read</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: typeColor }}>atomik_ctx_t</span>
          <span style={{ color: varColor }}> *ctx);</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>read</span>
          <span style={{ color: varColor }}>(): </span>
          <span style={{ color: typeColor }}>number</span>
        </code>
      ),
    },
    examples: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>0xFF00</span>
          <span style={{ color: varColor }}>)</span>
          {"\n"}
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>0x00FF</span>
          <span style={{ color: varColor }}>)</span>
          {"\n"}
          <span style={{ color: varColor }}>state = ctx.</span>
          <span style={{ color: fnColor }}>read</span>
          <span style={{ color: varColor }}>()</span>
          {"\n"}
          <span style={{ color: kwColor }}>assert</span>
          <span style={{ color: varColor }}> state == </span>
          <span style={{ color: numColor }}>0xFFFF</span>
          {"  "}
          <span style={{ color: cmtColor }}># 0xFF00 ^ 0x00FF</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: fnColor }}>atomik_load</span>
          <span style={{ color: varColor }}>(ctx, </span>
          <span style={{ color: numColor }}>0xFF00</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: fnColor }}>atomik_accum</span>
          <span style={{ color: varColor }}>(ctx, </span>
          <span style={{ color: numColor }}>0x00FF</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: typeColor }}>uint64_t</span>
          <span style={{ color: varColor }}> s = </span>
          <span style={{ color: fnColor }}>atomik_read</span>
          <span style={{ color: varColor }}>(ctx);</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// s == 0xFFFF  (0xFF00 ^ 0x00FF)</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>0xFF00</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>0x00FF</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: kwColor }}>const</span>
          <span style={{ color: varColor }}> state = ctx.</span>
          <span style={{ color: fnColor }}>read</span>
          <span style={{ color: varColor }}>();</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// state === 0xFFFF  (0xFF00 ^ 0x00FF)</span>
        </code>
      ),
    },
  },
  {
    name: "swap",
    description:
      "Checkpoint the current state: sets reference = reference XOR accumulator, then resets accumulator to zero. After swap, read() returns the same value as before, but the accumulator is clean for a new round of deltas.",
    returnValue:
      "The checkpointed state value (the new reference). In C, returned as uint64_t.",
    signatures: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>swap</span>
          <span style={{ color: varColor }}>() -&gt; </span>
          <span style={{ color: typeColor }}>int</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: typeColor }}>uint64_t</span>{" "}
          <span style={{ color: fnColor }}>atomik_swap</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: typeColor }}>atomik_ctx_t</span>
          <span style={{ color: varColor }}> *ctx);</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>swap</span>
          <span style={{ color: varColor }}>(): </span>
          <span style={{ color: typeColor }}>number</span>
        </code>
      ),
    },
    examples: {
      Python: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>1000</span>
          <span style={{ color: varColor }}>)</span>
          {"\n"}
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>50</span>
          <span style={{ color: varColor }}>)</span>
          {"\n"}
          <span style={{ color: varColor }}>saved = ctx.</span>
          <span style={{ color: fnColor }}>swap</span>
          <span style={{ color: varColor }}>()</span>
          {"\n"}
          <span style={{ color: cmtColor }}># saved == 1000 ^ 50 == 1018</span>
          {"\n"}
          <span style={{ color: cmtColor }}># ctx.reference == 1018, ctx.accumulator == 0</span>
        </code>
      ),
      C: (
        <code>
          <span style={{ color: fnColor }}>atomik_load</span>
          <span style={{ color: varColor }}>(ctx, </span>
          <span style={{ color: numColor }}>1000</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: fnColor }}>atomik_accum</span>
          <span style={{ color: varColor }}>(ctx, </span>
          <span style={{ color: numColor }}>50</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: typeColor }}>uint64_t</span>
          <span style={{ color: varColor }}> saved = </span>
          <span style={{ color: fnColor }}>atomik_swap</span>
          <span style={{ color: varColor }}>(ctx);</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// saved == 1018  (1000 ^ 50)</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// ctx-&gt;reference == 1018, ctx-&gt;accumulator == 0</span>
        </code>
      ),
      JavaScript: (
        <code>
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>load</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>1000</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: varColor }}>ctx.</span>
          <span style={{ color: fnColor }}>accum</span>
          <span style={{ color: varColor }}>(</span>
          <span style={{ color: numColor }}>50</span>
          <span style={{ color: varColor }}>);</span>
          {"\n"}
          <span style={{ color: kwColor }}>const</span>
          <span style={{ color: varColor }}> saved = ctx.</span>
          <span style={{ color: fnColor }}>swap</span>
          <span style={{ color: varColor }}>();</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// saved === 1018  (1000 ^ 50)</span>
          {"\n"}
          <span style={{ color: cmtColor }}>// ctx.reference === 1018, ctx.accumulator === 0</span>
        </code>
      ),
    },
  },
];

/* ── Tab colors ─────────────────────────────────────────────────────── */

const langColors: Record<string, string> = {
  Python: "#4f8fff",
  C: "#22c55e",
  JavaScript: "#d4a843",
};

const langs = ["Python", "C", "JavaScript"] as const;

/* ── Component ──────────────────────────────────────────────────────── */

export default function LangTabs() {
  const [activeLang, setActiveLang] = useState<string>("Python");

  return (
    <div>
      {/* Language tab bar */}
      <div className="flex gap-1 mb-8 p-1 rounded-lg w-fit" style={{ background: "#12121a", border: "1px solid #1e1e2e" }}>
        {langs.map((lang) => (
          <button
            key={lang}
            onClick={() => setActiveLang(lang)}
            className="px-4 py-2 rounded-md text-sm font-semibold transition-all duration-150"
            style={{
              background: activeLang === lang ? "rgba(79,143,255,0.1)" : "transparent",
              color: activeLang === lang ? langColors[lang] : "#8888a0",
              border: activeLang === lang ? `1px solid ${langColors[lang]}40` : "1px solid transparent",
            }}
          >
            {lang}
          </button>
        ))}
      </div>

      {/* Method cards */}
      <div className="space-y-8">
        {methods.map((method) => (
          <div
            key={method.name}
            className="rounded-xl border p-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            {/* Method name */}
            <h3
              className="text-xl font-bold mb-1 font-mono"
              style={{ color: langColors[activeLang] }}
            >
              {method.name}
            </h3>

            {/* Signature */}
            <div style={codeBlockStyle} className="mt-3 mb-4">
              <pre style={{ margin: 0 }}>
                {method.signatures[activeLang]}
              </pre>
            </div>

            {/* Description */}
            <p className="text-sm leading-relaxed mb-3" style={{ color: "#b0b0c0" }}>
              {method.description}
            </p>

            {/* Return value */}
            <div className="mb-4">
              <span className="text-xs font-semibold uppercase tracking-wider" style={{ color: "#8888a0" }}>
                Returns
              </span>
              <p className="text-sm mt-1" style={{ color: "#b0b0c0" }}>
                {method.returnValue}
              </p>
            </div>

            {/* Example */}
            <div>
              <span className="text-xs font-semibold uppercase tracking-wider" style={{ color: "#8888a0" }}>
                Example
              </span>
              <div style={codeBlockStyle} className="mt-2">
                <pre style={{ margin: 0 }}>
                  {method.examples[activeLang]}
                </pre>
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
