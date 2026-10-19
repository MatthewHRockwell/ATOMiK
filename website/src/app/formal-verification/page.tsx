import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import Footer from "@/components/Footer";

export const metadata: Metadata = {
  title: "Formal Verification — The Correctness Is Machine-Proven",
  description:
    "ATOMiK's delta-state algebra is formally verified in Lean 4: 108 theorems, zero sorry, depending only on Lean's standard axioms. The byte-identical-regardless-of-order property your contract rests on is mathematically proven, not just measured.",
  openGraph: {
    title: "ATOMiK — Formal Verification (Lean 4)",
    description:
      "108 machine-checked theorems. Zero sorry. Depends only on [propext, Quot.sound]. The order-independence guarantee, proven.",
    url: "https://atomik.tech/formal-verification",
    type: "website",
  },
};

const c = {
  bg: "#070807",
  panel: "#10130f",
  panel2: "#0c0f0c",
  text: "#f5f7ee",
  muted: "#b7c4bb",
  faint: "#7f8b81",
  border: "#2d3a34",
  cyan: "#22d3ee",
  green: "#2dd36f",
  amber: "#f5a524",
};

// Real, from math/proofs/ATOMiK/ — counts are `theorem` declarations per module.
const MODULES: { file: string; n: number; what: string }[] = [
  { file: "Properties.lean", n: 21, what: "Abelian-group axioms: associativity, commutativity, identity, inverse" },
  { file: "Composition.lean", n: 24, what: "Sequential & parallel composition laws" },
  { file: "Transition.lean", n: 19, what: "State-transition functions and determinism" },
  { file: "Closure.lean", n: 13, what: "Closure of deltas under composition" },
  { file: "Equivalence.lean", n: 13, what: "Computational equivalence with the traditional stateful model" },
  { file: "TuringComplete.lean", n: 14, what: "Turing completeness via counter-machine simulation" },
  { file: "Delta.lean", n: 4, what: "Core delta type and operations" },
];
const TOTAL = MODULES.reduce((s, m) => s + m.n, 0); // 108

const AXIOMS: { name: string; gloss: string; lean: string }[] = [
  {
    name: "delta_assoc",
    gloss: "Associativity — grouping of merges does not change the result.",
    lean: "theorem delta_assoc (a b c : Delta) :\n  compose (compose a b) c = compose a (compose b c)",
  },
  {
    name: "delta_comm",
    gloss: "Commutativity — arrival order does not change the result. This is the order-independence your contract rests on.",
    lean: "theorem delta_comm (a b : Delta) :\n  compose a b = compose b a",
  },
  {
    name: "delta_identity_unique",
    gloss: "Identity is unique — there is exactly one no-op delta (zero).",
    lean: "theorem delta_identity_unique (e : Delta)\n  (h : ∀ d, compose d e = d) : e = Delta.zero",
  },
  {
    name: "delta_self_inverse",
    gloss: "Self-inverse — applying a delta twice cancels it. The architectural basis of the security model.",
    lean: "theorem delta_self_inverse (d : Delta) :\n  compose d d = Delta.zero",
  },
  {
    name: "atomik_deterministic",
    gloss: "Determinism — the same deltas always reconstruct the same state (no hidden nondeterminism).",
    lean: "theorem atomik_deterministic … (Transition.lean)",
  },
  {
    name: "computational_equivalence",
    gloss: "Equivalence — the delta model computes exactly what the traditional stateful model does (encode/transition/decode round-trips).",
    lean: "theorem computational_equivalence :\n  (∀ s₁ s₂, transition s₁ (encode s₁ s₂) = s₂) ∧ …",
  },
];

function Card({ children, style }: { children: React.ReactNode; style?: React.CSSProperties }) {
  return (
    <div style={{ background: c.panel, border: `1px solid ${c.border}`, borderRadius: 14, padding: 22, ...style }}>
      {children}
    </div>
  );
}

