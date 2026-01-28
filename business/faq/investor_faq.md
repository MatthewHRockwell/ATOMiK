# ATOMiK — Investor FAQ

## Technology

### "How is this different from event sourcing?"

Event sourcing stores a log of all events and replays them to reconstruct state. Reconstruction cost is **O(N)** — it grows linearly with the number of events. ATOMiK accumulates deltas via XOR, so reconstruction is always **O(1)** — a single operation regardless of history length. Event sourcing also requires ordered replay; ATOMiK's commutativity means order doesn't matter, enabling lock-free parallelism.

### "Why XOR and not addition or multiplication?"

XOR has unique properties that make it ideal for delta-state computation:
- **No carry chains**: Addition requires carry propagation across all 64 bits (O(N) gate delay). XOR operates on each bit independently (O(1) gate delay).
- **Self-inverse**: `a XOR a = 0`. Addition requires a separate subtraction operation for undo.
- **Formally proven**: The XOR Abelian group properties are machine-verified in Lean4.
- **Hardware efficiency**: A 64-bit XOR uses 64 LUTs. A 64-bit adder uses 64 LUTs + 64 ALU carry chains.

### "Can you really do useful computation with just XOR?"

Yes. ATOMiK doesn't replace all computation with XOR — it uses XOR specifically for **state accumulation and reconstruction**, which is the bottleneck in state-heavy systems. The delta algebra handles:
- State tracking (accumulate changes)
- State reconstruction (initial XOR accumulated)
- Undo/rollback (re-apply the same delta)
- Distributed merge (XOR partial results)
- Equality checking (accumulator == 0 means no changes)

Application-specific logic (parsing, decision-making, I/O) runs alongside, feeding deltas into the accumulator.

### "How does this compare to CRDTs?"

CRDTs (Conflict-free Replicated Data Types) also enable distributed merge without coordination. Key differences:
- **Hardware acceleration**: ATOMiK runs at silicon speed (10.6 ns per operation). CRDTs are software-only.
- **Formal verification**: 92 Lean4 proofs vs. correctness arguments in academic papers.
- **Generality**: CRDTs require designing specific merge functions per data type. ATOMiK's XOR algebra is universal for any 64-bit state.
- **Performance**: CRDTs have variable merge cost. ATOMiK merge is always single-cycle.

### "What about quantum computing?"

Delta algebra maps naturally to quantum gate composition. Quantum gates are unitary transformations — composable, invertible, order-sensitive in general but with important commutative subgroups. ATOMiK's proven commutativity results could extend to quantum state tracking, making it a potential bridge technology.

### "Does this work on GPUs?"

Yes. XOR commutativity means delta accumulation is a **parallel reduction** — the same pattern GPUs excel at. A 64-bit XOR reduction across thousands of CUDA cores would achieve massive throughput. The FPGA implementation proves the concept; GPU and ASIC implementations would scale further.

### "What about larger state sizes (beyond 64 bits)?"

The architecture is parametric. The RTL supports configurable `DATA_WIDTH`. 128-bit and 256-bit versions are straightforward — the math is identical, only the wire width changes. The domain schemas already define widths up to 256 bits (video H.264 deltas).

## Business

### "What's the TAM?"

$500B+ across several markets:
- **Database infrastructure**: $100B+ (state management is core to every database)
- **Financial technology**: $150B+ (HFT, risk management, settlement)
- **IoT and edge computing**: $100B+ (sensor fusion, state sync)
- **Video processing**: $50B+ (streaming, encoding, surveillance)
- **Real-time systems**: $100B+ (gaming, simulation, digital twins)

ATOMiK addresses the **state management layer** common to all of these — not the full application stack.

### "What's your patent strategy?"

Patent application filed covering:
1. The delta-state accumulation architecture (XOR-based parallel banks)
2. The execution model (round-robin distribution + binary merge tree)
3. The schema-driven code generation pipeline

The 92 Lean4 proofs provide additional protection — they document the mathematical novelty and prior art establishment.

### "What's the revenue model?"

Three tiers:
1. **IP licensing** (high margin): License RTL IP blocks to SoC/ASIC designers
2. **Platform subscription** (recurring): SDK + code generation + support
3. **Professional services** (relationship): Custom integration for enterprise accounts

### "Who are potential acquirers?"

- **FPGA companies** (AMD/Xilinx, Intel/Altera, Lattice): Novel IP architecture
- **Database companies** (Snowflake, Databricks, MongoDB): State management acceleration
- **Fintech infrastructure** (LSEG, CME, Nasdaq): HFT acceleration
- **Cloud providers** (AWS, Azure, GCP): Accelerated state services
- **EDA companies** (Synopsys, Cadence): IP block portfolio

### "What's the competitive response risk?"

Low, because:
1. **Patent protection** on the architecture
2. **Formal proofs** create a "proof moat" — replicating 92 Lean4 theorems requires significant mathematical expertise
3. **Full-stack integration** (proofs + RTL + SDK + pipeline) is hard to replicate piecemeal
4. **First-mover** on hardware-validated delta-state computation

### "What's the team's background?"

The project demonstrates deep expertise across:
- Formal verification (Lean4 theorem proving)
- FPGA/RTL design (Gowin synthesis, timing closure, parallel architectures)
- Software engineering (Python SDK, 5-language code generation, agentic orchestration)
- Hardware validation (UART protocols, multi-board testing)

### "What are the key risks?"

1. **Market adoption**: Novel computational paradigm requires developer education
2. **Hardware scaling**: Larger FPGAs and ASICs require additional engineering
3. **Competition**: Established FPGA IP vendors could develop similar architectures
4. **Patent prosecution**: Timeline and scope of granted claims

**Mitigants**: Open-source SDK builds community. Formal proofs establish novelty. Full demo validates real-world applicability.
