# ATOMiK Phase 4: SDK Generator & Hardware Demonstrators

## Strategic Vision Alignment

### Business Objective
Create a **standardized template-driven SDK generation pipeline** that:
1. Transforms JSON schema → Verilog/Hardware configurations
2. Enables language-agnostic, hardware-agnostic deployment
3. Achieves organic industry adoption through simplicity
4. Establishes infrastructure dependency through critical primitive SDKs

### Core Philosophy: "Different Packages, Same Ingredients"
The algebraic properties (commutativity, associativity, self-inverse) mean **every ATOMiK application is fundamentally the same computation with different wrappers**. This enables:
- Unified code generation from declarative specs
- Predictable performance characteristics
- Automatic optimization across targets

---

## Revised Phase 4 Structure

### Phase 4A: SDK Generator Framework (Week 1-2)
**Goal**: Build the template-driven generation pipeline

### Phase 4B: Domain SDKs (Week 3-4)
**Goal**: Create 3 demonstrator SDKs targeting critical computational needs

### Phase 4C: Hardware Demonstrators (Week 5-6)
**Goal**: Deploy 3 ATOMiK-FPGA boards proving superiority in real workloads

---

## Phase 4A: SDK Generator Framework

### 4A.1 Schema Specification: Two-Part JSON Structure

**Critical Insight**: The JSON schema has TWO parts:
1. **Catalogue** - Hierarchical taxonomy (determines namespace/imports)
2. **Schema** - Computational specification (determines behavior)

This separation enables **automatic API generation** from catalogue position:

```python
# Catalogue position: Network.PacketAnalyzer
from atomik import Network
analyzer = Network.PacketAnalyzer(host)  # Auto-generated class
```

**Complete JSON Template**:
```json
{
  "catalogue": {
    "vertical": "Network",
    "field": {
      "domain": "CompSci",
      "subdomain": "NetworkSecurity"
    },
    "object": "PacketAnalyzer",
    "version": "1.0.0",
    "contributor": {
      "org": "Acme Security",
      "contact": "dev@acme.com",
      "verified": true
    },
    "tags": ["networking", "security", "real-time"],
    "license": "MIT"
  },
  
  "schema": {
    "target": {
      "hardware": ["gowin_gw1nr9", "xilinx_artix7", "lattice_ice40"],
      "software": ["python", "rust", "javascript", "c"]
    },
    "delta_fields": {
      "packet_header": {
        "type": "sparse_delta",
        "width": 64,
        "description": "Header fields that changed"
      },
      "payload_hash": {
        "type": "delta_stream",
        "width": 256
      },
      "flow_state": {
        "type": "bitmask_delta",
        "width": 32
      }
    },
    "operations": [
      {"name": "accumulate", "latency": "single_cycle"},
      {"name": "reconstruct", "latency": "combinational"},
      {"name": "classify", "motif_based": true}
    ],
    "constraints": {
      "max_memory_kb": 64,
      "max_power_mw": 50,
      "target_latency_ns": 100
    }
  }
}
```

### 4A.2 Generation Targets

| Target | Output | Extension |
|--------|--------|-----------|
| **Verilog RTL** | Synthesizable hardware | `.v` |
| **Python SDK** | Software library | `.py` |
| **Rust SDK** | Performance library | `.rs` |
| **JavaScript SDK** | Browser/Node | `.js` / `.ts` |
| **C SDK** | Embedded systems | `.c` / `.h` |
| **Genome File** | Hardware config | `.gnm` |
| **F# SDK** | Functional paradigm | `.fs` |

### 4A.3 Generator Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    ATOMiK SDK Generator                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────┐    ┌──────────────┐    ┌────────────────────┐    │
│  │  JSON    │───▶│   Parser &   │───▶│  Intermediate      │    │
│  │  Schema  │    │   Validator  │    │  Representation    │    │
│  └──────────┘    └──────────────┘    │  (ATOMiK-IR)       │    │
│                                       └─────────┬──────────┘    │
│                                                 │               │
│                    ┌────────────────────────────┼───────────────┤
│                    │                            │               │
│                    ▼                            ▼               │
│  ┌─────────────────────────┐    ┌─────────────────────────┐   │
│  │   Hardware Generators    │    │   Software Generators   │   │
│  ├─────────────────────────┤    ├─────────────────────────┤   │
│  │ • VerilogGenerator      │    │ • PythonGenerator       │   │
│  │ • GenomeCompiler        │    │ • RustGenerator         │   │
│  │ • ConstraintGenerator   │    │ • JSGenerator           │   │
│  │ • TestbenchGenerator    │    │ • CGenerator            │   │
│  └─────────────────────────┘    │ • FSharpGenerator       │   │
│                                  └─────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Output Packager                       │   │
│  │  • Creates complete SDK directory structure              │   │
│  │  • Generates build files (Makefile, pyproject.toml, etc)│   │
│  │  • Includes tests, docs, examples                        │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 4A.4 ATOMiK Intermediate Representation (ATOMiK-IR)

