# First Two Hires — Job Descriptions

*Budget: $450K–$600K (15% of raise) for first 18 months*

---

## Hire #1: FPGA/ASIC Design Engineer

**Role:** Lead hardware engineer responsible for ATOMiK IP core development across FPGA families and ASIC feasibility.

**Why first:** The Zynq port is validated but needs full PS+PL deployment. Mid-range FPGA (N=64+ banks) and ASIC feasibility study require dedicated hardware expertise. This hire unblocks the entire hardware roadmap.

### Responsibilities
- Own ATOMiK RTL development: parameterized core, parallel bank architecture, merge tree optimization
- Complete Zynq-7020 deployment (PS+PL block design, Linux driver integration)
- Port to mid-range FPGA targets (Xilinx Artix/Kintex, Lattice ECP5/Nexus)
- Drive ASIC feasibility study with foundry partner (gate-level synthesis, power/area estimation)
- Maintain synthesis sweep infrastructure (25+ configurations, automated regression)
- Collaborate on customer integration pilots

### Requirements
- 3–7 years FPGA design experience (Xilinx Vivado and/or Intel Quartus)
- Proficiency in Verilog/SystemVerilog, timing closure, resource optimization
- Experience with at least one ASIC tapeout or foundry engagement (28nm or below)
- Strong understanding of clock domain crossing, BRAM inference, carry-chain optimization
- Familiarity with AXI/AXI-Lite bus protocols

### Nice-to-Have
- RISC-V ISA experience (custom instruction integration)
- Formal verification exposure (Lean4, Coq, or industrial tools like JasperGold)
- Experience with Gowin FPGAs or other low-cost families
- Prior work at AMD/Xilinx, Lattice, Intel PSG, or FPGA-centric startup

### Compensation
- $150K–$200K base (remote-friendly, outside SF)
- 1.0–2.0% equity (4-year vest, 1-year cliff)
- Standard benefits package

### Target Companies for Sourcing
- AMD/Xilinx (RTL designers, especially from Vivado IP team)
- Lattice Semiconductor (lean FPGA design teams)
- Intel PSG / Altera (FPGA architects)
- SiFive, Tenstorrent, Esperanto (RISC-V + hardware)
- National labs (Sandia, LLNL — FPGA groups)

---

## Hire #2: Application Engineer / Developer Advocate

**Role:** Bridge between ATOMiK technology and customers. Owns SDK productionization, vertical module development, and early customer engagements.

**Why second:** Once the hardware roadmap is staffed, commercial traction depends on making ATOMiK accessible. This hire converts pilot interest into design wins by building vertical solutions and supporting integration.

### Responsibilities
- Production-harden the ATOMiK SDK (Python, C, Rust — focus on customer-facing quality)
- Build vertical modules: HFT tick processor, sensor fusion core, streaming transform library
- Own developer documentation, examples, and tutorials
- Support pilot customer integrations (on-site or remote)
- Build and maintain benchmark suite for customer-specific workloads
- Contribute to conference presentations and technical blog posts

### Requirements
- 3–5 years software engineering with systems-level experience (C, Rust, or C++ required)
- Experience building SDKs, libraries, or developer tools used by external customers
- Strong understanding of at least one target vertical:
  - **HFT/quant**: Market data processing, tick-to-trade pipelines, FIX/ITCH protocols
  - **Edge AI/IoT**: Sensor fusion, embedded inference, RTOS integration
  - **Streaming**: Kafka, Flink, or similar streaming frameworks
- Comfortable reading Verilog/FPGA documentation (not writing RTL, but understanding the hardware)
- Excellent technical writing and communication

### Nice-to-Have
- Python code generation / metaprogramming experience
- Prior developer relations or technical evangelist role
- Experience with formal methods or property-based testing
- Familiarity with FPGA development workflows (synthesis, bitstream, deployment)

### Compensation
- $130K–$170K base (remote-friendly)
- 0.5–1.5% equity (4-year vest, 1-year cliff)
- Standard benefits package

### Target Companies for Sourcing
- Trading firms (Jump Trading, Two Sigma, Hudson River — infrastructure teams)
- Embedded/IoT companies (Arduino, Particle, Edge Impulse)
- Developer tools companies (Vercel, Supabase, Fly.io — dev advocate alumni)
- Cloud FPGA teams (AWS F1, Azure, Intel DevCloud)

---

## Hiring Timeline

| Month | Action |
|-------|--------|
| 1–2 | Post roles, begin sourcing from target companies |
| 2–3 | Interview pipeline for Hire #1 (FPGA engineer) |
| 3–4 | **Hire #1 starts** — onboards with Zynq deployment task |
| 4–5 | Interview pipeline for Hire #2 (application engineer) |
| 5–6 | **Hire #2 starts** — onboards with SDK hardening task |
| 6+ | Both hires productive; evaluate need for Hire #3 |

### Onboarding Advantage
Every artifact is documented and reproducible:
- RTL: `make` builds and synthesizes from source
- Proofs: `lake build` checks all 108 Lean4 theorems
- SDK: `pytest` runs 353 tests across 5 languages
- Hardware: Flash scripts deploy to FPGA in <60 seconds
- Papers: 3 manuscripts document the full architecture

A new engineer can build, test, and deploy ATOMiK on their first day. No tribal knowledge required.

---

## Option Pool

Post-funding allocation: 10–15% option pool (standard seed stage)

| Role | Equity Range | Notes |
|------|-------------|-------|
| FPGA/ASIC Engineer (#1) | 1.0–2.0% | Senior technical, first hire premium |
| Application Engineer (#2) | 0.5–1.5% | Customer-facing, slightly later |
| Future CTO/co-founder | 3.0–5.0% | If identified post-funding |
| Advisors (3 target) | 0.25–0.5% each | 2-year vesting |
| Remaining pool | 3.0–8.0% | Future hires through Series A |
