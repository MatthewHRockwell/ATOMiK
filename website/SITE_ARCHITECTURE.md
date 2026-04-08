# atomik.tech — Site Architecture Plan

Based on analysis of ARM, SiFive, Cerebras, and Groq website structures.

## Key Principles (from research)

1. **Lead with one bold claim** — "Stop moving data. Start evolving it."
2. **Solutions-first navigation** — frame by customer need, not product spec (SiFive pattern)
3. **Performance numbers alongside pricing** — benchmarks ARE the sales pitch (Cerebras/Groq pattern)
4. **Free tier with instant access** — non-negotiable for developer adoption (Groq pattern)
5. **Customer multipliers over raw specs** — "8,192x less bandwidth" not "80KB transmitted" (Groq pattern)
6. **Founder credibility front and center** — the invention story IS the trust signal (SiFive pattern)
7. **Public documentation, gated tools** — specs freely available, configuration behind login (SiFive/ARM pattern)
8. **Interactive demos** — show what's possible, don't just describe it (Groq pattern)

---

## Site Navigation

```
atomik.tech
├── Home (landing page — hero, metrics, how it works, benchmarks, pricing, investors)
├── Product ▾
│   ├── Overview (the 4-op algebra, what it replaces)
│   ├── atomik-core (Python library — PyPI, zero deps)
│   ├── atomik-core-c (C99 header — any compiler)
│   ├── atomik-pro (Licensed — CPU/GPU optimized, multi-platform installers)
│   └── Hardware IP (FPGA/ASIC cores — licensing for silicon integration)
├── Solutions ▾
│   ├── Distributed Systems (consensus-free convergence)
│   ├── Edge & IoT (bandwidth reduction for constrained networks)
│   ├── Financial Services (real-time P&L, deterministic latency)
│   ├── AI & ML (checkpoint compression, weight delta streaming)
│   ├── Security (timing-safe operations, Spectre-immune architecture)
│   └── Telecommunications (state sync at scale)
├── Developers ▾
│   ├── Getting Started (quickstart guide)
│   ├── Documentation (API reference, integration guides)
│   ├── Examples (distributed cache, IoT fusion, analytics)
│   ├── Interactive Demo (live in-browser demo)
│   ├── GitHub (link)
│   └── PyPI (link)
├── Resources ▾
│   ├── Blog / Articles
│   ├── Whitepapers (formal proofs summary, performance analysis)
│   ├── Press / Newsroom
│   └── Case Studies (when available)
├── About ▾
│   ├── Company & Mission
│   ├── Founder (Matt Rockwell — story, credentials)
│   ├── Technology Timeline (concept → proofs → FPGA → silicon path)
│   ├── ASIC Roadmap (foundry engagement, Sky130, tape-out plans)
│   └── Careers (when applicable)
├── Pricing (free / professional / enterprise / hardware IP)
└── Contact
    ├── General (mrockwell@atomik.tech)
    ├── Sales (sales@atomik.tech)
    ├── Press (press@atomik.tech)
    └── Support (support@atomik.tech)
```

---

## Page Details

### 1. Homepage (`/`)
Already built. Current landing page with:
- Hero: "Stop moving data. Start evolving it." + matrix background
- 4 metric cards (99.9%, 92, 333,333x, 1,291x)
- How It Works: 4 operation cards + architecture diagram
- Features: 6-card grid
- Before/After comparison
- Code example
- Visual benchmarks with bar charts
- Throughput tiers (Python → C → FPGA)
- Pricing with Stripe checkout
- Investor section
- Mission statement
- Full footer

**ADD:** Security & latency specs section between benchmarks and pricing:
- Deterministic latency (no timing side channels)
- No speculative execution (Spectre-immune)
- No cache coherency attacks
- Energy per operation comparison vs traditional approaches

### 2. Product Pages

#### `/product` — Overview
- The paradigm shift: reconstruct vs store
- The 4-operation algebra with animated diagram
- Language/platform matrix (Python, C, Rust*, JS*, Verilog)
- Upgrade path visual (software → C → FPGA → ASIC)

#### `/product/atomik-core` — Python Library
- `pip install atomik-core` hero
- API reference with interactive code blocks
- Performance benchmarks
- Link to PyPI, GitHub

