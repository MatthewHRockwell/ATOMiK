import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title: "Database Change Detection Without Triggers — ATOMiK Solutions",
  description:
    "Detect row-level database changes in O(1) per row using XOR fingerprinting. No CDC, no triggers, no replication slots. Works with PostgreSQL, MySQL, SQLite.",
  keywords: ["database change detection", "CDC alternative", "row-level change tracking", "postgres change detection", "database sync without triggers"],
};

function Code({ children }: { children: string }) {
  return (
    <pre className="rounded-lg p-4 text-sm overflow-x-auto my-4" style={{ background: "#0a0a0f", border: "1px solid #1e1e2e", color: "#22d3ee", fontFamily: "'SF Mono', 'Fira Code', monospace", lineHeight: 1.7 }}>
      <code>{children}</code>
    </pre>
  );
}

export default function DatabaseSyncPage() {
  return (
    <div className="min-h-screen" style={{ background: "#0a0a0f", color: "#e0e0e8" }}>
      <Nav />

      <section className="text-center px-6 pt-20 pb-12">
        <h1 className="text-4xl font-bold tracking-tight mb-4">
          Database Change Detection{" "}
          <span className="bg-clip-text text-transparent" style={{ backgroundImage: "linear-gradient(135deg, #22c55e, #4f8fff)" }}>
            Without Triggers
          </span>
        </h1>
        <p className="text-lg max-w-2xl mx-auto" style={{ color: "#8888a0" }}>
          Detect which rows changed in O(1) per row. No CDC pipelines, no database triggers,
          no replication slots. Just XOR fingerprinting.
        </p>
      </section>

      <section className="max-w-4xl mx-auto px-6 pb-12">
        <div className="grid md:grid-cols-2 gap-6 mb-12">
          {[
            { problem: "CDC pipelines (Debezium)", pain: "Complex setup, WAL parsing, schema registry, connector maintenance", fix: "One Python import. No infrastructure." },
            { problem: "Database triggers", pain: "Schema changes, performance overhead on every write, vendor lock-in", fix: "Application-level detection. Zero database changes." },
            { problem: "updated_at columns", pain: "Requires schema modification, clock skew, doesn't detect which columns changed", fix: "Content-based detection. Catches any change, even backdoor SQL." },
            { problem: "Full-table comparison", pain: "O(n * row_size) — comparing 1M rows at 1KB each = 1 GB of comparison data", fix: "O(n * 8 bytes) — 64-bit fingerprint per row. 125x less memory." },
          ].map((item, i) => (
            <div key={i} className="rounded-xl border p-6" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
              <h3 className="font-semibold mb-2" style={{ color: "#ef4444" }}>{item.problem}</h3>
              <p className="text-sm mb-3" style={{ color: "#8888a0" }}>{item.pain}</p>
              <div className="rounded-lg p-3 text-sm" style={{ background: "rgba(34,197,94,0.05)", border: "1px solid rgba(34,197,94,0.15)", color: "#22c55e" }}>
                ATOMiK: {item.fix}
              </div>
            </div>
          ))}
        </div>
      </section>

      <section className="max-w-3xl mx-auto px-6 pb-12">
        <h2 className="text-2xl font-bold mb-4">Try it yourself — zero dependencies</h2>
        <Code>{`pip install atomik-core
python -m atomik_core.examples.sqlite_change_tracker`}</Code>
        <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
          Creates a 1000-row SQLite database, modifies 50 rows, and detects every change
          using XOR fingerprinting. No external dependencies — SQLite is built into Python.
        </p>

        <h2 className="text-2xl font-bold mb-4">How it works</h2>
        <Code>{`from atomik_core import Fingerprint, AtomikTable
import sqlite3

# 1. Connect to your database
db = sqlite3.connect("myapp.db")
tracker = AtomikTable(num_contexts=65536)

# 2. Fingerprint each row (initial snapshot)
for row in db.execute("SELECT id, name, email, balance FROM users"):
    fp = xor_reduce(serialize(row))  # 8-byte fingerprint
    tracker.load(addr=row[0], initial_state=fp)

# 3. On next sync cycle — detect changes in O(1) per row
changed_ids = []
for row in db.execute("SELECT id, name, email, balance FROM users"):
    fp = xor_reduce(serialize(row))
    if tracker.read(addr=row[0]) != fp:
        changed_ids.append(row[0])
        tracker.load(addr=row[0], initial_state=fp)

print(f"{len(changed_ids)} rows changed")
# Only sync/replicate the changed rows`}</Code>
      </section>

      <section className="max-w-4xl mx-auto px-6 pb-12">
        <h2 className="text-2xl font-bold mb-6 text-center">Comparison</h2>
        <div className="rounded-xl border overflow-hidden" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
          <table className="w-full text-sm">
            <thead>
              <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                <th className="text-left p-4" style={{ color: "#8888a0" }}>Property</th>
                <th className="text-center p-4" style={{ color: "#22d3ee" }}>ATOMiK</th>
                <th className="text-center p-4" style={{ color: "#8888a0" }}>CDC (Debezium)</th>
                <th className="text-center p-4" style={{ color: "#8888a0" }}>Triggers</th>
                <th className="text-center p-4" style={{ color: "#8888a0" }}>updated_at</th>
              </tr>
            </thead>
            <tbody style={{ color: "#b0b0c0" }}>
              {[
                ["Detection cost", "O(1) per row", "O(1) per write", "O(1) per write", "O(1) per row"],
                ["Schema changes", "None", "Connector config", "DDL required", "DDL required"],
                ["Infrastructure", "pip install", "Kafka + Connect", "DB-specific", "None"],
                ["Catches backdoor SQL", "Yes", "Yes (WAL)", "No", "No"],
                ["Works across DBs", "Yes", "Per-connector", "Per-DB syntax", "Yes"],
                ["Column-level detection", "Via field fingerprint", "Full row diff", "Custom logic", "No"],
                ["Dependencies", "Zero", "Kafka, ZooKeeper, Connect", "DB extensions", "None"],
              ].map(([prop, ...vals], i) => (
                <tr key={i} style={{ borderBottom: "1px solid #1e1e2e" }}>
                  <td className="p-4 font-medium" style={{ color: "#e0e0e8" }}>{prop}</td>
                  {vals.map((v, j) => (
                    <td key={j} className="text-center p-4">{v}</td>
                  ))}
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>

      <section className="max-w-3xl mx-auto px-6 pb-12">
        <div className="grid grid-cols-4 gap-4 mb-12">
          {[
            ["O(1)", "Per-row detection"],
            ["8 bytes", "Per fingerprint"],
            ["Zero", "Dependencies"],
            ["Artifact", "Memory claims"],
          ].map(([val, label]) => (
            <div key={label} className="text-center rounded-xl p-4" style={{ background: "#12121a", border: "1px solid #1e1e2e" }}>
              <div className="text-xl font-bold" style={{ color: "#4f8fff" }}>{val}</div>
              <div className="text-xs mt-1" style={{ color: "#8888a0" }}>{label}</div>
            </div>
          ))}
        </div>
      </section>

      {/* Workload Brief CTA */}
      <section className="max-w-3xl mx-auto px-6 pb-12">
        <Link
          href="/case-studies"
          className="block rounded-xl border p-8 transition-all hover:border-opacity-80"
          style={{
            background: "linear-gradient(135deg, rgba(139,92,246,0.06), rgba(79,143,255,0.06))",
            borderColor: "#8b5cf640",
          }}
        >
          <p
            className="text-xs font-semibold uppercase tracking-widest mb-3"
            style={{ color: "#8b5cf6" }}
          >
            Workload brief
          </p>
          <h3 className="text-xl font-bold mb-2">
            Evaluate database sync as a design-partner workload
          </h3>
          <p className="text-sm mb-4" style={{ color: "#8888a0" }}>
            Public deployment references are not claimed yet. Use the workload
            briefs page to frame CDC, replication, reconciliation, and audit
            paths that may merit a scoped evaluation.
          </p>
          <span
            className="text-sm font-semibold"
            style={{ color: "#8b5cf6" }}
          >
            View workload briefs &rarr;
          </span>
        </Link>
      </section>

      <section className="max-w-3xl mx-auto px-6 pb-24">
        <div className="rounded-xl border p-8 text-center" style={{ background: "#12121a", borderColor: "#1e1e2e" }}>
          <h3 className="text-xl font-bold mb-3">Stop building CDC pipelines</h3>
          <p className="text-sm mb-6" style={{ color: "#8888a0" }}>
            Bring a real CDC, replication, or reconciliation path and define
            the measured artifact needed for a credible evaluation.
          </p>
          <div className="flex flex-wrap justify-center gap-4">
            <Link href="/get-started" className="px-6 py-2.5 rounded-lg text-sm font-semibold" style={{ background: "#4f8fff", color: "#fff" }}>Request Evaluation Access</Link>
            <Link href="/docs/hardware" className="px-6 py-2.5 rounded-lg text-sm font-semibold" style={{ background: "transparent", color: "#e0e0e8", border: "1px solid #1e1e2e" }}>View Technical Proof</Link>
          </div>
        </div>
      </section>
    </div>
  );
}
