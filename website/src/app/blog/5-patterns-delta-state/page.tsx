import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

export const metadata: Metadata = {
  title: "5 Design Patterns for Delta-State Algebra — ATOMiK Blog",
  description: "Practical patterns: accumulator fan-in, epoch checkpointing, fingerprint gates, rollback chains, and multi-stream convergence.",
};

function Code({ children }: { children: string }) {
  return (
    <pre className="rounded-lg p-4 text-sm overflow-x-auto my-4" style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", color: "#22d3ee", fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace", lineHeight: 1.7 }}>
      <code>{children}</code>
    </pre>
  );
}

function Pattern({ num, title, children }: { num: number; title: string; children: React.ReactNode }) {
  return (
    <section className="mb-12">
      <div className="flex items-center gap-3 mb-4">
        <span className="shrink-0 w-8 h-8 rounded-lg flex items-center justify-center text-sm font-bold" style={{ background: "rgba(139,92,246,0.15)", color: "#8b5cf6" }}>{num}</span>
        <h2 className="text-2xl font-bold" style={{ color: "#e0e0e8" }}>{title}</h2>
      </div>
      <div className="space-y-4 text-base leading-relaxed" style={{ color: "#c8c8d4" }}>{children}</div>
    </section>
  );
}

export default function PatternsPost() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="Blog" />
      <article className="max-w-3xl mx-auto px-6 pt-16 pb-24">
        <div className="mb-12">
          <Link href="/blog" className="text-sm mb-6 inline-block hover:underline" style={{ color: "#4f8fff" }}>&larr; Back to blog</Link>
          <time className="block text-sm font-mono mb-3" style={{ color: "#8888a0" }}>March 16, 2026</time>
          <h1 className="text-3xl font-bold tracking-tight mb-4 leading-tight">5 Design Patterns for Delta-State Algebra</h1>
          <div className="flex gap-2">
            {["patterns", "architecture", "python"].map((t) => (
              <span key={t} className="text-xs px-2 py-0.5 rounded-full" style={{ background: "rgba(139,92,246,0.1)", color: "#8b5cf6", border: "1px solid rgba(139,92,246,0.2)" }}>{t}</span>
            ))}
          </div>
        </div>

        <p className="text-base leading-relaxed mb-8" style={{ color: "#c8c8d4" }}>
          ATOMiK gives you four operations. What you build with them depends on how you
          compose them. Here are five patterns we&apos;ve seen work well in production systems.
        </p>

        <Pattern num={1} title="Accumulator Fan-In">
          <p>
            Multiple producers write deltas to the same context from different threads,
            processes, or machines. Because XOR is commutative, no coordination is needed.
          </p>
          <Code>{`from atomik_core import AtomikContext
import threading

ctx = AtomikContext()
ctx.load(0)

def worker(deltas):
    for d in deltas:
        ctx.accum(d)  # Lock-free, order-independent

# 4 threads writing concurrently
threads = [threading.Thread(target=worker, args=([0xFF << (i*8)],))
           for i in range(4)]
for t in threads: t.start()
for t in threads: t.join()

# All deltas composed — result is deterministic
print(f"0x{ctx.read():08x}")  # 0xff_ff_ff_ff`}</Code>
          <p>
            <strong>Use when:</strong> Aggregating metrics, counters, or flags from multiple sources.
            Sensor fusion, multi-player game state, distributed log aggregation.
          </p>
        </Pattern>

        <Pattern num={2} title="Epoch Checkpointing">
          <p>
            Use SWAP to create periodic checkpoints. Each SWAP captures the current state
            as the new reference and resets the accumulator — starting a fresh epoch.
          </p>
          <Code>{`ctx = AtomikContext()
ctx.load(initial_state)

while running:
    # Accumulate deltas for this epoch
    for delta in incoming_deltas():
        ctx.accum(delta)

    # Checkpoint: capture state, start new epoch
    epoch_state = ctx.swap()
    save_checkpoint(epoch_state)
    # Accumulator is now 0, reference is epoch_state`}</Code>
          <p>
            <strong>Use when:</strong> Periodic snapshots (every N seconds, every N deltas),
            database write-ahead log compaction, or game save points.
          </p>
        </Pattern>

        <Pattern num={3} title="Fingerprint Gate">
          <p>
            Use Fingerprint as a fast gate before expensive operations. Only proceed
            if the data actually changed — O(1) check instead of byte-by-byte comparison.
          </p>
          <Code>{`from atomik_core import Fingerprint

fp = Fingerprint()
fp.load(current_data)

while True:
    new_data = read_sensor()
    fp.update(new_data)

    if fp.changed:
        # Data actually changed — do the expensive thing
        transmit(new_data)      # Network send
        update_database(new_data)  # DB write
        fp.load(new_data)       # Update reference
    # else: skip — data unchanged, save bandwidth + CPU`}</Code>
          <p>
            <strong>Use when:</strong> Polling loops, cache invalidation, sensor sampling,
            file sync. Anywhere you check &quot;did anything change?&quot; before acting.
          </p>
        </Pattern>

        <Pattern num={4} title="Rollback Chain">
          <p>
            Build an undo stack using self-inverse. Store each delta; to undo, re-apply
            it. No snapshots, no deep copies — just 8 bytes per undo level.
          </p>
          <Code>{`ctx = AtomikContext()
ctx.load(document_state)
undo_stack = []

def apply_edit(delta):
    ctx.accum(delta)
    undo_stack.append(delta)

def undo():
    if undo_stack:
        delta = undo_stack.pop()
        ctx.accum(delta)  # Self-inverse: re-apply = undo

# Apply 3 edits
apply_edit(0x0001)
apply_edit(0x0010)
apply_edit(0x0100)

# Undo last 2
undo()  # Removes 0x0100
undo()  # Removes 0x0010
assert ctx.read() == document_state ^ 0x0001  # Only first edit remains`}</Code>
          <p>
            <strong>Use when:</strong> Text editors, drawing apps, configuration management,
            transaction rollback, A/B testing (try a change, measure, undo if worse).
          </p>
        </Pattern>

        <Pattern num={5} title="Multi-Stream Convergence">
          <p>
            Run multiple DeltaStreams on different nodes. Exchange deltas over any
            transport (TCP, UDP, message queue). All streams converge to the same state.
          </p>
          <Code>{`from atomik_core import DeltaStream, DeltaMessage

# Node A
stream_a = DeltaStream()
stream_a.load(addr=0, initial_state=0xCAFE)

# Node B (different machine)
stream_b = DeltaStream()
stream_b.load(addr=0, initial_state=0xCAFE)

# A makes a change, sends delta to B
stream_a.accum(addr=0, delta=0x00FF)
msg = DeltaMessage(addr=0, delta=0x00FF, seq=1)
wire = msg.to_bytes()  # 16 bytes over the network

# B receives and applies (could arrive out of order)
received = DeltaMessage.from_bytes(wire)
stream_b.accum(received.addr, received.delta)

# Both converge
assert stream_a.read(0) == stream_b.read(0)  # ✓`}</Code>
          <p>
            <strong>Use when:</strong> Multi-region sync, edge-cloud convergence, peer-to-peer
            state sharing, real-time collaboration, multi-player games.
          </p>
        </Pattern>

        <div className="rounded-xl border p-8 mt-4" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
          <h3 className="text-xl font-bold mb-4 text-center">Try ATOMiK today</h3>
          <EmailCapture variant="blog" />
        </div>
      </article>
    </div>
  );
}
