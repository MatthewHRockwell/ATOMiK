# ATOMiK — Conditional Letter of Intent (DRAFT)

**This is a non-binding draft for discussion.** Binding terms require a mutually executed agreement, and any proprietary workload data requires an NDA and an agreed evaluation scope.

- **Drafted:** 2026-06-25
- **Between:** Rockwell Intelligence ("ATOMiK") and **________________________** ("Counterparty")
- **Workload:** Distinct-count cardinality rollup

## 1. Purpose
This letter records a conditional commitment: if ATOMiK demonstrates the bar in Section 3 on a live run of the Counterparty's workload on ATOMiK silicon (Section 4), the Counterparty intends to proceed to the next step in Section 5.

## 2. The workload
- Aggregation operator: **hll_merge** (abelian — order-independent)
- Required throughput to commit: **20 M/s**
- Correctness requirement: **byte-identical regardless of arrival order / parallelism**

## 3. ATOMiK's commitment (the bar)
ATOMiK commits that the parallel-bank delta-state accumulator will sustain **>= 20 M/s** at **byte-identical correctness across 1 / 2 / 4 / 8 banks** for the **hll_merge** aggregation, on the ALINX/HamGeek AX7020 (XC7Z020-2CLG484) at 100 MHz.

- Site-computed verdict vs the measured engine: **SURPASS** — 40.0x headroom (1 of 8 banks).
- Measured engine ceiling: **800 M/s** across 8 banks (100 M/s per bank).

## 4. Acceptance test
A PASS is a live on-silicon run at the Counterparty's scale where BOTH hold:
1. **Throughput:** sustained accumulation rate >= the required throughput in Section 3.
2. **Correctness:** the merged result is byte-identical across 1 / 2 / 4 / 8 banks (the order-independence proof).

Witness / reproduce the reference run: `abench 65537 0xDEADBEEF0BADF00D`. The Counterparty's run uses their operator and scale.

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
