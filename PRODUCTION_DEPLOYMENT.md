# ATOMiK Production Deployment — Tang Nano 9K SoC

**Date:** February 13, 2026
**Board:** Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
**Toolchain:** Gowin EDA V1.9.12.01
**Architecture:** PicoRV32 RISC-V CPU + ATOMiK Delta-State Accelerator
**Status:** ✅ **DEPLOYED** — Persistent flash, zero TNS, all tests passing

---

## Executive Summary

ATOMiK is deployed as a production SoC accelerator on the Tang Nano 9K. The system combines a PicoRV32 RISC-V CPU running at 25.2 MHz with a single-bank ATOMiK delta-state accumulator running at 81 MHz in an independent clock domain. Clock domain crossing is handled via a toggle-handshake CDC bridge. Both bitstream and firmware are in persistent SPI flash.

**Key Achievements:**
- ✅ Clean timing closure: 0 TNS on all clock domains
- ✅ ATOMiK @ 100.2 MHz (+23.6% margin above 81 MHz target)
- ✅ CPU @ 30.6 MHz (+21.4% margin above 25.2 MHz target)
- ✅ All 5 test suites passing on hardware
- ✅ Persistent deployment ready for field use

---

## Architecture

### Clock Domains

| Domain | Clock Source | Frequency | Fmax | Margin | TNS |
|--------|-------------|-----------|------|--------|-----|
| **ATOMiK Core** | Dedicated PLL | 81.0 MHz | 100.2 MHz | +23.6% | 0.000 |
| **CPU + Bus** | HDMI PLL ÷ 5 | 25.2 MHz | 30.6 MHz | +21.4% | 0.000 |
| **HDMI Pixel** | HDMI PLL ÷ 5 | 25.2 MHz | — | — | — |
| **HDMI Serial** | HDMI PLL | 126.0 MHz | — | — | — |

### Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Tang Nano 9K SoC                          │
│                                                               │
│  ┌─────────────────┐            ┌────────────────────────┐   │
│  │  PicoRV32 CPU   │            │   ATOMiK Accelerator   │   │
│  │  @ 25.2 MHz     │            │   @ 81 MHz             │   │
│  │                 │            │                        │   │
│  │  RV32I ISA      │◄──CDC──────┤  Single-Bank (N=1)     │   │
│  │  SPI XIP        │  Bridge    │  32-bit Delta Width    │   │
│  │                 │            │                        │   │
│  └────┬────────────┘            └────────────────────────┘   │
│       │                                                       │
│       ├─ SRAM (8KB)                                           │
│       ├─ SPI Flash (XIP + Persistent Storage)                │
│       ├─ UART (115200 baud)                                  │
│       ├─ GPIO (7 pins)                                       │
│       └─ HDMI Output                                         │
│                                                               │
│  PLLs: HDMI PLL (126 MHz) + ATOMiK PLL (81 MHz)              │
└─────────────────────────────────────────────────────────────┘
```

### Memory Map

| Address Range | Device | Access |
|--------------|--------|--------|
| `0x0000_0000 - 0x3FFF_FFFF` | SPI Flash XIP | R (instruction fetch) |
| `0x4000_0000 - 0x4000_1FFF` | SRAM 8KB | RW |
| `0x8000_0000 - 0x8000_1FFF` | Boot ROM | R |
| `0x8100_0000 - 0x8100_001F` | SPI Flash Config | RW |
| `0x8200_0000 - 0x8200_0007` | GPIO | RW |
| `0x8300_0000 - 0x8300_0007` | UART | RW |
| `0xC000_0000 - 0xC000_001F` | **ATOMiK** | RW |

### ATOMiK Register Map

| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| `0x00` | LOAD | W | Load initial state, clear accumulator |
| `0x04` | ACCUM | W | Accumulate delta via XOR |
| `0x08` | STATE | R | Read reconstructed state (initial ⊕ accumulator) |
| `0x0C` | STATUS | R | Bit 0: accumulator_zero flag |
| `0x10` | CONFIG | W | Bit 0: soft reset |
| `0x14` | INIT | R | Read initial_state (debug) |
| `0x18` | DELTA | R | Read accumulator (debug) |

---

## Resource Utilization

### Full SoC (PicoRV32 + ATOMiK + HDMI + Peripherals)

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| **LUT** | 3,838 | 8,640 | **44%** |
| **ALU** | 707 | — | — |
| **FF** | — | 6,693 | — |
| **CLS** | 3,103 | 4,320 | **72%** |
| **BSRAM** | 12 | 26 | **47%** |
| **rPLL** | 2 | 2 | **100%** |

### ATOMiK Contribution (Single-Bank @ 81 MHz)

| Resource | Baseline | With ATOMiK | Delta |
|----------|----------|-------------|-------|
| **LUT** | 3,608 | 3,838 | +230 (2.7%) |
| **FF** | 1,930 | — | ~+150 |

---

## Validation

### Hardware Test Results

All test suites pass on production hardware:

| Test Suite | Tests | Result | Performance |
|-----------|-------|--------|-------------|
| **[X] ATOMiK Hardware** | 11/11 | ✅ PASS | 224 cycles |
| **[P] Runtime Integration** | 10/10 | ✅ PASS | Runtime API validated |
| **[K] Checkpoint/Rollback** | — | ✅ PASS | 518-1342 cycles |
| **[M] Memory Benchmark** | — | ✅ PASS | 12-17k cycles |
| **[H] Heap Integrity** | — | ✅ PASS | 335 cycles |
| **[R] Performance Benchmark** | 550 measurements | ✅ PASS | Automated sweep |

### Test Suite Details

#### [X] ATOMiK Hardware Test (11/11 PASS)
- T1: Soft reset
- T2: Load 0xDEADBEEF
- T3: Accumulator zero flag
- T4: State equals initial
- T5: Accumulate 0xFF
- T6: Accumulator non-zero flag
- T7: State XOR verification
- T8: XOR cancellation (delta ⊕ delta = 0)
- T9: Accumulator zero again
- T10: Multi-delta sequence
- T11: Performance measurement (224 cycles)

#### [P] Runtime Integration Test (10/10 PASS)
- P1: API initialization
- P2: Fingerprint computation
- P3: Tracked memcpy
- P4: Unchanged region detection
- P5: Changed region detection
- P6: Verified memset
- P7: Checkpoint creation
- P8: Heap allocation
- P9: Heap integrity check
- P10: printf verification

---

## Clock Domain Crossing (CDC)

### Toggle-Handshake Protocol

The CDC bridge uses a toggle-handshake protocol to safely cross between the 25.2 MHz bus domain and the 81 MHz ATOMiK core domain:

**Forward Path (Bus → Core):**
1. Bus domain latches request data (addr, wdata, wstrb)
2. Bus domain toggles `req_toggle` signal
3. Core domain syncs via 2FF synchronizer
4. Core domain detects edge, executes operation
5. Core domain toggles `ack_toggle`

**Return Path (Core → Bus):**
1. Bus domain syncs `ack_toggle` via 2FF synchronizer
2. Bus domain detects edge, drives `mem_ready`
3. Response data (rdata) latched and stable

**Latency:** ~3 bus cycles per MMIO access (~120 ns @ 25.2 MHz)

**Safety:** Data is latched before toggle, sampled after 2FF sync. Standard CDC best practices.

---

## Build Instructions

### Prerequisites

```bash
# Gowin EDA V1.9.12.01
export GOWIN_HOME=/opt/gowin/IDE
export LD_PRELOAD=/lib/x86_64-linux-gnu/libstdc++.so.6
export LD_LIBRARY_PATH=$GOWIN_HOME/lib:/lib/x86_64-linux-gnu