```python
@dataclass
class ATOMiKIR:
    """Language and hardware agnostic representation."""
    
    name: str
    version: str
    
    # Delta field definitions
    fields: Dict[str, DeltaFieldSpec]
    
    # Core operations (always the same algebra)
    operations: List[Operation] = field(default_factory=lambda: [
        Operation("load", "Set initial state", cycles=1),
        Operation("accumulate", "XOR delta into accumulator", cycles=1),
        Operation("reconstruct", "Compute current = initial ⊕ acc", cycles=0),
        Operation("reset", "Clear accumulator", cycles=1),
    ])
    
    # Target-specific constraints
    constraints: Constraints
    
    # Derived from algebraic properties (not user-specified)
    guarantees: List[str] = field(default_factory=lambda: [
        "parallel_safe",      # From commutativity
        "instant_undo",       # From self-inverse  
        "order_independent",  # From associativity
        "deterministic",      # From closure
    ])
```

### 4A.5 Deliverables

| Deliverable | Description |
|-------------|-------------|
| `sdk_generator/parser.py` | JSON schema parser with validation |
| `sdk_generator/ir.py` | ATOMiK Intermediate Representation |
| `sdk_generator/generators/` | Language/hardware generators |
| `sdk_generator/templates/` | Jinja2 templates for code generation |
| `sdk_generator/packager.py` | Output packaging and structure |
| `sdk_generator/cli.py` | Command-line interface |
| `schemas/` | JSON schema specification + examples |

---

## Phase 4B: Domain SDKs (3 Demonstrators)

### SDK 1: Video/Image Delta Processing

**JSON Schema**: `schemas/video_delta.json`
```json
{
  "atomik_sdk": {
    "name": "ATOMiKVideo",
    "description": "Delta-state video processing - frame deltas not full frames",
    "delta_fields": {
      "frame_delta": {
        "type": "spatial_temporal_delta",
        "encoding": "4x4x4_voxel",
        "width": 64
      },
      "motion_vector": {
        "type": "compressed_delta",
        "width": 32
      }
    },
    "use_cases": [
      "4K video streaming (24MB → 0.1MB per frame)",
      "Security camera change detection",
      "Video conferencing bandwidth reduction"
    ]
  }
}
```

**Generated API**:
```python
from atomik.video import DeltaProcessor

# Usage matches your example
video = DeltaProcessor(format="h264_delta")
for frame in camera_stream:
    delta = video.compress(frame)  # Only what changed
    network.send(delta)            # 240:1 compression
    gpu.apply(delta)               # Memory bandwidth solved
```

---

### SDK 2: Sensor Fusion / Edge Computing

**JSON Schema**: `schemas/edge_sensor.json`
```json
{
  "atomik_sdk": {
    "name": "ATOMiKEdge", 
    "description": "Delta-state sensor fusion at the edge",
    "delta_fields": {
      "sensor_readings": {
        "type": "delta_compressed_stream",
        "sensors": ["temperature", "pressure", "humidity", "acceleration"],
        "width": 64
      },
      "model_updates": {
        "type": "parameter_deltas_only",
        "quantization": "int8",
        "width": 256
      },
      "alert_state": {
        "type": "bitmask_delta",
        "width": 32
      }
    },
    "constraints": {
      "max_memory_mb": 2,
      "max_power_mw": 50,
      "update_latency_ms": 1
    }
  }
}
```

**Generated API**:
```python
from atomik.edge import SensorFusion

fusion = SensorFusion(sensors=["temp", "accel", "gyro"])
while True:
    readings = get_sensor_data()
    delta = fusion.update(readings)  # Only changes transmitted
    
    if fusion.detect_anomaly(delta):
        alert(fusion.current_state)  # Instant reconstruction
```

---

### SDK 3: Financial Tick Processing

