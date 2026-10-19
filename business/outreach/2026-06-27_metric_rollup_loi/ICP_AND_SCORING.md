# ICP & Account Scoring — Metric / Counter Rollups

The point of a rubric is disciplined list-building: spend your outreach hours on
accounts that can actually say yes, and skip the rest fast. Score each candidate
0–2 on five signals. **Reach out at 7+. Below 5, drop it.**

## The five signals (0–2 each)

| # | Signal | 0 | 1 | 2 |
|---|--------|---|---|---|
| 1 | **Abelian workload** — counters, sums, counts, set-union, min/max, or HLL/sketch merges (NOT medians, percentiles, last-write-wins, ordered logs) | order-dependent core | mixed | core workload is abelian aggregation |
| 2 | **Concurrency-correctness pain** — they shard, lock, or serialize a hot path to keep counts exact under concurrent / out-of-order load | no evidence | plausible | public evidence (talk/blog/issue) of hot-key or shard contention |
| 3 | **Scale that makes it hurt** — high cardinality / high event rate where the lock actually caps them | small | moderate | millions of series/keys or ≥10 M events/s |
| 4 | **Reachable workload owner** — you can identify the engineer who owns the aggregation path | none found | team inbox only | named owner + a way to reach them |
| 5 | **Reason to move** — exactness matters (billing, thresholds, pacing) and the current fix is costly | nice-to-have | meaningful | exactness is load-bearing and they feel the cost |

**Disqualifiers (skip regardless of score):** the core operator is genuinely
order-dependent (medians/percentiles/ordered replay) and can't be reframed as a
counter/sketch; or there is no path to a technical owner.

## Sub-segments + where the pain lives (illustrative, public companies)

These name the *segments and recognizable players* so you know where to hunt.
They are NOT contacts or pipeline — find the owner and qualify each before reaching out.

- **Metrics / observability infrastructure** — teams running or building large
  Prometheus / Cortex / Mimir / Thanos / VictoriaMetrics / M3 / InfluxDB-class
  pipelines; monitoring SaaS. Pain: per-series counter/gauge/histogram rollups
  exact under high cardinality. → `/workloads/observability-metrics-rollup`
- **Ad-tech (exchanges / DSP / SSP)** — per-campaign / per-creative impression,
  click, spend counters for pacing & frequency capping. Pain: hot-campaign
  contention; stale counts = overspend. → `/workloads/adtech-impression-aggregation`
- **Fraud / risk / rate-limiting** — per-IP / per-user / per-card counters for
  allow/deny/challenge under attack traffic. Pain: hot-key contention exactly when
  the decision must be fastest. → `/workloads/realtime-fraud-counters`
- **Real-time / product analytics** — unique-count (HLL) estimation merged across
  shards and windows; funnels, dashboards. Pain: serialized reducer. →
  `/workloads/analytics-distinct-counts`
- **IoT / fleet telemetry** — per-device usage/event rollups with late,
  out-of-order delivery; billing & SLA. Pain: windowing + watermarks +
  reprocessing. → `/workloads/iot-fleet-telemetry-rollup`

**Where the qualifying evidence hides:** conference talks (Monitorama, KubeCon,
SRECon, P99 CONF), engineering blogs about "scaling counters / cardinality / hot
keys / sharding the write path," GitHub issues on counter contention, and job
posts for "high-throughput aggregation / time-series ingestion" engineers.

## Find the buyer (owner, not VP)

Right person: the **platform/infra engineer or staff/principal who owns the
counter hot path or the ingestion-aggregation pipeline.** The author of the blog
post or the talk *is* your buyer. Routes, in order of preference:

1. Direct, if public and clearly the owner (their blog/talk, GitHub, conference bio).
2. A specific named engineer via a warm intro (ask your network for the one person at X).
3. Founder/CTO at smaller infra startups (they still own the hot path).
4. Last resort: company technical contact, addressed to the owner by role.

Do not fabricate or scrape personal emails into this file. Capture what you
verify per account in `SEND_TRACKER.csv`.

## Target the message to the score

- **High on signal 2 (public contention evidence):** open with *their* artifact.
  "Read your post on sharding the counter write path — the reason it shards is…"
- **High on signal 1, low on 2:** lead with the archetype page; let them
  self-recognize.
- **Disqualified on signal 1:** don't pitch. If you must reply, the honest
  no-fit *builds* credibility for the next, real, opportunity.
