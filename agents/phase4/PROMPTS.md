# Phase 4 Agent Prompts

This document contains all task prompts for Phase 4 execution. Each prompt is designed for Claude Code CLI with specific model requirements and validation gates.

**Usage:**
```bash
# For implementation tasks
claude --model claude-sonnet-4-5 < agents/phase4/prompt_T4A.1.txt

# For validation tasks
claude --model claude-haiku-4-5 < agents/phase4/prompt_VALIDATION.txt

# For strategic escalation (rare)
claude --model claude-opus-4-5 < agents/phase4/prompt_ESCALATION.txt
```

---

## Phase 4A: SDK Generator Core

### T4A.1: JSON Schema Specification
**Model:** Claude Sonnet 4.5  
**Duration:** 2 days  
**Budget:** ~$8

**Prompt File:** `prompt_T4A.1.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `docs/ATOMiK_Development_Roadmap.md`
- `specs/rtl_architecture.md`
- `reports/PHASE_3_COMPLETION_REPORT.md`

**Task:**
Create JSON schema specification for ATOMiK SDK generation with:
1. Catalogue section (vertical, field, object, metadata)
2. Schema section (delta_fields, operations, constraints)
3. Hardware mapping (optional target device, RTL parameters)
4. Validation rules (required fields, types, ranges)
5. Three reference schemas (terminal-io, p2p-delta, matrix-ops)

**Output Files:**
- `specs/atomik_schema_v1.json`
- `specs/schema_validation_rules.md`
- `sdk/schemas/examples/terminal-io.json`
- `sdk/schemas/examples/p2p-delta.json`
- `sdk/schemas/examples/matrix-ops.json`
- `docs/SDK_SCHEMA_GUIDE.md` (technical reference)
- `docs/user/SDK_USER_MANUAL.md` (user guide - initial draft)

**Validation:**
- Schema is valid JSON Schema Draft 7
- All three examples validate against specification
- Documentation explains catalogue → namespace mapping

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.1_schema_spec: complete`
2. Commit with message: `[T4A.1] JSON schema specification complete`
3. **STOP - Human checkpoint required before T4A.2**

---

### T4A.2: Generator Framework
**Model:** Claude Sonnet 4.5  
**Duration:** 2 days  
**Budget:** ~$8

**Prompt File:** `prompt_T4A.2.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `specs/schema_validation_rules.md`
- `software/atomik_sdk/genome_compiler.py` (existing reference)

**Task:**
Create code generator framework that:
1. Parses JSON schema
2. Validates against specification
3. Extracts catalogue metadata (for namespace generation)
4. Extracts schema definitions (for code generation)
5. Provides plugin architecture for language targets
6. Handles error reporting and validation failures

**Output Files:**
- `software/atomik_sdk/generator/core.py` (main engine)
- `software/atomik_sdk/generator/schema_validator.py`
- `software/atomik_sdk/generator/code_emitter.py`
- `software/atomik_sdk/generator/namespace_mapper.py`
- `software/atomik_sdk/generator/__init__.py`
- `software/atomik_sdk/generator/README.md`
- `software/atomik_sdk/tests/test_generator_core.py`

**Validation:**
- Framework loads and validates all three reference schemas
- Error handling reports useful messages
- Unit tests pass

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.2_generator_framework: complete`
2. Commit with message: `[T4A.2] Code generator framework complete`

---

### T4A.3: Python SDK Generator
**Model:** Claude Sonnet 4.5  
**Duration:** 2 days  
**Budget:** ~$8

