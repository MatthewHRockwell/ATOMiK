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
  title: "Migrate from CRDTs to ATOMiK — Docs",
  description:
    "Step-by-step migration guide from CRDTs to ATOMiK delta-state algebra. Conceptual mapping, code examples, performance comparison, and checklist.",
  keywords: [
    "CRDT migration",
    "CRDT alternative",
    "delta-state algebra",
    "conflict-free replication",
    "ATOMiK migration guide",
    "CRDT vs ATOMiK",
    "distributed state sync",
  ],
  openGraph: {
    title: "Migrate from CRDTs to ATOMiK — ATOMiK Docs",
    description:
      "Replace join-semilattice CRDTs with Abelian group delta-state algebra. Migration guide with code examples and performance comparison.",
    type: "website",
  },
};

/* ------------------------------------------------------------------ */
/*  Data                                                               */
/* ------------------------------------------------------------------ */

const conceptMap = [
  {
    crdt: "CRDT State",
    atomik: "AtomikContext",
    notes: "Both represent a convergent replicated value. ATOMiK separates reference state from accumulated deltas.",
  },
  {
    crdt: "merge(a, b)",
    atomik: "ctx.accum(delta)",
    notes: "CRDT merge is a join on the semilattice. ATOMiK accumulation is XOR in an Abelian group. Both are commutative.",
  },
  {
    crdt: "CRDT Payload",
    atomik: "Delta (XOR diff)",
    notes: "CRDTs transmit full state or operation. ATOMiK transmits the XOR of what changed -- typically much smaller.",
  },
  {
    crdt: "Causality metadata",
    atomik: "Model-dependent",
    notes: "Vector clocks, version vectors, and dot stores may not be needed inside a fit XOR model. Protocol-level causality remains workload-specific.",
  },
  {
    crdt: "Conflict resolution",
    atomik: "Algebraic convergence",
    notes: "CRDTs resolve via lattice join (LWW, OR-Set, etc). ATOMiK converges via Abelian-group algebra when the state model fits; application conflicts may still need product-specific handling.",
  },
  {
    crdt: "query()",
    atomik: "ctx.read()",
    notes: "Both return current value. CRDT queries the lattice state. ATOMiK reconstructs: initial_state XOR accumulator.",
  },
];

const perfComparison = [
  {
    dimension: "Merge complexity",
    crdt: "O(n) -- depends on state size",
    atomik: "O(1) -- single XOR operation",
  },
  {
    dimension: "Metadata overhead",
    crdt: "Vector clocks: O(nodes), Dot stores: O(ops)",
    atomik: "Zero -- no causality metadata",
  },
  {
    dimension: "Sync bandwidth",
    crdt: "Full state (state-based) or op + context (op-based)",
    atomik: "8-byte delta only",
  },
  {
    dimension: "State reconstruction",
    crdt: "O(1) if materialized, O(n) if from ops",
    atomik: "O(1) always -- initial XOR accumulator",
  },
  {
    dimension: "Undo / rollback",
    crdt: "Not supported (lattice is monotonic)",
    atomik: "Free -- apply same delta again (self-inverse)",
  },
  {
    dimension: "Formal guarantees",
    crdt: "SEC (Strong Eventual Consistency)",
    atomik: "SEC + self-inverse + 108 Lean4 proofs",
  },
  {
    dimension: "Implementation complexity",
    crdt: "High -- many CRDT types, each with edge cases",
    atomik: "Minimal -- 4 operations, one algebraic structure",
  },
];

const migrationChecklist = [
  {
    phase: "Assessment",
    items: [
      "Inventory all CRDT types in use (G-Counter, PN-Counter, OR-Set, LWW-Register, etc.)",
      "Identify which CRDTs rely on monotonic growth (ATOMiK supports non-monotonic via self-inverse)",
      "Map causality metadata dependencies -- if you need causal ordering, ATOMiK alone may not suffice",
      "Benchmark current merge latency and sync bandwidth as a baseline",
    ],
  },
  {
    phase: "Prototype",
    items: [
      "Install atomik-core: pip install atomik-core",
      "Replace one simple CRDT (G-Counter or LWW-Register) with AtomikContext",
      "Validate convergence: feed the same deltas in different orders, confirm identical read()",
      "Measure bandwidth reduction: compare CRDT sync payload vs 8-byte ATOMiK delta",
    ],
  },
  {
    phase: "Integration",
    items: [
      "Replace CRDT merge logic with accum() calls in your replication layer",
      "Remove vector clock / dot store metadata from your wire protocol",
      "Update serialization: deltas are fixed-size integers, not structured CRDT payloads",
      "Implement self-inverse undo if your application needs rollback (was impossible with CRDTs)",
    ],
  },
  {
    phase: "Validation",
    items: [
      "Run convergence tests: all replicas must reach identical state regardless of delta order",
      "Verify self-inverse property: double-applying any delta returns to previous state",
      "Load test: confirm O(1) merge performance holds under production traffic",
      "Monitor bandwidth: measure actual reduction vs CRDT baseline",
    ],
  },
];

