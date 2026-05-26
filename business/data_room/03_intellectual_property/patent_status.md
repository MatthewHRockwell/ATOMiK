# Patent Status

> Current diligence draft: 2026-05-23. Confirm all legal dates and claim language
> with patent counsel before external distribution.

## Current Status

| Field | Details |
|---|---|
| Status | Provisional IP protection in place / patent-pending positioning |
| Jurisdiction | United States |
| Next gate | Convert provisional coverage into stronger non-provisional patent coverage |
| Working deadline | February 2027 conversion window noted in meeting prep; counsel should confirm exact statutory deadline |
| Funding relevance | Pre-seed capital should fund counsel, claim refinement, prior-art review, and diligence-ready IP package |

## Claim Themes Under Protection Review

1. **Delta-state accumulation architecture** - XOR-based state accumulation with
   reference state plus accumulated delta.
2. **Parallel accumulator bank / merge structure** - Independent accumulators
   with merge logic for scalable state paths.
3. **Execution model** - LOAD / ACCUM / READ / SWAP style operation model for
   state epochs and reconstruction.
4. **System integration path** - Hardware, software, and demo surfaces that show
   how the primitive can be evaluated against real workloads.

## Supporting Evidence

| Evidence | Label | Notes |
|---|---|---|
| Formal proof work | `SOFTWARE_VALIDATED` | Algebraic properties are documented in repo proof work. |
| FPGA / Zynq proof paths | `HARDWARE_VALIDATED` | Use only artifact-linked claims. |
| Standalone SD boot artifacts | `BUILD_ARTIFACT` | Local build output exists; public power-on artifact and autonomous handoff remain gates. |
| Customer-value claims | Evaluation target | Heat, water, battery, and footprint savings need workload-specific measurement. |

## Near-Term IP Work

- Have counsel reconcile current prototype scope against provisional claims.
- Draft non-provisional claim strategy around architecture, parallelism, and
  workload-evaluation path.
- Prepare a clean investor IP memo: what is filed, what is pending, what remains
  trade secret, and what must not be publicly disclosed.
- Coordinate with ASIC mentor before claim finalization so silicon feasibility
  and claim language do not diverge.
