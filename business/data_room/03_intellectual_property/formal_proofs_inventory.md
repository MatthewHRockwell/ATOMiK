# Formal Proof Work Inventory

> Current diligence draft: 2026-05-26. Do not quote a public theorem count from
> this file until the count is audited across the repo, website, deck, papers,
> and proof packet.

## Overview

ATOMiK's mathematical foundation includes Lean4 proof work for the delta-state
algebra. The safe public claim is that formal proof work exists for the algebraic
foundation. Public materials should not quote a theorem count unless the current
proof packet, repo, website, and investor deck all agree.

## What The Proof Work Supports

| Area | Public-safe meaning | Boundary |
|---|---|---|
| XOR algebra | The proof work covers properties such as identity, self-inverse, commutativity, and associativity. | Does not prove customer workload savings. |
| State reconstruction | The model supports `current_state = reference_state XOR accumulator`. | Integration behavior still needs artifact-specific validation. |
| Merge/coalescing reasoning | Algebraic properties explain why some deltas can be grouped or reordered. | Does not mean every workload benefits. |
| Correctness assurance | Machine-checked proof work is a strong technical assurance input. | Avoid unaudited counts and avoid claiming all commercial outcomes are formally proven. |

## Verification Command

```bash
cd math/proofs
lake build
```

Use the current build output for internal audit. Do not convert it into public
copy without updating the claims registry, proof packet, website, and deck.

## Public Language

Approved: "Formal proof work exists for the algebraic foundation; implementation
and workload claims remain separately evidence-labeled."

Avoid: theorem counts, "all commercial outcomes proven," "guaranteed savings,"
or claims that proof of algebra proves battery, heat, cooling, water, footprint,
or production readiness.