# RISC-V toolchain
sudo apt install gcc-riscv64-unknown-elf

# openFPGALoader
sudo apt install openfpgaloader
```

### Synthesis

```bash
git clone https://github.com/MatthewHRockwell/TangNano-9K-example.git
cd TangNano-9K-example/picotiny/project
$GOWIN_HOME/bin/gw_sh synth.tcl
```

**Output:** `impl/pnr/picotiny.fs` (bitstream)

### Flash Bitstream to Persistent Storage

```bash
openFPGALoader -b tangnano9k -f impl/pnr/picotiny.fs
```

### Build and Flash Firmware

```bash
# Build ATOMiK firmware
cd /path/to/ATOMiK/hardware/picorv32/firmware
make clean && make

# Reset board and flash firmware
# Method 1: Physical reset button, then immediately:
python3 /path/to/TangNano-9K-example/picotiny/sw/pico-programmer.py \
    build/atomik-fw.v /dev/ttyUSB1

# Method 2: JTAG reload (acts as reset), then flash:
openFPGALoader -b tangnano9k /path/to/picotiny.fs
# Then immediately:
python3 /path/to/TangNano-9K-example/picotiny/sw/pico-programmer.py \
    build/atomik-fw.v /dev/ttyUSB1
```

### Serial Console

```bash
# 115200 baud, 8N1
screen /dev/ttyUSB1 115200
# or
picocom -b 115200 /dev/ttyUSB1
```

**Firmware Menu:**
- `[X]` — ATOMiK hardware test (11/11)
- `[P]` — Runtime integration test (10/10)
- `[K]` — Checkpoint/rollback demo
- `[M]` — Memory benchmark
- `[H]` — Heap integrity demo
- `[F]` — Flash mode
- `[I]` — Read SPI flash ID
- `[S]`/`[D]`/`[C]` — SPI mode (single/dual/dual+CRM)
- `[R]` — Performance benchmark suite (550 measurements, machine-parseable)
- `[B]` — Simplistic benchmark
- `[1-6]` — Toggle LEDs

---

## Design Decisions

### Why Single-Bank (N=1) Instead of Multi-Bank?

The production deployment uses **N_BANKS=1** (single accumulator) running at 81 MHz for these reasons:

1. **Clean Timing Closure**: N=4 at 81 MHz had a -0.454 ns TNS violation. N=1 achieves 100.2 MHz Fmax with 23.6% margin.
2. **Resource Efficiency**: Single-bank uses 230 LUT overhead vs. ~400 LUT for N=4.
3. **Sufficient Throughput**: 81 Mops/s meets current requirements. The architecture scales to 1,056 Mops/s (N=16 @ 66 MHz) when needed.
4. **Simplified CDC**: Single accumulator = simpler bus wrapper, fewer cross-domain paths.

**Scaling Path:** The `atomik_parallel_acc` module supports N=1,2,4,8,16 via parameter. To scale, change `N_BANKS` in `atomik_bus_wrapper.v` and re-synthesize.

### Why Independent ATOMiK Clock Domain?

The ATOMiK core runs at 81 MHz on a dedicated PLL, not at the CPU's 25.2 MHz:

1. **Higher Throughput**: 81 MHz vs 25.2 MHz = 3.2× more operations per second.
2. **Architectural Separation**: ATOMiK timing is independent of CPU/peripheral timing.
3. **Future Scaling**: Higher frequencies (up to 100+ MHz) possible without impacting CPU.

**Tradeoff:** Requires CDC bridge (+~50 LUT, +3 cycle latency). Worth it for 3.2× throughput gain.

### Why Toggle-Handshake CDC Instead of FIFO?

Toggle-handshake is simpler and sufficient for this use case:

- **Latency:** ~3 bus cycles (120 ns) — acceptable for MMIO
- **Resources:** ~50 LUT vs 200+ LUT for async FIFO
- **Correctness:** Standard 2FF synchronizer, proven safe
- **Self-Throttling:** Bus automatically waits for core via `mem_ready` handshake

---

## Known Limitations

1. **Both PLLs Used**: HDMI PLL (126 MHz) + ATOMiK PLL (81 MHz) = 2/2 PLLs on GW1NR-9. No PLL headroom for additional clock domains.

2. **UART Baud Rate**: Fixed at 115200 baud (CLK_FREQ/UART_BAUD-2 divider in firmware). Higher rates require firmware recompile.

3. **SPI Flash Speed**: XIP runs in single-SPI mode. QSPI supported by hardware but not enabled (requires flash controller config). Instruction fetch bandwidth currently ~6.3 MB/s, could be 25.2 MB/s with QSPI.

4. **SRAM Size**: Only 8KB on-chip SRAM. Heap limited to ~2KB after firmware .data/.bss sections. Large datasets require external memory or SPI flash buffering.

5. **No DMA**: All ATOMiK operations are CPU-initiated MMIO. Future: DMA engine could stream deltas at line rate.

---

## Future Enhancements

### Near-Term (Requires Firmware Only)

- [ ] QSPI flash mode (4× instruction fetch bandwidth)
- [ ] Batch delta operations (write N deltas without per-delta overhead)
- [ ] Interrupt-driven accumulator (notify CPU on threshold)

### Medium-Term (Requires RTL Changes)

- [ ] Multi-bank scaling (N=4 or N=8) with per-bank addressing
- [ ] DMA engine for zero-copy delta streaming
- [ ] AXI bus interface (replace PicoRV32 valid/ready)
- [ ] External DRAM controller (expand to MB/GB datasets)

### Long-Term (New Platform)

- [ ] Port to Artix-7 (300 MHz, 16-bank = 4.8 Gops/s)
- [ ] Port to UltraScale+ (500 MHz, 16-bank = 8 Gops/s)
- [ ] ASIC tape-out (28nm target, 1 GHz = 16 Gops/s)

---

## Reproducibility

### Hardware Synthesis

```bash
# Clone repo
git clone https://github.com/MatthewHRockwell/TangNano-9K-example.git
cd TangNano-9K-example/picotiny

