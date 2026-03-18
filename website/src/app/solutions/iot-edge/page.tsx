import type { Metadata } from "next";
import Link from "next/link";
import Nav from "@/components/Nav";

export const metadata: Metadata = {
  title:
    "Edge Intelligence Without the Bandwidth — ATOMiK | IoT State Sync, Sensor Deduplication",
  description:
    "Reduce IoT sensor data transfer by 99% with 8-byte XOR deltas. Fingerprint-based change detection skips unchanged data. Order-free convergence works across intermittent cellular, satellite, and LoRa links.",
  keywords: [
    "edge state synchronization",
    "IoT bandwidth reduction",
    "sensor data deduplication",
    "edge-cloud sync",
    "IoT delta compression",
    "edge computing state management",
    "sensor data optimization",
    "intermittent connectivity sync",
    "LoRa state sync",
    "cellular IoT bandwidth",
    "edge device battery optimization",
    "IoT fleet state convergence",
  ],
  openGraph: {
    title: "Edge Intelligence Without the Bandwidth — ATOMiK",
    description:
      "Reduce IoT sensor data transfer by 99% with 8-byte XOR deltas. O(1) change detection. Works over intermittent links.",
    type: "website",
  },
};

const painPoints = [
  {
    label: "Bandwidth Constraints",
    icon: "\uD83D\uDCE1",
    problem:
      "Cellular, satellite, and LoRa links charge per byte or throttle after quotas. Uploading full sensor state every interval burns through data budgets. A fleet of 10,000 devices sending 4KB readings every minute is 57 GB/day \u2014 most of it redundant.",
    solution:
      "ATOMiK sends 8-byte XOR deltas instead of full payloads. Only the bits that actually changed are transmitted. Same 10,000-device fleet drops to under 1 GB/day \u2014 a 99% reduction that fits within even the most constrained cellular plans.",
  },
  {
    label: "Battery Drain",
    icon: "\uD83D\uDD0B",
    problem:
      "Radio transmission is the single largest power consumer on battery-powered edge devices. Every full-state upload means the radio stays on longer, draining batteries faster. Devices in the field need to last months or years without replacement.",
    solution:
      "Fingerprint-based change detection runs in O(1) time on-device. If the sensor reading hasn't changed, nothing is sent \u2014 the radio stays off. When data does change, the 8-byte delta transmits in a fraction of the time a full payload would take.",
  },
  {
    label: "Cloud Sync Latency",
    icon: "\u2601",
    problem:
      "Edge gateways batch sensor data, compress it, and push to the cloud. But full-state uploads create queuing delays. When connectivity drops and reconnects, the backlog of unsent readings must be replayed in order \u2014 adding minutes of sync lag.",
    solution:
      "XOR deltas are commutative and associative. There is no ordering requirement. Gateways accumulate deltas locally during outages. On reconnect, they send a single merged delta \u2014 not a queue of timestamped readings. Sync completes in one round-trip.",
  },
  {
    label: "Scale: Millions of Sensors",
    icon: "\uD83C\uDF10",
    problem:
      "Ingestion pipelines buckle under millions of concurrent devices. Each sensor sends full readings that must be parsed, deduplicated, and stored. Cloud costs scale linearly with device count. At fleet scale, the data volume becomes the primary infrastructure expense.",
    solution:
      "Fixed-size 8-byte deltas are trivially parseable \u2014 no schema negotiation, no decompression. The cloud accumulates deltas with a single XOR operation per device. State reconstruction is O(1), not O(n) log replay. Ingestion cost stays flat as the fleet grows.",
  },
];

const comparisonRows = [
  {
    metric: "Payload Size",
    atomik: "8 bytes (fixed)",
    mqtt: "Full state (variable)",
    coap: "Full state per notify",
    custom: "Variable (codec-dependent)",
  },
  {
    metric: "Change Detection",
    atomik: "O(1) fingerprint",
    mqtt: "Application-level diff",
    coap: "Server-side observe",
    custom: "Application-level diff",
  },
  {
    metric: "Ordering Requirement",
    atomik: "None (commutative)",
    mqtt: "QoS-dependent",
    coap: "Sequence numbers",
    custom: "Timestamps required",
  },
  {
    metric: "Offline Resilience",
    atomik: "Merge on reconnect",
    mqtt: "Queue + replay",
    coap: "Re-observe required",
    custom: "Queue + replay",
  },
  {
    metric: "Bandwidth at Scale",
    atomik: "O(devices)",
    mqtt: "O(devices x state)",
    coap: "O(devices x state)",
    custom: "O(devices x compressed)",
  },
  {
    metric: "Gateway Aggregation",
    atomik: "XOR merge (1 op)",
    mqtt: "Forward all messages",
    coap: "Proxy caching",
    custom: "Re-compress batch",
  },
  {
    metric: "Formal Guarantees",
    atomik: "92 Lean4 proofs",
    mqtt: "Delivery QoS only",
    coap: "Delivery semantics",
    custom: "None standard",
  },
];

