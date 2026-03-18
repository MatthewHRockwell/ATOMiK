"use client";

import Link from "next/link";
import Nav from "@/components/Nav";
import { CopyCodeBlock } from "@/components/CopyCodeBlock";
import EmailCapture from "@/components/EmailCapture";

/* ------------------------------------------------------------------ */
/*  Data                                                               */
/* ------------------------------------------------------------------ */

const integrations = [
  {
    name: "Docker",
    category: "Containerization",
    status: "Available",
    statusColor: "#22c55e",
    color: "#2496ed",
    icon: "D",
    description:
      "Run ATOMiK as a sidecar container alongside your application. The sidecar handles delta accumulation, change detection, and state reconstruction over a local Unix socket. Your application stays language-agnostic -- any language that can write to a socket can use ATOMiK.",
    code: `# docker-compose.yml
services:
  app:
    image: your-app:latest
    depends_on: [atomik]
    environment:
      ATOMIK_SOCKET: /tmp/atomik.sock
    volumes:
      - atomik-sock:/tmp

  atomik:
    image: ghcr.io/mattheewhrockwell/atomik:latest
    volumes:
      - atomik-sock:/tmp
    command: ["--socket", "/tmp/atomik.sock", "--banks", "4"]

volumes:
  atomik-sock:`,
  },
  {
    name: "Kubernetes",
    category: "Orchestration",
    status: "Coming Soon",
    statusColor: "#f59e0b",
    color: "#326ce5",
    icon: "K",
    description:
      "The ATOMiK Kubernetes Operator manages AtomikCluster custom resources. It handles automatic sidecar injection, horizontal scaling of delta accumulation banks, and cross-pod state convergence. Helm chart includes Prometheus ServiceMonitor and Grafana dashboard definitions.",
    code: `# atomik-cluster.yaml
apiVersion: atomik.tech/v1alpha1
kind: AtomikCluster
metadata:
  name: production
spec:
  replicas: 3
  banks: 16
  convergence:
    mode: eventual    # or "strong"
    interval: 100ms
  metrics:
    enabled: true
    port: 9090
  resources:
    limits:
      cpu: "500m"
      memory: "128Mi"`,
  },
  {
    name: "Prometheus + Grafana",
    category: "Observability",
    status: "Available",
    statusColor: "#22c55e",
    color: "#e6522c",
    icon: "P",
    description:
      "The ATOMiK metrics exporter exposes accumulator state, delta throughput, change detection rates, and convergence latency as Prometheus metrics. Pre-built Grafana dashboards visualize delta throughput per second, accumulator drift between replicas, and change detection hit/miss ratios.",
    code: `# prometheus.yml — scrape config
scrape_configs:
  - job_name: 'atomik'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: /metrics

# Exposed metrics:
#   atomik_deltas_total          — counter of accumulated deltas
#   atomik_reads_total           — counter of state reads
#   atomik_swaps_total           — counter of atomic swaps
#   atomik_change_detected_total — change detection hits
#   atomik_convergence_lag_ms    — replica convergence latency
#   atomik_accumulator_drift     — XOR distance between replicas`,
  },
  {
    name: "PostgreSQL",
    category: "Database",
    status: "Available",
    statusColor: "#22c55e",
    color: "#336791",
    icon: "PG",
    description:
      "Detect row-level changes without triggers, WAL parsing, or replication slots. ATOMiK fingerprints each row with an XOR accumulator. Change detection is O(1) per row: compare the fingerprint to the identity element. Works alongside your existing schema -- no DDL changes required on production tables.",
    code: `from atomik_core import AtomikContext
import psycopg2

conn = psycopg2.connect("dbname=mydb")
cur = conn.cursor()

# Track changes for a table
cur.execute("SELECT id, data FROM orders WHERE updated_at > %s", [last_sync])

for row_id, data in cur:
    ctx = AtomikContext()
    ctx.load(known_fingerprints.get(row_id, 0))
    ctx.accum(hash(data))

    if ctx.read() != 0:
        # Row changed — replicate it
        replicate(row_id, data)
        known_fingerprints[row_id] = hash(data)

# Zero triggers. Zero WAL parsing. O(1) per row.`,
  },
  {
    name: "Redis",
    category: "Key-Value Store",
    status: "Available",
    statusColor: "#22c55e",
    color: "#dc382d",
    icon: "R",
    description:
      "Track which Redis keys changed between sync intervals without subscribing to keyspace notifications or scanning the full key space. ATOMiK maintains a per-key fingerprint that detects changes in O(1). Ideal for cache invalidation pipelines where you need to know what changed, not subscribe to everything.",
    code: `from atomik_core import AtomikContext
import redis

r = redis.Redis()
trackers = {}  # key -> AtomikContext

def track_key(key: str):
    """Detect if a Redis key changed since last check."""
    value = r.get(key)
    if value is None:
        return False

    current_hash = hash(value)

    if key not in trackers:
        trackers[key] = AtomikContext()
        trackers[key].load(current_hash)
        return True  # First observation

    ctx = trackers[key]
    delta = current_hash ^ ctx.read()
    if delta == 0:
        return False  # Unchanged

    ctx.accum(delta)
    return True  # Changed

# Batch check: O(1) per key, no SCAN needed
changed = [k for k in watched_keys if track_key(k)]`,
  },
  {
    name: "GitHub Actions",
    category: "CI/CD",
    status: "Available",
    statusColor: "#22c55e",
    color: "#8b5cf6",
    icon: "GH",
    description:
      "Generate type-safe ATOMiK SDK bindings for Python, TypeScript, Go, Rust, and C as part of your CI/CD pipeline. The ATOMiK code generation action validates your schema, runs the 353-test suite, and publishes versioned packages to your registry. Integrates with existing release workflows.",
    code: `# .github/workflows/atomik-sdk.yml
name: Generate ATOMiK SDK
on:
  push:
    paths: ['schema/atomik.yaml']

jobs:
  generate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install ATOMiK SDK Generator
        run: pip install atomik-core[codegen]

      - name: Generate bindings
        run: |
          atomik-gen --schema schema/atomik.yaml \\
            --lang python,typescript,go,rust,c \\
            --out generated/

      - name: Run test suite
        run: atomik-gen test --all  # 353 tests

      - name: Publish to registry
        run: atomik-gen publish --registry $REGISTRY_URL
        env:
          REGISTRY_URL: \${{ secrets.REGISTRY_URL }}`,
  },
];