# Verify commit
git log --oneline -1
# Expected: 38819d4 Switch ATOMiK to single-bank (N_BANKS=1) for clean timing at 81 MHz

# Synthesize
cd project
$GOWIN_HOME/bin/gw_sh synth.tcl

# Verify timing
grep "Actual Fmax" impl/pnr/picotiny.tr
# Expected: ATOMiK @ 100.167 MHz, CPU @ 30.591 MHz

grep "setup.*0.000.*0$" impl/pnr/picotiny.tr | wc -l
# Expected: >10 (zero TNS on all domains)
```

### Firmware Build

```bash
# Clone ATOMiK repo
git clone https://github.com/MatthewHRockwell/ATOMiK.git
cd ATOMiK/hardware/picorv32/firmware

# Verify commit
git log --oneline -1
# Expected: d08d866 ATOMiK dual-clock 4-bank parallel bus wrapper with CDC bridge

# Build
make clean && make

# Verify output
ls -lh build/atomik-fw.v
# Expected: ~16KB Verilog firmware image
```

### Hardware Test

```bash
# Flash bitstream + firmware
openFPGALoader -b tangnano9k -f /path/to/picotiny.fs
# Wait 100ms, then:
python3 /path/to/pico-programmer.py /path/to/atomik-fw.v /dev/ttyUSB1

