# Competitive / Status Quo Memo

| Alternative | What it solves | What it does not solve | Why buyers use it | Where it breaks down | How ATOMiK differs | Proof required |
|---|---|---|---|---|---|---|
| More compute | Adds headroom | Does not remove wasted state movement | Fastest procurement path | Cost, power, heat, footprint | Evaluates whether less state needs to move | Workload baseline and update-cost proof |
| Bigger batteries | Extends runtime | Does not reduce work | Simple field fix | Weight, cost, size | Targets the work behind active time | Power/duty-cycle measurement |
| More cooling | Handles heat | Does not reduce heat source | Protects reliability | Enclosures, water, fans, cost | May reduce thermal pressure only if work drops | Thermal instrumentation |
| More bandwidth | Moves more data | Does not reduce data need | Keeps architecture stable | Cost, radio duty, intermittent links | Evaluates bytes avoided | Payload and transfer proof |
| Compression | Shrinks payloads | Still processes chosen payload | Mature and generic | CPU cost, entropy, latency | Asks whether state must move at all | Bytes moved and CPU cost proof |
| Caching | Keeps data closer | Does not define meaningful change | Mature latency tool | Staleness, invalidation | Tracks state change layer | Correctness and invalidation comparison |
| Deduplication | Removes repeated content | Does not model state transitions | Useful for storage/transfer | Less useful for live update semantics | Coalesces logical state changes | Trace-based coalescing proof |
| Sync protocols | Coordinate replicas | May still move large state | Required for distributed systems | Conflict/replay overhead | Evaluates state-movement path beneath sync | Sync-path workload proof |
| GPUs / NPUs / FPGAs | Accelerate operations | Can accelerate wasted work | Performance headroom | Power, integration, cost | Reduces work before acceleration | Baseline vs ATOMiK path proof |
| Cloud offload | Moves compute away | Adds latency/link dependence | Easy central scaling | Connectivity, privacy, bandwidth | Local state-aware evaluation | End-to-end path proof |
| Manual optimization | Removes known bottlenecks | Labor-intensive and local | Practical engineering habit | Fragile, hard to generalize | Provides repeatable evaluation frame | Engineer-reviewed workload map |
| Overbuilt hardware | Buys margin | Cost/size/power remain high | Low-risk schedule fix | BOM and field constraints | Evaluates whether margin is wasted | Economic threshold proof |
| Feature reduction | Cuts load | Reduces product capability | Last resort | Product quality loss | Seeks efficiency without dropping correctness | Correctness-preserving result |
