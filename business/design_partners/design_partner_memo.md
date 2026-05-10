# ATOMiK Design Partner Memo Template

Use this template to qualify and structure a design partner conversation.

## Partner Overview

- Company:
- Contact / champion:
- Role:
- Segment:
- Why this partner may be representative:

## Target Workload

- Workload name:
- Current system:
- State size:
- Update cadence:
- Current sync / replay / rollback path:
- Constraints: bandwidth, power, latency, determinism, hardware, deployment

## Current Pain

- Where full-state movement happens:
- Where change detection is expensive:
- Where replay or rollback hurts:
- Where synchronization is fragile:
- Current baseline artifacts available:

## Why ATOMiK May Fit

State-aware execution may fit if compact deltas, reconstruction on demand, or
explicit epoch transitions can reduce repeated work in the target path.

## Proof Status Today

- Live hardware prototype screenshot: `HARDWARE_VALIDATED`
- Board-run artifacts: use `LIVE_MEASURED` only when linked to raw output
- Software and proof work: `SOFTWARE_VALIDATED`
- Concept visuals: `CONCEPTUAL` or `ROADMAP`

## Evaluation Scope

- Objective:
- Duration:
- Required partner artifacts:
- ATOMiK deliverables:
- Meeting cadence:
- Out-of-scope items:

## Responsibilities

Partner:

- provide representative workload context
- identify current bottleneck and baseline
- review findings and decide continue / refine / stop

ATOMiK:

- map workload to state-aware execution model
- identify proof tier and artifact gaps
- deliver evaluation brief and recommendation

## Success Criteria

Use 3-5 criteria:

- reduction in state-transfer volume
- lower synchronization overhead
- clearer rollback / revert behavior
- determinism improvement
- integration viability with existing C / SDK / board path
- evidence good enough for an internal continue / stop decision

## Decision Path

- Continue to paid evaluation
- Refine workload and repeat discovery
- Park as no-fit for now

## Evidence Disclaimer

Live screenshots show current prototypes. Concept visuals show product direction
and are not represented as current shipped functionality. Performance claims
are only stated when backed by measured artifacts.