# Connect serial
screen /dev/ttyUSB1 115200

# Run tests (type each letter):
X   # ATOMiK hardware test → expect 11/11 PASS
P   # Runtime integration test → expect 10/10 PASS
K   # Checkpoint/rollback → expect PASS
M   # Memory benchmark → expect PASS
H   # Heap integrity → expect PASS
```

---

## Performance Metrics

### ATOMiK Core Operations (@ 81 MHz)

| Operation | Cycles | Latency | Throughput |
|-----------|--------|---------|------------|
| LOAD (initial_state) | 1 | 12.3 ns | 81 Mops/s |
| ACCUM (delta XOR) | 1 | 12.3 ns | 81 Mops/s |
| STATE (reconstruct) | 1 | 12.3 ns | 81 Mops/s |

### End-to-End (CPU → ATOMiK via CDC)

| Operation | CPU Cycles | Latency @ 25.2 MHz | Notes |
|-----------|-----------|-------------------|-------|
| MMIO Write (LOAD/ACCUM) | ~6 | ~238 ns | Includes bus arbitration + CDC |
| MMIO Read (STATE/STATUS) | ~8 | ~317 ns | Includes CDC + response |

### Application-Level Benchmarks

| Workload | Cycles | Time @ 25.2 MHz | Notes |
|----------|--------|-----------------|-------|
| **Checkpoint (4 fields)** | 518 | 20.6 μs | 4× LOAD + fingerprint |
| **Detect Change (4 fields)** | 1,342 | 53.3 μs | 5× deltas + read |
| **Rollback** | 419 | 16.6 μs | 4× LOAD to restore |
| **memcpy 256B (tracked)** | 17,357 | 689 μs | ATOMiK-aware copy |
| **memset 256B (verified)** | 13,389 | 531 μs | ATOMiK-aware set |
| **Change detection (256B)** | 13,194 | 524 μs | 5× faster than memcmp |

---

## Detailed Runtime Benchmarks

### Memory Operations (256 bytes / 64 words)

#### Copy Operations

| Operation | Cycles | Notes |
|---|---|---|
| `sw_memcpy` (byte-level) | 15,471 | Standard byte-by-byte copy |
| `atomik_memcpy_tracked` | 17,357 | Copy + XOR fingerprint in single pass |
| **Overhead** | **+12.2%** | Cost of integrity tracking |

#### Fill Operations

| Operation | Cycles | Notes |
|---|---|---|
| `sw_memset` (byte-level) | 11,572 | Standard byte-by-byte fill |
| `atomik_memset_verified` | 13,389 | Fill + XOR fingerprint in single pass |
| **Overhead** | **+15.7%** | Cost of integrity tracking |

#### Change Detection

| Operation | Cycles | Notes |
|---|---|---|
| `sw_memcmp` (identical buffers) | 67,354 | Compare 256 bytes between two buffers |
| `atomik_region_changed` (no change) | 13,194 | Recompute fingerprint, compare to saved |
| `atomik_region_changed` (1-bit flip) | 13,191 | Detects single-bit change |
| **Speedup** | **5.1x** | Single-buffer scan vs two-buffer comparison |

#### Fingerprint

| Operation | Cycles | Notes |
|---|---|---|
| `atomik_fingerprint` (64 words) | 12,798 | XOR-fold entire buffer via accumulator |

### Benchmark Analysis

The tracked memory operations (memcpy, memset) add 12-16% cycle overhead — the cost of one additional MMIO write per word to feed the ATOMiK accumulator. The CPU still moves every byte; ATOMiK does not accelerate the copy itself.

**Where ATOMiK delivers genuine value is change detection.** Once a fingerprint is saved, checking whether a memory region has changed requires scanning only one buffer (13K cycles) instead of comparing two buffers byte-by-byte (67K cycles). This 5.1x speedup compounds with buffer size. The fingerprint comparison itself is always 1 cycle regardless of buffer size.

For systems that need frequent dirty-checking (graphics framebuffers, sensor state, network packet deduplication), this is meaningful.

### Optimization Opportunities

The 12-16% tracked operation overhead measured here reflects the cost of CPU-mediated MMIO writes to a single-bank accumulator over SPI XIP flash. With QSPI (4:1 bandwidth, already supported by the Tang Nano 9K hardware) and code execution from SRAM, this overhead would decrease substantially. The multi-bank architecture eliminates the single-accumulator bottleneck entirely — parallel banks accept deltas simultaneously with no serialization.

---

## Firmware Source Files

| File | Purpose |
|---|---|
| `hardware/picorv32/firmware/atomik.h` | Hardware Abstraction Layer — bank-aware inline API |
| `hardware/picorv32/firmware/printf.h` | mini_printf header |
| `hardware/picorv32/firmware/printf.c` | mini_printf implementation (no div, powers-of-10 table) |
| `hardware/picorv32/firmware/atomik_mem.h` | Tracked memory operations header |
| `hardware/picorv32/firmware/atomik_mem.c` | Tracked memory operations + memset/memcpy symbols |
| `hardware/picorv32/firmware/atomik_alloc.h` | Bump allocator header |
| `hardware/picorv32/firmware/atomik_alloc.c` | Bump allocator with XOR integrity tracking |
| `hardware/picorv32/firmware/perf_bench.h` | Performance benchmark declarations |
| `hardware/picorv32/firmware/perf_bench.c` | Automated benchmark suite (550 measurements) |
| `hardware/picorv32/firmware/firmware.c` | Main firmware — HAL API, 6 menu commands |
| `hardware/picorv32/firmware/Makefile` | Build system (rv32i, -O3, -fno-builtin) |
| `hardware/picorv32/firmware/linker_flash.ld` | Linker script with 2KB heap region |
| `hardware/picorv32/firmware/crt_flash.S` | Startup code |

---

## Conclusion

ATOMiK is successfully deployed as a production SoC accelerator on the Tang Nano 9K. The system achieves clean timing closure with healthy margins (+23% on ATOMiK, +21% on CPU), passes all functional tests on hardware, and is ready for field deployment via persistent SPI flash.

The single-bank @ 81 MHz configuration provides 81 Mops/s throughput with minimal resource overhead (2.7% LUT). The architecture is proven to scale linearly to 1,056 Mops/s (16 banks @ 66 MHz) when higher throughput is required.

**Next Steps:** Real-world application integration, multi-bank scaling experiments, ASIC feasibility study.

For known hardware and software issues, see [`docs/KNOWN_ISSUES.md`](KNOWN_ISSUES.md).

---

**Repository:** [MatthewHRockwell/TangNano-9K-example](https://github.com/MatthewHRockwell/TangNano-9K-example/tree/main/picotiny)
**Commit:** `38819d4` (bitstream) / `d08d866` (firmware)
**License:** Apache 2.0 (subject to patent notice)

---
---

# ATOMiK v3 Production Deployment — Tang Nano 9K SoC

**Date:** March 6, 2026
**Board:** Tang Nano 9K (GW1NR-LV9QN88PC6/I5)
**Toolchain:** Gowin EDA V1.9.12.01
**Architecture:** Custom RV64I CPU + ATOMiK Direct-Wire (Single Clock Domain)
**Status:** ✅ **DEPLOYED** — Persistent flash, zero TNS, all tests passing

---

## Executive Summary

ATOMiK v3 is a ground-up redesign replacing the v2's PicoRV32+MMIO architecture with a custom RV64I CPU that has ATOMiK operations wired directly as custom RISC-V instructions. This eliminates the CDC bridge, bus wrapper, and MMIO overhead of v2, producing a 44% reduction in ATOMiK roundtrip latency and transforming ATOMiK-tracked memory operations from a 12% overhead (v2) into an **84.5% speedup** (v3).

**Key Achievements:**
- ✅ Custom RV64I CPU: 53/54 compliance tests (only `ma_data` by design)
- ✅ ATOMiK custom instructions: LOAD, ACCUM, READ, SWAP — zero bus overhead
- ✅ Delta-driven HDMI display: `pixel_out = pixel_ref ⊕ LUT[index]`
- ✅ Parallel banks: N=16 @ 67.5 MHz = 1,080 Mops/s (synthesis-validated)
- ✅ Persistent flash deployment with ISP programmer
- ✅ All 530 benchmark measurements perfectly deterministic (zero variance)

---

## Architecture

### Clock Domains

| Domain | Clock Source | Frequency | Fmax | Margin | TNS |
|--------|-------------|-----------|------|--------|-----|
| **CPU + ATOMiK** | PLL1 (108 MHz ÷ 5) | 21.6 MHz | 21.987 MHz | +1.8% | 0.000 |
| **HDMI Pixel** | PLL2 (126 MHz ÷ 5) | 25.2 MHz | 33.964 MHz | +34.8% | 0.000 |
| **HDMI Serial** | PLL2 | 126.0 MHz | — | — | — |

### Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Tang Nano 9K v3 SoC                       │
│                                                               │
│  ┌─────────────────────────────────────────────────────┐     │
│  │           atomik_v3_cpu @ 21.6 MHz                  │     │
│  │                                                     │     │
│  │  RV64I ISA (custom)        ATOMiK Accelerator       │     │
│  │  Multi-cycle FSM           (direct-wired)           │     │
│  │  BSRAM Register File       64-bit Delta Width       │     │
│  │                            256-entry State Table     │     │
│  │  Custom Instructions:                               │     │
│  │    atomik.load  (funct3=0)                          │     │
│  │    atomik.accum (funct3=1)                          │     │
│  │    atomik.read  (funct3=2)                          │     │
│  │    atomik.swap  (funct3=3)                          │     │
│  └────┬────────────────────────────────────────────────┘     │
│       │                                                       │
│       ├─ Data SRAM (8 KB, 4 BSRAM)                           │
│       ├─ Boot ROM (8 KB, 4 BSRAM) + ISP Flasher              │
│       ├─ SPI Flash XIP                                        │
│       ├─ UART (115200 baud)                                   │
│       ├─ GPIO (5 pins + 2 debug)                              │
│       └─ HDMI (640×480 @ 60 Hz) ──CDC──> Delta Display        │
│              Pipeline: pixel_ref ⊕ LUT[delta_index]           │
│                                                               │
│  PLLs: CPU PLL (108 MHz) + HDMI PLL (126 MHz)                │
└─────────────────────────────────────────────────────────────┘
```

