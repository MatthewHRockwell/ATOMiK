# Phase 3 Quick Start Guide

## Pre-Flight Checklist

Before starting Phase 3 with Claude Code CLI, complete these steps:

### 1. Local Environment Setup

```bash
# Verify Icarus Verilog is installed (for simulation)
iverilog -V

# Verify Verilator is installed (for linting)
verilator --version

# Verify Gowin EDA is accessible (for synthesis)
which gw_sh || echo "Add Gowin EDA to PATH"
```

### 2. Gowin rPLL Reference ✅ COMPLETE

The Gowin rPLL IP Core has been generated and is available at:
- `rtl/pll/gowin_rpll/gowin_rpll.v` - Main module with dynamic IDIV
- `rtl/pll/gowin_rpll/gowin_rpll_tmp.v` - Instantiation template

**Existing static PLLs** (used by atomik_top.v):
- `rtl/pll/atomik_pll_81m.v` - 81 MHz fixed output
- `rtl/pll/atomik_pll_94p5m.v` - 94.5 MHz fixed output (**currently in use**)

**Documentation**: See `docs/reference/gowin/CLOCK_REFERENCE.md` for:
- PLL frequency formulas
- VCO specifications
- Instantiation templates

### 3. Verify Existing RTL Compiles

```bash
cd C:\Users\matth\OneDrive\Personal\Projects\ATOMiK

# Check existing files compile
iverilog -o test_compile rtl/atomik_core.v rtl/atomik_top.v

# Clean up
rm test_compile
```

### 4. Verify PLL Configuration

```bash
# Confirm atomik_top.v uses correct PLL
grep "atomik_pll" rtl/atomik_top.v
# Should show: atomik_pll_94p5m
```

### 5. Set API Key

```bash
# Windows PowerShell
$env:ANTHROPIC_API_KEY = "your-key-here"

# Or add to ~/.bashrc / ~/.zshrc for persistent access
export ANTHROPIC_API_KEY="your-key-here"
```

### 6. Verify Claude Code CLI

```bash
claude --version
claude --model claude-sonnet-4-5 "Hello, confirm you can access the ATOMiK project"
```

---

## Reference Documentation

Before starting, familiarize yourself with the Gowin reference docs:

| Document | Location | Contents |
|----------|----------|----------|
| Clock/PLL Reference | `docs/reference/gowin/CLOCK_REFERENCE.md` | Formulas, specs, templates |
| GPIO Reference | `docs/reference/gowin/GPIO_REFERENCE.md` | CST syntax, IO standards |
| Timing Reference | `docs/reference/gowin/TIMING_REFERENCE.md` | SDC syntax, closure tips |
| Tang Nano 9K Pinout | `docs/reference/gowin/TANG_NANO_9K_PINOUT.md` | Board pin assignments |
| PLL Module README | `rtl/pll/README.md` | Module-specific info |

---

## Execution Order

### Phase 3 Task Sequence

```
T3.1 (Architecture Spec)
  ↓
T3.2 (Delta Accumulator) ←──────┐
  ↓                             │ Can run in parallel
T3.3 (State Reconstructor) ←────┘
  ↓
T3.4 (Integration)
  ↓
T3.5 (Simulation) ←─── Iterate if tests fail
  ↓
T3.6 (Timing Constraints)
  ↓
T3.7 (Synthesis Scripts)
  ↓
T3.8 (Resource Analysis) ←─── Requires synthesis run
  ↓
T3.9 (Validation Report)
```

### Quick Commands

```bash
# Start Phase 3
cd C:\Users\matth\OneDrive\Personal\Projects\ATOMiK

# Task T3.1 - Architecture Spec
claude --model claude-sonnet-4-5 "$(cat agents/phase3/prompt_T3.1.txt)"

# Check progress
cat .github/atomik-status.yml | grep -A5 "phase_3:"

# Run simulation tests
cd sim && ./run_all_tests.sh

# Run synthesis (after T3.7)
cd synth && ./run_synthesis.sh
```

For detailed execution instructions, see: `agents/phase3/PHASE_3_EXECUTION_GUIDE.md`

---

## Human Checkpoints

Pause for human review at these points:

1. **After T3.1**: Review architecture spec before implementation
2. **After T3.5**: Verify all simulation tests pass
3. **After T3.7**: Run synthesis manually, verify it completes
4. **After T3.9**: Final approval before marking Phase 3 complete

---

## Troubleshooting

### Simulation Fails
- Check for missing `timescale directive
- Verify reset is properly applied
- Check clock generation in testbench
- Reference: `docs/reference/gowin/TIMING_REFERENCE.md`

### Synthesis Fails
- Verify all files are added to project
- Check for unsupported Verilog constructs
- Review Gowin EDA error messages
- Reference: `docs/reference/gowin/CLOCK_REFERENCE.md` for PLL issues

### Timing Closure Fails
- Reduce target frequency (try 50 MHz instead of 94.5 MHz)
- Add pipeline registers to critical path
- Review SDC constraints
- Reference: `docs/reference/gowin/TIMING_REFERENCE.md`

### PLL Won't Lock
- Verify input clock is present (27 MHz)
- Check reset polarity (rPLL RESET is **active-HIGH**)
- Verify VCO is within 400-1200 MHz range
- Reference: `docs/reference/gowin/CLOCK_REFERENCE.md`

---

## Files Created by This Setup

```
ATOMiK/
├── .github/
│   └── atomik-status.yml              # Project status tracking
├── agents/
│   └── phase3/
│       ├── PROMPTS.md                 # All task prompts
│       ├── QUICKSTART.md              # This file
│       ├── PHASE_3_EXECUTION_GUIDE.md # Detailed execution guide
│       └── prompt_T3.*.txt            # Individual task prompts
├── docs/
│   └── reference/
│       └── gowin/
│           ├── README.md              # Documentation index
│           ├── CLOCK_REFERENCE.md     # PLL reference
│           ├── GPIO_REFERENCE.md      # IO reference
│           ├── TIMING_REFERENCE.md    # SDC reference
│           └── TANG_NANO_9K_PINOUT.md # Board pinout
├── rtl/
│   └── pll/
│       ├── README.md                  # PLL module docs
│       ├── atomik_pll_94p5m.v         # Active PLL (94.5 MHz)
│       ├── atomik_pll_81m.v           # Alternative (81 MHz)
│       └── gowin_rpll/                # Dynamic PLL IP
├── sim/
│   ├── README.md
│   └── vectors/                       # Test vector directory
└── synth/
    └── README.md
```

---

## Support

If issues arise during Phase 3:
1. Check `agents/phase3/PROMPTS.md` for task-specific guidance
2. Reference `docs/reference/gowin/` for Gowin-specific information
3. Use Haiku validation prompts for quick checks
4. Escalate to Opus only for complex architectural decisions
5. Reference Phase 1 proofs for mathematical guarantees
6. Reference Phase 2 benchmarks for performance targets