**JSON Schema**: `schemas/financial_ticks.json`
```json
{
  "atomik_sdk": {
    "name": "ATOMiKFinance",
    "description": "High-frequency tick processing with delta aggregation",
    "delta_fields": {
      "price_delta": {
        "type": "numeric_delta",
        "precision": "fixed_point_64",
        "width": 64
      },
      "volume_delta": {
        "type": "accumulating_delta",
        "width": 64
      },
      "order_book_delta": {
        "type": "sparse_delta",
        "levels": 10,
        "width": 128
      }
    },
    "operations": [
      {"name": "aggregate_window", "window_sizes": ["1s", "5s", "1m"]},
      {"name": "rollback", "instant": true}
    ],
    "constraints": {
      "target_latency_ns": 100,
      "throughput_mps": 10
    }
  }
}
```

**Generated API**:
```python
from atomik.finance import TickProcessor

processor = TickProcessor(symbols=["AAPL", "GOOGL"])
for tick in market_feed:
    delta = processor.ingest(tick)
    
    # Instant rollback (self-inverse property)
    if needs_correction:
        processor.rollback(delta)  # O(1), not O(n)
    
    # Parallel aggregation (commutative property)
    aggregates = processor.aggregate_parallel(windows=["1s", "1m"])
```

---

## Phase 4C: Hardware Demonstrators

### Board 1: Video Processing Demonstrator

**Hardware**: Tang Nano 9K + Camera Module + HDMI Output

**Demonstration**:
- Live camera feed → ATOMiK delta compression → Display
- Side-by-side: Traditional (full frame) vs ATOMiK (delta only)
- Metrics displayed: Bandwidth, latency, power

**Deliverables**:
- `hardware/video_demo/` - Complete Verilog + constraints
- `hardware/video_demo/demo.py` - Python control script
- `hardware/video_demo/README.md` - Setup instructions

---

### Board 2: Sensor Fusion Demonstrator  

**Hardware**: Tang Nano 9K + IMU + Temperature/Humidity + LEDs

**Demonstration**:
- Multi-sensor fusion with change-only transmission
- Anomaly detection with instant state reconstruction
- LED indicators for motif classification

**Deliverables**:
- `hardware/sensor_demo/` - Complete Verilog + constraints
- `hardware/sensor_demo/demo.py` - Python control script
- `hardware/sensor_demo/README.md` - Setup instructions

---

### Board 3: Network Packet Processor Demonstrator

**Hardware**: Tang Nano 9K + Ethernet PHY (or USB-Ethernet)

**Demonstration**:
- Network traffic analysis using delta patterns
- Real-time classification of traffic changes
- Bandwidth reduction for mirrored/replicated streams

**Deliverables**:
- `hardware/network_demo/` - Complete Verilog + constraints
- `hardware/network_demo/demo.py` - Python control script
- `hardware/network_demo/README.md` - Setup instructions

---

## Revised Task Breakdown

### Phase 4A: SDK Generator Framework (Budget: $80)

| Task | Description | Agent | Tokens |
|------|-------------|-------|--------|
| T4A.1 | JSON Schema Specification | SDK Agent | 10K |
| T4A.2 | ATOMiK-IR Design | SDK Agent | 10K |
| T4A.3 | Parser & Validator | SDK Agent | 15K |
| T4A.4 | VerilogGenerator | Synthesis Agent | 20K |
| T4A.5 | PythonGenerator | SDK Agent | 15K |
| T4A.6 | RustGenerator | SDK Agent | 15K |
| T4A.7 | GenomeCompiler Enhancement | SDK Agent | 10K |
| T4A.8 | CLI & Packager | SDK Agent | 10K |

### Phase 4B: Domain SDKs (Budget: $60)

| Task | Description | Agent | Tokens |
|------|-------------|-------|--------|
| T4B.1 | Video SDK Schema + Generation | SDK Agent | 20K |
| T4B.2 | Edge Sensor SDK Schema + Generation | SDK Agent | 20K |
| T4B.3 | Financial SDK Schema + Generation | SDK Agent | 20K |
| T4B.4 | SDK Test Suites | Validator Agent | 15K |
| T4B.5 | SDK Documentation | Documenter Agent | 10K |

### Phase 4C: Hardware Demonstrators (Budget: $100)

