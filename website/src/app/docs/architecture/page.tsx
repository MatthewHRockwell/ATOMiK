import type { Metadata } from "next";
export const metadata: Metadata = {
  title: "Architecture — ATOMiK Docs",
  description:
    "The fundamental equation, four algebraic properties, and four operations that define ATOMiK delta-state computing.",
};

const properties = [
  {
    property: "Commutativity",
    formula: "a \u2295 b = b \u2295 a",
    desc: "Deltas can arrive in any order. No coordination needed between producers.",
    color: "#8b5cf6",
  },
  {
    property: "Associativity",
    formula: "(a \u2295 b) \u2295 c = a \u2295 (b \u2295 c)",
    desc: "Deltas can be grouped and merged in any tree structure. Enables parallel reduction.",
    color: "#4f8fff",
  },
  {
    property: "Self-inverse",
    formula: "a \u2295 a = 0",
    desc: "Any delta applied twice cancels itself. Duplicate packets are harmless. Undo is free.",
    color: "#22c55e",
  },
  {
    property: "Identity",
    formula: "a \u2295 0 = a",
    desc: "Accumulating zero changes nothing. The accumulator starts empty and grows only with real deltas.",
    color: "#d4a843",
  },
];

const operations = [
  {
    op: "LOAD",
    desc: "Set initial reference state",
    detail: "reference = value; accumulator = 0",
    color: "#8b5cf6",
  },
  {
    op: "ACCUM",
    desc: "XOR delta into accumulator",
    detail: "accumulator ^= delta",
    color: "#4f8fff",
  },
  {
    op: "READ",
    desc: "Reconstruct current state",
    detail: "return reference ^ accumulator",
    color: "#22c55e",
  },
  {
    op: "SWAP",
    desc: "Atomic snapshot + reset",
    detail: "old = read(); reference = old; accumulator = 0; return old",
    color: "#d4a843",
  },
];

export default function ArchitecturePage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#22c55e" }}
      >
        Foundations
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">Architecture</h1>
      <p className="text-lg mb-10" style={{ color: "#8888a0" }}>
        ATOMiK is not Von Neumann. State is reconstructed, not stored.
      </p>

      <div
        className="rounded-2xl p-8 sm:p-12 border mb-8"
        style={{
          background: "linear-gradient(135deg, #12121a, #181824)",
          borderColor: "#1e1e2e",
        }}
      >
        {/* Core equation */}
        <div className="text-center mb-10">
          <p
            className="text-sm font-mono tracking-widest uppercase mb-4"
            style={{ color: "#8888a0" }}
          >
            The fundamental equation
          </p>
          <div
            className="inline-block px-8 py-4 rounded-xl border"
            style={{ background: "#0d0d14", borderColor: "#8b5cf630" }}
          >
            <code className="text-xl sm:text-2xl font-mono font-bold">
              <span style={{ color: "#4f8fff" }}>current_state</span>{" "}
              <span style={{ color: "#8888a0" }}>=</span>{" "}
              <span style={{ color: "#c084fc" }}>initial_state</span>{" "}
              <span style={{ color: "#22c55e" }}>&oplus;</span>{" "}
              <span style={{ color: "#d4a843" }}>accumulator</span>
            </code>
          </div>
        </div>

        {/* Four properties */}
        <div className="grid grid-cols-1 sm:grid-cols-2 gap-6">
          {properties.map((prop) => (
            <div
              key={prop.property}
              className="rounded-xl p-5 border"
              style={{ background: "#0d0d14", borderColor: "#1e1e2e" }}
            >
              <h4 className="font-bold mb-1" style={{ color: prop.color }}>
                {prop.property}
              </h4>
              <code
                className="block text-sm font-mono mb-3 px-3 py-1.5 rounded-lg"
                style={{ background: "#12121a", color: "#e0e0e8" }}
              >
                {prop.formula}
              </code>
              <p className="text-sm" style={{ color: "#8888a0" }}>
                {prop.desc}
              </p>
            </div>
          ))}
        </div>
      </div>

      {/* Four operations */}
      <h2 className="text-2xl font-bold mb-6">Operations</h2>
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-4">
        {operations.map((item) => (
          <div
            key={item.op}
            className="rounded-xl p-5 border"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div
              className="text-sm font-mono font-bold mb-2 px-2 py-0.5 rounded inline-block"
              style={{ background: "#0d0d14", color: item.color }}
            >
              {item.op}
            </div>
            <p className="text-sm font-semibold text-white mb-1">{item.desc}</p>
            <p className="text-xs font-mono" style={{ color: "#8888a0" }}>
              {item.detail}
            </p>
          </div>
        ))}
      </div>

    </div>
  );
}