### Memory Map

| Address Range | Device | Access |
|--------------|--------|--------|
| `0x0000_0000 - 0x007F_FFFF` | SPI Flash XIP (8 MB) | R (instruction fetch) |
| `0x4000_0000 - 0x4000_1FFF` | SRAM 8KB | RW |
| `0x8000_0000 - 0x8000_1FFF` | Boot ROM (ISP Flasher) | R |
| `0x8100_0000 - 0x8100_001F` | SPI Flash Config | RW |
| `0x8200_0000 - 0x8200_0007` | GPIO | RW |
| `0x8300_0000 - 0x8300_0007` | UART | RW |
| `0xC000_0000 - 0xC000_000F` | Display Pipeline MMIO | RW (via CDC) |

### ATOMiK Custom Instruction Encoding

ATOMiK operations use the Custom-0 opcode (0x0B) in R-type format:

| Instruction | funct3 | Operation | Result |
|-------------|--------|-----------|--------|
| `atomik.load rd, rs1, rs2` | 000 | Load initial state (addr=rs1, init=rs2) | rd = acc_zero |
| `atomik.accum rd, rs1` | 001 | Accumulate delta (delta=rs1) | rd = acc_zero |
| `atomik.read rd, rs1` | 010 | Read state (addr=rs1) | rd = initial ⊕ accumulator |
| `atomik.swap rd, rs1` | 011 | Swap reference (addr=rs1) | rd = previous state |

