# ATOMiK Phase 3: Hardware Synthesis - Execution Guide

**Date**: January 25, 2026  
**Status**: Ready to Begin  
**Budget**: $150 allocated, $0 used  
**Estimated Time**: 4-6 hours of agent execution

---

## Pre-Flight Checklist

Before starting, verify these are complete:

```bash
# 1. Navigate to project directory
cd C:\Users\matth\OneDrive\Personal\Projects\ATOMiK

# 2. Verify Icarus Verilog is installed
iverilog -V

# 3. Verify Verilator is installed (optional but recommended)
verilator --version

# 4. Verify existing RTL compiles
iverilog -o test_compile rtl/atomik_core.v rtl/atomik_top.v
rm test_compile

# 5. Verify API key is set
echo $ANTHROPIC_API_KEY

# 6. Verify Claude Code CLI
claude --version
```

---

## Execution Sequence

### TASK T3.1: RTL Architecture Specification
**Model**: Claude Sonnet 4.5  
**Dependencies**: None (entry point)  
**Human Review**: ✅ REQUIRED after completion

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.1.txt)"
```

**Validation After T3.1**:
```bash
# Verify file created
cat specs/rtl_architecture.md | head -50

# Check status updated
grep "T3.1" .github/atomik-status.yml
```

**🛑 HUMAN CHECKPOINT**: Review `specs/rtl_architecture.md` before proceeding.
- Verify block diagram makes sense
- Confirm interface matches existing atomik_top.v
- Approve before implementation begins

---

### TASK T3.2: Delta Accumulator Module
**Model**: Claude Sonnet 4.5  
**Dependencies**: T3.1 complete

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.2.txt)"
```

**Immediate Validation (Run After T3.2)**:
```bash
# Lint check
verilator --lint-only -Wall rtl/atomik_delta_acc.v

# Run unit tests
cd sim && chmod +x run_delta_acc.sh && ./run_delta_acc.sh
cd ..
```

**Expected Output**: All tests pass, no lint warnings.

---

### TASK T3.3: State Reconstructor Module
**Model**: Claude Sonnet 4.5  
**Dependencies**: T3.1 complete (can run parallel with T3.2)

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.3.txt)"
```

**Immediate Validation (Run After T3.3)**:
```bash
# Lint check
verilator --lint-only -Wall rtl/atomik_state_rec.v

# Run unit tests
cd sim && chmod +x run_state_rec.sh && ./run_state_rec.sh
cd ..
```

**Expected Output**: All tests pass, no lint warnings.

---

### TASK T3.4: Integrated Core v2
**Model**: Claude Sonnet 4.5  
**Dependencies**: T3.2 AND T3.3 complete

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.4.txt)"
```

**Immediate Validation (Run After T3.4)**:
```bash
# Lint check
verilator --lint-only -Wall rtl/atomik_core_v2.v rtl/atomik_delta_acc.v rtl/atomik_state_rec.v

# Quick compile test
iverilog -o test_core rtl/atomik_core_v2.v rtl/atomik_delta_acc.v rtl/atomik_state_rec.v
rm test_core
```

---

### TASK T3.5: Comprehensive Simulation
**Model**: Claude Sonnet 4.5  
**Dependencies**: T3.4 complete  
**Human Review**: ✅ REQUIRED - critical validation point

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.5.txt)"
```

**Run Full Test Suite**:
```bash
cd sim
chmod +x run_all_tests.sh
./run_all_tests.sh
cd ..
```

**🛑 HUMAN CHECKPOINT**: ALL TESTS MUST PASS before proceeding.
- Review test output for any failures
- Check waveforms if issues arise (use GTKWave)
- Verify equivalence with Python reference model

**If Tests Fail - Debug Loop**:
```bash
# Use Haiku for quick debug assistance
claude --model claude-haiku-4-5 "The following simulation test failed: [paste error]. Review rtl/atomik_core_v2.v and suggest fix."
```

---

### TASK T3.6: Timing Constraints
**Model**: Claude Sonnet 4.5  
**Dependencies**: T3.5 complete (all tests passing)

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.6.txt)"
```

**Validation**:
```bash
# Verify constraint files created
ls -la constraints/atomik_timing.sdc
ls -la constraints/atomik_constraints.cst
```

---

### TASK T3.7: Synthesis Scripts
**Model**: Claude Sonnet 4.5  
**Dependencies**: T3.6 complete

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.7.txt)"
```

**🛑 HUMAN CHECKPOINT**: Run synthesis manually
```bash
# Make scripts executable
chmod +x synth/run_synthesis.sh
chmod +x synth/run_yosys.sh