function Code({ children }: { children: string }) {
  return (
    <pre
      style={{
        margin: "10px 0 0", padding: "12px 14px", background: c.panel2, border: `1px solid ${c.border}`,
        borderRadius: 8, color: c.muted, fontSize: 12.5, lineHeight: 1.5, overflowX: "auto",
        fontFamily: "ui-monospace, SFMono-Regular, Menlo, monospace", whiteSpace: "pre-wrap",
      }}
    >
      {children}
    </pre>
  );
}

export default function FormalVerificationPage() {
  return (
    <main style={{ background: c.bg, color: c.text, minHeight: "100vh" }}>
      <Nav active="Proof" />
      <section style={{ maxWidth: 980, margin: "0 auto", padding: "56px 22px 80px" }}>
        <div
          style={{
            display: "inline-block", border: `1px solid ${c.border}`, borderRadius: 999,
            padding: "4px 12px", fontSize: 12, letterSpacing: 0.5, color: c.green, marginBottom: 18,
          }}
        >
          FORMALLY_VERIFIED · LEAN 4
        </div>
        <h1 style={{ fontSize: 38, lineHeight: 1.1, margin: "0 0 14px", fontWeight: 800 }}>
          The correctness isn&apos;t just measured. It&apos;s machine-proven.
        </h1>
        <p style={{ color: c.muted, fontSize: 17, lineHeight: 1.55, maxWidth: 760 }}>
          Every verdict and LOI on this site commits to one property:{" "}
          <strong style={{ color: c.text }}>the merged result is byte-identical regardless of
          arrival order or parallelism.</strong>{" "}
          That property is the definition of an abelian group — and ATOMiK&apos;s delta-state algebra
          is formally verified as one in <strong style={{ color: c.text }}>Lean 4</strong>:{" "}
          {TOTAL} theorems, zero <code style={{ color: c.amber }}>sorry</code>, depending only on
          Lean&apos;s standard axioms. The guarantee is mathematical, not just observed on one chip.
        </p>

        {/* verification facts */}
        <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(150px, 1fr))", gap: 12, margin: "28px 0 8px" }}>
          {[
            { k: `${TOTAL}`, v: "theorems proven" },
            { k: "0", v: "sorry / admit" },
            { k: "7", v: "proof modules" },
            { k: "Lean 4.27.0", v: "self-contained (no external libs)" },
          ].map((s) => (
            <Card key={s.v} style={{ padding: 18 }}>
              <div style={{ fontSize: 24, fontWeight: 800, color: c.cyan }}>{s.k}</div>
              <div style={{ color: c.muted, fontSize: 13, marginTop: 4 }}>{s.v}</div>
            </Card>
          ))}
        </div>

        {/* the gold-standard axiom check */}
        <Card style={{ marginTop: 16 }}>
          <h2 style={{ fontSize: 18, fontWeight: 700, margin: "0 0 6px" }}>
            What &ldquo;machine-proven&rdquo; actually means here
          </h2>
          <p style={{ color: c.muted, fontSize: 14.5, lineHeight: 1.6, margin: 0 }}>
            The Lean 4 kernel type-checks every proof. We don&apos;t ask you to trust that — you can
            ask Lean what each theorem rests on. Every result depends <em>only</em> on Lean&apos;s two
            standard axioms (<code style={{ color: c.cyan }}>propext</code>,{" "}
            <code style={{ color: c.cyan }}>Quot.sound</code>) — no custom axioms, and critically no{" "}
            <code style={{ color: c.amber }}>sorryAx</code>, which would appear if any proof were
            stubbed. Reproduce it yourself:
          </p>
          <Code>{`$ lake build
Build completed successfully (10 jobs).

$ #print axioms delta_self_inverse
'ATOMiK.delta_self_inverse' depends on axioms: [propext, Quot.sound]
$ #print axioms delta_comm
'ATOMiK.delta_comm' depends on axioms: [propext, Quot.sound]`}</Code>
        </Card>

        {/* the theorems */}
        <h2 style={{ fontSize: 22, fontWeight: 800, margin: "40px 0 6px" }}>The theorems</h2>
        <p style={{ color: c.faint, fontSize: 14, lineHeight: 1.6, maxWidth: 760, margin: "0 0 18px" }}>
          The headline results, by the group axiom they establish. Names are the actual Lean
          identifiers in <code style={{ color: c.muted }}>math/proofs/ATOMiK/</code>.
        </p>
        <div style={{ display: "grid", gap: 14 }}>
          {AXIOMS.map((a) => (
            <Card key={a.name}>
              <div style={{ display: "flex", alignItems: "baseline", gap: 10, flexWrap: "wrap" }}>
                <code style={{ color: c.green, fontSize: 14.5, fontWeight: 700 }}>{a.name}</code>
              </div>
              <p style={{ color: c.muted, fontSize: 14, lineHeight: 1.55, margin: "8px 0 0" }}>{a.gloss}</p>
              <Code>{a.lean}</Code>
            </Card>
          ))}
        </div>

        {/* module breakdown */}
        <h2 style={{ fontSize: 18, fontWeight: 700, margin: "36px 0 12px" }}>All {TOTAL} theorems, by module</h2>
        <Card style={{ padding: 0, overflow: "hidden" }}>
          {MODULES.map((m, i) => (
            <div
              key={m.file}
              style={{
                display: "flex", justifyContent: "space-between", gap: 14, padding: "12px 18px",
                borderTop: i === 0 ? "none" : `1px solid ${c.border}`, alignItems: "baseline",
              }}
            >
              <code style={{ color: c.text, fontSize: 14 }}>{m.file}</code>
              <span style={{ color: c.muted, fontSize: 13.5, flex: 1, textAlign: "left", marginLeft: 18 }}>{m.what}</span>
              <span style={{ color: c.cyan, fontWeight: 700, fontSize: 14, whiteSpace: "nowrap" }}>{m.n} thm</span>
            </div>
          ))}
        </Card>

        {/* honest scope */}
        <Card style={{ marginTop: 28, borderColor: c.amber }}>
          <h2 style={{ fontSize: 17, fontWeight: 700, margin: "0 0 8px", color: c.amber }}>
            Scope — what is proven vs. what is measured
          </h2>
          <p style={{ color: c.muted, fontSize: 14.5, lineHeight: 1.6, margin: 0 }}>
            The Lean proofs verify the <strong style={{ color: c.text }}>delta-state algebra</strong>{" "}
            — the mathematical model. They do not, on their own, verify the FPGA gateware. The bridge
            is empirical and independent: the hardware engine produces a{" "}
            <strong style={{ color: c.text }}>byte-identical result across 1/2/4/8 banks</strong>,{" "}
            measured on silicon. So the order-independence is <em>proven in the math</em> and{" "}
            <em>observed in the hardware</em> — two independent lines of evidence for the same
            property, which is exactly what a correctness-critical commitment should rest on.
          </p>
          <p style={{ color: c.faint, fontSize: 13, lineHeight: 1.6, margin: "12px 0 0" }}>
            See the measured side on the{" "}
            <Link href="/benchmark-proof" style={{ color: c.cyan }}>benchmark tool</Link>{" "}
            (the recorded reference run and the byte-identical result hash). The full Lean source
            (<code style={{ color: c.muted }}>math/proofs/ATOMiK/</code>) is available for review under
            evaluation.
          </p>
        </Card>

        <div style={{ marginTop: 30, paddingTop: 22, borderTop: `1px solid ${c.border}`, color: c.faint, fontSize: 13, lineHeight: 1.6 }}>
          Browse{" "}
          <Link href="/workloads" style={{ color: c.cyan }}>workload archetypes</Link>, test your bar on
          the{" "}
          <Link href="/benchmark-proof" style={{ color: c.cyan }}>benchmark tool</Link>, or{" "}
          <Link href="/contact?intent=live-benchmark" style={{ color: c.cyan }}>
            request a live run + conditional LOI
          </Link>
          . The correctness gate in every proof is the property these theorems establish.
        </div>
      </section>
      <Footer />
    </main>
  );
}
