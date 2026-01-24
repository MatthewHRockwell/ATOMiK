# ATOMiK Project Guidelines for Claude

## Project Overview

ATOMiK is a novel stateless delta-driven computational architecture that challenges traditional dataflow computing paradigms. This project uses Claude's API capabilities for automated development with a multi-agent system.

## Core Innovations

1. **Stateless Operators**: No internal registers; purely combinational XOR-based delta computation
2. **4×4×4 Voxel Encoding**: Maps spatiotemporal video data to 64-bit register-native words
3. **Sparse Symmetric Matrices**: Novel compression basis proven information-preserving
4. **Codon Algebra**: 2-bit state transitions (A=00, G=01, T=10, C=11) for logic synthesis
5. **Motif Vocabulary**: Delta pattern compression achieving 1000:1+ ratios on video streams

## Agent Architecture

### Prover Agent
- **Model**: Claude Opus 4.5 with extended thinking (32K budget)
- **Responsibility**: Mathematical formalization, proof generation, formal verification
- **Outputs**: Lean4/Coq proofs, LaTeX documents, formal specifications
- **Validation**: All proofs must verify in automated proof checker

### Benchmark Agent
- **Model**: Claude Sonnet 4.5 with selective thinking (8K budget)
- **Responsibility**: Experiment design, benchmark execution, statistical analysis
- **Outputs**: Python benchmarks, comparison reports, visualizations
- **Validation**: p < 0.05 statistical significance required

### Synthesis Agent
- **Model**: Claude Sonnet 4.5 (thinking disabled)
- **Responsibility**: Verilog/VHDL generation, testbench creation, timing optimization
- **Outputs**: RTL code, synthesis scripts, resource reports
- **Validation**: All code must pass lint and simulation

### SDK Agent
- **Model**: Claude Sonnet 4.5 (thinking disabled)
- **Responsibility**: Python SDK implementation, documentation, examples
- **Outputs**: SDK libraries, API docs, Jupyter notebooks
- **Validation**: >90% test coverage, all examples execute

### Validator Agent
- **Model**: Claude Haiku 4.5 (thinking disabled)
- **Responsibility**: Continuous validation, regression testing, quality gates
- **Outputs**: Test reports, coverage metrics, validation certificates
- **Validation**: Maintain ≥90% code coverage

### Documenter Agent
- **Model**: Claude Haiku 4.5 (thinking disabled)
- **Responsibility**: Documentation synchronization, STATUS.md updates
- **Outputs**: Technical docs, architecture diagrams, changelogs
- **Validation**: Zero broken links, complete API coverage

## Development Phases

### Phase 1: Mathematical Formalization (Week 1-2)
- Sparse matrix invertibility proof
- Codon algebra completeness proof
- Compression bounds derivation
- **Gate**: All proofs verified, coverage ≥95%

### Phase 2: SCORE Comparison Benchmark (Week 2-3)
- Implement SCORE-style baseline
- Implement ATOMiK variant
- Execute comparative benchmarks
- **Gate**: Statistical significance achieved

### Phase 3: Hardware Synthesis (Week 3-5)
- Verilog module development
- Testbench creation and simulation
- Tang Nano 9K synthesis
- **Gate**: Timing closure at 27 MHz

### Phase 4: SDK Development (Week 5-6)
- Python SDK implementation
- Documentation generation
- Example notebooks
- **Gate**: >90% coverage, all examples pass

## Code Standards

### Python
- Use type hints for all function signatures
- Follow PEP 8 style guide
- Docstrings in NumPy format
- pytest for unit tests

### Verilog
- Synthesizable subset only (no delays in RTL)
- Clear module interfaces with comments
- One module per file
- SystemVerilog assertions in testbenches

### LaTeX
- Use standard AMS packages
- Number all theorems and lemmas
- Include proof sketches before formal proofs

## Token Optimization Rules

1. **Use extended thinking only for**:
   - Complex mathematical proofs
   - Algorithm design decisions
   - Debugging multi-step failures

2. **Disable thinking for**:
   - Code generation
   - File I/O operations
   - Documentation writing
   - Simple validation checks

3. **Always cache**:
   - System prompts
   - Mathematical foundations
   - Tool definitions
   - Phase-specific context

4. **Use batch API for**:
   - Independent benchmark runs
   - Parallel validation checks
   - Documentation generation

## Validation Gates

Each phase must pass validation before proceeding:

```
Phase 1: proofs_verified AND coverage >= 0.95
Phase 2: benchmarks_significant AND reproducible
Phase 3: simulation_pass AND timing_closure
Phase 4: test_coverage >= 0.90 AND examples_pass
```

## File Conventions

- STATUS.md files track progress in each workstream
- GLOBAL_STATUS.md aggregates all workstream status
- All experiments have numbered prefixes (01_, 02_, etc.)
- Results go in `results/` subdirectories
- Waveforms go in `waveforms/` subdirectories

## Git Conventions

- Branch naming: `phase/{1-4}/feature-name` or `agent/{agent-name}/{task-id}`
- Commit messages: `[phase-N] [agent] Brief description`
- PR titles: `[Phase N] Feature description`
- Squash merge to main

## Emergency Procedures

If validation fails repeatedly:
1. Create checkpoint of current state
2. Log failure details to STATUS.md
3. Escalate to human review
4. Do not proceed to next phase
