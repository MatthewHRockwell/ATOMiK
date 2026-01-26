# Phase 4 Quick Start Guide

## Prerequisites

Before starting Phase 4 execution:

### 1. Phase 3 Completion
- [ ] Phase 3 complete (hardware validated on Tang Nano 9K)
- [ ] All Phase 3 documentation updated
- [ ] `.github/atomik-status.yml` shows Phase 3 complete
- [ ] Git repository clean (no uncommitted changes)

### 2. Documentation Updates from Phase 3→4 Transition
- [ ] Complete checklist in `docs/PHASE_3_TO_4_DOCUMENTATION_UPDATES.md`
- [ ] Commit Phase 3 completion with comprehensive message
- [ ] Push to GitHub

### 3. Hardware Readiness
- [ ] 1 Tang Nano 9K board (from Phase 3) - working
- [ ] 2 additional Tang Nano 9K boards ordered/arriving (for Phase 4C)
- [ ] Camera module (~$15) ordered for video demo
- [ ] Sensor modules (~$10) ordered for edge demo
- [ ] Power measurement module (INA219, ~$5) ordered

### 4. Software Environment
- [ ] Claude Code CLI installed and configured
- [ ] Python 3.8+ installed
- [ ] Rust toolchain installed (cargo)
- [ ] GCC/Clang installed (for C)
- [ ] Node.js installed (for JavaScript)
- [ ] Verilator installed (for Verilog linting)
- [ ] Gowin EDA installed (for synthesis)

### 5. Agent Framework Ready
- [ ] `agents/phase4/` directory exists
- [ ] `PHASE4_OVERVIEW.md` reviewed
- [ ] `PROMPTS.md` reviewed
- [ ] `prompt_T4A.1.txt` ready for execution

---

## Execution Workflow

### Step 1: Execute First Task (T4A.1)

```bash
cd C:\Users\matth\OneDrive\Personal\Projects\ATOMiK

# Execute T4A.1 with Sonnet 4.5
claude --model claude-sonnet-4-5 < agents\phase4\prompt_T4A.1.txt
```

**Expected duration:** ~2 hours (including context reading and file generation)

**Expected output:**
- 7 new files created in `specs/`, `sdk/`, `docs/`
- Status update in `.github/atomik-status.yml`
- Git commit message generated
- Completion report displayed

### Step 2: Validation (After T4A.1)

```bash
# Run validation with Haiku 4.5
claude --model claude-haiku-4-5 < agents\phase4\prompt_VALIDATION.txt
```

**What validation checks:**
- All required files exist
- JSON schema is valid
- Example schemas validate
- Documentation is complete

**If validation fails:** Review errors, fix issues, re-validate

**If validation passes:** Proceed to human checkpoint

### Step 3: Human Checkpoint

**Review T4A.1 output and answer:**

1. **Does schema support hierarchical namespace mapping?**
   - Check: vertical/field/object → Python import path
   - Example: `Video/Stream/H264Delta` → `from atomik.Video.Stream import H264Delta`

2. **Are all three example domains covered appropriately?**
   - terminal-io.json: Control/system primitive ✓
   - p2p-delta.json: Network primitive ✓
   - matrix-ops.json: Compute primitive ✓

3. **Is schema extensible for community contributions?**
   - Can new verticals be added easily? ✓
   - Can new fields be added within verticals? ✓
   - Are validation rules clear for contributors? ✓

**If approved:** Continue to T4A.2
**If not approved:** Use escalation prompt with Opus 4.5

### Step 4: Commit and Continue

```bash
# If not already done by agent
git add .
git commit -m "[T4A.1] JSON schema specification complete

..."

git push origin main
```

### Step 5: Execute Next Task (T4A.2)

```bash
claude --model claude-sonnet-4-5 < agents\phase4\prompt_T4A.2.txt
```

Repeat workflow for each subsequent task.

---

## Task Sequence with Checkpoints