#### `/product/atomik-core-c` — C Library
- Single-header pattern explanation
- Platform compatibility matrix
- Performance benchmarks (500M ops/sec)

#### `/product/pro` — ATOMiK Pro (Licensed)
- What you get: compiled C extensions, GPU acceleration, multi-platform installers
- Platform downloads (Linux, Windows, macOS)
- License key activation flow
- Pricing → Stripe checkout

#### `/product/hardware` — Hardware IP
- FPGA core specifications (N=1 through N=512)
- Zynq characterization results table
- ASIC roadmap (Sky130 trial → production foundry)
- Contact sales for licensing

### 3. Solutions Pages (one per vertical)

Each follows a template (SiFive pattern):
1. Hero with vertical-specific headline
2. The problem in this domain
3. How ATOMiK solves it (with domain-specific metrics)
4. Architecture diagram for this use case
5. Code example / demo relevant to this vertical
6. Contact CTA

#### `/solutions/distributed-systems`
- Problem: consensus protocols, full-state replication
- ATOMiK: order-independent convergence, 99.9% bandwidth reduction
- Demo: 3-node convergence visualizer

#### `/solutions/edge-iot`
- Problem: constrained bandwidth, battery-powered sensors
- ATOMiK: 8-byte deltas, 99.2% bandwidth savings
- Energy comparison: XOR vs full-state copy power consumption

#### `/solutions/financial`
- Problem: real-time P&L tracking, deterministic latency requirement
- ATOMiK: constant-time operations, multi-context tables
- Demo: 64-instrument P&L tracker

#### `/solutions/ai-ml`
- Problem: model checkpoint size, distributed training sync
- ATOMiK: delta-based checkpointing, weight diff streaming
- Comparison: 2.4 GB checkpoint vs ATOMiK delta

#### `/solutions/security`
- Problem: Spectre, Meltdown, timing side channels, cache attacks
- ATOMiK: deterministic execution, no speculation, no cache dependency
- **Interactive demo: Spectre attack simulation**
  - Side-by-side: traditional CPU (vulnerable) vs ATOMiK (immune)
  - Visual showing timing variation (traditional) vs flat timing (ATOMiK)
  - Explains WHY ATOMiK is immune (no speculation, no shared cache state)

#### `/solutions/telecom`
- Problem: state sync across thousands of network elements
- ATOMiK: delta streaming, order-independent merge

### 4. Developer Pages

#### `/developers` — Hub
- Quick start (3 steps to first delta)
- Link to docs, examples, GitHub, PyPI
- Community links (when established)

#### `/developers/docs` — Documentation
- Getting Started guide
- API Reference (Python, C)
- Architecture deep-dive
- Integration patterns (Redis, gRPC, WebSocket)
- FAQ

#### `/developers/demo` — Interactive Demo
**Primary demo: Delta Streaming Visualizer (JavaScript, runs in browser)**
- 3-5 animated nodes generating and exchanging deltas
- Real-time bandwidth counter (ATOMiK vs traditional)
- Visual convergence animation
- User can add/remove nodes, trigger deltas

**Security demo: Timing Attack Visualizer**
- Shows operation timing for traditional CPU (variable, leaks data)
- Shows ATOMiK timing (constant, deterministic)
- Explains Spectre/Meltdown attack vectors and why they fail

### 5. About Pages

#### `/about` — Company & Mission (ARM/SiFive pattern)
- Mission statement (expanded from landing page)
- The invention story — from mathematical insight to silicon
- Key proof points: 108 theorems, 417+ tests, 3 FPGA platforms, 69.7 Gops/s
- Technology timeline:
  - 2025: Mathematical formalization (108 Lean4 theorems)
  - 2025: Software SDK (5 languages, 353 tests)
  - 2025: FPGA v1 — PicoRV32 + ATOMiK on Tang Nano 9K
  - 2025: FPGA v2 — Custom RV64I CPU with ATOMiK ISA
  - 2025: FPGA v3 — 1280x720 HDMI, multi-node convergence proven
  - 2026: Zynq characterization — 69.7 Gops/s, 444 MHz single-bank
  - 2026: Software licensing launch (atomik-core on PyPI)
  - 2026: ASIC path initiated (Sky130 evaluation)