/* ------------------------------------------------------------------ */
/*  Page                                                               */
/* ------------------------------------------------------------------ */

export default function MigrateCrdtPage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#8b5cf6" }}
      >
        Migration Guide
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">
        Migrate from CRDTs to ATOMiK
      </h1>
      <p className="text-lg mb-6" style={{ color: "#8888a0" }}>
        CRDTs and ATOMiK both solve distributed convergence without coordination.
        This guide maps CRDT concepts to ATOMiK operations, shows before/after
        code for three common patterns, and provides a step-by-step migration
        checklist.
      </p>

      {/* When to migrate */}
      <div
        className="rounded-xl p-6 border mb-12"
        style={{
          background: "linear-gradient(135deg, rgba(139,92,246,0.06), rgba(79,143,255,0.06))",
          borderColor: "#8b5cf630",
        }}
      >
        <p className="text-sm font-semibold mb-3" style={{ color: "#8b5cf6" }}>
          When to migrate (and when not to)
        </p>
        <div className="grid md:grid-cols-2 gap-6">
          <div>
            <p className="text-sm font-semibold mb-2" style={{ color: "#22c55e" }}>
              Migrate when:
            </p>
            <ul className="space-y-2 text-sm" style={{ color: "#b0b0c0" }}>
              <li className="flex gap-2">
                <span style={{ color: "#22c55e" }} className="shrink-0">{"\u2713"}</span>
                Your CRDT merge is a performance bottleneck (O(n) state merges)
              </li>
              <li className="flex gap-2">
                <span style={{ color: "#22c55e" }} className="shrink-0">{"\u2713"}</span>
                Causality metadata (vector clocks, dot stores) dominates bandwidth
              </li>
              <li className="flex gap-2">
                <span style={{ color: "#22c55e" }} className="shrink-0">{"\u2713"}</span>
                You need undo/rollback (impossible with monotonic CRDTs)
              </li>
              <li className="flex gap-2">
                <span style={{ color: "#22c55e" }} className="shrink-0">{"\u2713"}</span>
                Your data is numeric or binary (counters, flags, fingerprints)
              </li>
              <li className="flex gap-2">
                <span style={{ color: "#22c55e" }} className="shrink-0">{"\u2713"}</span>
                You want formal verification (108 Lean4 proofs vs informal CRDT specs)
              </li>
            </ul>
          </div>
          <div>
            <p className="text-sm font-semibold mb-2" style={{ color: "#ef4444" }}>
              Stay with CRDTs when:
            </p>
            <ul className="space-y-2 text-sm" style={{ color: "#b0b0c0" }}>
              <li className="flex gap-2">
                <span style={{ color: "#ef4444" }} className="shrink-0">{"\u2717"}</span>
                You need rich set semantics (OR-Set add/remove with tombstones)
              </li>
              <li className="flex gap-2">
                <span style={{ color: "#ef4444" }} className="shrink-0">{"\u2717"}</span>
                Your data requires causal ordering guarantees between operations
              </li>
              <li className="flex gap-2">
                <span style={{ color: "#ef4444" }} className="shrink-0">{"\u2717"}</span>
                You use collaborative text editing (CRDTs like Yjs/Automerge excel here)
              </li>
              <li className="flex gap-2">
                <span style={{ color: "#ef4444" }} className="shrink-0">{"\u2717"}</span>
                Your CRDT library already meets performance requirements
              </li>
            </ul>
          </div>
        </div>
      </div>

      {/* Concept mapping table */}
      <h2 className="text-2xl font-bold mb-4">Conceptual Mapping</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        CRDTs use a join-semilattice. ATOMiK uses an Abelian group. The algebraic
        structure is different, but the modeled convergence property is related -- any
        permutation of operations produces the same final state.
      </p>
      <div
        className="rounded-xl border overflow-x-auto mb-12"
        style={{ background: "#12121a", borderColor: "#1e1e2e" }}
      >
        <table className="w-full text-sm min-w-[640px]">
          <thead>
            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
              <th className="text-left px-5 py-4 font-semibold" style={{ color: "#8888a0" }}>
                CRDT
              </th>
              <th className="text-left px-5 py-4 font-semibold" style={{ color: "#8b5cf6" }}>
                ATOMiK
              </th>
              <th className="text-left px-5 py-4 font-semibold" style={{ color: "#8888a0" }}>
                Notes
              </th>
            </tr>
          </thead>
          <tbody>
            {conceptMap.map((row, i) => (
              <tr
                key={row.crdt}
                style={{
                  borderBottom: i < conceptMap.length - 1 ? "1px solid #1e1e2e" : "none",
                }}
              >
                <td className="px-5 py-3.5 font-mono text-xs" style={{ color: "#ff6b6b" }}>
                  {row.crdt}
                </td>
                <td className="px-5 py-3.5 font-mono text-xs" style={{ color: "#22c55e" }}>
                  {row.atomik}
                </td>
                <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                  {row.notes}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {/* Pattern 1: G-Counter */}
      <h2 className="text-2xl font-bold mb-4">Pattern 1: G-Counter / PN-Counter</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        CRDTs use separate increment/decrement maps per node, merged with max().
        ATOMiK uses a single accumulator with self-inverse deltas.
      </p>
      <div className="grid md:grid-cols-2 gap-4 mb-12">
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#ef4444" }}>
            Before: CRDT PN-Counter
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: cmtColor }}># Each node maintains its own counter vector</span>
                {"\n"}
                <span style={{ color: kwColor }}>class</span>
                <span style={{ color: typeColor }}> PNCounter</span>
                <span style={{ color: varColor }}>:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> __init__</span>
                <span style={{ color: varColor }}>(self, node_id, num_nodes):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}self.p = [0] * num_nodes</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}self.n = [0] * num_nodes</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}self.id = node_id</span>
                {"\n\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> increment</span>
                <span style={{ color: varColor }}>(self): self.p[self.id] += 1</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> decrement</span>
                <span style={{ color: varColor }}>(self): self.n[self.id] += 1</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> value</span>
                <span style={{ color: varColor }}>(self): </span>
                <span style={{ color: kwColor }}>return</span>
                <span style={{ color: varColor }}> sum(self.p) - sum(self.n)</span>
                {"\n\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> merge</span>
                <span style={{ color: varColor }}>(self, other):</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"        "}# O(n) merge per node</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>for</span>
                <span style={{ color: varColor }}> i </span>
                <span style={{ color: kwColor }}>in</span>
                <span style={{ color: varColor }}> range(len(self.p)):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.p[i] = max(self.p[i], other.p[i])</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.n[i] = max(self.n[i], other.n[i])</span>
              </code>
            </pre>
          </div>
        </div>
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#22c55e" }}>
            After: ATOMiK
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: kwColor }}>from</span>
                <span style={{ color: varColor }}> atomik_core </span>
                <span style={{ color: kwColor }}>import</span>
                <span style={{ color: typeColor }}> AtomikContext</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Single context, any node can accumulate</span>
                {"\n"}
                <span style={{ color: varColor }}>counter = </span>
                <span style={{ color: typeColor }}>AtomikContext</span>
                <span style={{ color: varColor }}>()</span>
                {"\n"}
                <span style={{ color: varColor }}>counter.</span>
                <span style={{ color: fnColor }}>load</span>
                <span style={{ color: varColor }}>(</span>
                <span style={{ color: numColor }}>0</span>
                <span style={{ color: varColor }}>)</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Any node: accumulate a delta</span>
                {"\n"}
                <span style={{ color: varColor }}>counter.</span>
                <span style={{ color: fnColor }}>accum</span>
                <span style={{ color: varColor }}>(delta_value)</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Read: O(1), no merge needed</span>
                {"\n"}
                <span style={{ color: varColor }}>current = counter.</span>
                <span style={{ color: fnColor }}>read</span>
                <span style={{ color: varColor }}>()</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Undo: apply same delta again</span>
                {"\n"}
                <span style={{ color: varColor }}>counter.</span>
                <span style={{ color: fnColor }}>accum</span>
                <span style={{ color: varColor }}>(delta_value)</span>
                <span style={{ color: cmtColor }}> # self-inverse</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Sync: send 8-byte delta, not full state</span>
                {"\n"}
                <span style={{ color: cmtColor }}># No node IDs. No vector clocks.</span>
                {"\n"}
                <span style={{ color: cmtColor }}># No O(n) merge. Just XOR.</span>
              </code>
            </pre>
          </div>
        </div>
      </div>

      {/* Pattern 2: LWW-Register */}
      <h2 className="text-2xl font-bold mb-4">Pattern 2: LWW-Register</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        Last-Writer-Wins registers require synchronized clocks or hybrid logical clocks.
        ATOMiK can remove timestamp dependency in the modeled register path -- convergence is algebraic, not temporal.
      </p>
      <div className="grid md:grid-cols-2 gap-4 mb-12">
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#ef4444" }}>
            Before: LWW-Register
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: kwColor }}>class</span>
                <span style={{ color: typeColor }}> LWWRegister</span>
                <span style={{ color: varColor }}>:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> __init__</span>
                <span style={{ color: varColor }}>(self):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}self.value = </span>
                <span style={{ color: kwColor }}>None</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}self.timestamp = </span>
                <span style={{ color: numColor }}>0</span>
                {"\n\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> set</span>
                <span style={{ color: varColor }}>(self, val, ts):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>if</span>
                <span style={{ color: varColor }}> ts {">"} self.timestamp:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.value = val</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.timestamp = ts</span>
                {"\n\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> merge</span>
                <span style={{ color: varColor }}>(self, other):</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"        "}# Requires synchronized clocks!</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>if</span>
                <span style={{ color: varColor }}> other.timestamp {">"} self.timestamp:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.value = other.value</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.timestamp = other.timestamp</span>
              </code>
            </pre>
          </div>
        </div>
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#22c55e" }}>
            After: ATOMiK
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: kwColor }}>from</span>
                <span style={{ color: varColor }}> atomik_core </span>
                <span style={{ color: kwColor }}>import</span>
                <span style={{ color: typeColor }}> AtomikContext</span>
                {"\n\n"}
                <span style={{ color: varColor }}>reg = </span>
                <span style={{ color: typeColor }}>AtomikContext</span>
                <span style={{ color: varColor }}>()</span>
                {"\n"}
                <span style={{ color: varColor }}>reg.</span>
                <span style={{ color: fnColor }}>load</span>
                <span style={{ color: varColor }}>(initial_value)</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Update: compute delta from current state</span>
                {"\n"}
                <span style={{ color: varColor }}>delta = new_value ^ reg.</span>
                <span style={{ color: fnColor }}>read</span>
                <span style={{ color: varColor }}>()</span>
                {"\n"}
                <span style={{ color: varColor }}>reg.</span>
                <span style={{ color: fnColor }}>accum</span>
                <span style={{ color: varColor }}>(delta)</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># No timestamps. No clock sync.</span>
                {"\n"}
                <span style={{ color: cmtColor }}># Convergence is algebraic:</span>
                {"\n"}
                <span style={{ color: cmtColor }}># all replicas accumulating the same</span>
                {"\n"}
                <span style={{ color: cmtColor }}># deltas reach the same state.</span>
              </code>
            </pre>
          </div>
        </div>
      </div>

      {/* Pattern 3: OR-Set */}
      <h2 className="text-2xl font-bold mb-4">Pattern 3: Distributed State Sync</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        Multi-node state synchronization using CRDTs requires transmitting full state or
        operation logs with causal metadata. ATOMiK transmits only the delta and converges
        regardless of delivery order.
      </p>
      <div className="grid md:grid-cols-2 gap-4 mb-12">
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#ef4444" }}>
            Before: State-based CRDT Sync
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: cmtColor }}># Node A sends full state to Node B</span>
                {"\n"}
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> sync_to_peer</span>
                <span style={{ color: varColor }}>(local_crdt, peer):</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# Serialize entire CRDT state</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}payload = local_crdt.</span>
                <span style={{ color: fnColor }}>serialize</span>
                <span style={{ color: varColor }}>()</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# Includes: values + vector clock</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# + tombstones + node metadata</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}peer.</span>
                <span style={{ color: fnColor }}>send</span>
                <span style={{ color: varColor }}>(payload)</span>
                <span style={{ color: cmtColor }}> # O(state_size)</span>
                {"\n\n"}
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> on_receive</span>
                <span style={{ color: varColor }}>(local_crdt, payload):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}remote = </span>
                <span style={{ color: typeColor }}>CRDT</span>
                <span style={{ color: varColor }}>.</span>
                <span style={{ color: fnColor }}>deserialize</span>
                <span style={{ color: varColor }}>(payload)</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}local_crdt.</span>
                <span style={{ color: fnColor }}>merge</span>
                <span style={{ color: varColor }}>(remote)</span>
                <span style={{ color: cmtColor }}> # O(n) merge</span>
              </code>
            </pre>
          </div>
        </div>
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#22c55e" }}>
            After: ATOMiK Delta Sync
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: cmtColor }}># Node A sends 8-byte delta to Node B</span>
                {"\n"}
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> sync_to_peer</span>
                <span style={{ color: varColor }}>(ctx, peer, delta):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}peer.</span>
                <span style={{ color: fnColor }}>send</span>
                <span style={{ color: varColor }}>(delta)</span>
                <span style={{ color: cmtColor }}> # 8 bytes, always</span>
                {"\n\n"}
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> on_receive</span>
                <span style={{ color: varColor }}>(ctx, delta):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}ctx.</span>
                <span style={{ color: fnColor }}>accum</span>
                <span style={{ color: varColor }}>(delta)</span>
                <span style={{ color: cmtColor }}> # O(1), done</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># No serialization overhead</span>
                {"\n"}
                <span style={{ color: cmtColor }}># No vector clocks</span>
                {"\n"}
                <span style={{ color: cmtColor }}># No tombstone garbage collection</span>
                {"\n"}
                <span style={{ color: cmtColor }}># No merge conflicts</span>
                {"\n"}
                <span style={{ color: cmtColor }}># Order-independent convergence</span>
              </code>
            </pre>
          </div>
        </div>
      </div>

      {/* Performance comparison */}
      <h2 className="text-2xl font-bold mb-4">Performance Comparison</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        Where CRDTs incur per-node or per-operation overhead for causality tracking,
        ATOMiK operates with constant-time, constant-space accumulation.
      </p>
      <div
        className="rounded-xl border overflow-x-auto mb-12"
        style={{ background: "#12121a", borderColor: "#1e1e2e" }}
      >
        <table className="w-full text-sm min-w-[640px]">
          <thead>
            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
              <th className="text-left px-5 py-4 font-semibold" style={{ color: "#8888a0" }}>
                Dimension
              </th>
              <th className="text-left px-5 py-4 font-semibold" style={{ color: "#8888a0" }}>
                CRDTs
              </th>
              <th className="text-left px-5 py-4 font-semibold" style={{ color: "#8b5cf6" }}>
                ATOMiK
              </th>
            </tr>
          </thead>
          <tbody>
            {perfComparison.map((row, i) => (
              <tr
                key={row.dimension}
                style={{
                  borderBottom: i < perfComparison.length - 1 ? "1px solid #1e1e2e" : "none",
                }}
              >
                <td className="px-5 py-3.5 font-medium" style={{ color: "#b0b0c0" }}>
                  {row.dimension}
                </td>
                <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                  {row.crdt}
                </td>
                <td className="px-5 py-3.5" style={{ color: "#22c55e" }}>
                  {row.atomik}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {/* Migration checklist */}
      <h2 className="text-2xl font-bold mb-6">Migration Checklist</h2>
      <div className="space-y-6 mb-12">
        {migrationChecklist.map((phase, pi) => (
          <div
            key={phase.phase}
            className="rounded-xl border p-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div className="flex items-center gap-3 mb-4">
              <span
                className="w-8 h-8 rounded-lg flex items-center justify-center text-sm font-bold"
                style={{
                  background: "rgba(139, 92, 246, 0.1)",
                  border: "1px solid rgba(139, 92, 246, 0.2)",
                  color: "#8b5cf6",
                }}
              >
                {pi + 1}
              </span>
              <h3 className="text-lg font-semibold">{phase.phase}</h3>
            </div>
            <ul className="space-y-3">
              {phase.items.map((item) => (
                <li key={item} className="flex gap-3 text-sm" style={{ color: "#b0b0c0" }}>
                  <span
                    className="w-5 h-5 rounded border flex items-center justify-center shrink-0 mt-0.5"
                    style={{ borderColor: "#333", background: "#0a0a0f" }}
                  />
                  {item}
                </li>
              ))}
            </ul>
          </div>
        ))}
      </div>

      {/* Key algebraic difference callout */}
      <div
        className="rounded-xl p-6 border mb-12"
        style={{
          background: "linear-gradient(135deg, rgba(139,92,246,0.06), rgba(79,143,255,0.06))",
          borderColor: "#8b5cf630",
        }}
      >
        <p className="text-sm font-semibold mb-2" style={{ color: "#8b5cf6" }}>
          The algebraic difference
        </p>
        <p style={{ color: "#b0b0c0" }}>
          CRDTs form a <strong>join-semilattice</strong>: merge is commutative, associative,
          and idempotent, but not invertible. State can only grow. ATOMiK forms an{" "}
          <strong>Abelian group</strong>: merge (XOR) is commutative, associative, has an
          identity element, and every element is its own inverse. This gives you free undo,
          constant-size state and lower metadata pressure -- at the cost of lattice
          monotonicity. If your application needs monotonic convergence (e.g., grow-only sets),
          CRDTs remain the right choice.
        </p>
      </div>

    </div>
  );
}
