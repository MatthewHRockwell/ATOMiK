# First Hires And Fractional Roles

> Current diligence draft: 2026-05-23. Compensation and timing are planning
> assumptions only; final hiring timing depends on the approved pre-seed close and CFO/counsel-reviewed budget.

## Sequencing Principle

ATOMiK should not hire broadly before the next proof gates are clear. The first
people should convert founder-led proof into investor- and customer-diligence
quality.

## Fractional / Advisor Roles First

| Role | Why now | Output |
|---|---|---|
| Fractional CFO | Approve terms, valuation cap, runway, and financing structure | Investor-ready financial model and use-of-funds plan |
| ASIC mentor | Pressure-test feasibility before tape-out claims | ASIC feasibility scope and technical diligence notes |
| Customer advisor | Turn architecture into buyer-specific evaluation | First design-partner target list and success metrics |

## Hire #1: FPGA / ASIC Design Engineer

**Role:** Own hardware IP development, Zynq proof hardening, and ASIC feasibility
support.

**Why first:** The technical roadmap needs an engineer who can turn prototype
proof into repeatable hardware evaluation packages and support external ASIC
review.

### Responsibilities

- Own ATOMiK RTL development and regression discipline.
- Harden Zynq proof paths and standalone demo packaging.
- Prepare FPGA targets and synthesis reports for diligence.
- Support ASIC feasibility work with the ASIC mentor.
- Build workload-specific measurement harnesses for customer evaluations.

### Requirements

- FPGA design experience with Vivado or comparable toolchains.
- Verilog/SystemVerilog, timing closure, and resource optimization.
- Familiarity with AXI/AXI-Lite or memory-mapped hardware integration.
- Ability to document evidence boundaries clearly.

## Hire #2: Application / Customer Evaluation Engineer

**Role:** Convert customer constraints into measurable evaluation workloads.

**Why second:** ATOMiK needs design-partner proof, not only more demo polish.
This hire bridges SDK, examples, customer integrations, and benchmark packaging.

### Responsibilities

- Build evaluation harnesses for heat, power, bandwidth, latency, or footprint
  hypotheses.
- Harden customer-facing SDK examples and documentation.
- Support design-partner pilots and technical diligence requests.
- Package benchmark artifacts with raw output, interpretation, and evidence
  labels.

### Requirements

- Systems software experience in C, Rust, Python, or C++.
- Comfort with low-level performance measurement and hardware-facing APIs.
- Strong technical writing and customer-facing communication.
- Ability to keep measured results separate from projections.

## Hiring Timeline

| Stage | Action |
|---|---|
| Pre-seed close | Use fractional CFO and ASIC mentor to finalize plan. |
| First 30-60 days | Contract or hire FPGA / ASIC engineer if budget supports it. |
| 60-120 days | Add application / customer evaluation engineer after first evaluation target is defined. |
| After first measured customer proof | Reassess full-time team needs and option pool. |

## Onboarding Advantage

The repo already contains proofs, RTL, SDK, Zynq artifacts, demo assets, and
public-claim hygiene rules. New hires should be onboarded against evidence
production, not just feature velocity.