/* ------------------------------------------------------------------ */
/*  Page                                                               */
/* ------------------------------------------------------------------ */

export default function IntegrationsPage() {
  return (
    <div
      className="min-h-screen"
      style={{ background: "#0a0a0f", color: "#e0e0e8" }}
    >
      <Nav />

      {/* Hero */}
      <section className="text-center px-6 pt-20 pb-16 max-w-4xl mx-auto">
        <p
          className="text-sm font-semibold uppercase tracking-widest mb-4"
          style={{ color: "#8b5cf6" }}
        >
          Integrations
        </p>
        <h1 className="text-4xl md:text-5xl font-bold tracking-tight mb-6">
          ATOMiK Fits Your{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{
              backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            Existing Stack
          </span>
        </h1>
        <p
          className="text-lg md:text-xl max-w-3xl mx-auto leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          Delta-state algebra works alongside the tools you already use.
          Containers, databases, observability, CI/CD -- ATOMiK integrates
          without replacing your infrastructure.
        </p>
      </section>

      {/* Category filters */}
      <section className="max-w-5xl mx-auto px-6 mb-12">
        <div className="flex flex-wrap gap-2 justify-center">
          {["All", ...new Set(integrations.map((i) => i.category))].map((cat) => (
            <span
              key={cat}
              className="px-3 py-1.5 rounded-lg text-xs font-medium"
              style={{
                background: cat === "All" ? "#1e1e2e" : "transparent",
                color: cat === "All" ? "#e0e0e8" : "#8888a0",
                border: "1px solid #1e1e2e",
              }}
            >
              {cat}
            </span>
          ))}
        </div>
      </section>

      {/* Integration cards */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <div className="space-y-8">
          {integrations.map((integration) => (
            <div
              key={integration.name}
              className="rounded-xl border overflow-hidden"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              {/* Header */}
              <div
                className="px-6 py-5 flex flex-col sm:flex-row sm:items-center gap-4"
                style={{ borderBottom: "1px solid #1e1e2e" }}
              >
                <div className="flex items-center gap-4 flex-1">
                  <div
                    className="w-12 h-12 rounded-xl flex items-center justify-center text-sm font-bold shrink-0"
                    style={{
                      background: `${integration.color}15`,
                      border: `1px solid ${integration.color}30`,
                      color: integration.color,
                    }}
                  >
                    {integration.icon}
                  </div>
                  <div>
                    <div className="flex items-center gap-3">
                      <h3 className="text-lg font-bold">{integration.name}</h3>
                      <span
                        className="text-xs font-mono px-2 py-0.5 rounded"
                        style={{
                          color: integration.statusColor,
                          background: `${integration.statusColor}15`,
                          border: `1px solid ${integration.statusColor}33`,
                        }}
                      >
                        {integration.status}
                      </span>
                    </div>
                    <p className="text-xs mt-0.5" style={{ color: "#8888a0" }}>
                      {integration.category}
                    </p>
                  </div>
                </div>
              </div>

              {/* Body */}
              <div className="p-6">
                <p
                  className="text-sm leading-relaxed mb-6"
                  style={{ color: "#b0b0c0" }}
                >
                  {integration.description}
                </p>
                <CopyCodeBlock code={integration.code} />
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* How integrations work */}
      <section className="max-w-4xl mx-auto px-6 pb-20">
        <h2 className="text-2xl font-bold text-center mb-4">
          How ATOMiK Integrates
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          ATOMiK is not middleware. It is a library with four operations. Integrations
          are thin wrappers that expose those operations to your existing stack.
        </p>
        <div className="grid md:grid-cols-3 gap-6">
          <div
            className="rounded-xl border p-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div
              className="w-10 h-10 rounded-lg flex items-center justify-center text-lg mb-4"
              style={{
                background: "rgba(79, 143, 255, 0.1)",
                border: "1px solid rgba(79, 143, 255, 0.2)",
              }}
            >
              1
            </div>
            <h3 className="text-sm font-semibold mb-2">Install</h3>
            <p className="text-sm" style={{ color: "#8888a0" }}>
              <code
                className="font-mono text-xs px-1.5 py-0.5 rounded"
                style={{ background: "#0a0a0f", color: "#22d3ee" }}
              >
                pip install atomik-core
              </code>{" "}
              or add the sidecar container. No infrastructure changes.
            </p>
          </div>
          <div
            className="rounded-xl border p-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div
              className="w-10 h-10 rounded-lg flex items-center justify-center text-lg mb-4"
              style={{
                background: "rgba(139, 92, 246, 0.1)",
                border: "1px solid rgba(139, 92, 246, 0.2)",
              }}
            >
              2
            </div>
            <h3 className="text-sm font-semibold mb-2">Instrument</h3>
            <p className="text-sm" style={{ color: "#8888a0" }}>
              Call{" "}
              <code
                className="font-mono text-xs px-1.5 py-0.5 rounded"
                style={{ background: "#0a0a0f", color: "#22d3ee" }}
              >
                load()
              </code>
              ,{" "}
              <code
                className="font-mono text-xs px-1.5 py-0.5 rounded"
                style={{ background: "#0a0a0f", color: "#22d3ee" }}
              >
                accum()
              </code>
              ,{" "}
              <code
                className="font-mono text-xs px-1.5 py-0.5 rounded"
                style={{ background: "#0a0a0f", color: "#22d3ee" }}
              >
                read()
              </code>
              ,{" "}
              <code
                className="font-mono text-xs px-1.5 py-0.5 rounded"
                style={{ background: "#0a0a0f", color: "#22d3ee" }}
              >
                swap()
              </code>{" "}
              in your hot path. Four operations, same API everywhere.
            </p>
          </div>
          <div
            className="rounded-xl border p-6"
            style={{ background: "#12121a", borderColor: "#1e1e2e" }}
          >
            <div
              className="w-10 h-10 rounded-lg flex items-center justify-center text-lg mb-4"
              style={{
                background: "rgba(34, 197, 94, 0.1)",
                border: "1px solid rgba(34, 197, 94, 0.2)",
              }}
            >
              3
            </div>
            <h3 className="text-sm font-semibold mb-2">Observe</h3>
            <p className="text-sm" style={{ color: "#8888a0" }}>
              Metrics export to Prometheus automatically. Grafana dashboards show
              delta throughput, convergence lag, and change detection rates.
            </p>
          </div>
        </div>
      </section>

      {/* Email capture */}
      <section className="max-w-xl mx-auto px-6 pb-16">
        <EmailCapture />
      </section>

      {/* CTA */}
      <section
        className="text-center px-6 py-20"
        style={{ borderTop: "1px solid #1e1e2e" }}
      >
        <h2 className="text-3xl font-bold mb-4">
          Need a custom integration?
        </h2>
        <p className="mb-8 max-w-xl mx-auto" style={{ color: "#8888a0" }}>
          ATOMiK&apos;s four-operation API makes integration straightforward.
          If you need help connecting to your specific stack, our team can help.
        </p>
        <div className="flex flex-wrap justify-center gap-4">
          <Link
            href="/get-started"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#4f8fff", color: "#fff" }}
          >
            Get Started Free
          </Link>
          <Link
            href="/contact"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#8b5cf6",
              border: "1px solid #8b5cf6",
            }}
          >
            Contact Us
          </Link>
        </div>
      </section>
    </div>
  );
}