- Investor logos (when applicable)
- Awards / recognition (when applicable)

#### `/about/founder` — Matt Rockwell
- Background, vision, credentials
- The founding insight
- Professional photo
- Contact links

#### `/about/asic-roadmap` — Silicon Path
- Current: FPGA validated (Gowin, Xilinx)
- Next: Sky130 trial tape-out (Efabless/Silicon Catalyst)
- Future: Production foundry (TSMC/Samsung/GlobalFoundries)
- SRAM compiler swap procedure
- Gate count estimates
- Partner logos (foundry, EDA when established)

### 6. Pricing (`/pricing`)
Dedicated page expanding on landing page pricing section.
- Community (Free) / Professional ($99/mo) / Enterprise ($499/mo) / Hardware IP (Contact)
- Feature comparison matrix (detailed)
- FAQ (what's included, what needs production license, etc.)
- Stripe checkout integration

### 7. Resources

#### `/blog` — Articles & Technical Content
- Featured post hero (Cerebras/Groq pattern)
- Chronological listing below
- Initial articles:
  - "Why XOR? The Mathematics Behind ATOMiK"
  - "From 92 Proofs to Production Silicon"
  - "Replacing Consensus Protocols with Algebra"
  - "ATOMiK vs Event Sourcing: A Quantitative Comparison"
  - "Building a Custom RISC-V CPU on a $13.50 FPGA"

#### `/press` — Newsroom
- Press releases
- Media kit (logo, brand assets, product screenshots, boilerplate)
- Press contact (press@atomik.tech)

#### `/whitepapers` — Technical Deep-Dives
- Formal proof summary (Lean4 theorem catalog)
- Performance characterization (Gowin + Xilinx results)
- Security analysis (timing safety, Spectre immunity)

### 8. Legal Pages
- `/privacy` — Privacy policy
- `/terms` — Terms of service
- `/license` — License agreement details

---

## Specs Section for Landing Page

Add between benchmarks and pricing:

### Performance & Efficiency

| Metric | ATOMiK | Traditional | Advantage |
|--------|--------|-------------|-----------|
| Operation latency | 10.6 ns (FPGA) | Variable (cache-dependent) | **Deterministic** |
| Energy per op | ~1 pJ (single XOR gate) | ~100 pJ (cache lookup + compare) | **~100x less** |
| Memory per rollback | 24 bytes (constant) | 8 bytes × history depth | **333,333x less** |
| Bandwidth per update | 8 bytes (delta) | Full state size | **8,192x less** |
| Time complexity | O(1) all operations | O(n) for replay/scan | **Constant** |

### Security Properties

| Attack Vector | Traditional CPU | ATOMiK |
|--------------|----------------|--------|
| Spectre (speculative execution) | Vulnerable | **Immune** — no speculation |
| Meltdown (privilege escalation) | Vulnerable | **Immune** — no out-of-order memory access |
| Cache timing attacks | Vulnerable | **Immune** — no cache-dependent timing |
| Rowhammer (DRAM bit flips) | Vulnerable | **Detected** — XOR fingerprint catches flips |
| State tampering | Undetected | **Detected** — accumulator integrity check |
| Replay attacks | Must track sequence numbers | **Self-cancelling** — XOR self-inverse |

---

## Implementation Priority

### Phase 1 — Revenue (this week)
1. ✅ Landing page with Stripe checkout
2. ✅ Payment API routes
3. ✅ Success page with license delivery
4. Add specs/security section to landing page
5. Test complete payment flow in sandbox
6. Deploy to Vercel + connect atomik.tech domain

### Phase 2 — Credibility (next week)
7. About page (mission, timeline, founder)
8. ASIC roadmap page
9. Pricing page (expanded)
10. Blog with 2-3 launch articles
11. Press page with media kit
12. Privacy + Terms pages

### Phase 3 — Developer Adoption (week after)
13. Developer hub with docs
14. Interactive delta streaming demo (JavaScript)
15. Security timing demo
16. Solution vertical pages (start with 2-3)

### Phase 4 — Scale
17. Cross-platform installer builds
18. Inference demo (if GPU backend available)
19. Case studies (as customers arrive)
20. Community / Discord
