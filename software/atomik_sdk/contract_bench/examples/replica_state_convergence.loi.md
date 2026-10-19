# ATOMiK — Conditional Letter of Intent (DRAFT)

**This is a non-binding draft for discussion.** Binding terms require a mutually executed agreement, and any proprietary workload data requires an NDA and an agreed evaluation scope.

- **Drafted:** 2026-06-25
- **Between:** Rockwell Intelligence ("ATOMiK") and **________________________** ("Counterparty")
- **Workload:** Replica state convergence

## 1. Purpose
This letter records a conditional commitment: if ATOMiK demonstrates the bar in Section 3 on a live run of the Counterparty's workload on ATOMiK silicon (Section 4), the Counterparty intends to proceed to the next step in Section 5.

## 2. The workload
- Pattern: replica / state convergence by delta exchange
- Conventional full-state sync: **2048 B/tick**
- ATOMiK delta sync: **160 B/tick** at ~5% of state changed per tick

## 3. ATOMiK's commitment (the bar)
ATOMiK commits that replicas exchanging delta-state **converge to byte-identical state with no coordinator or consensus** — any node, any order, after any partition (the same order-independence proven on silicon) — and that a sync tick moves **160 B vs 2048 B** of full state (**~92% less**) at ~5% of state changed per tick.

## 4. Acceptance test
A PASS is a live on-silicon run where BOTH hold:
1. **Convergence:** replicas merge to a byte-identical result regardless of delta order / partition.
2. **Data moved:** the delta tick is materially smaller than full-state sync at the Counterparty's change density.

Witness / reproduce the order-independence: `abench 65537 0xDEADBEEF0BADF00D`.

## 5. Counterparty's conditional commitment
Upon a PASS, the Counterparty intends in good faith to proceed to (select one):
- [ ] Paid pilot / evaluation
- [ ] Design-partner LOI
- [ ] Purchase order / commercial terms

within **____ days** of the acceptance test.

## 6. Scope & limitations (read before signing)
- The throughput commitment covers the **engine's accumulation / merge step measured on silicon**, not end-to-end system throughput, which depends on the Counterparty's ingest path.
- The correctness / order-independence guarantee applies **only to abelian operators**. For order-dependent operators ATOMiK makes no reorder-stable claim.
- Any sync bandwidth figure **scales with change density** and is not a universal multiplier.
- This draft is **non-binding**; binding terms require a mutually executed agreement.

## 7. Provenance
- Engine: parallel-bank delta-state accumulator @0xF0021000
- Measured: 2026-06-21 on ALINX/HamGeek AX7020 (XC7Z020-2CLG484)
- Byte-identical result hash across banks: 0x17e9fe29b5dc0aad
- Reproduce: abench 65537 0xDEADBEEF0BADF00D

## 8. Signatures

ATOMiK / Rockwell Intelligence — Matthew Rockwell   ____________________   Date ________

________________________ — ____________________   Date ________