**No bus overhead.** Custom instructions are decoded and executed in the CPU's normal pipeline — no MMIO writes, no CDC crossings, no bus arbitration.

---

## Resource Utilization

### Full v3 SoC (CPU + ATOMiK + HDMI + Display Pipeline + Peripherals)

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| **LUT** | 5,966 | 8,640 | **69%** |
| **CLS** | 3,782 | 4,320 | **88%** |
| **BSRAM** | 19 | 26 | **74%** |
| **rPLL** | 2 | 2 | **100%** |

### BSRAM Allocation

| Blocks | Purpose |
|--------|---------|
| 4 | Register file (2 SDP banks × 2 for 64-bit width) |
| 2 | ATOMiK state table (256 × 64-bit) |
| 4 | Boot ROM (8 KB) |
| 4 | Data SRAM (8 KB) |
| 1 | SPI flash controller |
| 1 | HDMI character buffer |
| 1 | Delta color LUT (256 × 24-bit) |
| 1 | Scanline delta buffer (1024 × 9-bit) |
| 1 | SVO HDMI overlay |
| **19** | **Total (74%)** |

### v2 vs v3 Resource Comparison

| Resource | v2 | v3 | Notes |
|----------|-----|-----|-------|
| **LUT** | 3,838 (44%) | 5,966 (69%) | 64-bit datapath + display pipeline |
| **CLS** | 3,103 (72%) | 3,782 (88%) | Higher utilization but fits |
| **BSRAM** | 12 (47%) | 19 (74%) | + regfile, display LUT, scanline buffer |
| **PLL** | 2 (100%) | 2 (100%) | Both use dual-PLL |

---

## Validation

### Hardware Test Results

| Test Suite | Tests | Result | Notes |
|-----------|-------|--------|-------|
| **[X] ATOMiK Hardware** | 9/9 | ✅ PASS | Custom instruction operations |
| **[P] Runtime Integration** | 10/10 | ✅ PASS | Tracked memcpy, fingerprint, checkpoint |
| **[K] Checkpoint/Rollback** | — | ✅ PASS | Delta-chain rollback verified |
| **[M] Memory Benchmark** | — | ✅ PASS | 6.4x faster than sw_memcpy |
| **[H] Heap Integrity** | — | ✅ PASS | XOR-tagged allocator |
| **[V] Display Tests** | 6/6 | ✅ PASS | Delta LUT, scanline buffer, frame timing |
| **[R] Performance Benchmark** | 530 measurements | ✅ PASS | Zero variance across all metrics |
| **Compliance** | 53/54 | ✅ PASS | Only `ma_data` fails (by design) |
| **Parallel Banks** | 20/20 | ✅ PASS | iverilog simulation (N=1,4,8,16) |

