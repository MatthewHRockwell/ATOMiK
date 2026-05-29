# Investor Materials Audit - 2026-05-23

> Scope: business collateral, pitch deck sources, data-room financials, ASIC
> economics language, and high-risk legacy materials. Goal: Aggie Angel pitch
> readiness with the customer-value narrative and evidence boundaries intact.

## Executive Verdict

The current investor packet is usable only if the team starts from the current
files listed in `business/PUBLICATION_NOTES.md`. Older business materials still
exist for history, but many contain outdated performance, TAM, valuation,
revenue, or tape-out language. They should not be shared into the Friday pitch
without a separate claim review.

## Narrative Standard

The story should stay narrow and customer-first:

- Data centers: less heat, lower cooling pressure, water-aware operations, rack
  density.
- Edge devices: longer battery life, lower enclosure heat, smaller hardware
  profiles.
- AI at the edge: less context movement, lower memory/bandwidth pressure.
- Defense / remote: every watt, ounce, packet, and minute of runtime matters.

Technical explanation should support those outcomes, not lead the pitch.

## Financial Diligence Updates Made

| Area | Issue found | Action taken |
|---|---|---|
| Financial model | Too little source-backed market context. | Added IEA, LBNL, SIA/WSTS, Carta, PitchBook/NVCA, YC, Techstars, and ASIC-cost source register. |
| Revenue model | Unsupported customer counts, Y5 revenue scenarios, implied Series A valuations, and return multiples. | Reset to qualitative scenario-planning worksheet with explicit non-claims. |
| ASIC economics | Unsupported dollar ranges and performance language could imply pre-seed funds tape-out. | Reframed to feasibility-first, quote-backed stage gates. |
| Data-room generator | Legacy generator could overwrite reviewed files with stale TAM/projection claims. | Disabled generator with an explicit error. |
| Founder/development-cost docs | Historical cost and proof-count claims were not pitch-safe. | Rewritten as internal capital-efficiency context with do-not-quote boundaries. |
| Customer pipeline | Old vertical map did not match the updated narrative. | Reframed around current customer-value segments and evaluation targets. |
| Pitch deck source notes | Old AI/Groq source notes were not needed and could distract. | Replaced with current source-backed financing/market appendix. |

## Current Investor Packet

Use these first:

- `business/pitch_deck/ATOMiK_Investor_Deck.pptx`
- `business/pitch_deck/ATOMiK_Aggie_Angel_Deck.pptx`
- `business/pitch_deck/slides.md`
- `business/one_pager/atomik_one_pager.md`
- `business/faq/investor_faq.md`
- `business/data_room/README.md`
- `business/data_room/01_financial/financial_model.md`

## Do Not Send Without Review

- `business/score_package/`
- `business/cosmos-cookoff/`
- `business/launch_posts.md`
- `business/marketing_posts_v2.md`
- `business/meeting_prep_*.md`
- `business/vc_diligence_response_*.md`
- `business/comparisons/`
- old generated data-room or deck scripts

Reason: these areas contain historical performance, valuation, market-size,
customer, or development-cost statements that are not aligned with the current
proof-bound investor narrative.

## Source Facts Now Approved For Context

These facts can be used as market context only:

- IEA: global data-center electricity use was about 415 TWh in 2024 and is
  projected at about 945 TWh by 2030 in the base case; cooling ranges from about
  7% to more than 30% of data-center electricity depending on facility type.
- LBNL: U.S. data centers used 176 TWh in 2023 and are projected at roughly
  325-580 TWh in 2028; direct U.S. data-center water consumption reached about
  66 billion liters in 2023.
- SIA/WSTS: global semiconductor sales are projected at $772.2B in 2025 and
  $975.4B in 2026.
- Carta: Q1 2026 U.S. pre-seed funding on Carta involved about 3,000 startups,
  over $2.3B recorded, and an expected final total around $2.9B.
- PitchBook/NVCA: Q1 2026 median U.S. VC seed pre-money valuation was $18.4M.
- YC / Techstars: accelerator terms are useful dilution context but not direct
  angel-round comps.
- AnySilicon: ASIC economics should be treated as a cost stack and quote-backed
  feasibility process, not a single generic number.

## Remaining Risks

- The packet now has a $2.0M target ask, $1.25M minimum close, and $2.75M stretch plan. CFO/counsel still need to approve final SAFE terms, valuation cap, discount, and close mechanics.
- Counsel still needs to confirm patent conversion timing and what can be said
  externally.
- No heat, water, battery, power, or footprint savings are yet measured ATOMiK
  results for a customer workload.
- Current public proof is compelling prototype evidence, not commercial product
  maturity.

## Next Highest-Value Slice

Financing talk track and valuation-term notes are now drafted. Remaining work is live practice with the CFO/CMO and counsel approval of final SAFE terms.