# Run Gowin synthesis (if Gowin EDA available)
cd synth && ./run_synthesis.sh

# OR run open-source flow
cd synth && ./run_yosys.sh
```

**Expected**: Synthesis completes successfully. Timing should close at 50 MHz+.

---

### TASK T3.8: Resource Analysis
**Model**: Claude Sonnet 4.5  
**Dependencies**: T3.7 complete + synthesis run

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.8.txt)"
```

**Validation**:
```bash
cat reports/resource_utilization.md
```

---

### TASK T3.9: Hardware Validation Report
**Model**: Claude Sonnet 4.5  
**Dependencies**: ALL T3.1-T3.8 complete  
**Human Review**: ✅ REQUIRED - final approval

```bash
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.9.txt)"
```

**🛑 FINAL HUMAN CHECKPOINT**:
- Review `reports/PHASE_3_COMPLETION_REPORT.md`
- Verify all validation gates passed
- Approve before marking Phase 3 complete

---

## Escalation Protocol

### When to Use Opus (Complex Issues Only)

If you encounter architectural issues that Sonnet cannot resolve:

```bash
claude --model claude-opus-4-5 "
I'm working on ATOMiK Phase 3 Hardware Synthesis. 

[Describe the specific issue]

Relevant proven properties from Phase 1:
- Commutativity: δ₁⊕δ₂ = δ₂⊕δ₁
- Associativity: (δ₁⊕δ₂)⊕δ₃ = δ₁⊕(δ₂⊕δ₃)
- Self-inverse: δ⊕δ = 0

Phase 2 benchmark target: Eliminate 32% read penalty via O(1) reconstruction.

Please propose a solution that maintains single-cycle operation and synthesizes on Gowin GW1NR-9.
"
```

### Haiku Quick Checks

For quick lint/syntax issues:
```bash
claude --model claude-haiku-4-5 "Run verilator lint on rtl/atomik_core_v2.v and report any errors."
```

---

## Budget Tracking

| Task | Model | Est. Tokens | Est. Cost | Status |
|------|-------|-------------|-----------|--------|
| T3.1 | Sonnet | 25K | $8 | ⏳ |
| T3.2 | Sonnet | 20K | $6 | ⏳ |
| T3.3 | Sonnet | 20K | $6 | ⏳ |
| T3.4 | Sonnet | 25K | $8 | ⏳ |
| T3.5 | Sonnet | 30K | $10 | ⏳ |
| T3.6 | Sonnet | 20K | $6 | ⏳ |
| T3.7 | Sonnet | 20K | $6 | ⏳ |
| T3.8 | Sonnet | 15K | $5 | ⏳ |
| T3.9 | Sonnet | 20K | $6 | ⏳ |
| Validation | Haiku | 50K | $3 | ⏳ |
| Contingency | - | - | $36 | Reserved |
| **Total** | - | ~245K | **$100** | Under $150 budget |

---

## Git Workflow

### After Each Task Completion

```bash
# Stage changes
git add -A

# Commit with appropriate tag
git commit -m "[synthesis] T3.X: Description

- List key changes
- Token usage: XXK"

# Example for T3.1:
git commit -m "[synthesis] T3.1: RTL architecture specification

- Created specs/rtl_architecture.md
- Defined atomik_delta_acc, atomik_state_rec, atomik_core_v2 interfaces
- Documented timing diagrams and verification plan
- Token usage: ~25K"
```

### After All Tasks Complete

```bash
# Final push with all Phase 3 changes
git push origin main
```

---

## Summary Checklist

- [ ] T3.1: RTL Architecture Spec → **Human Review**
- [ ] T3.2: Delta Accumulator → Run unit tests
- [ ] T3.3: State Reconstructor → Run unit tests
- [ ] T3.4: Integrated Core → Lint check
- [ ] T3.5: Simulation Suite → **Human Review** (ALL TESTS PASS)
- [ ] T3.6: Timing Constraints → Verify files created
- [ ] T3.7: Synthesis Scripts → **Human Review** (Run synthesis)
- [ ] T3.8: Resource Analysis → Review report
- [ ] T3.9: Validation Report → **Final Human Approval**
- [ ] Git commit and push
- [ ] Update atomik-status.yml to phase_3.status: complete

---

## Next Steps After Phase 3

Once Phase 3 is complete and validated:
1. Update README.md with Phase 3 completion status
2. Begin Phase 4: SDK Development
3. Consider optional hardware programming to Tang Nano 9K
