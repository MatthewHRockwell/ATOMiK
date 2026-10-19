# Zynq Claims Registry Update Plan

## When to update
Update the claims registry only after a workload has:

- SD boot or JTAG/hardware path context clearly recorded.
- Board and build details captured.
- Baseline and ATOMiK paths run on identical traces.
- Correctness pass recorded.
- Raw artifacts saved.
- Proof card written.

## Required registry fields
- Claim ID
- Public-safe claim text
- Evidence label
- Artifact path
- Workload context
- Board model
- Build/bitstream version
- Baseline path
- ATOMiK path
- Measurement method
- Correctness result
- Caveat
- Overclaim risks
- Allowed language
- Disallowed language
- Owner
- Status

## Evidence labels
Use HARDWARE_VALIDATED if demonstrated on physical Zynq hardware with reproducible logs. Use LIVE_MEASURED only when raw measurement artifacts from a running system are sufficient to reproduce the result. Use BUILD_ARTIFACT for build-only evidence. Use PROJECTED or ROADMAP for modeled or planned outcomes.

## Website update sequence
1. Update claims registry.
2. Update proof page with artifact-bound proof card.
3. Update `/pitch` only with simplified proof if clean.
4. Update homepage only with conservative summary language.

## Disallowed claim promotion
Do not promote battery, heat, cooling, water, power-bill, smaller hardware, production-readiness, generalized AI inference, or universal-speedup claims from Zynq workload results unless those exact outcomes were measured with responsible methodology.
