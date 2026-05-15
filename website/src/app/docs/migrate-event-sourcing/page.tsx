import type { Metadata } from "next";
import {
  codeBlockStyle,
  kwColor,
  fnColor,
  numColor,
  cmtColor,
  typeColor,
  varColor,
  strColor,
} from "../shared";

export const metadata: Metadata = {
  title: "Migrate from Event Sourcing to ATOMiK — Docs",
  description:
    "Replace event logs and snapshot replay with ATOMiK O(1) state reconstruction. Migration guide with code examples and trade-off analysis.",
  keywords: [
    "event sourcing migration",
    "event sourcing alternative",
    "CQRS alternative",
    "snapshot elimination",
    "delta-state algebra",
    "ATOMiK migration guide",
    "event replay elimination",
  ],
  openGraph: {
    title: "Migrate from Event Sourcing to ATOMiK — ATOMiK Docs",
    description:
      "Eliminate event replay and snapshot overhead. O(1) state reconstruction replaces O(n) event log replay.",
    type: "website",
  },
};

/* ------------------------------------------------------------------ */
/*  Data                                                               */
/* ------------------------------------------------------------------ */

const conceptMap = [
  {
    es: "Event Log",
    atomik: "Accumulator",
    notes: "Event sourcing appends every event to an ordered log (linear growth). ATOMiK XOR-accumulates every delta into a single fixed-size value (constant space).",
  },
  {
    es: "Event Replay",
    atomik: "ctx.read()",
    notes: "ES replays all events since last snapshot to reconstruct state: O(n). ATOMiK reconstructs in O(1): initial_state XOR accumulator.",
  },
  {
    es: "Snapshot",
    atomik: "ctx.swap()",
    notes: "ES periodically snapshots to bound replay cost. ATOMiK swap() atomically captures current state and resets the accumulator -- no serialization needed.",
  },
  {
    es: "Compensating Event",
    atomik: "Re-apply same delta",
    notes: "ES undo requires publishing a compensating event with reversal logic. ATOMiK undo is algebraic: XOR is self-inverse, so re-applying the same delta reverses it.",
  },
  {
    es: "Projection / Read Model",
    atomik: "ctx.read()",
    notes: "ES projects events into read-optimized views (CQRS). ATOMiK read() returns current state directly -- no separate read model needed.",
  },
  {
    es: "Event Schema Evolution",
    atomik: "Fixed-size delta",
    notes: "ES events require versioning and upcasting as schemas evolve. ATOMiK deltas are fixed-size integers -- no schema to evolve.",
  },
];

const tradeoffs = [
  {
    dimension: "State reconstruction",
    es: "O(n) -- replay from last snapshot",
    atomik: "O(1) -- initial XOR accumulator",
    winner: "atomik",
  },
  {
    dimension: "Storage growth",
    es: "Linear -- every event stored forever",
    atomik: "Constant -- single accumulator value",
    winner: "atomik",
  },
  {
    dimension: "Undo / Compensation",
    es: "Manual compensating events",
    atomik: "Free -- self-inverse (XOR twice = identity)",
    winner: "atomik",
  },
  {
    dimension: "Full audit trail",
    es: "Yes -- every event preserved with ordering",
    atomik: "No -- deltas are accumulated, not stored individually",
    winner: "es",
  },
  {
    dimension: "Temporal queries",
    es: "Yes -- replay to any point in time",
    atomik: "No -- only current state is available",
    winner: "es",
  },
  {
    dimension: "CQRS pattern",
    es: "Native -- separate command and query models",
    atomik: "Not needed -- read() returns current state directly",
    winner: "atomik",
  },
  {
    dimension: "Ordering requirements",
    es: "Strict -- events must be totally ordered",
    atomik: "None -- deltas commute (any order, same result)",
    winner: "atomik",
  },
  {
    dimension: "Infrastructure complexity",
    es: "High -- event store, projectors, snapshot store",
    atomik: "Minimal -- single context object, pip install",
    winner: "atomik",
  },
];

