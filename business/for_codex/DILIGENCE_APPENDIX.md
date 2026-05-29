# ATOMiK Friday Diligence Appendix

Last updated: 2026-05-27

Use this as the controlled appendix for investors, technical reviewers, and advisors. It is intentionally direct: what is validated, what is pending, what the round buys, and what must not be claimed.

## 1. Validated / Measured / Pending

| Item | Evidence label | Artifact | What it supports | What it does not support |
|---|---|---|---|---|
| ATOMiK Desk v0.39-K on Zynq | HARDWARE_VALIDATED | `website/public/09-current-live-atomik-desk-v039k.png` | Current live demo surface on Zynq hardware | Product maturity, performance, battery, heat, water, or production readiness |
| Linux userspace-to-FPGA path | HARDWARE_VALIDATED | `docs/LINUX_USERSPACE_PROOF.md`, `hardware/zynq/BASELINE.md` | OS-to-bus-to-core path and 16/16 algebraic property checks | Customer workload value or downstream physical outcomes |
| AX7020 board-run matrix | LIVE_MEASURED | `results/perf_matrix_ax7020_20260509.txt`, `docs/perf/20260509_matrix_interpretation.md` | Workload-specific measured matrix with wins and losses | Universal speedup or thermal/power/battery savings |
| Formal proof work | SOFTWARE_VALIDATED | `math/proofs/` | Algebraic proof foundation exists in repo | Public theorem counts until audited across materials |
| SD boot artifacts | BUILD_ARTIFACT | `hardware/zynq/fsbl_build/BOOT.bin`, related bitstream artifacts | Local build artifacts exist | Standalone power-on boot validation until recorded run exists |

## 2. Friday Ask

| Ask | Detail | Human gate |
|---|---|---|
| Capital | $2.0M target pre-seed; $1.25M minimum; $2.75M stretch | CFO/counsel approve final SAFE terms |
| Customer access | Introductions to edge/embedded teams with one state-heavy workload, baseline, and painful constraint | Must not imply signed customers |
| Technical diligence | ASIC/IP advisors, hardware reviewers, proof reviewers | Must keep proof labels intact |
| Design partners | Qualified evaluation conversations | SOW/LOI status must be documented before claimed |

## 3. Use Of Funds

| Category | Target allocation | Milestone |
|---|---:|---|
| Engineering + demo hardening | $600K | Repeatable proof system and workload tooling |
| Customer proof | $400K | Measured workload artifact and evaluation process |
| IP + legal | $300K | Patent conversion, prior-art review, diligence packet |
| ASIC feasibility | $300K | Expert feasibility review before tape-out decision |
| Finance/GTM/ops + reserve | $400K | CFO support, reporting, outreach, runway buffer |

## 3A. Financial Due Diligence Snapshot

| Check | Result | Caveat |
|---|---|---|
| Target use-of-funds sum | $2.0M exactly | Formula-backed in workbook; vendor quote detail still needed. |
| Minimum / target / stretch totals | $1.25M / $2.0M / $2.75M | These are financing scenarios, not commitments to close all three. |
| Target runway math | ~$111K/month gross; ~$103K/month excluding reserve | Average planning budget, not fixed monthly burn. |
| Reservation pricing | $750 proof review; $2,500 technical evaluation reservation | Qualification signal only; not forecast revenue. |
| SAFE dilution sensitivity | $2.0M at $10M post-money cap implies 20% SAFE ownership | Sensitivity only; not a cap recommendation. |
| Tape-out spend | Excluded | Round funds ASIC feasibility and quote-backed go/no-go path only. |

If the CFO is unavailable, Matt should not state a valuation cap. The safe answer is: "The ask and use of funds are set; the cap, discount, pro-rata, side letters, and close mechanics will be finalized with counsel/CFO."

## 4. Missing Before Serious Follow-Up Diligence

- CFO-reviewed SAFE terms, valuation cap, discount, pro-rata rights, close mechanics, and dilution model.
- Counsel-reviewed entity formation, founder stock issuance, IP assignment, SAFE template, and patent conversion plan.
- CFO/accounting-reviewed monthly burn/runway model with taxes, benefits, insurance, vendor quotes, and exact hiring timing.
- Counsel/founder-reviewed evaluation price bands and SOW terms; current worksheet/template are planning drafts.
- Direct artifact links or data-room copies for every proof card.
- Recorded SD power-on boot artifact if SD boot is to be promoted beyond `BUILD_ARTIFACT`.
- Correctness-passing Zynq workload artifacts before any new workload performance claim.

## 5. Claims Not Supported Today

Do not claim:

- universal speedup
- production readiness
- commercial product maturity
- guaranteed battery extension
- measured heat reduction
- cooling reduction
- water savings
- smaller hardware footprint
- CPU/GPU/NPU replacement
- signed customers, LOIs, or revenue
- tape-out funded by this round
- acquisition outcome or return multiple

## 6. What Friday Capital Buys

The fundable claim is that a focused pre-seed round can move ATOMiK from hardware-backed primitive proof to measured customer-value proof and diligence-ready IP.

The most important milestone is not a bigger benchmark number. It is one correctness-preserving customer or representative workload artifact that shows where state-aware execution reduces redundant state movement against a baseline.