| Task | Description | Agent | Tokens |
|------|-------------|-------|--------|
| T4C.1 | Video Demo RTL | Synthesis Agent | 30K |
| T4C.2 | Sensor Demo RTL | Synthesis Agent | 30K |
| T4C.3 | Network Demo RTL | Synthesis Agent | 30K |
| T4C.4 | Demo Integration & Testing | Validator Agent | 20K |
| T4C.5 | Demo Documentation | Documenter Agent | 10K |

**Total Budget**: $240 (within $325 remaining)

---

## File Structure

```
ATOMiK/
├── sdk_generator/                 # Phase 4A
│   ├── __init__.py
│   ├── cli.py                     # atomik-gen command
│   ├── parser.py                  # JSON → IR
│   ├── ir.py                      # ATOMiK Intermediate Representation
│   ├── validators.py              # Schema validation
│   ├── packager.py                # Output packaging
│   ├── generators/
│   │   ├── __init__.py
│   │   ├── base.py                # Base generator class
│   │   ├── verilog.py             # → .v files
│   │   ├── python.py              # → .py files
│   │   ├── rust.py                # → .rs files
│   │   ├── javascript.py          # → .js/.ts files
│   │   ├── c.py                   # → .c/.h files
│   │   ├── fsharp.py              # → .fs files
│   │   └── genome.py              # → .gnm files
│   └── templates/
│       ├── verilog/
│       ├── python/
│       ├── rust/
│       └── ...
│
├── schemas/                       # SDK definitions
│   ├── schema_spec.json           # Meta-schema
│   ├── video_delta.json           # Phase 4B.1
│   ├── edge_sensor.json           # Phase 4B.2
│   └── financial_ticks.json       # Phase 4B.3
│
├── generated_sdks/                # Output from generator
│   ├── atomik_video/
│   │   ├── python/
│   │   ├── rust/
│   │   └── verilog/
│   ├── atomik_edge/
│   └── atomik_finance/
│
├── hardware/                      # Phase 4C
│   ├── video_demo/
│   │   ├── rtl/
│   │   ├── constraints/
│   │   ├── demo.py
│   │   └── README.md
│   ├── sensor_demo/
│   └── network_demo/
│
└── docs/
    ├── sdk_generator_guide.md
    ├── schema_reference.md
    └── hardware_demos.md
```

---

## Success Criteria

### Phase 4A Success
- [ ] Generator produces valid Verilog from any conforming JSON schema
- [ ] Generator produces working Python SDK from same schema
- [ ] Generated code passes lint and basic tests
- [ ] CLI tool functional: `atomik-gen --schema video.json --targets python,verilog`

### Phase 4B Success
- [ ] 3 domain SDKs generated and functional
- [ ] Python SDKs installable via pip
- [ ] Each SDK has working examples
- [ ] Documentation complete

### Phase 4C Success
- [ ] 3 FPGA boards programmed and operational
- [ ] Each demonstrates measurable improvement over traditional approach
- [ ] Reproducible demo scripts
- [ ] Video recordings of demonstrations

---

## Key Differentiators

### Why This Approach Wins

1. **Simplicity from Mathematics**: The algebra guarantees behavior, so SDK generation is mechanical, not creative

2. **One Source of Truth**: JSON schema defines everything; hardware and software stay synchronized

3. **Organic Adoption Path**:
   - Developer finds SDK for their domain
   - Uses Python/JS version immediately
   - Performance needs grow → same schema → Rust/Verilog
   - No learning curve, same API

4. **Infrastructure Lock-in (the good kind)**:
   - Once deltas flow through systems, traditional state is expensive
   - ATOMiK becomes the "delta layer" everything depends on
   - Like how everything depends on TCP/IP

---

## Ecosystem Strategy: Community-Populated Catalogues

### The Flywheel Model

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     ATOMiK ECOSYSTEM FLYWHEEL                           │
│                                                                         │
│    ┌──────────────┐         ┌──────────────┐         ┌──────────────┐  │
│    │  ATOMiK      │         │  Industry    │         │  Catalogue   │  │
│    │  Provides    │────────▶│  Developers  │────────▶│  Grows       │  │
│    │  Framework   │         │  Contribute  │         │  (Free R&D)  │  │
│    └──────────────┘         └──────────────┘         └──────────────┘  │
│           ▲                                                  │          │
│           │                                                  │          │
│           │         ┌──────────────┐                        │          │
│           │         │  More Users  │                        │          │
│           └─────────│  Adopt       │◀───────────────────────┘          │
│                     │  ATOMiK      │                                    │
│                     └──────────────┘                                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Hierarchical Taxonomy: Verticals → Fields → Objects