**Prompt File:** `prompt_T4A.3.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `software/atomik_sdk/generator/core.py`
- `sdk/schemas/examples/*.json`

**Task:**
Implement Python code generation that produces:
1. Module structure from catalogue metadata
2. Delta operation classes (Accumulator, Reconstructor)
3. Type hints and docstrings
4. Unit test templates
5. `setup.py` for packaging

**Generated Code Structure:**
```python
# From sdk/schemas/examples/terminal-io.json
# Generates: atomik/terminal/io.py

from atomik.core import DeltaAccumulator, StateReconstructor

class TerminalIO:
    def __init__(self, ...):
        self.accumulator = DeltaAccumulator(width=64)
        self.reconstructor = StateReconstructor(width=64)
    
    def process_delta(self, delta):
        return self.accumulator.accumulate(delta)
    
    def get_state(self):
        return self.reconstructor.reconstruct(self.accumulator.state)
```

**Output Files:**
- `software/atomik_sdk/templates/python_template.py`
- `software/atomik_sdk/generator/python_generator.py`
- `software/atomik_sdk/tests/test_python_generator.py`
- Generated test outputs in `software/atomik_sdk/generated/python/` (from examples)

**Validation:**
- Generated Python code has no syntax errors
- Generated code passes `python -m py_compile`
- Generated code imports successfully
- Basic operations execute correctly

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.3_python_generator: complete`
2. Update `docs/user/language_guides/python_guide.md`
3. Commit with message: `[T4A.3] Python SDK generator complete`

---

### T4A.4: Rust SDK Generator
**Model:** Claude Sonnet 4.5  
**Duration:** 2 days  
**Budget:** ~$8

**Prompt File:** `prompt_T4A.4.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `software/atomik_sdk/generator/core.py`

**Task:**
Implement Rust code generation that produces:
1. Crate structure from catalogue metadata
2. Delta operation structs and traits
3. Proper ownership and borrowing semantics
4. Integration tests
5. `Cargo.toml` for packaging

**Generated Code Structure:**
```rust
// From sdk/schemas/examples/terminal-io.json
// Generates: atomik-terminal-io crate

use atomik_core::{DeltaAccumulator, StateReconstructor};

pub struct TerminalIO {
    accumulator: DeltaAccumulator<64>,
    reconstructor: StateReconstructor<64>,
}

impl TerminalIO {
    pub fn new() -> Self { ... }
    pub fn process_delta(&mut self, delta: u64) -> u64 { ... }
    pub fn get_state(&self) -> u64 { ... }
}
```

**Output Files:**
- `software/atomik_sdk/templates/rust_template.rs`
- `software/atomik_sdk/generator/rust_generator.py`
- `software/atomik_sdk/tests/test_rust_generator.py`
- Generated test outputs in `software/atomik_sdk/generated/rust/`

**Validation:**
- Generated Rust code passes `cargo check`
- Generated code passes `cargo build`
- Generated tests pass `cargo test`
- No compiler warnings

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.4_rust_generator: complete`
2. Update `docs/user/language_guides/rust_guide.md`
3. Commit with message: `[T4A.4] Rust SDK generator complete`

---

### T4A.5: C SDK Generator
**Model:** Claude Sonnet 4.5  
**Duration:** 2 days  
**Budget:** ~$8

**Prompt File:** `prompt_T4A.5.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `software/atomik_sdk/generator/core.py`

**Task:**
Implement C code generation that produces:
1. Header/implementation file pairs
2. Delta operation functions
3. Proper memory management (no leaks)
4. Thread-safe operations (if concurrent)
5. `Makefile` for building

**Generated Code Structure:**
```c
// From sdk/schemas/examples/terminal-io.json
// Generates: atomik_terminal_io.h and atomik_terminal_io.c

typedef struct {
    uint64_t accumulator;
    uint64_t state;
} atomik_terminal_io_t;

void atomik_terminal_io_init(atomik_terminal_io_t* io);
uint64_t atomik_terminal_io_process_delta(atomik_terminal_io_t* io, uint64_t delta);
uint64_t atomik_terminal_io_get_state(const atomik_terminal_io_t* io);
```

**Output Files:**
- `software/atomik_sdk/templates/c_template.c`
- `software/atomik_sdk/templates/c_template.h`
- `software/atomik_sdk/generator/c_generator.py`
- `software/atomik_sdk/tests/test_c_generator.py`
- Generated test outputs in `software/atomik_sdk/generated/c/`

**Validation:**
- Generated C code compiles with `gcc -Wall -Werror`
- No memory leaks (valgrind if available)
- Generated code runs correctly
- Test programs execute successfully

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.5_c_generator: complete`
2. Update `docs/user/language_guides/c_guide.md`
3. Commit with message: `[T4A.5] C SDK generator complete`

---

### T4A.6: Verilog RTL Generator
**Model:** Claude Sonnet 4.5  
**Duration:** 2 days  
**Budget:** ~$8

**Prompt File:** `prompt_T4A.6.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `specs/rtl_architecture.md`
- `rtl/atomik_core_v2.v` (Phase 3 reference)
- `software/atomik_sdk/generator/core.py`

**Task:**
Implement Verilog code generation that produces:
1. RTL modules from catalogue metadata
2. Delta accumulator logic
3. State reconstructor logic
4. Proper clock/reset handling
5. Synthesis-ready code
6. Testbench for simulation

**Generated Code Structure:**
```verilog
// From sdk/schemas/examples/terminal-io.json
// Generates: atomik_terminal_io.v

module atomik_terminal_io #(
    parameter DATA_WIDTH = 64
)(
    input wire clk,
    input wire rst_n,
    input wire [DATA_WIDTH-1:0] delta_in,
    input wire delta_valid,
    output wire [DATA_WIDTH-1:0] state_out,
    output wire state_valid
);

// Delta accumulator
reg [DATA_WIDTH-1:0] accumulator;
always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        accumulator <= 0;
    else if (delta_valid)
        accumulator <= accumulator ^ delta_in;
end

// State output
assign state_out = accumulator;
assign state_valid = 1'b1;

endmodule
```

**Output Files:**
- `software/atomik_sdk/templates/verilog_template.v`
- `software/atomik_sdk/generator/verilog_generator.py`
- `software/atomik_sdk/tests/test_verilog_generator.py`
- Generated test outputs in `software/atomik_sdk/generated/verilog/`

**Validation (CRITICAL):**
- Generated Verilog passes `verilator --lint-only -Wall`
- Generated code synthesizes with Gowin EDA
- Simulation testbench passes (Icarus Verilog or ModelSim)
- Resource utilization reasonable (<10% LUTs for primitives)
- Timing analysis passes
- **Bitstream generation succeeds** (proves synthesis complete)

**Note:** Bitstream generated but NOT programmed to FPGA yet (Phase 4C)

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.6_verilog_generator: complete`
2. Update `docs/user/language_guides/verilog_guide.md`
3. Update `docs/technical/HARDWARE_INTEGRATION.md`
4. Commit with message: `[T4A.6] Verilog RTL generator complete`

---

### T4A.7: JavaScript SDK Generator
**Model:** Claude Sonnet 4.5  
**Duration:** 2 days  
**Budget:** ~$8

**Prompt File:** `prompt_T4A.7.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `software/atomik_sdk/generator/core.py`

**Task:**
Implement JavaScript/TypeScript code generation that produces:
1. NPM package structure from catalogue
2. Delta operation classes
3. TypeScript type definitions
4. Jest test templates
5. `package.json` for publishing

**Generated Code Structure:**
```javascript
// From sdk/schemas/examples/terminal-io.json
// Generates: @atomik/terminal-io package

class TerminalIO {
    constructor() {
        this.accumulator = 0n;
    }

    processDelta(delta) {
        this.accumulator ^= BigInt(delta);
        return this.accumulator;
    }

    getState() {
        return this.accumulator;
    }
}

module.exports = { TerminalIO };
```

**Output Files:**
- `software/atomik_sdk/templates/javascript_template.js`
- `software/atomik_sdk/templates/typescript_template.ts`
- `software/atomik_sdk/generator/javascript_generator.py`
- `software/atomik_sdk/tests/test_javascript_generator.py`
- Generated test outputs in `software/atomik_sdk/generated/javascript/`

**Validation:**
- Generated JavaScript executes in Node.js
- Generated code passes ESLint
- TypeScript definitions compile
- Jest tests pass

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.7_javascript_generator: complete`
2. Update `docs/user/language_guides/javascript_guide.md`
3. Commit with message: `[T4A.7] JavaScript SDK generator complete`

---

### T4A.8: Generator Integration Tests
**Model:** Claude Sonnet 4.5  
**Duration:** 1 day  
**Budget:** ~$5

**Prompt File:** `prompt_T4A.8.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `software/atomik_sdk/generator/`
- All language generator modules

**Task:**
Create comprehensive integration tests that:
1. Generate code from all reference schemas in all languages
2. Verify compilation/execution for each
3. Test cross-language consistency
4. Validate generated code behavior matches specification
5. Performance benchmarks (optional)

**Output Files:**
- `software/atomik_sdk/tests/test_generator_integration.py`
- `software/atomik_sdk/tests/integration_test_runner.sh`
- `software/atomik_sdk/tests/INTEGRATION_TEST_REPORT.md`

**Validation:**
- All reference schemas generate valid code in all 5 languages
- Generated code from same schema behaves identically
- All language-specific tests pass
- Integration test suite executes successfully

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.8_integration_tests: complete`
2. Commit with message: `[T4A.8] Generator integration tests complete`

---

### T4A.9: SDK Documentation Suite
**Model:** Claude Sonnet 4.5  
**Duration:** 2 days  
**Budget:** ~$10

**Prompt File:** `prompt_T4A.9.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `software/atomik_sdk/generator/`
- All generated example code

**Task:**
Create comprehensive SDK documentation including:

**Technical Documentation:**
1. `docs/technical/SDK_ARCHITECTURE.md` - Generator design
2. `docs/technical/CODE_GENERATION_SPEC.md` - Template system
3. `docs/technical/PRIMITIVE_OPERATIONS.md` - Core operations reference

**User Documentation:**
1. `docs/user/QUICKSTART_GUIDE.md` - 10-minute onboarding
2. `docs/user/SDK_USER_MANUAL.md` - Complete guide
3. `docs/user/PATTERN_LIBRARY.md` - Pattern catalog (intro)
4. `docs/user/BUILDING_ON_ATOMIK.md` - Domain mapping guide
5. Per-language guides (update existing stubs):
   - `docs/user/language_guides/python_guide.md`
   - `docs/user/language_guides/rust_guide.md`
   - `docs/user/language_guides/c_guide.md`
   - `docs/user/language_guides/verilog_guide.md`
   - `docs/user/language_guides/javascript_guide.md`

**Business Documentation (initial):**
1. `business/analysis/technical_capabilities.md` - Feature matrix
2. `business/analysis/competitive_analysis.md` - ATOMiK vs alternatives

**Validation:**
- Documentation is clear and comprehensive
- Quickstart guide actually takes <10 minutes
- Examples in documentation execute correctly
- All cross-references are valid

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4A.9_sdk_documentation: complete`
2. Update `README.md` with SDK status
3. Commit with message: `[T4A.9] SDK documentation suite complete`
4. **STOP - Human checkpoint required before Phase 4B**

---

## Phase 4B: Reference Patterns

### T4B.1: Event Sourcing Pattern
**Model:** Claude Sonnet 4.5  
**Duration:** 1.5 days  
**Budget:** ~$7

**Prompt File:** `prompt_T4B.1.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `docs/user/BUILDING_ON_ATOMIK.md`
- `software/atomik_sdk/generator/`

**Task:**
Create minimal event sourcing pattern demonstration (<200 LOC):
1. JSON schema: `sdk/patterns/event_sourcing/event_pattern.json`
2. Python implementation showing:
   - Event log as delta stream
   - State reconstruction from events
   - Temporal rollback capability
   - CRDT-style merging
3. Documentation explaining pattern

**Pattern Focus:**
- Show log-based systems use case
- Demonstrate distributed consensus capability
- Illustrate temporal rollback
- **NOT a product** - just pattern demonstration

**Output Files:**
- `sdk/patterns/event_sourcing/event_pattern.json`
- `sdk/patterns/event_sourcing/event_example.py` (<200 LOC)
- `sdk/patterns/event_sourcing/README.md`
- `sdk/patterns/event_sourcing/test_event_pattern.py`

**Validation:**
- Pattern code <200 LOC
- Demonstrates capability without being a product
- Documentation explains "how to build event sourcing on ATOMiK"
- Tests pass

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4B.1_event_sourcing_pattern: complete`
2. Update `docs/user/PATTERN_LIBRARY.md` with entry
3. Commit with message: `[T4B.1] Event sourcing pattern complete`

---

### T4B.2: Streaming Pipeline Pattern
**Model:** Claude Sonnet 4.5  
**Duration:** 1.5 days  
**Budget:** ~$7

**Prompt File:** `prompt_T4B.2.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `docs/user/BUILDING_ON_ATOMIK.md`

**Task:**
Create minimal streaming pipeline pattern demonstration (<200 LOC):
1. JSON schema: `sdk/patterns/streaming_pipeline/stream_pattern.json`
2. Python implementation showing:
   - Frame-to-frame delta encoding
   - Low-latency processing
   - Compression characteristics
   - Pipeline composition
3. Documentation explaining pattern

**Pattern Focus:**
- Show video/audio use case
- Demonstrate compression (240:1 potential)
- Illustrate real-time processing
- **NOT a codec** - just pattern demonstration

**Output Files:**
- `sdk/patterns/streaming_pipeline/stream_pattern.json`
- `sdk/patterns/streaming_pipeline/stream_example.py` (<200 LOC)
- `sdk/patterns/streaming_pipeline/README.md`
- `sdk/patterns/streaming_pipeline/test_stream_pattern.py`

**Validation:**
- Pattern code <200 LOC
- Demonstrates streaming capability
- Documentation explains "how to build pipelines on ATOMiK"
- Tests pass

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4B.2_streaming_pipeline_pattern: complete`
2. Update `docs/user/PATTERN_LIBRARY.md` with entry
3. Commit with message: `[T4B.2] Streaming pipeline pattern complete`

---

### T4B.3: Sensor Fusion Pattern
**Model:** Claude Sonnet 4.5  
**Duration:** 1.5 days  
**Budget:** ~$7

**Prompt File:** `prompt_T4B.3.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `docs/user/BUILDING_ON_ATOMIK.md`

**Task:**
Create minimal sensor fusion pattern demonstration (<200 LOC):
1. JSON schema: `sdk/patterns/sensor_fusion/sensor_pattern.json`
2. Python implementation showing:
   - Multi-sensor delta integration
   - Bandwidth reduction (1000:1 potential)
   - Power-constrained operation
   - Edge deployment characteristics
3. Documentation explaining pattern

**Pattern Focus:**
- Show edge/IoT use case
- Demonstrate bandwidth reduction
- Illustrate low-power operation
- **NOT an IoT platform** - just pattern demonstration

**Output Files:**
- `sdk/patterns/sensor_fusion/sensor_pattern.json`
- `sdk/patterns/sensor_fusion/sensor_example.py` (<200 LOC)
- `sdk/patterns/sensor_fusion/README.md`
- `sdk/patterns/sensor_fusion/test_sensor_pattern.py`

**Validation:**
- Pattern code <200 LOC
- Demonstrates sensor fusion capability
- Documentation explains "how to build edge apps on ATOMiK"
- Tests pass

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4B.3_sensor_fusion_pattern: complete`
2. Update `docs/user/PATTERN_LIBRARY.md` with entry
3. Commit with message: `[T4B.3] Sensor fusion pattern complete`

---

### T4B.4: Pattern Documentation & Business Analysis
**Model:** Claude Sonnet 4.5 + Claude Opus 4.5 (business strategy)  
**Duration:** 2 days  
**Budget:** ~$15

**Prompt File:** `prompt_T4B.4.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- All pattern implementations from T4B.1-3
- `docs/user/BUILDING_ON_ATOMIK.md`
- `business/analysis/technical_capabilities.md`

**Task:**
Complete pattern documentation and business analysis:

**Pattern Documentation:**
1. Finalize `docs/user/PATTERN_LIBRARY.md` - Complete catalog
2. Update `docs/technical/PATTERN_ARCHITECTURE.md` - Design principles

**Business Analysis (Use Opus 4.5):**
1. `business/analysis/use_case_matrix.md` - Markets × capabilities
2. `business/analysis/customer_personas.md` - Target developers
3. `business/analysis/market_sizing.md` - TAM/SAM/SOM estimates

**Validation:**
- Pattern library is comprehensive and clear
- Business analysis identifies actionable targets
- Market sizing is data-driven and defensible

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4B.4_pattern_documentation: complete`
2. Commit with message: `[T4B.4] Pattern documentation and business analysis complete`
3. **STOP - Human checkpoint required before Phase 4C**

---

## Phase 4C: Hardware Capability Proofs

### T4C.1: Video Compression Demo
**Model:** Claude Sonnet 4.5 (Opus 4.5 escalation if needed)  
**Duration:** 4 days  
**Budget:** ~$12 + hardware

**Prompt File:** `prompt_T4C.1.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `sdk/patterns/streaming_pipeline/stream_pattern.json`
- `software/atomik_sdk/generator/verilog_generator.py`
- `rtl/atomik_core_v2.v` (Phase 3 reference)
- `synth/gowin_synth.tcl` (synthesis script)

**Hardware Required:**
- Tang Nano 9K (Board 1)
- Camera module (OV2640 or similar, ~$15)
- HDMI output (built-in)
- Power measurement module (INA219, ~$5)

**Task:**
Create complete video compression demonstration on physical hardware:

**Day 1-2: Schema and Verilog Implementation**
1. Create `demos/video_compression/video_compression.json` schema
2. Generate Verilog using SDK generator
3. Integrate with camera interface and HDMI output
4. Synthesize and generate bitstream

**Day 3: Hardware Validation**
1. Program bitstream to Tang Nano 9K Board 1
2. Connect camera module and HDMI display
3. Verify live video processing
4. Collect benchmark measurements:
   - Compression ratio (target: 240:1 for steady scenes)
   - Frame rate (target: 30 FPS)
   - Power consumption (measure with INA219)
   - Latency (if scope available)

**Day 4: Documentation and Analysis**
1. Take photos of physical setup
2. Record video of live demo
3. Generate benchmark graphs
4. Write comprehensive results

**Output Files:**
- `demos/video_compression/video_compression.json`
- `demos/video_compression/video_delta_codec.v` (generated)
- `demos/video_compression/video_demo.py` (control script)
- `demos/video_compression/RESULTS.md` (benchmark data + graphs)
- `demos/video_compression/SETUP.md` (hardware guide)
- `demos/video_compression/photos/` (physical setup images)
- `demos/video_compression/video/` (demo recording)

**Validation (CRITICAL - MUST RUN ON SILICON):**
- [ ] Bitstream programs successfully to Tang Nano 9K
- [ ] Live camera feed displays on HDMI
- [ ] Delta compression visibly working
- [ ] Compression ratio measured ≥240:1 for steady scenes
- [ ] Frame rate ≥30 FPS
- [ ] Power consumption measured and documented
- [ ] Photos and video captured for documentation

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4C.1_video_demo: complete`
2. Update `business/validation/hardware_results_summary.md`
3. Commit with message: `[T4C.1] Video compression demo validated on silicon`
4. **STOP - Human checkpoint: Verify physical demo working**

---

### T4C.2: Edge Sensor Fusion Demo
**Model:** Claude Sonnet 4.5  
**Duration:** 4 days  
**Budget:** ~$12 + hardware

**Prompt File:** `prompt_T4C.2.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `sdk/patterns/sensor_fusion/sensor_pattern.json`
- `software/atomik_sdk/generator/verilog_generator.py`

**Hardware Required:**
- Tang Nano 9K (Board 2)
- IMU sensor (MPU6050, ~$3)
- Temperature sensor (DS18B20, ~$2)
- LoRa module (optional, ~$10) or UART
- Power measurement module (INA219, ~$5)

**Task:**
Create complete edge sensor fusion demonstration on physical hardware:

**Day 1-2: Schema and Verilog Implementation**
1. Create `demos/edge_sensor_fusion/sensor_fusion.json` schema
2. Generate Verilog using SDK generator
3. Integrate with IMU and temperature sensor interfaces
4. Synthesize and generate bitstream

**Day 3: Hardware Validation**
1. Program bitstream to Tang Nano 9K Board 2
2. Connect IMU and temperature sensors
3. Verify multi-sensor data fusion
4. Collect benchmark measurements:
   - Bandwidth reduction (target: 1000:1 for steady state)
   - Update latency (target: <1ms)
   - Power consumption
   - Memory usage (verify <2MB)

**Day 4: Documentation and Analysis**
1. Take photos of physical setup
2. Create live monitoring dashboard
3. Generate benchmark graphs
4. Write comprehensive results

**Output Files:**
- `demos/edge_sensor_fusion/sensor_fusion.json`
- `demos/edge_sensor_fusion/sensor_fusion.v` (generated)
- `demos/edge_sensor_fusion/sensor_monitor.py` (control/monitoring)
- `demos/edge_sensor_fusion/RESULTS.md` (benchmark data + graphs)
- `demos/edge_sensor_fusion/SETUP.md` (hardware guide)
- `demos/edge_sensor_fusion/photos/` (physical setup images)

**Validation (CRITICAL - MUST RUN ON SILICON):**
- [ ] Bitstream programs successfully to Tang Nano 9K
- [ ] Sensors connected and reading data
- [ ] Delta fusion processing working
- [ ] Bandwidth reduction measured ≥1000:1 for steady state
- [ ] Update latency <1ms
- [ ] Power consumption measured and documented
- [ ] Photos captured for documentation

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4C.2_sensor_demo: complete`
2. Update `business/validation/hardware_results_summary.md`
3. Commit with message: `[T4C.2] Edge sensor fusion demo validated on silicon`
4. **STOP - Human checkpoint: Verify physical demo working**

---

### T4C.3: Network Packet Analysis Demo
**Model:** Claude Sonnet 4.5  
**Duration:** 4 days  
**Budget:** ~$12 + hardware

**Prompt File:** `prompt_T4C.3.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `specs/atomik_schema_v1.json`
- `software/atomik_sdk/generator/verilog_generator.py`

**Hardware Required:**
- Tang Nano 9K (Board 3)
- Ethernet PHY (optional, ~$8) or UART packet injection
- Power measurement module (INA219, ~$5)

**Task:**
Create complete network packet analysis demonstration on physical hardware:

**Day 1-2: Schema and Verilog Implementation**
1. Create `demos/network_packet_analysis/packet_analysis.json` schema
2. Generate Verilog using SDK generator
3. Integrate with packet input interface (Ethernet or UART)
4. Implement simple pattern detection/anomaly detection
5. Synthesize and generate bitstream

**Day 3: Hardware Validation**
1. Program bitstream to Tang Nano 9K Board 3
2. Connect packet source (Ethernet PHY or PC via UART)
3. Generate packet streams for testing
4. Collect benchmark measurements:
   - Throughput (packets/second)
   - Detection latency
   - Resource utilization (<5% LUT target for packet logic)
   - Power vs traditional state-based approach

**Day 4: Documentation and Analysis**
1. Take photos of physical setup
2. Create real-time analysis visualization
3. Generate benchmark graphs
4. Write comprehensive results

**Output Files:**
- `demos/network_packet_analysis/packet_analysis.json`
- `demos/network_packet_analysis/packet_analyzer.v` (generated)
- `demos/network_packet_analysis/packet_generator.py` (test traffic)
- `demos/network_packet_analysis/RESULTS.md` (benchmark data + graphs)
- `demos/network_packet_analysis/SETUP.md` (hardware guide)
- `demos/network_packet_analysis/photos/` (physical setup images)

**Validation (CRITICAL - MUST RUN ON SILICON):**
- [ ] Bitstream programs successfully to Tang Nano 9K
- [ ] Packet stream processing working
- [ ] Pattern detection functioning correctly
- [ ] Throughput measured and documented
- [ ] Detection latency measured
- [ ] Resource utilization <5% LUTs
- [ ] Photos captured for documentation

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4C.3_network_demo: complete`
2. Update `business/validation/hardware_results_summary.md`
3. Commit with message: `[T4C.3] Network packet analysis demo validated on silicon`
4. **STOP - Human checkpoint: Verify physical demo working**

---

### T4C.4: Hardware Validation Report & Business Materials
**Model:** Claude Opus 4.5 (strategic synthesis)  
**Duration:** 2 days  
**Budget:** ~$15

**Prompt File:** `prompt_T4C.4.txt`

**Context Files to Read:**
- `agents/phase4/PHASE4_OVERVIEW.md`
- `demos/*/RESULTS.md` (all three demo results)
- `reports/PHASE_3_COMPLETION_REPORT.md`
- `business/analysis/*.md` (all business analysis)

**Task:**
Create comprehensive hardware validation report and investor materials:

**Hardware Validation Report:**
1. `reports/HARDWARE_DEMOS_REPORT.md`:
   - Executive summary of all three demos
   - Comprehensive benchmark data with graphs
   - Comparison to theoretical predictions
   - Comparison to traditional approaches
   - Photos and videos of physical setups
   - Methodology transparency

**Business Validation Materials:**
1. `business/validation/technology_validation.md`:
   - Proof points for investors
   - Claims backed by measured data
   - Academic foundation summary (Phase 1-3)

2. `business/validation/competitive_benchmarks.md`:
   - ATOMiK vs state-of-the-art
   - Performance comparisons
   - Efficiency advantages

3. `business/validation/hardware_results_summary.md`:
   - One-page summary of key metrics
   - Investor-facing format

**Investor Demo Package:**
1. `business/pitch/EXECUTIVE_SUMMARY.md` (2 pages)
   - Problem statement
   - Solution (delta-state computing)
   - Proven results (Phase 1-4 summary)
   - Market opportunity
   - Ask & use of funds

2. `business/pitch/PITCH_DECK.pptx` (10-15 slides)
   - Problem
   - Solution
   - Technology validation
   - Market opportunity
   - Team & roadmap
   - Ask

3. `business/pitch/DEMO_SCRIPT.md`
   - 10-minute demo walkthrough
   - Key talking points
   - Benchmark highlights

4. `business/applications/YC_APPLICATION.md`
   - Draft YC application
   - Tailored to YC format
   - All sections complete

5. `business/applications/INVESTOR_FAQ.md`
   - Common investor questions
   - Prepared answers with data

**Output Files:**
- `reports/HARDWARE_DEMOS_REPORT.md`
- `business/validation/` (3 documents)
- `business/pitch/` (3+ documents)
- `business/applications/` (2 documents)

**Validation:**
- All claims backed by measured data from Phase 1-4
- Investor materials compelling and complete
- YC application follows standard format
- FAQ addresses likely questions
- Pitch deck tells clear story

**Post-Task Actions:**
1. Update `.github/atomik-status.yml`: `T4C.4_validation_report: complete`
2. Update `README.md` with Phase 4 complete status
3. Update `docs/ATOMiK_Development_Roadmap.md`:
   - Mark Phase 4 complete
   - Add Phase 4 metrics
   - Update Phase 5 status to "Ready"
4. Commit with message: `[T4C.4] Hardware validation report and investor package complete`
5. **STOP - Final human checkpoint: Review investor package**

---

## Validation Prompt Templates

### Standard Validation (After Each Task)
**Model:** Claude Haiku 4.5  
**File:** `prompt_VALIDATION.txt`

```
You are validating the completion of Phase 4, Task [TASK_ID].

CONTEXT FILES TO READ:
- agents/phase4/PROMPTS.md (this file)
- [Task-specific output files]

VALIDATION CHECKLIST:
[Task-specific validation requirements from above]

VALIDATION TASKS:
1. Verify all required output files exist
2. Check file content completeness
3. Run automated tests (if applicable):
   - For Python: python -m pytest
   - For Rust: cargo test
   - For C: make test
   - For Verilog: verilator --lint-only
   - For JavaScript: npm test
4. Check for compilation warnings/errors
5. Verify documentation completeness
6. Confirm status update in .github/atomik-status.yml

REPORT:
- [ ] All required files present
- [ ] Content meets requirements
- [ ] Tests pass (if applicable)
- [ ] No warnings/errors
- [ ] Documentation complete
- [ ] Status updated

If any issues found, provide specific recommendations for fixes.
If all checks pass, confirm task completion.
```

---

### Escalation Prompt (For Architectural Issues)
**Model:** Claude Opus 4.5  
**File:** `prompt_ESCALATION.txt`

```
You are addressing an architectural issue in Phase 4, Task [TASK_ID].

CONTEXT FILES TO READ:
- agents/phase4/PHASE4_OVERVIEW.md
- agents/phase4/PROMPTS.md
- [Relevant Phase 1-3 reports]
- [Current task files]

ISSUE DESCRIPTION:
[Human describes specific problem]

CONSTRAINTS FROM PREVIOUS PHASES:
- Mathematical foundation: XOR forms Abelian group (commutativity, associativity, identity)
- Hardware validation: 2.7% LUT, 94.5 MHz timing closure, single-cycle operations
- Phase 4 goals:
  * SDK generator must produce synthesis-ready Verilog
  * Maintain simplicity (primitives not products)
  * Hierarchical catalogue → namespace mapping
  * Support community contribution

REQUIREMENTS:
1. Propose solution that maintains algebraic properties
2. Ensure Verilog remains synthesizable on Gowin GW1NR-9
3. Keep generator architecture simple and extensible
4. Enable community contributions without central bottleneck

DELIVERABLE:
- Architectural recommendation
- Implementation guidance
- Trade-offs analysis
- Risk assessment
```

---

## Cost Tracking

### Estimated Costs by Phase

**Phase 4A (SDK Generator):**
- T4A.1-9: 9 tasks × $8 avg = $72
- Validations: ~$5 (Haiku)
- **Subtotal: ~$77**

**Phase 4B (Patterns):**
- T4B.1-3: 3 tasks × $7 avg = $21
- T4B.4: $15 (includes Opus for business)
- Validations: ~$3 (Haiku)
- **Subtotal: ~$39**

**Phase 4C (Hardware Demos):**
- T4C.1-3: 3 tasks × $12 avg = $36
- T4C.4: $15 (Opus for synthesis)
- Validations: ~$4 (Haiku)
- Hardware components: ~$65
- **Subtotal: ~$120**

**Escalations (if needed):**
- Opus calls: ~$15 budget

**TOTAL PHASE 4: ~$251** (slightly over $240 budget, but within variance)

---

## Task Execution Checklist

Before starting each task, verify:
- [ ] Previous task completed and validated
- [ ] Human checkpoint passed (if required)
- [ ] All context files are current
- [ ] Git working directory is clean
- [ ] Status manifest reflects current state

After completing each task, verify:
- [ ] All output files created
- [ ] Validation passed
- [ ] Documentation updated
- [ ] Status manifest updated
- [ ] Git commit created with proper message
- [ ] Human notified if checkpoint required

---

*Phase 4 Prompts Master Document*
*Generated January 2026*
*Version 1.0*
