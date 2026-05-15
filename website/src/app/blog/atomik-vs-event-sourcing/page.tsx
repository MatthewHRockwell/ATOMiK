import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";
import EmailCapture from "@/components/EmailCapture";

export const metadata: Metadata = {
  title: "ATOMiK vs Event Sourcing: When XOR Beats Append-Only Logs — ATOMiK Blog",
  description:
    "A quantitative comparison of delta-state algebra vs event sourcing for state management. O(1) reconstruction vs O(n) replay, 8 bytes vs unbounded logs.",
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
        lineHeight: 1.6,
      }}
    >
      <code>{children}</code>
    </pre>
  );
}

export default function EventSourcingPost() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav active="Blog" />

      <article className="max-w-3xl mx-auto px-6 pt-16 pb-24">
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
          <h1 className="text-3xl font-bold tracking-tight mb-4 leading-tight">
            ATOMiK vs Event Sourcing: When XOR Beats Append-Only Logs
          </h1>
          <div className="flex gap-2">
            {["architecture", "comparison", "event-sourcing"].map((tag) => (
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
          <p>
            Event sourcing is elegant. Append-only logs, immutable facts, complete audit
            trails. But elegance has a cost — and for many workloads, that cost is
            measured in unbounded storage, O(n) replay, and snapshot complexity.
          </p>
          <p>
            ATOMiK&apos;s delta-state algebra offers a different trade-off. Instead of
            recording every event and replaying them to reconstruct state, you XOR
            deltas into an accumulator and reconstruct in O(1). Here&apos;s when each
            approach wins.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            The fundamental difference
          </h2>

          <div
            className="rounded-xl border p-6 my-6 grid md:grid-cols-2 gap-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div>
              <h3 className="font-bold mb-2" style={{ color: "#f59e0b" }}>Event Sourcing</h3>
              <Code>{`events = [
  UserCreated(name="Alice"),
  BalanceSet(100),
  Deposit(50),
  Withdrawal(30),
  Deposit(20),
  ...  # grows forever
]

# Reconstruct: replay all events
state = {}
for event in events:  # O(n)
    state = apply(state, event)`}</Code>
            </div>
            <div>
              <h3 className="font-bold mb-2" style={{ color: "#22d3ee" }}>ATOMiK</h3>
              <Code>{`ctx = AtomikContext()
ctx.load(initial_state)

ctx.accum(delta_1)  # XOR
ctx.accum(delta_2)  # XOR
ctx.accum(delta_3)  # XOR

# Reconstruct: one XOR operation
state = ctx.read()  # O(1)
# state = reference ^ accumulator`}</Code>
            </div>
          </div>

          <p>
            Event sourcing stores <em>what happened</em>. ATOMiK stores <em>what
            changed</em>. The distinction matters because deltas compose — applying
            delta A then delta B is the same as applying their XOR — while events
            generally don&apos;t.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Quantitative comparison
          </h2>

          <div
            className="rounded-xl border overflow-hidden my-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <table className="w-full text-sm">
              <thead>
                <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <th className="text-left p-4 font-semibold" style={{ color: "#8888a0" }}>Property</th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#f59e0b" }}>Event Sourcing</th>
                  <th className="text-center p-4 font-semibold" style={{ color: "#22d3ee" }}>ATOMiK</th>
                </tr>
              </thead>
              <tbody style={{ color: "#b0b0c0" }}>
                {[
                  ["State reconstruction", "O(n) — replay log", "O(1) — single XOR"],
                  ["Storage growth", "Unbounded (all events)", "Constant (ref + acc)"],
                  ["Bandwidth per update", "Full event payload", "8 bytes (one delta)"],
                  ["Undo operation", "Complex (compensating events)", "Re-apply delta (self-inverse)"],
                  ["Multi-node sync", "Log shipping + ordering", "XOR deltas (order-free)"],
                  ["Snapshot overhead", "Periodic compaction needed", "Not needed (state is always current)"],
                  ["Conflict resolution", "Application-specific merge", "Automatic (XOR commutativity)"],
                  ["Audit trail", "Complete (every event)", "Cumulative (current delta only)"],
                  ["Formal proof coverage", "Implementation-dependent", "108 Lean4 theorems"],
                ].map(([prop, es, atomik], i) => (
                  <tr key={i} style={{ borderBottom: "1px solid #1e1e2e" }}>
                    <td className="p-4 font-medium" style={{ color: "#e0e0e8" }}>{prop}</td>
                    <td className="text-center p-4">{es}</td>
                    <td className="text-center p-4">{atomik}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Where event sourcing wins
          </h2>
          <p>
            Let&apos;s be honest — event sourcing is the right choice when:
          </p>
          <ul className="list-disc pl-6 space-y-2">
            <li>
              <strong>You need a complete audit trail.</strong> Financial transactions,
              regulatory compliance, forensic analysis. If you need to answer &quot;what
              happened at 3:47 PM on Tuesday?&quot;, you need the full event log.
            </li>
            <li>
              <strong>Events have complex semantics.</strong> &quot;User changed their email&quot;
              and &quot;Admin reset password&quot; are different events that happen to affect the
              same entity. Event sourcing preserves this intent.
            </li>
            <li>
              <strong>You need time travel.</strong> Replaying events to a specific point
              in time for debugging or analytics requires the full log.
            </li>
          </ul>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Where ATOMiK wins
          </h2>
          <p>
            Delta-state algebra is the better choice when:
          </p>
          <ul className="list-disc pl-6 space-y-2">
            <li>
              <strong>You care about current state, not history.</strong> Sensor data,
              cache state, configuration, game state, session data. Most systems only
              ask &quot;what is the current value?&quot; — not &quot;what were all the intermediate
              values?&quot;
            </li>
            <li>
              <strong>Updates are high-frequency.</strong> IoT sensors at 1000 Hz,
              real-time analytics, trading systems. Event logs grow linearly; ATOMiK&apos;s
              accumulator stays constant.
            </li>
            <li>
              <strong>Multi-node convergence without coordination.</strong> ATOMiK&apos;s
              XOR commutativity means nodes converge regardless of message ordering.
              No vector clocks, no causal delivery, no consensus.
            </li>
            <li>
              <strong>Bandwidth is constrained.</strong> Edge devices, mobile, satellite
              links. An 8-byte XOR delta vs a serialized event payload is orders of
              magnitude less data.
            </li>
            <li>
              <strong>Undo must be instant.</strong> <code style={{ color: "#22d3ee" }}>accum(delta)</code>{" "}
              followed by <code style={{ color: "#22d3ee" }}>accum(delta)</code> cancels
              out — self-inverse, proven in Lean4. No compensating events needed.
            </li>
          </ul>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            The hybrid approach
          </h2>
          <p>
            ATOMiK and event sourcing aren&apos;t mutually exclusive. A practical
            architecture uses event sourcing at the domain boundary (capturing business
            intent) and ATOMiK internally for efficient state synchronization:
          </p>
          <Code>{`# Domain layer: events capture intent
event_store.append(BalanceAdjusted(account="A", amount=50))

# Sync layer: ATOMiK propagates state efficiently
delta = compute_delta(old_state, new_state)
for replica in replicas:
    replica.accum(delta)  # 8 bytes, order-free, O(1) apply`}</Code>
          <p>
            The event log serves audit and compliance. ATOMiK handles the hot path —
            keeping replicas converged with minimal bandwidth and zero coordination.
          </p>

          <h2 className="text-2xl font-bold pt-4" style={{ color: "#e0e0e8" }}>
            Try it yourself
          </h2>
          <Code>{`pip install atomik-core
python -m atomik_core.benchmark  # See the numbers on your hardware`}</Code>

          <div
            className="rounded-xl border p-8 mt-8"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <h3 className="text-xl font-bold mb-4 text-center">Try ATOMiK today</h3>
            <EmailCapture variant="blog" />
          </div>
        </div>
      </article>
    </div>
  );
}