The catalogue is a **tree structure** where position determines namespace:

```
atomik/                              # Root namespace
│
├── Edge/                            # VERTICAL: Edge Computing
│   ├── Sensor/                      # Field: Sensors
│   │   ├── Vibration.json          # Object → atomik.Edge.Sensor.Vibration
│   │   ├── Temperature.json        # Object → atomik.Edge.Sensor.Temperature
│   │   └── IMU.json                # Object → atomik.Edge.Sensor.IMU
│   ├── IoT/                         # Field: IoT Devices
│   │   ├── SmartMeter.json         # Object → atomik.Edge.IoT.SmartMeter
│   │   └── Thermostat.json         # Object → atomik.Edge.IoT.Thermostat
│   └── Wearable/                    # Field: Wearables
│       └── HealthMonitor.json      # Object → atomik.Edge.Wearable.HealthMonitor
│
├── Video/                           # VERTICAL: Video Processing
│   ├── Stream/                      # Field: Streaming
│   │   ├── H264Delta.json          # Object → atomik.Video.Stream.H264Delta
│   │   └── FPVCodec.json           # Object → atomik.Video.Stream.FPVCodec
│   ├── Security/                    # Field: Security Cameras
│   │   └── MotionDetector.json     # Object → atomik.Video.Security.MotionDetector
│   └── Medical/                     # Field: Medical Imaging
│       └── DicomDelta.json         # Object → atomik.Video.Medical.DicomDelta
│
├── Finance/                         # VERTICAL: Financial Systems
│   ├── Trading/                     # Field: Trading
│   │   ├── TickProcessor.json      # Object → atomik.Finance.Trading.TickProcessor
│   │   └── OrderBook.json          # Object → atomik.Finance.Trading.OrderBook
│   ├── Risk/                        # Field: Risk Management
│   │   └── Aggregator.json         # Object → atomik.Finance.Risk.Aggregator
│   └── Payment/                     # Field: Payments
│       └── Processor.json          # Object → atomik.Finance.Payment.Processor
│
├── Network/                         # VERTICAL: Networking
│   ├── Packet/                      # Field: Packet Processing
│   │   ├── Analyzer.json           # Object → atomik.Network.Packet.Analyzer
│   │   └── Firewall.json           # Object → atomik.Network.Packet.Firewall
│   ├── Flow/                        # Field: Flow Analysis
│   │   └── LoadBalancer.json       # Object → atomik.Network.Flow.LoadBalancer
│   └── Protocol/                    # Field: Protocol Analysis
│       └── DNSMonitor.json         # Object → atomik.Network.Protocol.DNSMonitor
│
└── Science/                         # VERTICAL: Scientific Computing
    ├── Bio/                         # Field: Bioinformatics
    │   └── GenomeSequencer.json    # Object → atomik.Science.Bio.GenomeSequencer
    ├── Physics/                     # Field: Physics
    │   └── ParticleDetector.json   # Object → atomik.Science.Physics.ParticleDetector
    └── Climate/                     # Field: Climate Science
        └── ModelDelta.json         # Object → atomik.Science.Climate.ModelDelta
```

### Auto-Generated Imports

The tree position **automatically generates** the import path:

```python
# From catalogue position: Network/Packet/Analyzer.json
from atomik.Network.Packet import Analyzer
analyzer = Analyzer(host="192.168.1.1")

# Or import the vertical
from atomik import Network
analyzer = Network.Packet.Analyzer(host="192.168.1.1")

# Or wildcard
from atomik.Edge.Sensor import *
vib = Vibration(pin=3)
temp = Temperature(bus="i2c")
```

```rust
// Rust equivalent
use atomik::Network::Packet::Analyzer;
let analyzer = Analyzer::new("192.168.1.1");
```

```javascript
// JavaScript equivalent
import { Analyzer } from 'atomik/Network/Packet';
const analyzer = new Analyzer({ host: '192.168.1.1' });
```

### How It Works

#### 1. ATOMiK Provides the Framework (We Do This)
- SDK Generator (Phase 4A)
- Core algebraic primitives
- Reference implementations (Phase 4B)
- Hardware validation (Phase 4C)
- Documentation & tutorials