---

## Performance Metrics

### ATOMiK Core Operations (v3 @ 21.6 MHz)

| Operation | Cycles | Latency @ 21.6 MHz | v2 Cycles | Improvement |
|-----------|--------|-------------------|-----------|-------------|
| LOAD | 64 | 2.96 µs | 64 | 0% |
| ACCUM | 64 | 2.96 µs | 70 | **-8.6%** |
| READ | 96 | 4.44 µs | 99 | **-3.0%** |
| Roundtrip | 160 | 7.41 µs | 285 | **-43.9%** |
| SWAP | 96 | 4.44 µs | N/A | New in v3 |

### Application-Level Benchmarks (256 bytes)

| Operation | v3 Cycles | v3 ATOMiK vs SW | v2 ATOMiK vs SW |
|-----------|-----------|-----------------|-----------------|
| **memcpy (ATOMiK)** | 13,522 | **-84.5%** (6.4x faster) | +12.4% (slower) |
| **memset (ATOMiK)** | 11,698 | **-83.4%** (6.0x faster) | +15.7% (slower) |
| **change detection** | 9,234 | **-89.4%** (9.4x faster) | -80.5% (faster) |

**Headline result:** ATOMiK-tracked memcpy went from a 12% overhead on v2 to an **84.5% speedup** on v3. The custom instruction architecture eliminates the MMIO bottleneck that dominated v2 tracking cost.

### Parallel Bank Throughput (Synthesis-Validated)

| Banks | Frequency | LUT | Fmax | Timing | Throughput |
|------:|----------:|----:|-----:|:------:|-----------:|
| 1 | 67.5 MHz | 472 | 100.5 MHz | MET | 67.5 Mops/s |
| 4 | 67.5 MHz | 737 | 90.4 MHz | MET | 270.0 Mops/s |
| 8 | 67.5 MHz | 1,126 | 70.5 MHz | MET | 540.0 Mops/s |
| 16 | 67.5 MHz | 1,778 | 70.5 MHz | MET | **1,080.0 Mops/s** |

v3 parallel banks achieve 1,080 Mops/s vs v2's 1,056 Mops/s (+2.3%) while using 50% fewer LUTs per bit per bank (1.36 vs 2.71).

---

## Build Instructions

### Prerequisites

```bash
# Gowin EDA V1.9.12.01
export GOWIN_HOME=/opt/gowin/IDE
export LD_PRELOAD=/lib/x86_64-linux-gnu/libstdc++.so.6
export LD_LIBRARY_PATH=$GOWIN_HOME/lib:/lib/x86_64-linux-gnu

# RISC-V 64-bit toolchain
sudo apt install gcc-riscv64-unknown-elf

# openFPGALoader
sudo apt install openfpgaloader
```

### Synthesis

```bash
cd ATOMiK/hardware/v3/synth
$GOWIN_HOME/bin/gw_sh synth_v3.tcl
```

**Output:** `impl/pnr/atomik_v3_soc.fs`

### Flash Bitstream

```bash
# Persistent flash
openFPGALoader -b tangnano9k -f hardware/v3/synth/impl/pnr/atomik_v3_soc.fs

# SRAM (volatile, for testing)
openFPGALoader -b tangnano9k hardware/v3/synth/impl/pnr/atomik_v3_soc.fs
```

### Build and Flash Firmware

```bash
# Build firmware (RV64I, -Os)
cd ATOMiK/hardware/v3/soc/firmware/fw-flash
make clean && make

# Flash via ISP programmer (start programmer, then trigger reboot)
python3 ../../scripts/isp_flash_programmer.py build/fw-flash.v /dev/ttyUSB1 &
sleep 2
openFPGALoader -b tangnano9k /path/to/atomik_v3_soc.fs
```

### Serial Console

```bash
screen /dev/ttyUSB1 115200
# or
picocom -b 115200 /dev/ttyUSB1
```

**Firmware Menu:**
- `[X]` — ATOMiK hardware test (9/9)
- `[P]` — Runtime integration test (10/10)
- `[K]` — Checkpoint/rollback demo
- `[M]` — Memory benchmark
- `[H]` — Heap integrity demo
- `[V]` — Display pipeline test (6/6)
- `[B]` — XOR benchmark
- `[R]` — Performance suite (530 measurements)
- `[F]` — Flash mode
- `[1-6]` — Toggle LEDs