const keepESWhen = [
  {
    reason: "Regulatory audit trails",
    detail: "Financial services, healthcare, and compliance domains often require a complete, ordered, tamper-evident record of every state change. ATOMiK accumulates deltas into a single value -- individual events are not preserved. If you must answer \"what happened at 14:32:07 on March 3rd?\", keep your event log.",
  },
  {
    reason: "Temporal queries / time travel",
    detail: "Event sourcing lets you reconstruct state at any historical point by replaying events up to that timestamp. ATOMiK provides only the current state. If your domain requires \"show me the account balance as of last Tuesday\", event sourcing is the right tool.",
  },
  {
    reason: "Complex domain event choreography",
    detail: "If your system relies on event-driven sagas, process managers, or cross-aggregate reactions where the semantics of individual events matter (OrderPlaced triggers InventoryReserved triggers PaymentCharged), ATOMiK's opaque deltas cannot replace meaningful domain events.",
  },
  {
    reason: "Existing projections that work well",
    detail: "If your CQRS read models are performant and your event store handles production load without issues, migration cost may exceed benefit. ATOMiK is most valuable when replay latency, snapshot overhead, or storage growth are actual pain points.",
  },
];

/* ------------------------------------------------------------------ */
/*  Page                                                               */
/* ------------------------------------------------------------------ */

