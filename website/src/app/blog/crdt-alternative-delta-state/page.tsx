import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

export const metadata: Metadata = {
  title: "CRDT Alternative: Delta-State Algebra — ATOMiK Blog",
  description:
    "CRDTs are powerful but complex. Delta-state algebra offers a different modeled convergence path with one core operation and machine-checked algebra proofs.",
  keywords: [
    "CRDT alternative",
    "simpler than CRDTs",
    "conflict-free without CRDTs",
    "CRDT replacement",
    "conflict-free replication",
    "delta-state algebra",
    "XOR convergence",
    "ATOMiK",
    "distributed state",
  ],
  openGraph: {
    title: "CRDT Alternative: Delta-State Algebra",
    description:
      "CRDTs require type-specific merge functions, metadata, and garbage collection. Delta-state algebra offers a narrower XOR-based model.",
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

export default function CrdtAlternativePage() {
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
            CRDT Alternative: Delta-State Algebra
          </h1>
          <p className="text-lg mb-4" style={{ color: "#8888a0" }}>
            CRDTs are powerful. They are also complex, metadata-heavy, and require
            choosing the right type for each use case. There is a simpler path to
            conflict-free convergence.
          </p>
          <div className="flex flex-wrap gap-2">
            {["CRDTs", "comparison", "distributed-systems", "pillar"].map((tag) => (
              <span
                key={tag}
                className="text-xs px-2 py-0.5 rounded-full"
                style={{
                  background: "rgba(34, 197, 94, 0.1)",
                  color: "#22c55e",
                  border: "1px solid rgba(34, 197, 94, 0.2)",
                }}
              >
                {tag}
              </span>
            ))}
          </div>
        </div>

        <div className="space-y-6 text-base leading-relaxed" style={{ color: "#c8c8d4" }}>
          {/* The CRDT Problem */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            The CRDT complexity problem
          </h2>

          <p>
            CRDTs (Conflict-free Replicated Data Types) are one of the most elegant ideas
            in distributed systems. The promise is compelling: data structures that
            can converge across replicas under clear assumptions. Consensus
            protocols and leader election remain relevant outside those assumptions.
          </p>

          <p>
            The reality is more nuanced. After a decade of CRDT adoption in production
            systems, a pattern has emerged: teams spend more time choosing, implementing,
            and debugging CRDTs than they spend on the application logic that uses them.
          </p>

          <p>Here are the pain points that practitioners encounter repeatedly:</p>

          <div
            className="rounded-xl border p-6 my-6 space-y-4"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div>
              <p className="font-semibold mb-1" style={{ color: "#f59e0b" }}>
                Type proliferation
              </p>
              <p className="text-sm" style={{ color: "#b0b0c0" }}>
                G-Counter, PN-Counter, G-Set, OR-Set, LWW-Register, MV-Register,
                RGA, Treedoc, LSEQ, Logoot... Each has different semantics, different
                merge behavior, and different performance characteristics. Choosing wrong
                means silent data loss or unbounded growth.
              </p>
            </div>
            <div>
              <p className="font-semibold mb-1" style={{ color: "#f59e0b" }}>
                Metadata overhead
              </p>
              <p className="text-sm" style={{ color: "#b0b0c0" }}>
                A G-Counter with 100 nodes stores 100 integers — one per node. An OR-Set
                stores unique tags per element per node. In practice, CRDT metadata often
                exceeds the actual data being replicated. This metadata grows monotonically
                without garbage collection.
              </p>
            </div>
            <div>
              <p className="font-semibold mb-1" style={{ color: "#f59e0b" }}>
                Garbage collection complexity
              </p>
              <p className="text-sm" style={{ color: "#b0b0c0" }}>
                Tombstones in OR-Sets, causal histories in delta-state CRDTs, and vector
                clocks all require periodic garbage collection. This reintroduces
                coordination, which is one of the operational costs CRDT users may be
                trying to reduce.
              </p>
            </div>
            <div>
              <p className="font-semibold mb-1" style={{ color: "#f59e0b" }}>
                Composition is hard
              </p>
              <p className="text-sm" style={{ color: "#b0b0c0" }}>
                Composing CRDTs into larger structures (e.g., a CRDT map of CRDT counters)
                requires careful analysis. The merge semantics of the container interact
                with the merge semantics of the contents in subtle ways that can break
                convergence assumptions.
              </p>
            </div>
            <div>
              <p className="font-semibold mb-1" style={{ color: "#f59e0b" }}>
                No undo
              </p>
              <p className="text-sm" style={{ color: "#b0b0c0" }}>
                CRDTs are designed to only grow (monotonic join semilattice). A G-Counter
                cannot decrement. An OR-Set&apos;s remove is actually an add of a tombstone.
                Undo semantics require compensating operations that vary by CRDT type.
              </p>
            </div>
          </div>

          {/* The ATOMiK Alternative */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            The ATOMiK alternative: convergence in 4 lines
          </h2>

          <p>
            ATOMiK&apos;s delta-state algebra solves the same convergence problem —
            multiple nodes reach the same state without coordination — using a single
            algebraic operation: XOR.
          </p>

          <Code>{`from atomik_core import AtomikContext

ctx = AtomikContext()
ctx.load(initial_state)    # Set reference
ctx.accum(delta)           # Apply change (XOR)
state = ctx.read()         # Reconstruct: reference ^ accumulator`}</Code>

          <p>
            That is the entire API. No type selection. No merge function. No metadata.
            No garbage collection inside the fit model. Convergence is a
            mathematical property of XOR commutativity, covered by
            machine-checked Lean4 proof artifacts.
          </p>

          {/* Side-by-side comparison */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Side-by-side: G-Counter vs AtomikContext
          </h2>

          <p>
            The G-Counter is the simplest CRDT — a counter that only increments. Even for
            this basic case, the complexity difference is striking.
          </p>

          <div
            className="rounded-xl border p-6 my-6 grid md:grid-cols-2 gap-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div>
              <h3 className="font-bold mb-2" style={{ color: "#f59e0b" }}>CRDT G-Counter</h3>
              <Code>{`class GCounter:
    def __init__(self, node_id, n_nodes):
        self.id = node_id
        # O(n) metadata: one slot per node
        self.counts = [0] * n_nodes

    def increment(self):
        self.counts[self.id] += 1

    def value(self):
        return sum(self.counts)  # O(n)

    def merge(self, other):
        # Type-specific merge: element-wise max
        for i in range(len(self.counts)):
            self.counts[i] = max(
                self.counts[i],
                other.counts[i]
            )

# 3 nodes, each increments
a = GCounter(0, 3)
b = GCounter(1, 3)
c = GCounter(2, 3)
a.increment()  # [1, 0, 0]
b.increment()  # [0, 1, 0]
c.increment()  # [0, 0, 1]

# Merge: ship full vector to each
a.merge(b); a.merge(c)
# a.counts = [1, 1, 1]
# a.value() = 3`}</Code>
            </div>
            <div>
              <h3 className="font-bold mb-2" style={{ color: "#22d3ee" }}>ATOMiK</h3>
              <Code>{`from atomik_core import AtomikContext

ctx = AtomikContext()
ctx.load(0)  # Reference = 0

# 3 nodes send deltas
# (order doesn't matter)
ctx.accum(0x01)  # Node A's delta
ctx.accum(0x02)  # Node B's delta
ctx.accum(0x04)  # Node C's delta

state = ctx.read()
# state = 0x07 (all bits set)

# Metadata: 0 bytes
# Merge function: XOR (universal)
# Garbage collection: not needed
# Undo any delta: re-apply it
ctx.accum(0x02)  # Undo node B
# state = 0x05`}</Code>
            </div>
          </div>

          <p>
            The G-Counter requires O(n) metadata per node, an O(n) merge function, and
            full vector exchange on every sync. ATOMiK requires 8 bytes of fixed metadata
            (the accumulator), a constant-time XOR merge, and sends only the 8-byte
            delta. As the number of nodes grows, the G-Counter&apos;s overhead grows
            linearly while ATOMiK stays constant.
          </p>

          {/* Comparison Table */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            CRDTs vs ATOMiK: 8 dimensions
          </h2>

          <div
            className="rounded-xl border overflow-hidden my-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <table className="w-full text-sm">
              <thead>
                <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>
                    Dimension
                  </th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#f59e0b" }}>
                    CRDTs
                  </th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#22d3ee" }}>
                    ATOMiK
                  </th>
                </tr>
              </thead>
              <tbody style={{ color: "#b0b0c0" }}>
                {[
                  [
                    "Merge function",
                    "Type-specific (max, union, LWW, etc.)",
                    "Universal (XOR)",
                  ],
                  [
                    "Metadata per node",
                    "O(n) — grows with cluster size",
                    "O(1) in fit model",
                  ],
                  [
                    "Sync payload",
                    "Full state vector or delta + causal context",
                    "8-byte delta",
                  ],
                  [
                    "Garbage collection",
                    "Required (tombstones, causal histories)",
                    "Not needed",
                  ],
                  [
                    "Undo support",
                    "Type-dependent, often impossible",
                    "Built-in (self-inverse)",
                  ],
                  [
                    "Types to learn",
                    "12+ (G-Counter, OR-Set, RGA, ...)",
                    "1 (AtomikContext)",
                  ],
                  [
                    "Formal verification",
                    "Per-type proofs (often informal)",
                    "Machine-checked algebra proofs",
                  ],
                  [
                    "Hardware acceleration",
                    "Impractical (complex merge logic)",
                    "Mapped to FPGA proof artifacts",
                  ],
                ].map(([dim, crdt, atomik], i) => (
                  <tr key={i} style={{ borderBottom: "1px solid #1e1e2e" }}>
                    <td className="p-4 font-medium" style={{ color: "#e0e0e8" }}>
                      {dim}
                    </td>
                    <td className="text-center p-4">{crdt}</td>
                    <td className="text-center p-4">{atomik}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          {/* Scaling comparison */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            How they scale differently
          </h2>

          <p>
            The scaling difference becomes dramatic in real deployments. Consider a
            100-node cluster syncing state every second:
          </p>

          <div
            className="rounded-xl border p-6 my-6 grid md:grid-cols-2 gap-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div>
              <h3 className="font-bold mb-3" style={{ color: "#f59e0b" }}>
                CRDT G-Counter (100 nodes)
              </h3>
              <div className="space-y-2 text-sm" style={{ color: "#b0b0c0" }}>
                <p>Metadata per node: <strong style={{ color: "#e0e0e8" }}>800 bytes</strong> (100 x 8-byte counters)</p>
                <p>Sync payload: <strong style={{ color: "#e0e0e8" }}>800 bytes</strong> (full vector)</p>
                <p>Network per second: <strong style={{ color: "#e0e0e8" }}>~80 KB</strong> (100 nodes x 800 bytes)</p>
                <p>Merge cost: <strong style={{ color: "#e0e0e8" }}>O(100)</strong> comparisons per merge</p>
                <p>With 1,000 nodes: <strong style={{ color: "#e0e0e8" }}>~8 MB/s</strong> just for metadata</p>
              </div>
            </div>
            <div>
              <h3 className="font-bold mb-3" style={{ color: "#22d3ee" }}>
                ATOMiK (100 nodes)
              </h3>
              <div className="space-y-2 text-sm" style={{ color: "#b0b0c0" }}>
                <p>Metadata per node: <strong style={{ color: "#e0e0e8" }}>8 bytes</strong> (one accumulator)</p>
                <p>Sync payload: <strong style={{ color: "#e0e0e8" }}>8 bytes</strong> (one delta)</p>
                <p>Network per second: <strong style={{ color: "#e0e0e8" }}>~800 bytes</strong> (100 nodes x 8 bytes)</p>
                <p>Merge cost: <strong style={{ color: "#e0e0e8" }}>O(1)</strong> (single XOR)</p>
                <p>With 1,000 nodes: <strong style={{ color: "#e0e0e8" }}>~8 KB/s</strong> (same per-node cost)</p>
              </div>
            </div>
          </div>

          <p>
            At 1,000 nodes, the illustrative CRDT approach can consume more network
            bandwidth for a comparable modeled convergence path. Quote production
            reduction only with a measured workload artifact.
          </p>

          {/* When to use CRDTs */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            When CRDTs are still the right choice
          </h2>

          <p>
            This is not a &quot;CRDTs are bad&quot; article. CRDTs solve real problems that
            ATOMiK does not. Use CRDTs when:
          </p>

          <ul className="list-disc pl-6 space-y-3">
            <li>
              <strong>You need rich data type semantics.</strong> A collaborative text
              editor needs character-level insert/delete with intent preservation. CRDTs
              like RGA or Yjs are purpose-built for this. ATOMiK operates on binary state,
              not structured documents.
            </li>
            <li>
              <strong>You need a monotonic counter.</strong> If &quot;the count only goes
              up&quot; is a business invariant (e.g., distributed likes, event counters),
              a G-Counter enforces this at the type level. ATOMiK&apos;s XOR is invertible
              by design — it does not enforce monotonicity.
            </li>
            <li>
              <strong>You need set semantics with add/remove.</strong> The OR-Set provides
              &quot;add wins&quot; semantics where concurrent add and remove of the same
              element resolves to add. ATOMiK does not have built-in set operations.
            </li>
            <li>
              <strong>Your ecosystem already uses CRDTs.</strong> If you&apos;re building on
              Automerge, Yjs, or Riak, you already have CRDT infrastructure. Introducing
              ATOMiK alongside may not be worth the integration cost.
            </li>
          </ul>

          {/* When to use ATOMiK */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            When ATOMiK is the better choice
          </h2>

          <p>Use delta-state algebra instead of CRDTs when:</p>

          <ul className="list-disc pl-6 space-y-3">
            <li>
              <strong>You need binary state convergence.</strong> Configuration flags,
              sensor readings, game state, cache entries, fingerprints. The data is
              bytes, not a structured document.
            </li>
            <li>
              <strong>Metadata overhead matters.</strong> Edge devices, IoT sensors,
              mobile apps, satellite links. Compare fixed-width delta artifacts against
              measured CRDT payloads before making constrained-hardware claims.
            </li>
            <li>
              <strong>You need undo.</strong> Self-inverse means re-applying a delta
              cancels it. No compensating operations, no tombstones, no undo stack.
            </li>
            <li>
              <strong>You want one API.</strong> Four operations, one type, universal
              merge. No type selection matrix, no merge function design, no
              convergence proof per type.
            </li>
            <li>
              <strong>Performance is critical.</strong> XOR is a single CPU instruction
              and a single logic gate. ATOMiK has synthesis-characterized FPGA
              scaling notes, but workload claims should be repeated only with the
              linked artifact and evidence label.
            </li>
            <li>
              <strong>Many nodes, frequent sync.</strong> ATOMiK&apos;s O(1) metadata and
              O(1) merge mean overhead is constant regardless of cluster size. CRDTs
              scale with node count.
            </li>
            <li>
              <strong>Formal proof coverage matters.</strong> The algebraic proof artifacts
              cover the fit state model. Deployment claims still need workload-specific
              validation.
            </li>
          </ul>

          {/* The deeper difference */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            The deeper architectural difference
          </h2>

          <p>
            CRDTs are <em>data structures</em>. Each one encodes a specific merge
            strategy for a specific data type. The merge function is part of the type
            definition.
          </p>

          <p>
            ATOMiK is a <em>state primitive</em>. It does not know what your data means.
            It knows that deltas compose via XOR, that composition is commutative and
            associative, and that every delta is self-inverse. These properties are
            universal — they hold for any data, any topology, any order of operations.
          </p>

          <p>
            This is the same distinction as between a hash map (data structure with
            specific semantics) and a memory cell (universal primitive). You can build
            a hash map from memory cells, but you cannot build a memory cell from a hash
            map. Similarly, you can encode CRDT-like semantics on top of ATOMiK contexts,
            but the reverse is not true.
          </p>

          <Code>{`# CRDT approach: choose a type, implement merge
class MyCounter(CRDT):
    type = "G-Counter"
    merge = element_wise_max
    metadata = vector_of_node_counts

# ATOMiK approach: use the primitive
ctx = AtomikContext()
ctx.load(initial)
ctx.accum(delta)
# Convergence is algebraic, not type-dependent`}</Code>

          {/* Getting started */}
          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Getting started
          </h2>

          <p>
            If you are evaluating CRDTs for a new project, try ATOMiK first. The Python
            SDK installs in seconds and the API is four functions:
          </p>

          <Code>{`pip install atomik-core`}</Code>

          <Code>{`from atomik_core import AtomikContext

# Create two "nodes"
a = AtomikContext()
b = AtomikContext()

# Same initial state
a.load(0xCAFEBABE)
b.load(0xCAFEBABE)

# Each makes independent changes
a.accum(0x0000FFFF)
b.accum(0xFFFF0000)

# Exchange deltas (any order, any transport)
a.accum(0xFFFF0000)  # A receives B's delta
b.accum(0x0000FFFF)  # B receives A's delta

# Both converge under the XOR algebra model
assert a.read() == b.read()  # 0x3531B541
print(f"Converged: 0x{a.read():08X}")`}</Code>

          <p>
            For the full category overview including comparisons with event sourcing, OT,
            and consensus protocols, see{" "}
            <Link href="/blog/what-is-delta-state-computing" className="underline" style={{ color: "#4f8fff" }}>
              What is Delta-State Computing?
            </Link>
          </p>

          <p>
            For kernel-level COW detection and per-container waste tracking
            notes, see{" "}
            <Link href="/docs/kernel-module" className="underline" style={{ color: "#4f8fff" }}>
              the kernel module docs
            </Link>
            {" "}and start a{" "}
            <Link href="/pricing" className="underline" style={{ color: "#4f8fff" }}>
              request evaluation access
            </Link>.
          </p>

          <div
            className="rounded-xl border p-8 mt-8"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <h3 className="text-xl font-bold mb-4 text-center">
              Build conflict-free systems without the complexity
            </h3>
            <EmailCapture variant="blog" />
          </div>
        </div>
      </article>
    </div>
  );
}
