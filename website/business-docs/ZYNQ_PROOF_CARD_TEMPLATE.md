# Zynq Proof Card Template

Status: template only. Fill one proof card per correctness-passing workload result.

## Proof card
- Workload name:
- Buyer pain represented:
- Hardware:
- Board model:
- Build/bitstream version:
- Evidence label:
- Baseline path:
- ATOMiK path:
- Input trace:
- Trace seed:
- Number of runs:
- Measurement method:
- Correctness result:
- Baseline output hash:
- ATOMiK output hash:
- Primary measured result:
- Secondary measured results:
- Bytes moved:
- Bytes avoided:
- Operations received:
- Operations emitted:
- Operations coalesced:
- Latency p50:
- Latency p95:
- Caveats:
- Artifact links:
- Public-safe claim:
- Claims not supported:

## Public-safe claim pattern
In a representative Zynq [workload name] workload, ATOMiK reduced [artifact-backed metric] versus [baseline] while preserving correctness. This validates the state-movement reduction mechanism for this workload. Battery, thermal, cooling, water, power-bill, footprint, production-readiness, and universal-speedup claims remain customer-environment evaluation targets.

## Required caveat
Workload-specific. Artifact-bound. Correctness-passing only. Does not prove universal speedup or downstream device/business outcomes.