export default function MigrateEventSourcingPage() {
  return (
    <div className="max-w-5xl mx-auto px-6 py-16">
      <p
        className="text-sm font-mono tracking-widest uppercase mb-4"
        style={{ color: "#8b5cf6" }}
      >
        Migration Guide
      </p>
      <h1 className="text-4xl font-bold tracking-tight mb-4">
        Migrate from Event Sourcing to ATOMiK
      </h1>
      <p className="text-lg mb-6" style={{ color: "#8888a0" }}>
        Event sourcing gives you a complete history. ATOMiK gives you instant current
        state. This guide shows how to evaluate replacing event replay with O(1) state
        reconstruction, reduce snapshot pressure, and simplify undo -- and when to keep
        your event log.
      </p>

      {/* Core insight */}
      <div
        className="rounded-xl p-6 border mb-12"
        style={{
          background: "linear-gradient(135deg, rgba(139,92,246,0.06), rgba(79,143,255,0.06))",
          borderColor: "#8b5cf630",
        }}
      >
        <p className="text-sm font-semibold mb-2" style={{ color: "#8b5cf6" }}>
          The fundamental shift
        </p>
        <p style={{ color: "#b0b0c0" }}>
          Event sourcing stores <strong>every change</strong> and replays them to reconstruct state.
          ATOMiK <strong>accumulates</strong> changes into a single value and reconstructs state in one operation:{" "}
          <code
            className="text-sm font-mono px-2 py-0.5 rounded"
            style={{ background: "#1e1e2e", color: "#22c55e" }}
          >
            current_state = initial_state &oplus; accumulator
          </code>
          . You trade history for speed: O(1) instead of O(n), constant storage instead of
          linear growth, and free undo via self-inverse instead of compensating events.
        </p>
      </div>

      {/* Concept mapping table */}
      <h2 className="text-2xl font-bold mb-4">Event Sourcing to ATOMiK Mapping</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        Some event sourcing concepts have accumulator-model counterparts in ATOMiK.
      </p>
      <div
        className="rounded-xl border overflow-x-auto mb-12"
        style={{ background: "#12121a", borderColor: "#1e1e2e" }}
      >
        <table className="w-full text-sm min-w-[640px]">
          <thead>
            <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
              <th className="text-left px-5 py-4 font-semibold" style={{ color: "#8888a0" }}>
                Event Sourcing
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
                key={row.es}
                style={{
                  borderBottom: i < conceptMap.length - 1 ? "1px solid #1e1e2e" : "none",
                }}
              >
                <td className="px-5 py-3.5 font-mono text-xs" style={{ color: "#ff6b6b" }}>
                  {row.es}
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

      {/* Pattern 1: Event Replay to O(1) Read */}
      <h2 className="text-2xl font-bold mb-4">Pattern 1: Eliminate Event Replay</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        The most impactful migration: replace O(n) event replay with O(1) state
        reconstruction. No more snapshot management, no more replay lag on startup.
      </p>
      <div className="grid md:grid-cols-2 gap-4 mb-12">
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#ef4444" }}>
            Before: Event Sourcing
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: kwColor }}>class</span>
                <span style={{ color: typeColor }}> Account</span>
                <span style={{ color: varColor }}>:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> rebuild_from_events</span>
                <span style={{ color: varColor }}>(self, event_store):</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"        "}# Load latest snapshot (if any)</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}snap = event_store.</span>
                <span style={{ color: fnColor }}>latest_snapshot</span>
                <span style={{ color: varColor }}>(self.id)</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>if</span>
                <span style={{ color: varColor }}> snap:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.state = snap.state</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}start = snap.version</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>else</span>
                <span style={{ color: varColor }}>:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.state = </span>
                <span style={{ color: numColor }}>0</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}start = </span>
                <span style={{ color: numColor }}>0</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>{"        "}# Replay all events since snapshot</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}events = event_store.</span>
                <span style={{ color: fnColor }}>load</span>
                <span style={{ color: varColor }}>(</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.id, since=start</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "})</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>for</span>
                <span style={{ color: varColor }}> e </span>
                <span style={{ color: kwColor }}>in</span>
                <span style={{ color: varColor }}> events:</span>
                <span style={{ color: cmtColor }}> # O(n)!</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}self.</span>
                <span style={{ color: fnColor }}>apply</span>
                <span style={{ color: varColor }}>(e)</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>{"        "}# Maybe snapshot for next time</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>if</span>
                <span style={{ color: varColor }}> len(events) {">"} </span>
                <span style={{ color: numColor }}>1000</span>
                <span style={{ color: varColor }}>:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "}event_store.</span>
                <span style={{ color: fnColor }}>save_snapshot</span>
                <span style={{ color: varColor }}>(</span>
                {"\n"}
                <span style={{ color: varColor }}>{"                "}self.id, self.state</span>
                {"\n"}
                <span style={{ color: varColor }}>{"            "})</span>
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
                <span style={{ color: kwColor }}>class</span>
                <span style={{ color: typeColor }}> Account</span>
                <span style={{ color: varColor }}>:</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> __init__</span>
                <span style={{ color: varColor }}>(self):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}self.ctx = </span>
                <span style={{ color: typeColor }}>AtomikContext</span>
                <span style={{ color: varColor }}>()</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}self.ctx.</span>
                <span style={{ color: fnColor }}>load</span>
                <span style={{ color: varColor }}>(</span>
                <span style={{ color: numColor }}>0</span>
                <span style={{ color: varColor }}>)</span>
                {"\n\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> apply_change</span>
                <span style={{ color: varColor }}>(self, delta):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}self.ctx.</span>
                <span style={{ color: fnColor }}>accum</span>
                <span style={{ color: varColor }}>(delta)</span>
                {"\n\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> current_state</span>
                <span style={{ color: varColor }}>(self):</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"        "}# O(1). Always. No replay.</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>return</span>
                <span style={{ color: varColor }}> self.ctx.</span>
                <span style={{ color: fnColor }}>read</span>
                <span style={{ color: varColor }}>()</span>
                {"\n\n"}
                <span style={{ color: varColor }}>{"    "}</span>
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> checkpoint</span>
                <span style={{ color: varColor }}>(self):</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"        "}# Atomic snapshot + reset</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}</span>
                <span style={{ color: kwColor }}>return</span>
                <span style={{ color: varColor }}> self.ctx.</span>
                <span style={{ color: fnColor }}>swap</span>
                <span style={{ color: varColor }}>()</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># No event store. No snapshots.</span>
                {"\n"}
                <span style={{ color: cmtColor }}># No replay. No snapshot scheduling.</span>
              </code>
            </pre>
          </div>
        </div>
      </div>

      {/* Pattern 2: Compensating Events to Self-Inverse */}
      <h2 className="text-2xl font-bold mb-4">Pattern 2: Eliminate Compensating Events</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        In event sourcing, undo requires designing and publishing a compensating event
        with reversal semantics. In ATOMiK, undo is algebraic and free.
      </p>
      <div className="grid md:grid-cols-2 gap-4 mb-12">
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#ef4444" }}>
            Before: Compensating Events
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: cmtColor }}># To undo a transfer, publish a reversal</span>
                {"\n"}
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> cancel_transfer</span>
                <span style={{ color: varColor }}>(original_event):</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# Must design reversal logic</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}compensation = </span>
                <span style={{ color: typeColor }}>TransferReversed</span>
                <span style={{ color: varColor }}>(</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}from_acct=original_event.to_acct,</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}to_acct=original_event.from_acct,</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}amount=original_event.amount,</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}reason=</span>
                <span style={{ color: strColor }}>&quot;cancellation&quot;</span>
                <span style={{ color: varColor }}>,</span>
                {"\n"}
                <span style={{ color: varColor }}>{"        "}ref=original_event.id</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "})</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}event_store.</span>
                <span style={{ color: fnColor }}>publish</span>
                <span style={{ color: varColor }}>(compensation)</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}>{"    "}# Log grows. Complexity grows.</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# Every event type needs a reversal.</span>
              </code>
            </pre>
          </div>
        </div>
        <div>
          <p className="text-xs font-mono uppercase tracking-widest mb-2" style={{ color: "#22c55e" }}>
            After: ATOMiK Self-Inverse
          </p>
          <div style={codeBlockStyle}>
            <pre style={{ margin: 0 }}>
              <code>
                <span style={{ color: cmtColor }}># To undo: apply the same delta again</span>
                {"\n"}
                <span style={{ color: kwColor }}>def</span>
                <span style={{ color: fnColor }}> cancel_transfer</span>
                <span style={{ color: varColor }}>(ctx, original_delta):</span>
                {"\n"}
                <span style={{ color: varColor }}>{"    "}ctx.</span>
                <span style={{ color: fnColor }}>accum</span>
                <span style={{ color: varColor }}>(original_delta)</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# That&apos;s it.</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}#</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# XOR is self-inverse:</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# state XOR delta XOR delta = state</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}#</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# No reversal logic to design.</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# No compensating event schema.</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# No additional storage.</span>
                {"\n"}
                <span style={{ color: cmtColor }}>{"    "}# Works for every delta type.</span>
                {"\n\n"}
                <span style={{ color: cmtColor }}># Algebraic property:</span>
                {"\n"}
                <span style={{ color: cmtColor }}># a XOR a = 0 (proven in Lean4)</span>
              </code>
            </pre>
          </div>
        </div>
      </div>

      {/* Trade-off comparison */}
      <h2 className="text-2xl font-bold mb-4">Trade-off Comparison</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        Event sourcing and ATOMiK optimize for different things. This table shows
        where each approach wins.
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
                Event Sourcing
              </th>
              <th className="text-left px-5 py-4 font-semibold" style={{ color: "#8b5cf6" }}>
                ATOMiK
              </th>
            </tr>
          </thead>
          <tbody>
            {tradeoffs.map((row, i) => (
              <tr
                key={row.dimension}
                style={{
                  borderBottom: i < tradeoffs.length - 1 ? "1px solid #1e1e2e" : "none",
                }}
              >
                <td className="px-5 py-3.5 font-medium" style={{ color: "#b0b0c0" }}>
                  {row.dimension}
                </td>
                <td
                  className="px-5 py-3.5"
                  style={{ color: row.winner === "es" ? "#22c55e" : "#8888a0" }}
                >
                  {row.es}
                </td>
                <td
                  className="px-5 py-3.5"
                  style={{ color: row.winner === "atomik" ? "#22c55e" : "#8888a0" }}
                >
                  {row.atomik}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {/* When event sourcing is still better */}
      <h2 className="text-2xl font-bold mb-6">When Event Sourcing Is Still Better</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        ATOMiK is not a universal replacement. These are legitimate reasons to keep
        your event log.
      </p>
      <div className="space-y-4 mb-12">
        {keepESWhen.map((item) => (
          <div
            key={item.reason}
            className="rounded-xl border p-5"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div className="flex items-start gap-3">
              <span className="mt-0.5 shrink-0" style={{ color: "#f59e0b" }}>
                {"\u26A0"}
              </span>
              <div>
                <p className="text-sm font-semibold mb-1">{item.reason}</p>
                <p className="text-sm leading-relaxed" style={{ color: "#8888a0" }}>
                  {item.detail}
                </p>
              </div>
            </div>
          </div>
        ))}
      </div>

      {/* Hybrid approach */}
      <h2 className="text-2xl font-bold mb-4">The Hybrid Approach</h2>
      <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
        You do not have to choose exclusively. Many teams keep event sourcing for
        audit-critical aggregates while using ATOMiK for hot-path state where
        performance matters most.
      </p>
      <div style={codeBlockStyle}>
        <pre style={{ margin: 0 }}>
          <code>
            <span style={{ color: cmtColor }}># Hybrid: event sourcing for audit trail,</span>
            {"\n"}
            <span style={{ color: cmtColor }}># ATOMiK for real-time state queries</span>
            {"\n\n"}
            <span style={{ color: kwColor }}>class</span>
            <span style={{ color: typeColor }}> HybridAggregate</span>
            <span style={{ color: varColor }}>:</span>
            {"\n"}
            <span style={{ color: varColor }}>{"    "}</span>
            <span style={{ color: kwColor }}>def</span>
            <span style={{ color: fnColor }}> __init__</span>
            <span style={{ color: varColor }}>(self):</span>
            {"\n"}
            <span style={{ color: varColor }}>{"        "}self.event_store = </span>
            <span style={{ color: typeColor }}>EventStore</span>
            <span style={{ color: varColor }}>()</span>
            <span style={{ color: cmtColor }}> # audit trail</span>
            {"\n"}
            <span style={{ color: varColor }}>{"        "}self.ctx = </span>
            <span style={{ color: typeColor }}>AtomikContext</span>
            <span style={{ color: varColor }}>()</span>
            <span style={{ color: cmtColor }}>{"          "}# hot-path state</span>
            {"\n"}
            <span style={{ color: varColor }}>{"        "}self.ctx.</span>
            <span style={{ color: fnColor }}>load</span>
            <span style={{ color: varColor }}>(</span>
            <span style={{ color: numColor }}>0</span>
            <span style={{ color: varColor }}>)</span>
            {"\n\n"}
            <span style={{ color: varColor }}>{"    "}</span>
            <span style={{ color: kwColor }}>def</span>
            <span style={{ color: fnColor }}> apply</span>
            <span style={{ color: varColor }}>(self, event, delta):</span>
            {"\n"}
            <span style={{ color: cmtColor }}>{"        "}# Write path: both systems</span>
            {"\n"}
            <span style={{ color: varColor }}>{"        "}self.event_store.</span>
            <span style={{ color: fnColor }}>append</span>
            <span style={{ color: varColor }}>(event)</span>
            <span style={{ color: cmtColor }}> # for compliance</span>
            {"\n"}
            <span style={{ color: varColor }}>{"        "}self.ctx.</span>
            <span style={{ color: fnColor }}>accum</span>
            <span style={{ color: varColor }}>(delta)</span>
            <span style={{ color: cmtColor }}>{"          "}# for speed</span>
            {"\n\n"}
            <span style={{ color: varColor }}>{"    "}</span>
            <span style={{ color: kwColor }}>def</span>
            <span style={{ color: fnColor }}> current_state</span>
            <span style={{ color: varColor }}>(self):</span>
            {"\n"}
            <span style={{ color: cmtColor }}>{"        "}# Read path: O(1) via ATOMiK</span>
            {"\n"}
            <span style={{ color: varColor }}>{"        "}</span>
            <span style={{ color: kwColor }}>return</span>
            <span style={{ color: varColor }}> self.ctx.</span>
            <span style={{ color: fnColor }}>read</span>
            <span style={{ color: varColor }}>()</span>
            <span style={{ color: cmtColor }}> # no replay</span>
            {"\n\n"}
            <span style={{ color: varColor }}>{"    "}</span>
            <span style={{ color: kwColor }}>def</span>
            <span style={{ color: fnColor }}> audit_at</span>
            <span style={{ color: varColor }}>(self, timestamp):</span>
            {"\n"}
            <span style={{ color: cmtColor }}>{"        "}# Time travel: via event store</span>
            {"\n"}
            <span style={{ color: varColor }}>{"        "}</span>
            <span style={{ color: kwColor }}>return</span>
            <span style={{ color: varColor }}> self.event_store.</span>
            <span style={{ color: fnColor }}>replay_to</span>
            <span style={{ color: varColor }}>(timestamp)</span>
          </code>
        </pre>
      </div>

      <div
        className="rounded-xl p-6 border mt-12 mb-12"
        style={{
          background: "linear-gradient(135deg, rgba(139,92,246,0.06), rgba(79,143,255,0.06))",
          borderColor: "#8b5cf630",
        }}
      >
        <p className="text-sm font-semibold mb-2" style={{ color: "#8b5cf6" }}>
          Key takeaway
        </p>
        <p style={{ color: "#b0b0c0" }}>
          Event sourcing optimizes for <strong>understanding the past</strong>. ATOMiK optimizes
          for <strong>knowing the present</strong>. If you need both, use both -- ATOMiK on the
          hot read path, event sourcing for audit and temporal queries. The write path
          (one accum() call) adds negligible overhead.
        </p>
      </div>

    </div>
  );
}