#### 2. Industry Developers Contribute (They Do This For Free)
- Domain-specific JSON schemas
- Optimized configurations for their use case
- Real-world tested parameters
- Edge case handling

#### 3. Why They Contribute

| Motivation | Explanation |
|------------|-------------|
| **Self-interest** | They need the SDK anyway; sharing costs nothing |
| **Reputation** | "Official ATOMiK schema for X" is a badge |
| **Compatibility** | Others in their industry adopt same schema = interop |
| **Improvements** | Community finds bugs, suggests optimizations |
| **Recruitment** | Shows technical leadership in their domain |

#### 4. Why This Beats Traditional Models

| Traditional SDK | ATOMiK Catalogue |
|-----------------|------------------|
| Vendor builds everything | Community builds domain expertise |
| Slow iteration | Parallel development across industries |
| Generic, not optimized | Domain-expert tuned |
| Vendor = bottleneck | Permissionless innovation |
| Expensive R&D | Free market research |

### Catalogue Schema Format

```json
{
  "catalogue_entry": {
    "class": "edge-devices",
    "name": "industrial-vibration-monitor",
    "version": "1.2.0",
    "contributor": {
      "organization": "Acme Industrial",
      "contact": "iot-team@acme.com",
      "verified": true
    },
    "description": "Vibration monitoring for predictive maintenance",
    "tags": ["manufacturing", "predictive-maintenance", "IIoT"],
    
    "atomik_sdk": {
      "name": "VibrationMonitor",
      "delta_fields": {
        "vibration_spectrum": {
          "type": "fft_delta",
          "bins": 256,
          "width": 64,
          "notes": "Only transmit bins that changed >threshold"
        },
        "alarm_state": {
          "type": "bitmask_delta",
          "width": 16
        }
      },
      "constraints": {
        "sample_rate_hz": 10000,
        "max_latency_ms": 10,
        "power_budget_mw": 100
      },
      "proven_on": ["gowin_gw1nr9", "lattice_ice40up5k"]
    },
    
    "benchmarks": {
      "bandwidth_reduction": "94%",
      "latency_improvement": "12x",
      "power_reduction": "60%"
    },
    
    "adoption": {
      "downloads": 1247,
      "deployments_reported": 89,
      "stars": 156
    }
  }
}
```

### Governance Model

#### Submission Process
1. Developer creates schema following ATOMiK spec
2. Automated validation (generator must produce valid output)
3. Optional: Hardware verification on reference board
4. Community review period (7 days)
5. Merge into catalogue

#### Quality Tiers

| Tier | Requirements | Badge |
|------|--------------|-------|
| **Community** | Passes automated validation | 🔵 |
| **Tested** | + Includes test suite | 🟢 |
| **Verified** | + Hardware validated on ref board | ✅ |
| **Certified** | + Production deployment reported | ⭐ |

#### Versioning & Compatibility
- Schemas follow semver
- Breaking changes = new major version
- Old versions remain available
- Deprecation notices with migration paths

### Revenue Opportunities (Future)

While the catalogue is free, monetization comes from:

1. **Certified Hardware**: ATOMiK-certified FPGA boards
2. **Enterprise Support**: SLA-backed support contracts  
3. **Custom Development**: Schema development consulting
4. **Training**: ATOMiK certification programs
5. **Cloud Services**: Hosted SDK generation, testing

### Phase 4 Catalogue Integration

#### 4A Addition: Catalogue Infrastructure
| Task | Description |
|------|-------------|
| T4A.9 | Catalogue schema specification |
| T4A.10 | Submission validation pipeline |
| T4A.11 | Static site generator for atom-ik.dev/catalogue |

#### 4B Becomes: Seed Catalogue
The 3 domain SDKs become the **seed entries** that demonstrate quality:
- `video-processing/reference-delta-codec.json` (ATOMiK official)
- `edge-devices/reference-sensor-fusion.json` (ATOMiK official)
- `financial-systems/reference-tick-processor.json` (ATOMiK official)

These set the standard that community contributions must meet.

---

## Next Steps

1. **Approve this revised Phase 4 plan**
2. **Procure 2 additional Tang Nano 9K boards** (you have 1 from Phase 3)
3. **Begin Phase 4A.1**: JSON Schema Specification
4. **Parallel**: Order any additional hardware (camera module, IMU, etc.)

---

*Revised Phase 4 Plan - January 25, 2026*
*Aligned with SDK Generator + Hardware Demonstrator Strategy*
