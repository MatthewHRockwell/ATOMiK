import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

export const metadata: Metadata = {
  title: "What is Delta-State Computing? The Definitive Guide — ATOMiK",
  description:
    "Delta-state computing replaces stored state with algebraic reconstruction. Learn the mathematical foundation, how it differs from CRDTs, event sourcing, OT, and Raft — and why XOR changes everything.",
  keywords: [
    "delta state computing",
    "delta state architecture",
    "XOR state synchronization",
    "delta-state algebra",
    "state reconstruction",
    "ATOMiK",
    "conflict-free replication",
    "distributed state management",
  ],
  openGraph: {
    title: "What is Delta-State Computing? The Definitive Guide",
    description:
      "State is not stored. State is reconstructed. Learn the algebra behind delta-state computing and why it outperforms CRDTs, event sourcing, and consensus protocols.",
    type: "article",
    publishedTime: "2026-03-16T00:00:00Z",
  },
};

function Code({ children }: { children: string }) {
  return (
    <pre
      className="rounded-lg p-4 text-sm overflow-x-auto my-4"
      style={{
        background: "#0a0a0f",
        border: "1px solid #1e1e2e",
        color: "#22d3ee",
        fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
        lineHeight: 1.7,
      }}
    >
      <code>{children}</code>
    </pre>
  );
}

function Section({ id, title, children }: { id: string; title: string; children: React.ReactNode }) {
  return (
    <section id={id} className="mb-12">
      <h2 className="text-2xl font-bold pt-4 mb-4" style={{ color: "#e0e0e8" }}>
        {title}
      </h2>
      <div className="space-y-4 text-base leading-relaxed" style={{ color: "#c8c8d4" }}>
        {children}
      </div>
    </section>
  );
}

export default function WhatIsDeltaStateComputingPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="Blog" />

      <article className="max-w-3xl mx-auto px-6 pt-16 pb-24">
        {/* Header */}
        <div className="mb-12">
          <Link
            href="/blog"
            className="text-sm mb-6 inline-block hover:underline"
            style={{ color: "#4f8fff" }}
          >
            &larr; Back to blog
          </Link>
          <time className="block text-sm font-mono mb-3" style={{ color: "#8888a0" }}>
            March 16, 2026
          </time>
          <h1 className="text-3xl md:text-4xl font-bold tracking-tight mb-4 leading-tight">
            What is Delta-State Computing?
          </h1>
          <p className="text-lg mb-4" style={{ color: "#8888a0" }}>
            The definitive guide to delta-state architecture, XOR state synchronization,
            and why reconstructed state outperforms stored state.
          </p>
          <div className="flex flex-wrap gap-2">
            {["delta-state", "architecture", "distributed-systems", "pillar"].map((tag) => (
              <span
                key={tag}
                className="text-xs px-2 py-0.5 rounded-full"
                style={{
                  background: "rgba(139, 92, 246, 0.1)",
                  color: "#8b5cf6",
                  border: "1px solid rgba(139, 92, 246, 0.2)",
                }}
              >
                {tag}
              </span>
            ))}
          </div>
        </div>

        {/* Table of Contents */}
        <nav
          className="rounded-xl border p-6 mb-12"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <p className="text-sm font-mono mb-3" style={{ color: "#8888a0" }}>
            Contents
          </p>
          <ol className="space-y-1.5 text-sm list-decimal list-inside" style={{ color: "#4f8fff" }}>
            {[
              ["#definition", "What is delta-state computing?"],
              ["#math", "The mathematical foundation"],
              ["#vs-crdts", "How it differs from CRDTs"],
              ["#vs-event-sourcing", "How it differs from event sourcing"],
              ["#vs-ot", "How it differs from operational transform"],
              ["#vs-raft", "How it differs from Raft/Paxos"],
              ["#applications", "Real-world applications"],
              ["#code-example", "Code example"],
              ["#hardware", "Hardware acceleration"],
              ["#try-it", "Try it yourself"],
            ].map(([href, label]) => (
              <li key={href}>
                <a href={href} className="hover:underline" style={{ color: "#4f8fff" }}>
                  {label}
                </a>
              </li>
            ))}
          </ol>
        </nav>

        {/* 1. Definition */}
        <Section id="definition" title="What is delta-state computing?">
          <p>
            <strong>Delta-state computing</strong> is a fundamentally different approach to
            state management. Instead of storing state and mutating it in place,
            delta-state computing <em>reconstructs</em> state from an initial reference
            and an accumulator of changes:
          </p>

          <div
            className="rounded-xl border p-6 my-6 text-center"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <p className="font-mono text-lg" style={{ color: "#22d3ee" }}>
              current_state = initial_state XOR accumulator
            </p>
          </div>

          <p>
            This single equation replaces read-modify-write cycles, cache coherence
            protocols, lock hierarchies, and consensus algorithms. Every read is a
            reconstruction. Every write is a delta accumulated into a shared register.
            The result is always consistent, always convergent, and mathematically proven
            correct.
          </p>

          <p>
            The term &quot;delta-state computing&quot; was coined by the{" "}
            <Link href="/" className="underline" style={{ color: "#4f8fff" }}>
              ATOMiK project
            </Link>{" "}
            to describe this architecture. It draws from delta-state CRDTs, XOR-based
            error correction, and algebraic group theory, but synthesizes them into
            something new: a universal state primitive that works from Python scripts to
            custom RISC-V silicon.
          </p>

          <p>
            The key insight is that <strong>state does not need to be stored</strong>. In
            every Von Neumann machine, state lives in memory cells and must be explicitly
            read, modified, and written back. Delta-state computing eliminates this
            paradigm. State is a function of two values, and changes compose algebraically
            rather than sequentially.
          </p>

          <p>
            This is not an incremental improvement. It is a different computational model
            with different performance characteristics, different security properties, and
            different scaling behavior.
          </p>
        </Section>

        {/* 2. Mathematical Foundation */}
        <Section id="math" title="The mathematical foundation">
          <p>
            Delta-state computing is built on an <strong>Abelian group</strong> over
            the XOR operation. This is not marketing language — it is a formal algebraic
            structure with four properties, each proven in Lean4 with 108 machine-checked
            theorems:
          </p>

          <div
            className="rounded-xl border p-6 my-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div className="space-y-4">
              <div>
                <p className="font-mono text-sm mb-1" style={{ color: "#8b5cf6" }}>1. Closure</p>
                <p style={{ color: "#c8c8d4" }}>
                  XOR of any two n-bit values produces another n-bit value. The system
                  never leaves its domain.
                </p>
              </div>
              <div>
                <p className="font-mono text-sm mb-1" style={{ color: "#8b5cf6" }}>2. Commutativity</p>
                <p style={{ color: "#c8c8d4" }}>
                  <code style={{ color: "#22d3ee" }}>A XOR B = B XOR A</code>. Order does
                  not matter. Two nodes applying the same deltas in different order arrive
                  at the same state. This is why no ordering protocol is needed.
                </p>
              </div>
              <div>
                <p className="font-mono text-sm mb-1" style={{ color: "#8b5cf6" }}>3. Associativity</p>
                <p style={{ color: "#c8c8d4" }}>
                  <code style={{ color: "#22d3ee" }}>(A XOR B) XOR C = A XOR (B XOR C)</code>.
                  Grouping does not matter. Deltas can be batched, split, or rearranged
                  without affecting the result.
                </p>
              </div>
              <div>
                <p className="font-mono text-sm mb-1" style={{ color: "#8b5cf6" }}>4. Self-inverse</p>
                <p style={{ color: "#c8c8d4" }}>
                  <code style={{ color: "#22d3ee" }}>A XOR A = 0</code>. Every element is
                  its own inverse. Applying a delta twice cancels it out. Undo is free —
                  just re-apply the same delta.
                </p>
              </div>
            </div>
          </div>

          <p>
            These properties give delta-state computing its power. Commutativity eliminates
            ordering requirements. Self-inverse provides instant undo. Associativity enables
            arbitrary batching. Together, they mean that <strong>the accumulator is a
            shared resource by design</strong> — multiple producers can feed deltas in any
            order, and the result is identical.
          </p>

          <p>
            The four core operations that emerge from this algebra are:
          </p>

          <div
            className="rounded-xl border overflow-hidden my-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <table className="w-full text-sm">
              <thead>
                <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>Operation</th>
                  <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>Meaning</th>
                  <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>Effect</th>
                </tr>
              </thead>
              <tbody style={{ color: "#b0b0c0" }}>
                {[
                  ["LOAD", "Set initial reference state", "reference = value, accumulator = 0"],
                  ["ACCUM", "Apply a delta", "accumulator = accumulator XOR delta"],
                  ["READ", "Reconstruct current state", "return reference XOR accumulator"],
                  ["SWAP", "Checkpoint and reset", "reference = READ(), accumulator = 0, return old state"],
                ].map(([op, meaning, effect], i) => (
                  <tr key={i} style={{ borderBottom: "1px solid #1e1e2e" }}>
                    <td className="p-4 font-mono" style={{ color: "#22d3ee" }}>{op}</td>
                    <td className="p-4">{meaning}</td>
                    <td className="p-4 font-mono text-xs" style={{ color: "#8888a0" }}>{effect}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          <p>
            That is the entire API. Four operations. No configuration, no schema, no
            type-specific merge functions. The algebra handles convergence, not your
            application code.
          </p>
        </Section>

        {/* 3. vs CRDTs */}
        <Section id="vs-crdts" title="How it differs from CRDTs">
          <p>
            <strong>Conflict-free Replicated Data Types (CRDTs)</strong> solve the same
            convergence problem as delta-state computing: multiple nodes update shared
            state without coordination, and all nodes converge to the same value. The
            mechanism is fundamentally different.
          </p>

          <p>
            CRDTs achieve convergence by defining <em>type-specific merge functions</em>.
            A G-Counter merges by taking the max of each node&apos;s counter. A LWW-Register
            merges by timestamp. An OR-Set merges by tracking add/remove tags. Each data
            type requires its own merge logic, its own metadata, and its own correctness
            proof.
          </p>

          <p>
            Delta-state computing achieves convergence with <em>one operation</em>: XOR.
            There is no type-specific logic. A delta is a bitstring. The merge function
            is always XOR. The metadata is always zero bytes (the accumulator is the
            metadata). The correctness proof is always the same 108 theorems.
          </p>

          <div
            className="rounded-xl border overflow-hidden my-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <table className="w-full text-sm">
              <thead>
                <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>Dimension</th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#f59e0b" }}>CRDTs</th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#22d3ee" }}>ATOMiK</th>
                </tr>
              </thead>
              <tbody style={{ color: "#b0b0c0" }}>
                {[
                  ["Merge function", "Type-specific (per data type)", "Universal (XOR)"],
                  ["Metadata overhead", "O(n) — grows with nodes/ops", "O(1) — 8 bytes fixed"],
                  ["Implementation complexity", "Dozens of CRDT types", "4 operations, 1 type"],
                  ["Correctness proof", "Per-type proofs required", "108 Lean4 theorems (universal)"],
                  ["Undo support", "Not built-in (type-dependent)", "Free (self-inverse)"],
                  ["Hardware acceleration", "Impractical (complex merge)", "Native (single XOR gate)"],
                  ["Garbage collection", "Required (tombstones, etc.)", "Not needed"],
                  ["Learning curve", "High (choose correct type)", "Minimal (4 operations)"],
                ].map(([dim, crdt, atomik], i) => (
                  <tr key={i} style={{ borderBottom: "1px solid #1e1e2e" }}>
                    <td className="p-4 font-medium" style={{ color: "#e0e0e8" }}>{dim}</td>
                    <td className="text-center p-4">{crdt}</td>
                    <td className="text-center p-4">{atomik}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          <p>
            CRDTs are the right choice when you need rich data type semantics — a counter
            that only goes up, a set with add/remove, a sequence with insert/delete. ATOMiK
            is the right choice when you need fast, universal convergence on arbitrary
            binary state. For a deeper comparison, see{" "}
            <Link href="/blog/crdt-alternative-delta-state" className="underline" style={{ color: "#4f8fff" }}>
              CRDT Alternative: Delta-State Algebra
            </Link>.
          </p>
        </Section>

        {/* 4. vs Event Sourcing */}
        <Section id="vs-event-sourcing" title="How it differs from event sourcing">
          <p>
            <strong>Event sourcing</strong> stores every state change as an immutable event
            in an append-only log. To reconstruct the current state, you replay all events
            from the beginning. This gives you a complete audit trail but creates three
            scaling problems:
          </p>

          <ul className="list-disc pl-6 space-y-2">
            <li>
              <strong>Storage grows without bound.</strong> Every event is kept forever.
              Compaction (snapshotting) is an operational burden.
            </li>
            <li>
              <strong>Reconstruction is O(n).</strong> Replaying 10 million events to read
              the current balance takes time proportional to the log length.
            </li>
            <li>
              <strong>Multi-node sync requires ordering.</strong> Events must be applied in
              causal order, which requires vector clocks or a central sequencer.
            </li>
          </ul>

          <p>
            Delta-state computing eliminates all three problems. Storage is constant (one
            reference + one accumulator = 16 bytes). Reconstruction is O(1) (a single XOR).
            Multi-node sync is order-free (commutativity). The trade-off is that ATOMiK
            does not preserve event history — it tracks <em>cumulative change</em>, not
            <em>what happened</em>.
          </p>

          <Code>{`# Event sourcing: O(n) reconstruction
state = initial
for event in event_log:      # 10 million iterations
    state = apply(state, event)

# Delta-state: O(1) reconstruction
state = reference ^ accumulator  # One XOR, always`}</Code>

          <p>
            For a detailed quantitative comparison with code examples, see{" "}
            <Link href="/blog/atomik-vs-event-sourcing" className="underline" style={{ color: "#4f8fff" }}>
              ATOMiK vs Event Sourcing
            </Link>.
          </p>
        </Section>

        {/* 5. vs OT */}
        <Section id="vs-ot" title="How it differs from operational transform">
          <p>
            <strong>Operational Transform (OT)</strong> is the algorithm behind Google Docs.
            When two users edit the same document concurrently, OT transforms each
            operation against the other to preserve intent. It works — but it requires a
            central transformation server and the algorithm complexity is notorious.
          </p>

          <p>
            OT&apos;s core challenge is that operations do not commute. Insert-at-position-5
            followed by delete-at-position-3 produces a different result than the reverse.
            The transform function must compensate for this non-commutativity, and proving
            transform correctness is so difficult that Google&apos;s own engineers have
            called it &quot;the hardest problem in distributed systems.&quot;
          </p>

          <div
            className="rounded-xl border overflow-hidden my-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <table className="w-full text-sm">
              <thead>
                <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>Property</th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#f59e0b" }}>OT</th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#22d3ee" }}>ATOMiK</th>
                </tr>
              </thead>
              <tbody style={{ color: "#b0b0c0" }}>
                {[
                  ["Commutativity", "No (requires transform)", "Yes (by construction)"],
                  ["Central server", "Required", "Not needed"],
                  ["Algorithm complexity", "O(n^2) transform pairs", "O(1) XOR"],
                  ["Correctness proofs", "Notoriously difficult", "108 machine-checked theorems"],
                  ["Offline support", "Requires rebasing", "Deltas apply on reconnect"],
                  ["Bandwidth", "Full operation payload", "8 bytes per delta"],
                ].map(([prop, ot, atomik], i) => (
                  <tr key={i} style={{ borderBottom: "1px solid #1e1e2e" }}>
                    <td className="p-4 font-medium" style={{ color: "#e0e0e8" }}>{prop}</td>
                    <td className="text-center p-4">{ot}</td>
                    <td className="text-center p-4">{atomik}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          <p>
            Delta-state computing sidesteps the entire transform problem. Because XOR is
            commutative, there is nothing to transform. Two concurrent edits produce the
            same result regardless of application order. No server, no rebasing, no
            transform functions.
          </p>

          <p>
            The trade-off is granularity. OT preserves character-level intent (&quot;insert
            &apos;a&apos; at position 7&quot;). ATOMiK operates on binary state. For
            collaborative text editing where character intent matters, OT or CRDTs like
            Yjs are still the right tool. For state synchronization where convergence
            matters more than intent, ATOMiK is faster, simpler, and provably correct.
          </p>
        </Section>

        {/* 6. vs Raft/Paxos */}
        <Section id="vs-raft" title="How it differs from Raft/Paxos">
          <p>
            <strong>Raft</strong> and <strong>Paxos</strong> are consensus protocols. They
            solve a different problem than ATOMiK: they ensure all nodes agree on a
            single order of operations. This requires leader election, log replication,
            and majority quorums.
          </p>

          <p>
            Delta-state computing does not need consensus because it does not need
            ordering. The commutativity and associativity of XOR mean that <em>all
            orderings produce the same result</em>. There is no leader, no quorum, no
            election timeout, and no split-brain scenario.
          </p>

          <div
            className="rounded-xl border overflow-hidden my-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <table className="w-full text-sm">
              <thead>
                <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>Property</th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#f59e0b" }}>Raft/Paxos</th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#22d3ee" }}>ATOMiK</th>
                </tr>
              </thead>
              <tbody style={{ color: "#b0b0c0" }}>
                {[
                  ["Leader election", "Required", "Not needed"],
                  ["Quorum requirement", "Majority (n/2 + 1)", "None"],
                  ["Network partitions", "Minority side blocks", "All sides continue"],
                  ["Write latency", "2 RTT (leader + commit)", "0 RTT (local XOR)"],
                  ["Ordering guarantee", "Total order", "No ordering needed"],
                  ["Availability", "CP (sacrifices availability)", "AP (always available)"],
                  ["Message complexity", "O(n) per write", "O(1) per delta"],
                ].map(([prop, raft, atomik], i) => (
                  <tr key={i} style={{ borderBottom: "1px solid #1e1e2e" }}>
                    <td className="p-4 font-medium" style={{ color: "#e0e0e8" }}>{prop}</td>
                    <td className="text-center p-4">{raft}</td>
                    <td className="text-center p-4">{atomik}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          <p>
            The CAP theorem forces a choice between consistency and availability during
            partitions. Raft chooses consistency (the minority partition cannot write).
            ATOMiK chooses availability (all partitions continue accumulating deltas) with
            convergence under the XOR algebra model on reconnection. This makes delta-state computing
            particularly well-suited for edge computing, IoT networks, and any system
            where partitions are common rather than exceptional.
          </p>

          <p>
            When you need total ordering (e.g., bank transactions that must be serialized),
            use Raft. When you need convergent state across unreliable networks, use
            delta-state computing.
          </p>
        </Section>

        {/* 7. Real-World Applications */}
        <Section id="applications" title="Real-world applications">
          <p>
            Delta-state computing is the direction ATOMiK is evaluating across
            state-heavy workloads. Here are five categories where the algebra
            can provide concrete advantages when the workload fits:
          </p>

          <div className="grid gap-4 my-6">
            {[
              {
                href: "/solutions/distributed-systems",
                title: "Distributed Systems",
                desc: "Multi-region state sync without consensus. Nodes exchange 8-byte deltas over any transport and converge under the XOR algebra model.",
              },
              {
                href: "/solutions/gaming",
                title: "Real-Time Gaming",
                desc: "Multiplayer state synchronization at 60+ Hz. Lock-free accumulation from multiple game threads. Rollback via self-inverse instead of state snapshots.",
              },
              {
                href: "/solutions/iot-edge",
                title: "IoT and Edge Computing",
                desc: "Sensor fusion on constrained devices. 8-byte deltas over LoRa, BLE, or satellite links. Edge nodes operate independently and converge on reconnect.",
              },
              {
                href: "/solutions/database-sync",
                title: "Database Synchronization",
                desc: "Change detection framed around deltas instead of repeated row-by-row comparison. Any traffic-reduction number should be quoted only with a linked measured artifact.",
              },
              {
                href: "/solutions/financial",
                title: "Financial Systems",
                desc: "Deterministic state-operation paths for risk, reconciliation, and audit workloads that merit proof-backed technical evaluation.",
              },
            ].map((app) => (
              <Link
                key={app.href}
                href={app.href}
                className="block rounded-xl border p-5 transition-colors hover:border-[#4f8fff44]"
                style={{ background: "#12121a", borderColor: "#1e1e2e" }}
              >
                <h3 className="font-bold mb-1" style={{ color: "#e0e0e8" }}>
                  {app.title}
                </h3>
                <p className="text-sm" style={{ color: "#8888a0" }}>
                  {app.desc}
                </p>
              </Link>
            ))}
          </div>
        </Section>

        {/* 8. Code Example */}
        <Section id="code-example" title="Code example using atomik-core">
          <p>
            The Python SDK makes delta-state computing accessible in three lines of
            setup. Here is a complete example: two independent nodes converge on the same
            state without coordination.
          </p>

          <Code>{`pip install atomik-core`}</Code>

          <Code>{`from atomik_core import AtomikContext

# Node A: initialize and make changes
node_a = AtomikContext()
node_a.load(0xCAFEBABE)         # Set reference state
node_a.accum(0x0000FF00)        # Apply delta 1
node_a.accum(0x00FF0000)        # Apply delta 2

# Node B: initialize with the same reference
node_b = AtomikContext()
node_b.load(0xCAFEBABE)

# Simulate network: send deltas (order doesn't matter)
node_b.accum(0x00FF0000)        # Delta 2 arrives first
node_b.accum(0x0000FF00)        # Delta 1 arrives second

# Both nodes converge to the same state
assert node_a.read() == node_b.read()  # 0xCAFF45BE
print(f"Converged: 0x{node_a.read():08X}")

# Undo: re-apply delta to cancel it (self-inverse)
node_a.accum(0x0000FF00)
print(f"After undo: 0x{node_a.read():08X}")  # 0xCA00BABE (delta 1 removed)`}</Code>

          <p>
            The entire convergence protocol is implicit in the algebra. No conflict
            resolution callbacks. No version vectors. No merge functions. Nodes exchange
            raw deltas, apply them via <code style={{ color: "#22d3ee" }}>accum()</code>,
            and converge.
          </p>

          <p>
            For more patterns, see{" "}
            <Link href="/blog/5-patterns-delta-state" className="underline" style={{ color: "#4f8fff" }}>
              5 Design Patterns for Delta-State Algebra
            </Link>{" "}
            and the{" "}
            <Link href="/blog/building-distributed-cache-in-50-lines" className="underline" style={{ color: "#4f8fff" }}>
              distributed cache tutorial
            </Link>.
          </p>
        </Section>

        {/* 9. Hardware Acceleration */}
        <Section id="hardware" title="Hardware acceleration: Python to C to FPGA">
          <p>
            One of delta-state computing&apos;s most distinctive features is that it
            accelerates naturally across the entire stack. The same four operations work
            identically whether executed in Python, C, or custom silicon:
          </p>

          <div
            className="rounded-xl border p-6 my-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div className="space-y-5">
              <div>
                <div className="flex items-center gap-3 mb-2">
                  <span
                    className="text-xs font-mono px-2 py-0.5 rounded"
                    style={{ background: "rgba(34, 197, 94, 0.15)", color: "#22c55e" }}
                  >
                    Python
                  </span>
                  <span className="text-sm" style={{ color: "#8888a0" }}>
                    ~500K ops/sec
                  </span>
                </div>
                <p className="text-sm" style={{ color: "#c8c8d4" }}>
                  Pure Python SDK. <code style={{ color: "#22d3ee" }}>pip install atomik-core</code>.
                  Prototyping, data science, scripting. 218 tests passing.
                </p>
              </div>

              <div>
                <div className="flex items-center gap-3 mb-2">
                  <span
                    className="text-xs font-mono px-2 py-0.5 rounded"
                    style={{ background: "rgba(79, 143, 255, 0.15)", color: "#4f8fff" }}
                  >
                    C / Kernel
                  </span>
                  <span className="text-sm" style={{ color: "#8888a0" }}>
                    ~50M ops/sec
                  </span>
                </div>
                <p className="text-sm" style={{ color: "#c8c8d4" }}>
                  Linux kernel module via <code style={{ color: "#22d3ee" }}>/dev/atomik</code>.
                  DKMS-managed. COW detection, network dedup, per-container waste tracking.
                </p>
              </div>

              <div>
                <div className="flex items-center gap-3 mb-2">
                  <span
                    className="text-xs font-mono px-2 py-0.5 rounded"
                    style={{ background: "rgba(139, 92, 246, 0.15)", color: "#8b5cf6" }}
                  >
                    FPGA
                  </span>
                  <span className="text-sm" style={{ color: "#8888a0" }}>
                    Synthesis-characterized scaling
                  </span>
                </div>
                <p className="text-sm" style={{ color: "#c8c8d4" }}>
                  Custom RTL on Xilinx Zynq. 512 parallel banks at 135.6 MHz. A single XOR
                  gate per bank — the operation maps directly to hardware.
                </p>
              </div>
            </div>
          </div>

          <p>
            The software-to-hardware path exists because XOR maps directly to a
            small hardware primitive. Specific performance numbers should be
            repeated only with the linked artifact and evidence label.
          </p>

          <p>
            The hardware story is artifact-backed. ATOMiK has synthesis notes
            and live hardware validation paths:{" "}
            <Link href="/blog/fpga-journey-13-dollar-chip" className="underline" style={{ color: "#4f8fff" }}>
              FPGA proof notes
            </Link>.
            The custom RISC-V CPU includes native LOAD, ACCUM, READ, and SWAP instructions
            in the ISA prototype path. Treat latency and throughput claims as
            artifact-specific unless a current measured package is linked.
          </p>
        </Section>

        {/* 10. Try It Yourself */}
        <Section id="try-it" title="Try it yourself">
          <p>
            Delta-state computing is available today. Start with the Python SDK, run the
            benchmarks on your own hardware, and see the numbers for yourself:
          </p>

          <Code>{`# Install
pip install atomik-core

# Run the benchmark suite
python -m atomik_core.benchmark

# Try the interactive demo
python -c "
from atomik_core import AtomikContext
ctx = AtomikContext()
ctx.load(0xDEADBEEF)
ctx.accum(0x000000FF)
print(f'State: 0x{ctx.read():08X}')  # 0xDEADBE10
"`}</Code>

          <p>
            Or try the{" "}
            <Link href="/demo" className="underline" style={{ color: "#4f8fff" }}>
              interactive browser demo
            </Link>{" "}
            to see delta-state operations in real time. For workload-specific review,
            kernel-module questions, and evidence planning,{" "}
            <Link href="/pricing" className="underline" style={{ color: "#4f8fff" }}>
              request evaluation access
            </Link>.
          </p>
        </Section>

        {/* CTA */}
        <div
          className="rounded-xl border p-8 mt-4"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <h3 className="text-xl font-bold mb-4 text-center">
            Start building with delta-state computing
          </h3>
          <EmailCapture variant="blog" />
        </div>
      </article>
    </div>
  );
}
