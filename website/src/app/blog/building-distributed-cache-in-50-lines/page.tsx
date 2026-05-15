import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

export const metadata: Metadata = {
  title: "Build a Distributed Cache in 50 Lines of Python — ATOMiK Blog",
  description:
    "A step-by-step tutorial for a 3-node distributed-cache model using ATOMiK delta-state algebra. Educational pattern, not a production distributed-systems guarantee.",
};

function Code({ children }: { children: string }) {
  return (
    <pre className="rounded-lg p-4 text-sm overflow-x-auto my-4" style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", color: "#22d3ee", fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace", lineHeight: 1.7 }}>
      <code>{children}</code>
    </pre>
  );
}

export default function DistributedCachePost() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="Blog" />

      <article className="max-w-3xl mx-auto px-6 pt-16 pb-24">
        <div className="mb-12">
          <Link href="/blog" className="text-sm mb-6 inline-block hover:underline" style={{ color: "#4f8fff" }}>&larr; Back to blog</Link>
          <time className="block text-sm font-mono mb-3" style={{ color: "#8888a0" }}>March 16, 2026</time>
          <h1 className="text-3xl font-bold tracking-tight mb-4 leading-tight">
            Build a Distributed Cache in 50 Lines of Python
          </h1>
          <div className="flex gap-2">
            {["tutorial", "python", "distributed-systems"].map((tag) => (
              <span key={tag} className="text-xs px-2 py-0.5 rounded-full" style={{ background: "rgba(34, 211, 238, 0.1)", color: "#22d3ee", border: "1px solid rgba(34, 211, 238, 0.2)" }}>{tag}</span>
            ))}
          </div>
        </div>

        <div className="space-y-6 text-base leading-relaxed" style={{ color: "#c8c8d4" }}>
          <p>
            Traditional distributed caches need leader election, consensus rounds, and
            conflict resolution. With ATOMiK, you can build a 3-node cache where writes
            from any node converge under the fit model — in about 50 lines of Python.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>Setup</h2>
          <Code>{`pip install atomik-core`}</Code>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>Step 1: Create the cache node</h2>
          <p>Each node maintains its own <code style={{ color: "#22d3ee" }}>DeltaStream</code> — a collection of delta-state contexts indexed by address.</p>
          <Code>{`from atomik_core import DeltaStream

class CacheNode:
    def __init__(self, node_id: str):
        self.node_id = node_id
        self.stream = DeltaStream()
        self.peers: list["CacheNode"] = []

    def connect(self, peer: "CacheNode"):
        self.peers.append(peer)
        peer.peers.append(self)`}</Code>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>Step 2: Write = LOAD + broadcast</h2>
          <p>
            When a node writes a new value, it LOADs the value locally and broadcasts
            the delta to all peers. The delta is just 8 bytes — the XOR difference.
          </p>
          <Code>{`    def put(self, key: int, value: int):
        """Write a value and broadcast the delta."""
        old = self.stream.read(key)
        self.stream.load(key, value)
        delta = old ^ value  # XOR delta
        # Broadcast to peers
        for peer in self.peers:
            peer.receive_delta(key, delta)`}</Code>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>Step 3: Receive = ACCUM</h2>
          <p>
            When a peer receives a delta, it accumulates it. XOR is commutative —
            the order deltas arrive doesn&apos;t matter. Every node converges to the same state.
          </p>
          <Code>{`    def receive_delta(self, key: int, delta: int):
        """Apply a delta from a peer."""
        self.stream.accum(key, delta)

    def get(self, key: int) -> int:
        """Read the current value."""
        return self.stream.read(key)`}</Code>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>Step 4: Wire it up</h2>
          <Code>{`# Create 3 nodes
a = CacheNode("A")
b = CacheNode("B")
c = CacheNode("C")

# Full mesh topology
a.connect(b)
a.connect(c)
b.connect(c)

# Initialize all nodes with the same reference
for node in [a, b, c]:
    node.stream.load(0, 0xCAFEBABE)

# Node A writes a new value
a.put(0, 0xDEADBEEF)

# All nodes converge — no consensus needed
assert a.get(0) == 0xDEADBEEF
assert b.get(0) == 0xDEADBEEF
assert c.get(0) == 0xDEADBEEF
print("All 3 nodes converged!")`}</Code>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>Why it works</h2>
          <p>The magic is XOR commutativity. When Node A writes <code style={{ color: "#22d3ee" }}>0xDEADBEEF</code> to a slot that held <code style={{ color: "#22d3ee" }}>0xCAFEBABE</code>, the delta is:</p>
          <Code>{`delta = 0xCAFEBABE ^ 0xDEADBEEF = 0x14531455`}</Code>
          <p>
            Nodes B and C each accumulate this delta. Since <code style={{ color: "#22d3ee" }}>reference XOR accumulator = current_state</code>, they reconstruct the same value:
          </p>
          <Code>{`0xCAFEBABE ^ 0x14531455 = 0xDEADBEEF  ✓`}</Code>
          <p>
            If multiple nodes write simultaneously, deltas compose. Node B writes to
            key 1 while Node C writes to key 2 — both deltas propagate and apply
            independently. No conflicts, no resolution logic, no coordinator.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>Concurrent writes to the same key</h2>
          <p>
            What happens if A and B both write to key 0 at the same time? Both deltas
            propagate to all nodes. The final state is deterministic — it&apos;s the XOR of
            both deltas applied to the reference. The &quot;last writer wins&quot; semantic is
            replaced by &quot;all writers compose&quot; — which is correct for many workloads
            (counters, flags, accumulated state).
          </p>
          <p>
            For workloads where you need last-writer-wins, use <code style={{ color: "#22d3ee" }}>SWAP</code>
            to create epochs — each SWAP resets the accumulator and promotes the current
            state to the new reference.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>Add wire serialization</h2>
          <p>
            In v0.3.0, <code style={{ color: "#22d3ee" }}>DeltaMessage</code> gained
            compact wire format support:
          </p>
          <Code>{`from atomik_core import DeltaMessage

# On sender
msg = DeltaMessage(addr=0, delta=0x14531455, seq=1)
wire = msg.to_bytes()  # 16 bytes, network byte order

# On receiver
msg = DeltaMessage.from_bytes(wire)
stream.accum(msg.addr, msg.delta)`}</Code>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>What you get</h2>
          <ul className="list-disc pl-6 space-y-2">
            <li><strong>Modeled coordination reduction.</strong> The example avoids leader election inside the toy XOR state path.</li>
            <li><strong>8-byte delta artifact.</strong> The tutorial uses a fixed-width delta representation for its model.</li>
            <li><strong>O(1) model operations.</strong> The accumulator path is constant-time in this simplified example.</li>
            <li><strong>Modeled convergence.</strong> Nodes reach the same state when the assumptions of the tutorial hold.</li>
            <li><strong>Proof-scoped.</strong> Lean4 proof artifacts verify the algebraic properties used by the model.</li>
          </ul>

          <div className="rounded-xl border p-8 mt-8" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
            <h3 className="text-xl font-bold mb-4 text-center">Try ATOMiK today</h3>
            <EmailCapture variant="blog" />
          </div>
        </div>
      </article>
    </div>
  );
}