### Phase 4A: SDK Generator Core (Week 1-2)

| Day | Task | Model | Checkpoint |
|-----|------|-------|------------|
| 1-2 | T4A.1 | Sonnet | ✓ HUMAN |
| 3-4 | T4A.2 | Sonnet | Auto |
| 5-6 | T4A.3 | Sonnet | Auto |
| 7-8 | T4A.4 | Sonnet | Auto |
| 9-10 | T4A.5 | Sonnet | Auto |
| 11-12 | T4A.6 | Sonnet | Auto |
| 13 | T4A.7 | Sonnet | Auto |
| 14 | T4A.8-9 | Sonnet | ✓ HUMAN |

**After Phase 4A:** Review complete SDK documentation before Phase 4B

### Phase 4B: Reference Patterns (Week 3-4)

| Day | Task | Model | Checkpoint |
|-----|------|-------|------------|
| 15-16 | T4B.1 | Sonnet | Auto |
| 17-18 | T4B.2 | Sonnet | Auto |
| 19-20 | T4B.3 | Sonnet | Auto |
| 21-28 | T4B.4 | Sonnet+Opus | ✓ HUMAN |

**After Phase 4B:** Review patterns and business analysis before Phase 4C

### Phase 4C: Hardware Capability Proofs (Week 5-6)

| Day | Task | Model | Hardware | Checkpoint |
|-----|------|-------|----------|------------|
| 29-32 | T4C.1 | Sonnet | Board 1 | ✓ HUMAN (Demo working?) |
| 33-36 | T4C.2 | Sonnet | Board 2 | ✓ HUMAN (Demo working?) |
| 37-40 | T4C.3 | Sonnet | Board 3 | ✓ HUMAN (Demo working?) |
| 41-44 | T4C.4 | Opus | All 3 | ✓ HUMAN (Investor package ready?) |
| 45-48 | Buffer | - | - | Final polish |

**After Phase 4C:** Final review of investor demo package

---

## Validation Commands by Language

### Python
```bash
# Syntax check
python -m py_compile generated/python/*.py

# Run tests
python -m pytest software/atomik_sdk/tests/test_python_*

# Import test
python -c "import sys; sys.path.insert(0, 'software/atomik_sdk/generated/python'); import atomik; print('OK')"
```

### Rust
```bash
cd software/atomik_sdk/generated/rust

# Check syntax
cargo check

# Build
cargo build

# Run tests
cargo test

# No warnings
cargo clippy -- -D warnings
```

### C
```bash
cd software/atomik_sdk/generated/c

# Compile with strict warnings
gcc -Wall -Werror -std=c11 -c *.c

# Run tests (if test.c exists)
gcc -Wall test.c *.c -o test && ./test

# Memory leak check (if valgrind available)
valgrind --leak-check=full ./test
```

### Verilog
```bash
cd software/atomik_sdk/generated/verilog

# Lint check
verilator --lint-only -Wall *.v

# Synthesis (if Gowin EDA available)
gw_sh -tcl synth.tcl

# Simulation (if Icarus Verilog available)
iverilog -o test *.v testbench.v
vvp test
```

### JavaScript
```bash
cd software/atomik_sdk/generated/javascript

# Syntax check
node -c *.js

# Run tests (if Jest configured)
npm test

# Lint (if ESLint available)
eslint *.js
```

---

## Escalation Protocol

### When to Escalate to Opus 4.5

1. **Architectural Decisions:**
   - Schema design conflicts with Phase 1-3 principles
   - Generator framework architecture unclear
   - Hardware mapping strategy needs refinement

2. **Strategic Questions:**
   - Business analysis for investor materials
   - Market sizing and competitive positioning
   - Pitch deck narrative and messaging

3. **Synthesis Issues:**
   - Generated Verilog won't synthesize
   - Timing closure problems
   - Resource utilization exceeds expectations

### How to Escalate