const metrics = [
  {
    value: "99%",
    label: "Bandwidth Reduction",
    detail: "8-byte deltas vs. multi-KB full-state uploads",
  },
  {
    value: "8 B",
    label: "Per Delta",
    detail: "Fixed-size, schema-free, zero overhead",
  },
  {
    value: "O(1)",
    label: "Change Detection",
    detail: "Fingerprint check before any transmission",
  },
  {
    value: "\u221E",
    label: "Offline Tolerance",
    detail: "Merge on reconnect, no replay needed",
  },
];

export default function IoTEdgePage() {
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
          Solutions for IoT &amp; Edge Computing
        </p>
        <h1 className="text-4xl md:text-5xl font-bold tracking-tight mb-6">
          Edge Intelligence{" "}
          <span
            className="bg-clip-text text-transparent"
            style={{
              backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
            }}
          >
            Without the Bandwidth
          </span>
        </h1>
        <p
          className="text-lg md:text-xl max-w-3xl mx-auto leading-relaxed"
          style={{ color: "#8888a0" }}
        >
          ATOMiK reduces sensor data transfer by 99% with fixed-size XOR deltas.
          Fingerprint-based change detection skips unchanged readings.
          Order-free convergence syncs across intermittent cellular, satellite,
          and LoRa links &mdash; no backlog replay, no ordering constraints.
        </p>
        <div className="flex flex-wrap justify-center gap-4 mt-10">
          <Link
            href="/get-started"
            className="px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#8b5cf6", color: "#fff" }}
          >
            Get Started Free
          </Link>
          <Link
            href="/pricing"
            className="px-6 py-3 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#4f8fff",
              border: "1px solid #4f8fff",
            }}
          >
            View Pricing
          </Link>
        </div>
      </section>

      {/* Problem / Solution */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          The IoT Data Problem
        </h2>
        <p
          className="text-center mb-12 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Edge devices generate massive data volumes, but most of it is
          redundant. Traditional approaches pay for every byte &mdash; in
          bandwidth, battery, and cloud compute.
        </p>

        <div className="grid md:grid-cols-2 gap-6">
          {painPoints.map((point) => (
            <div
              key={point.label}
              className="rounded-xl border p-6"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div className="flex items-center gap-3 mb-3">
                <span
                  className="w-10 h-10 rounded-lg flex items-center justify-center text-lg"
                  style={{
                    background: "rgba(139, 92, 246, 0.1)",
                    border: "1px solid rgba(139, 92, 246, 0.2)",
                  }}
                >
                  {point.icon}
                </span>
                <h3 className="text-lg font-semibold">{point.label}</h3>
              </div>
              <p className="text-sm mb-4" style={{ color: "#ff6b6b" }}>
                {point.problem}
              </p>
              <div
                className="rounded-lg p-4 text-sm"
                style={{
                  background: "rgba(34, 197, 94, 0.05)",
                  border: "1px solid rgba(34, 197, 94, 0.15)",
                  color: "#b0b0c0",
                }}
              >
                <span
                  className="font-semibold text-xs uppercase tracking-wider"
                  style={{ color: "#22c55e" }}
                >
                  ATOMiK Solution
                </span>
                <p className="mt-1">{point.solution}</p>
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Architecture Diagram */}
      <section className="max-w-4xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          Edge-to-Cloud Delta Flow
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Deltas shrink at every hop. Edge devices send 8-byte deltas to
          gateways. Gateways XOR-merge deltas from hundreds of devices into a
          single update for the cloud.
        </p>
        <div
          className="rounded-xl border p-6 md:p-10 overflow-x-auto"
          style={{
            background: "#12121a",
            borderColor: "#1e1e2e",
          }}
        >
          <pre
            className="text-sm md:text-base leading-relaxed"
            style={{
              color: "#8888a0",
              fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
            }}
          >
            <code>{`  EDGE DEVICES                   GATEWAY                      CLOUD
  ─────────────                   ───────                      ─────

  ┌─────────────┐
  │ Sensor A    │── 8B delta ──┐
  │ temp: 22.1C │              │
  └─────────────┘              │
                               │   ┌───────────────┐
  ┌─────────────┐              ├──►│               │
  │ Sensor B    │── 8B delta ──┤   │   XOR Merge   │── 8B merged ──►  ┌──────────┐
  │ humid: 45%  │              ├──►│   (1 op per   │    delta         │  Cloud   │
  └─────────────┘              │   │    device)    │                  │  State   │
                               │   └───────────────┘                  │  Store   │
  ┌─────────────┐              │                                      └──────────┘
  │ Sensor C    │── 8B delta ──┤           │
  │ press: 1013 │   or 0B*     │      Intermittent          Reconnect: send
  └─────────────┘              │      link? Deltas           one merged delta,
                               │      accumulate             not a backlog
  ┌─────────────┐              │      locally.
  │ Sensor ...N │── 8B delta ──┘
  │             │
  └─────────────┘
                        * 0B when fingerprint detects no change

  Per-device payload:     4,096 bytes (full state)  →  8 bytes (delta)
  Fleet of 10,000 @ 1/min:  57 GB/day              →  0.5 GB/day
  Gateway upload:           forward all             →  single XOR merge`}</code>
          </pre>
        </div>
      </section>

      {/* Code Example */}
      <section className="max-w-3xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          Edge Sync in 12 Lines
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Fingerprint the sensor reading on-device. If it changed, compute and
          send an 8-byte delta. The gateway and cloud converge automatically
          &mdash; no timestamps, no ordering.
        </p>
        <div
          className="rounded-xl border overflow-hidden"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <div
            className="px-4 py-2.5 border-b text-xs font-semibold flex items-center gap-2"
            style={{ borderColor: "#1e1e2e", color: "#8888a0" }}
          >
            <span
              className="w-2 h-2 rounded-full"
              style={{ background: "#22c55e" }}
            ></span>
            edge_sensor.py
          </div>
          <pre
            className="p-6 text-sm overflow-x-auto"
            style={{
              background: "#0a0a0f",
              fontFamily: "'SF Mono', 'Fira Code', 'Consolas', monospace",
            }}
          >
            <code>
              <span style={{ color: "#8b5cf6" }}>from</span>
              <span style={{ color: "#e0e0e8" }}> atomik_core </span>
              <span style={{ color: "#8b5cf6" }}>import</span>
              <span style={{ color: "#e0e0e8" }}> AtomikContext, Fingerprint</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Initialize on the edge device
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>ctx = </span>
              <span style={{ color: "#4f8fff" }}>AtomikContext</span>
              <span style={{ color: "#e0e0e8" }}>()</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>fp = </span>
              <span style={{ color: "#4f8fff" }}>Fingerprint</span>
              <span style={{ color: "#e0e0e8" }}>()</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>fp.</span>
              <span style={{ color: "#22d3ee" }}>update</span>
              <span style={{ color: "#e0e0e8" }}>(sensor_reading)</span>
              {"\n\n"}
              <span style={{ color: "#6a6a80" }}>
                # Each interval: check if anything actually changed
              </span>
              {"\n"}
              <span style={{ color: "#8b5cf6" }}>if</span>
              <span style={{ color: "#e0e0e8" }}> fp.</span>
              <span style={{ color: "#22d3ee" }}>check</span>
              <span style={{ color: "#e0e0e8" }}>(new_reading):</span>
              <span style={{ color: "#6a6a80" }}>
                {"  "}# O(1) change detection
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>{"    "}delta = ctx.</span>
              <span style={{ color: "#22d3ee" }}>compute_delta</span>
              <span style={{ color: "#e0e0e8" }}>(old_reading, new_reading)</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>{"    "}</span>
              <span style={{ color: "#22d3ee" }}>send_to_gateway</span>
              <span style={{ color: "#e0e0e8" }}>(delta)</span>
              <span style={{ color: "#6a6a80" }}>
                {"     "}# 8 bytes, not 4KB
              </span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>{"    "}fp.</span>
              <span style={{ color: "#22d3ee" }}>update</span>
              <span style={{ color: "#e0e0e8" }}>(new_reading)</span>
              {"\n"}
              <span style={{ color: "#8b5cf6" }}>else</span>
              <span style={{ color: "#e0e0e8" }}>:</span>
              {"\n"}
              <span style={{ color: "#e0e0e8" }}>{"    "}</span>
              <span style={{ color: "#8b5cf6" }}>pass</span>
              <span style={{ color: "#6a6a80" }}>
                {"                        "}# Nothing changed -- radio stays off
              </span>
            </code>
          </pre>
        </div>
      </section>

      {/* Metrics */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-10">
          Performance at the Edge
        </h2>
        <div className="grid grid-cols-2 md:grid-cols-4 gap-6">
          {metrics.map((metric) => (
            <div
              key={metric.label}
              className="rounded-xl border p-6 text-center"
              style={{ background: "#12121a", borderColor: "#1e1e2e" }}
            >
              <div
                className="text-3xl md:text-4xl font-bold mb-2 bg-clip-text text-transparent"
                style={{
                  backgroundImage: "linear-gradient(135deg, #8b5cf6, #4f8fff)",
                }}
              >
                {metric.value}
              </div>
              <div className="text-sm font-semibold mb-1">{metric.label}</div>
              <div className="text-xs" style={{ color: "#8888a0" }}>
                {metric.detail}
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Comparison Table */}
      <section className="max-w-5xl mx-auto px-6 pb-20">
        <h2 className="text-3xl font-bold text-center mb-4">
          How ATOMiK Compares
        </h2>
        <p
          className="text-center mb-10 max-w-2xl mx-auto"
          style={{ color: "#8888a0" }}
        >
          Side-by-side comparison across the dimensions that matter for IoT
          state synchronization at scale.
        </p>
        <div
          className="rounded-xl border overflow-hidden overflow-x-auto"
          style={{ background: "#12121a", borderColor: "#1e1e2e" }}
        >
          <table className="w-full text-sm" style={{ minWidth: "640px" }}>
            <thead>
              <tr style={{ borderBottom: "1px solid #1e1e2e" }}>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  Metric
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8b5cf6" }}
                >
                  ATOMiK
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  MQTT Full-State
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  CoAP Observe
                </th>
                <th
                  className="text-left px-5 py-4 font-semibold"
                  style={{ color: "#8888a0" }}
                >
                  Custom Compression
                </th>
              </tr>
            </thead>
            <tbody>
              {comparisonRows.map((row, i) => (
                <tr
                  key={row.metric}
                  style={{
                    borderBottom:
                      i < comparisonRows.length - 1
                        ? "1px solid #1e1e2e"
                        : "none",
                  }}
                >
                  <td
                    className="px-5 py-3.5 font-medium"
                    style={{ color: "#b0b0c0" }}
                  >
                    {row.metric}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#22c55e" }}>
                    {row.atomik}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.mqtt}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.coap}
                  </td>
                  <td className="px-5 py-3.5" style={{ color: "#8888a0" }}>
                    {row.custom}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>

      {/* Case Study CTA */}
      <section className="max-w-4xl mx-auto px-6 pb-20">
        <Link
          href="/case-studies"
          className="block rounded-xl border p-8 transition-all hover:border-opacity-80"
          style={{
            background: "linear-gradient(135deg, rgba(34,197,94,0.06), rgba(139,92,246,0.06))",
            borderColor: "#22c55e40",
          }}
        >
          <p
            className="text-xs font-semibold uppercase tracking-widest mb-3"
            style={{ color: "#22c55e" }}
          >
            See it in action
          </p>
          <h3 className="text-xl font-bold mb-2">
            See how SensorGrid IoT achieved 97.8% bandwidth reduction with ATOMiK
          </h3>
          <p className="text-sm mb-4" style={{ color: "#8888a0" }}>
            SensorGrid cut 340 GB/day of sensor telemetry down to 7.5 GB/day across
            50,000 industrial sensors &mdash; saving $180K annually in cloud and cellular costs.
          </p>
          <span
            className="text-sm font-semibold"
            style={{ color: "#22c55e" }}
          >
            Read case study &rarr;
          </span>
        </Link>
      </section>

      {/* CTA */}
      <section
        className="text-center px-6 py-20"
        style={{ borderTop: "1px solid #1e1e2e" }}
      >
        <h2 className="text-3xl font-bold mb-4">
          Ready to cut your IoT bandwidth by 99%?
        </h2>
        <p className="mb-8 max-w-xl mx-auto" style={{ color: "#8888a0" }}>
          Start with the free Python SDK on a single edge device. Scale to
          millions of sensors with the same 8-byte delta protocol.
        </p>
        <div className="flex flex-wrap justify-center gap-4">
          <Link
            href="/get-started"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{ background: "#8b5cf6", color: "#fff" }}
          >
            Get Started Free
          </Link>
          <Link
            href="/pricing"
            className="px-8 py-3.5 rounded-lg text-sm font-semibold transition-opacity hover:opacity-90"
            style={{
              background: "transparent",
              color: "#4f8fff",
              border: "1px solid #4f8fff",
            }}
          >
            View Pricing
          </Link>
        </div>
      </section>
    </div>
  );
}