---

## Design Decisions

### Why Custom Instructions Instead of MMIO?

v3 replaces v2's memory-mapped ATOMiK interface with custom RISC-V instructions:

1. **Zero bus overhead**: No MMIO write cycle, no bus arbitration, no peripheral address decode
2. **No CDC**: Single clock domain eliminates the toggle-handshake bridge (saved ~50 LUT)
3. **Register-direct**: Operands come from CPU registers, results written back directly — no load/store required
4. **SWAP operation**: New atomic "swap reference state" operation impossible with MMIO

**Tradeoff:** Requires custom CPU (can't use off-the-shelf PicoRV32). Worth it for 44% roundtrip reduction.

### Why 21.6 MHz Instead of 25.2 MHz?

The v3 CPU's multi-cycle decode path has 13-14 logic levels, producing Fmax ~24.7 MHz at 25.2 MHz target (V3-020). Lowering to 21.6 MHz provides clean timing closure.

**Future improvement:** Pipelining the decode stage would restore 25.2 MHz operation.

### Why Dual-PLL for HDMI?

CPU and HDMI need different frequencies (21.6 vs 25.2 MHz). Sharing a PLL (V3-021 original approach) forced HDMI to non-standard timing. The dedicated HDMI PLL produces standard 640×480 @ 60 Hz.

---

## Firmware Source Files

| File | Purpose |
|---|---|
| `hardware/v3/soc/firmware/fw-flash/atomik_v3.h` | Custom instruction wrappers (LOAD, ACCUM, READ, SWAP) |
| `hardware/v3/soc/firmware/fw-flash/firmware.c` | Main firmware — test suites, menu, benchmarks |
| `hardware/v3/soc/firmware/fw-flash/atomik_mem.h` | Tracked memory operations header |
| `hardware/v3/soc/firmware/fw-flash/atomik_mem.c` | Tracked memcpy/memset/change-detect (64-bit) |
| `hardware/v3/soc/firmware/fw-flash/atomik_alloc.h` | Bump allocator header |
| `hardware/v3/soc/firmware/fw-flash/atomik_alloc.c` | XOR-tagged bump allocator |
| `hardware/v3/soc/firmware/fw-flash/printf.h` | mini_printf header |
| `hardware/v3/soc/firmware/fw-flash/printf.c` | mini_printf (no div, powers-of-10 table) |
| `hardware/v3/soc/firmware/fw-flash/perf_bench_v3.c` | Performance benchmark suite (530 measurements) |
| `hardware/v3/soc/firmware/fw-flash/Makefile` | Build system (rv64i, -Os, -fno-builtin) |
| `hardware/v3/soc/firmware/fw-flash/linker_flash.ld` | Linker script |
| `hardware/v3/soc/firmware/fw-flash/crt_flash.S` | Startup code (RV64I) |

---

## Known Limitations

1. **CLS at 88%**: Tightest resource constraint. Future phases must avoid wide combinational fan-in.
2. **Fmax margin thin at +1.8%**: Production-safe but not generous. Decode pipeline optimization would help.
3. **Both PLLs used**: No PLL headroom for additional clock domains.
4. **SRAM 8 KB**: Heap limited to ~1 KB after firmware sections.
5. **No DMA**: ATOMiK operations are CPU-initiated. Custom instructions are fast enough that DMA is less critical than on v2.
6. **HDMI pixel clock on separate PLL**: Display MMIO requires CDC bridge (~5-6 cycle latency).

---

## Parallel Bank Architecture

The v3 parallel bank module (`atomik_v3_parallel.v`) supports N=1,2,4,8,16 banks with:

- **XOR merge tree**: O(log₂(N)) depth, no carry propagation
- **Shared BSRAM state table**: Constant 2 blocks regardless of N (vs per-bank in v2)
- **Round-robin distribution**: Sequential accum operations distributed across banks
- **Parallel input mode**: N independent delta ports for maximum throughput

| Banks | LUT | FF | BSRAM | Fmax (@ 67.5 MHz target) | Throughput |
|------:|----:|---:|:-----:|-------------------------:|-----------:|
| 1 | 472 | 489 | 2 | 100.5 MHz | 67.5 Mops/s |
| 4 | 737 | 683 | 2 | 90.4 MHz | 270.0 Mops/s |
| 8 | 1,126 | 940 | 2 | 70.5 MHz | 540.0 Mops/s |
| 16 | 1,778 | 1,453 | 2 | 70.5 MHz | **1,080.0 Mops/s** |

**Scaling**: Sub-linear LUT growth (3.8x for 16x throughput). Per-bit LUT efficiency: 1.36 LUT/bit/bank (v3) vs 2.71 (v2) — 50% more efficient.

---

**Repository:** [MatthewHRockwell/ATOMiK](https://github.com/MatthewHRockwell/ATOMiK)
**License:** Apache 2.0 (subject to patent notice)