```bash
# Create escalation context file
echo "ISSUE: [Describe specific problem]

CONTEXT:
- Current task: T4X.Y
- What was attempted: [...]
- What went wrong: [...]
- Error messages: [...]

CONSTRAINTS:
[List relevant constraints from Phase 1-3]

QUESTION:
[Specific question for Opus]
" > escalation_context.txt

# Run escalation prompt
claude --model claude-opus-4-5 < agents/phase4/prompt_ESCALATION.txt
```

---

## Cost Monitoring

### Expected Costs by Phase

**Phase 4A (SDK Generator):** ~$80
- 9 Sonnet tasks @ ~$8 each
- 9 Haiku validations @ ~$0.50 each

**Phase 4B (Patterns):** ~$60
- 3 Sonnet tasks @ ~$7 each
- 1 Sonnet + 1 Opus task @ ~$15
- 4 Haiku validations @ ~$0.50 each

**Phase 4C (Hardware Demos):** ~$100
- 3 Sonnet tasks @ ~$12 each (includes hardware integration time)
- 1 Opus synthesis @ ~$15
- 4 Haiku validations @ ~$1 each
- Hardware components: ~$65

**Escalations:** ~$15 budget

**Total:** ~$255 (within variance of $240 budget)

### Monitoring Commands

```bash
# Check token usage after each task
# (manually track from Claude Code output)

# Keep running total in spreadsheet or:
echo "T4A.1: $8" >> phase4_costs.txt
echo "T4A.1_validation: $0.50" >> phase4_costs.txt
# ... etc
```

---

## Troubleshooting

### Common Issues

**Issue:** Agent doesn't read context files
**Solution:** Ensure file paths are correct, agent has read access

**Issue:** Generated code has syntax errors
**Solution:** Run validation immediately, fix before committing

**Issue:** Verilog won't synthesize
**Solution:** Check against Phase 3 proven modules, escalate if needed

**Issue:** Hardware demo doesn't work
**Solution:** Verify hardware connections, check bitstream programming, review Phase 3 setup

**Issue:** Validation fails repeatedly
**Solution:** Review validation requirements, check example files, escalate if stuck

### Getting Unstuck

1. **Review context:** Re-read PHASE4_OVERVIEW.md and task prompt
2. **Check examples:** Look at Phase 3 completed tasks for patterns
3. **Validate incrementally:** Don't wait until end to check if things work
4. **Use checkpoints:** Stop at human checkpoints, don't push through issues
5. **Escalate appropriately:** Don't waste time on issues Opus can solve quickly

---

## Success Indicators

### After Phase 4A (Week 2)
- [ ] SDK generates valid code in all 5 languages
- [ ] All reference schemas work correctly
- [ ] Documentation complete and clear
- [ ] Ready to demonstrate SDK to developers

### After Phase 4B (Week 4)
- [ ] Three patterns demonstrate key capabilities
- [ ] Patterns remain minimal (<200 LOC)
- [ ] Business analysis identifies markets
- [ ] Ready to explain use cases to investors

### After Phase 4C (Week 6)
- [ ] Three hardware demos working on silicon
- [ ] Benchmark data collected (240:1, 1000:1, etc.)
- [ ] Photos/videos of physical setups
- [ ] Investor demo package complete
- [ ] Ready for investor meetings and YC application

---

## Next Steps After Phase 4 Complete

1. **Investor Outreach:**
   - Schedule meetings using demo package
   - Submit YC application
   - Present at investor events

2. **Phase 5 Planning:**
   - Design convention-ready demos
   - Plan booth setup
   - Prepare demo scripts

3. **Community Preparation:**
   - Open source SDK on GitHub
   - Announce on Hacker News
   - Begin developer outreach

4. **Academic Publication:**
   - Finalize Paper 2 with Phase 4 results
   - Submit to arXiv
   - Consider conference submissions

---

*Phase 4 Quick Start Guide*
*Generated January 2026*
*Ready for Execution*
