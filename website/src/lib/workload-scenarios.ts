// Workload Catalog — representative aggregation archetypes a buyer recognizes.
//
// IMPORTANT (honesty): these are REPRESENTATIVE ARCHETYPES, not named customers
// or case studies. The verdict numbers come from the silicon-measured engine
// (see benchmark-proof.ts / the Python harness). The value framing is
// lock-elimination + the byte-identical guarantee — real architectural facts,
// no invented multipliers, nothing from the unsupported traffic-reduction range.

export interface Scenario {
  slug: string;
  vertical: string;
  name: string;
  tagline: string;
  // workload spec — drives the silicon-backed verdict (evaluate())
  aggregation: string; // must be abelian for the correctness gate to pass
  requiredThroughput: number; // events/s the archetype gates on
  requiresReorderStability: boolean;
  // narrative — the LOI argument
  whoThisIs: string[]; // self-identification ("this is us if...")
  pattern: string[]; // what the workload is
  todayPain: string; // the lock/serialization story
  withAtomik: string; // what ATOMiK changes
  valuePoints: string[]; // the value, honestly (lock elimination)
}

export const SCENARIOS: Scenario[] = [
  {
    slug: "observability-metrics-rollup",
    vertical: "Observability",
    name: "High-cardinality metric rollup",
    tagline:
      "Counters, gauges and histogram buckets from thousands of agents, rolled up exactly per series.",
    aggregation: "sum",
    requiredThroughput: 25_000_000,
    requiresReorderStability: true,
    whoThisIs: [
      "You run a Prometheus / Datadog / StatsD-shape metrics pipeline.",
      "You shard or lock to keep per-series counters exact under load.",
      "High-cardinality / hot series is your scaling pain.",
    ],
    pattern: [
      "Thousands of agents emit per-series counter & gauge deltas and histogram-bucket increments.",
      "Deltas arrive out of order over lossy transport, concurrently across the fleet.",
      "Each series must roll up to an exact value per scrape / flush window.",
      "Millions of active series — high cardinality.",
    ],
    todayPain:
      "To keep per-series values exact under concurrent, out-of-order arrival, the rollup serializes per series (locks or a sharded hot path). Hot series contend; throughput is capped by the lock, not the silicon.",
    withAtomik:
      "Agents feed deltas into a shared accumulator in any order. The merge is order-independent and lock-free, and the rolled-up value is byte-identical regardless of arrival order or how many banks process it.",
    valuePoints: [
      "No per-series lock or hot-shard contention — agents never coordinate.",
      "Linear scaling with allocated banks (1 / 2 / 4 / 8×).",
      "Exact, byte-identical rollups under reordering and out-of-order arrival.",
      "No global sequencing or backpressure on ingest — order is irrelevant.",
    ],
  },
  {
    slug: "realtime-fraud-counters",
    vertical: "Risk & Fraud",
    name: "Real-time per-entity counters",
    tagline:
      "Per-IP / per-user / per-card event counters for fraud and rate-limit decisions, exact under concurrency.",
    aggregation: "sum",
    requiredThroughput: 100_000_000,
    requiresReorderStability: true,
    whoThisIs: [
      "You keep per-entity counters for fraud, abuse, or rate-limiting.",
      "Hot keys contend on your counter store under attack / traffic spikes.",
      "You need exact counts for allow / deny / challenge — not approximate.",
    ],
    pattern: [
      "Per-entity (IP, user, card, device) event counters over sliding windows.",
      "Updated concurrently from many edge nodes / PoPs.",
      "A threshold decision reads the count, so it must be exact.",
      "Out-of-order and retried events from lossy edges.",
    ],
    todayPain:
      "Exact per-key counts under concurrent edge updates means a distributed lock / CAS or a serialized hot path per key. Hot keys (a botnet IP, a viral user) contend; the lock caps throughput and adds tail latency exactly when the decision needs to be fastest.",
    withAtomik:
      "Edge nodes feed count deltas into a shared accumulator with no per-key lock. The count is order-independent and byte-identical, so the threshold decision is made on an exact value regardless of which node's events landed first.",
    valuePoints: [
      "No per-key (hot-key) lock contention — the botnet-IP hot path doesn't serialize.",
      "Linear scaling with banks; decisions stay fast under spikes.",
      "Exact, byte-identical counts for thresholding under reordering.",
      "Edges never coordinate or sequence — order is irrelevant.",
    ],
  },
  {
    slug: "adtech-impression-aggregation",
    vertical: "Ad-Tech",
    name: "Impression & click aggregation",
    tagline:
      "Per-campaign impression, click and spend counters aggregated across bid/serve nodes in real time, exact for pacing and capping.",
    aggregation: "sum",
    requiredThroughput: 200_000_000,
    requiresReorderStability: true,
    whoThisIs: [
      "You run a programmatic exchange, DSP, or SSP.",
      "You keep per-campaign / per-creative budget, pacing, and frequency counters.",
      "Hot campaigns contend, and stale counts mean overspend or over-serving.",
    ],
    pattern: [
      "Per-campaign / creative / placement counters: impressions, clicks, conversions, spend.",
      "Updated concurrently across many bid and serve nodes.",
      "Budget pacing and frequency capping read the counts, so they must be exact.",
      "Events arrive out of order across the serving fleet.",
    ],
    todayPain:
      "Exact per-campaign counts under concurrent serving means a per-campaign lock / CAS — or accepting stale, approximate counts and eating the overspend / over-serve. Hot campaigns contend; the lock caps serve throughput at exactly peak traffic.",
    withAtomik:
      "Serve nodes feed count deltas into a shared accumulator with no per-campaign lock. The count is order-independent and byte-identical, so pacing and capping decisions use an exact count regardless of which node served first.",
    valuePoints: [
      "No per-campaign lock contention — hot campaigns don't serialize the serve path.",
      "Exact counts for budget pacing & frequency capping — no overspend from stale reads.",
      "Linear scaling with banks; serve throughput holds under traffic spikes.",
      "Serve nodes never coordinate — order of impressions is irrelevant.",
    ],
  },
  {
    slug: "iot-fleet-telemetry-rollup",
    vertical: "IoT / Edge",
    name: "Fleet telemetry rollup",
    tagline:
      "Per-device telemetry deltas from a large fleet, rolled up exactly despite late, out-of-order delivery — no windowing, no watermarks.",
    aggregation: "sum",
    requiredThroughput: 40_000_000,
    requiresReorderStability: true,
    whoThisIs: [
      "You run a large connected-device fleet (cellular / LoRa / NB-IoT).",
      "You roll up per-device / per-metric usage counters for billing, SLA, or alerting.",
      "Late and out-of-order arrivals force buffering, watermarks, or reprocessing.",
    ],
    pattern: [
      "Millions of devices emit per-metric deltas: usage counters, event tallies, totals.",
      "Transport is lossy and high-latency; deltas arrive wildly out of order and delayed.",
      "Rollups must be exact per device / metric per window — billing and SLA depend on it.",
      "Devices are concurrent and uncoordinated.",
    ],
    todayPain:
      "Exact rollups under late, out-of-order arrival usually means per-device windowing with watermarks plus a serialized commit path — adding latency, memory, and reprocessing whenever late data shows up after the window closed.",
    withAtomik:
      "Devices feed deltas into a shared accumulator in any order, at any time. The rollup is order-independent and byte-identical, so a delta that arrives an hour late simply accumulates into the exact result — no sequencing, no watermark stalls, no reprocessing.",
    valuePoints: [
      "Late and out-of-order data accumulates correctly — no watermarks, no reprocessing.",
      "No per-device sequencing / windowing lock on the commit path.",
      "Exact, byte-identical rollups for billing and SLA.",
      "Linear scaling across the fleet; devices never coordinate.",
    ],
  },
];

export function getScenario(slug: string): Scenario | undefined {
  return SCENARIOS.find((s) => s.slug === slug);
}
