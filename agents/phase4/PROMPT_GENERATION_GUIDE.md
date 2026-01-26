# Phase 4 Prompt Generation Guide

This document provides the template and instructions for generating all remaining Phase 4 task prompts.

## Prompt Template Structure

Each prompt file follows this structure:

```
You are implementing Phase 4, Task T4[X].[Y] of the ATOMiK project.

FIRST: Read these context files in order:
1. agents/phase4/PHASE4_OVERVIEW.md
2. agents/phase4/PROMPTS.md
3. [Task-specific context files]

CRITICAL CONTEXT:
[Phase-specific context and strategy]

TASK T4[X].[Y]: [Task Name]

[Task description]

## Requirements

[Detailed requirements with examples]

## Output Files (Must Create)

[List of all files to create]

## Validation Requirements

[Specific validation steps]

## Post-Task Actions (You Must Do)

1. **Update Status**
2. **Git Commit**
3. **Report Completion**

[Details for each]

## Important Notes

[Task-specific guidance]

---

Execute this task carefully and completely.
STOP after completing [unless otherwise specified].
```

## Remaining Prompts to Create

### Phase 4A (SDK Generator Core)

- [ ] `prompt_T4A.2.txt` - Generator Framework
- [ ] `prompt_T4A.3.txt` - Python SDK Generator
- [ ] `prompt_T4A.4.txt` - Rust SDK Generator
- [ ] `prompt_T4A.5.txt` - C SDK Generator
- [ ] `prompt_T4A.6.txt` - Verilog RTL Generator (CRITICAL - silicon validation)
- [ ] `prompt_T4A.7.txt` - JavaScript SDK Generator
- [ ] `prompt_T4A.8.txt` - Generator Integration Tests
- [ ] `prompt_T4A.9.txt` - SDK Documentation Suite

### Phase 4B (Reference Patterns)

- [ ] `prompt_T4B.1.txt` - Event Sourcing Pattern
- [ ] `prompt_T4B.2.txt` - Streaming Pipeline Pattern
- [ ] `prompt_T4B.3.txt` - Sensor Fusion Pattern
- [ ] `prompt_T4B.4.txt` - Pattern Documentation & Business Analysis (Opus)

### Phase 4C (Hardware Capability Proofs)

- [ ] `prompt_T4C.1.txt` - Video Compression Demo (SILICON VALIDATION)
- [ ] `prompt_T4C.2.txt` - Edge Sensor Fusion Demo (SILICON VALIDATION)
- [ ] `prompt_T4C.3.txt` - Network Packet Analysis Demo (SILICON VALIDATION)
- [ ] `prompt_T4C.4.txt` - Hardware Validation Report & Investor Package (Opus)

### Validation & Escalation

- [ ] `prompt_VALIDATION.txt` - Standard validation template (Haiku)
- [ ] `prompt_ESCALATION.txt` - Architectural escalation template (Opus)

## Prompt Generation Instructions

For each prompt file:

1. **Copy template structure above**
2. **Fill in task-specific details from `PROMPTS.md`**
3. **Specify context files to read** (see PROMPTS.md for each task)
4. **List all output files** (from PROMPTS.md deliverables section)
5. **Define validation requirements** (from PROMPTS.md validation section)
6. **Include post-task actions** (status update, git commit, report)
7. **Add task-specific notes** (strategic alignment, simplicity reminders)

### Special Considerations

**For Verilog Tasks (T4A.6, T4C.1-3):**
- Emphasize synthesis validation (lint, synthesis, timing)
- Reference Phase 3 proven modules
- Specify bitstream generation (T4A.6) or silicon validation (T4C.1-3)
- Include "MUST RUN ON SILICON" for Phase 4C demos

**For Hardware Demo Tasks (T4C.1-3):**
- List specific hardware components needed
- Include day-by-day breakdown (4 days per demo)
- Specify measurement requirements (compression ratio, bandwidth, power, latency)
- Require photos, videos, graphs from physical setup
- Include critical validation: "Bitstream programs successfully", "Measured on hardware"

**For Business/Strategic Tasks (T4B.4, T4C.4):**
- Recommend Claude Opus 4.5 model
- Reference investor demo requirements
- Include YC application format requirements
- Emphasize data-backed claims

**For Validation Prompts:**
- Use Haiku 4.5 model (cost-efficient)
- Reference task-specific validation requirements
- Include automated test commands
- Provide clear pass/fail criteria

## Automated Prompt Generation

To generate all remaining prompts programmatically:

```python
# This script can be used to generate prompts from PROMPTS.md
# Location: agents/phase4/generate_prompts.py

import os
from pathlib import Path

PROMPT_DIR = Path("agents/phase4")
PROMPTS_MD = PROMPT_DIR / "PROMPTS.md"

# Parse PROMPTS.md and extract task specifications
# Generate prompt files following template
# Ensure all context files, deliverables, and validations are included
```

## Human Review Checklist

Before using generated prompts, verify:

- [ ] All context files are correctly specified
- [ ] Output files list is complete
- [ ] Validation requirements are specific and actionable
- [ ] Post-task actions include status update and git commit
- [ ] Strategic notes maintain alignment with Phase 4 goals
- [ ] Hardware demo prompts emphasize SILICON VALIDATION
- [ ] Cost-efficient model selection (Sonnet/Haiku/Opus)

## Next Steps

1. Generate remaining prompt files using this guide
2. Review each prompt for completeness
3. Validate prompt quality with T4A.1 as reference
4. Create validation and escalation prompts
5. Test first prompt (T4A.1) execution
6. Iterate based on results

---

*Generated January 2026*
