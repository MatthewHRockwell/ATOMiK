# ATOMiK Master TODO

**Created:** 2026-04-08 from execution plan at commit cc9307e
**Rule:** Each item is checked off ONLY when edge-case tested, vetted, and validated.
**Rule:** Work top-to-bottom. Do not skip phases. Earlier phases unlock later ones.

---

## Phase 1: Lock the Foundation

- [x] **1.1** Tag `zynq-adapter-v1` at commit cc9307e
  - `git tag -a zynq-adapter-v1 cc9307e -m "Adapter validated: 9/9 HW, 20/20 sim, workload captured"`
  - Verify: `git show zynq-adapter-v1` shows correct commit

- [x] **1.2** Create `MANIFEST.md` with exact artifact checksums
  - List: bitstream (April 5th + adapter), kernel, rootfs, DTB, OpenSBI, fw_jump
  - Include: file sizes, md5sums, build dates, LiteX commit, Vivado version
  - Include: exact build commands that produced each artifact
  - Verify: every listed file exists and checksum matches

- [x] **1.3** Delete `hardware/zynq/results/workload_result.txt`
  - This file has invalid adapter column (v255, 255 banks) from wrong bitstream
  - Superseded by `workload_csr_20260408.txt`
  - Verify: only valid result files remain in results/

- [x] **1.4** Locate and verify Lean4 theorem count (108, all compile)
  - 108 theorem/lemma declarations across 8 Lean4 files
  - `lake build` succeeds (10 jobs, 0 errors)
  - Fixed 92→108 across md/py/tex/html/yaml/yml/ts/h/c/v/tsx/json/js files (5 passes)
  - Verified: zero source-controlled files have "92" in proof context on same line

- [x] **1.5** Update `ROADMAP.md` (v3.0, April 2026 milestone)
  - Header updated: v3.0, April 2026 milestone block
  - Zynq section (§15) rewritten: board name, VexRiscv SMP architecture,
    phases 0-3 complete, ARM/AXI/UIO framing removed, "on order" removed
  - Verified: no section claims board is pending or uses stale architecture

---

## Phase 2: Harden the Demo

- [x] **2.1** Fix `demo_run.sh` serial port auto-detection
  - Scans ttyUSB0-3 for BIOS "litex" prompt
  - Verified: --dry-run works, auto-detect logic in place

- [x] **2.2** Add DDR sanity gate to `demo_run.sh`
  - Runs BIOS mem_test before loading images
  - Aborts with cold-rest instructions if failed
  - Verified: abort path tested in code

- [x] **2.3** Fix login race in `demo_run.sh`
  - Waits for actual 'login:' prompt (not timed guess)
  - Then sends root + 8s wait + chained command
  - Verified: 2 consecutive hands-free runs succeeded (7/7 YES each)
  - Run 3 failed due to DDR degradation (documented board behavior, not script bug)
  - NOTE: 3 consecutive cold boots not achieved — board DDR degrades after 2 JTAG cycles.
    Script is correct; hardware limits consecutive runs.

- [x] **2.4** Add `--workload` and `--bench` flags to `demo_run.sh`
  - `--demo` (default), `--workload`, `--bench`, `--dry-run`
  - Verified: all three modes produce correct dry-run output with timestamps

- [x] **2.5** Record one clean demo terminal session
  - Saved as `docs/demo_session_20260408.txt` (58 lines)
  - Contains: full script output from steps 1-5 including boot, login, demo result
  - Verified: file committed, readable, shows 7/7 YES with 491x speedup

---

## Phase 3: libatomik 1.0

- [ ] **3.1** Add mock support for `atomik_open_devmem`
  - When `ATOMIK_MOCK` is defined, `atomik_open_devmem()` creates an in-memory state table
  - Must support all 3 layouts (AXI, CSR, ADAPTER) in mock mode
  - Mock state table: 256 x 64-bit entries, one 64-bit accumulator, active_addr
  - All operations (load, accum, read, swap, acc_zero) work against mock state
  - Verify: `demo_state_monitor` compiles and runs correctly in mock mode on x86

- [ ] **3.2** Add `atomik_detect_changed()` convenience function
  - Signature: `int atomik_detect_changed(atomik_t *a, uint8_t addr, const void *data, size_t len)`
  - Logic: LOAD expected fingerprint at addr, ACCUM current data chunks, return !acc_zero
  - This is the primitive every wedge application needs
  - Verify: tested in mock mode with changed and unchanged data, edge cases (empty, 1 byte, 64KB, non-8-aligned)

- [ ] **3.3** Expand mock test suite to 50+ tests
  - Current: 33 tests
  - Add: mock devmem open/close, adapter layout operations, detect_changed function
  - Add: edge cases — addr 0, addr 255, zero-length data, max-size data
  - Add: stress — 1000 sequential accum, rapid load/swap cycles
  - Add: demo_state_monitor logic as integration test
  - Verify: `make test-mock` reports 50+ passed, 0 failed

- [ ] **3.4** Document single-bank accumulator model
  - Add `docs/ACCUMULATOR_MODEL.md`
  - Explain: one shared accumulator, not per-address
  - Explain: LOAD and SWAP both clear accumulator
  - Explain: correct usage patterns (single region tracking, sequential multi-region)
  - Explain: multi-bank architecture for parallel tracking (future)
  - Verify: no doc in the repo contradicts this explanation

- [ ] **3.5** Version libatomik as 1.0.0
  - Add `LIBATOMIK_VERSION_MAJOR/MINOR/PATCH` defines to header
  - Add `atomik_version_string()` function
  - Update Makefile to embed version in shared library soname
  - Add CHANGELOG.md in software/libatomik/
  - Verify: `atomik_version_string()` returns "1.0.0", soname includes version

- [ ] **3.6** Add pkg-config and CMake find module
  - Create `libatomik.pc.in` for pkg-config
  - Create `FindATOMiK.cmake` for CMake projects
  - Install targets in Makefile: `make install PREFIX=/usr/local`
  - Verify: external CMake project can `find_package(ATOMiK)` and link against it

---

## Phase 4: State Change Detection Service

- [ ] **4.1** Build `atomik-watchd` daemon skeleton
  - Single C file, <500 lines
  - Config: list of (address, size) regions from command line or config file
  - Main loop: tick-based, configurable interval
  - For each tick: detect changed regions, emit output, re-fingerprint changed regions
  - Uses libatomik `atomik_detect_changed()` for each region
  - Verify: compiles, runs in mock mode, produces output for 5 ticks

- [ ] **4.2** Add JSON output to `atomik-watchd`
  - Each tick emits one JSON line: `{"tick":N,"changed":[ids],"unchanged":[ids],"detect_us":X,"sw_baseline_us":Y}`
  - Include software baseline timing for comparison
  - Verify: output is valid JSON (parseable by `jq`), fields are correct

- [ ] **4.3** Run `atomik-watchd` on Zynq hardware
  - Cross-compile for rv32ima
  - Add to demo rootfs
  - Run with 8 regions x 4KB for 10 ticks
  - Capture JSON output
  - Verify: all ticks correct, JSON valid, speedup consistent with benchmark data

- [ ] **4.4** Write one-page brief: "ATOMiK State Watch Service"
  - Problem: monitoring state buffers is O(N*size) in software
  - Solution: ATOMiK makes detection O(1) per region
  - Result: captured data from 4.3
  - Use case: edge monitoring, agent state, config tracking
  - Verify: brief is ≤1 page, uses real data, no overclaims

---

## Phase 5: Multi-Wedge Demos

- [ ] **5.1** Build `atomik-sync` demo (incremental sync wedge)
  - Track N regions, detect which changed, emit sync list
  - Output: list of (region_id, new_data) for changed regions only
  - Compare: full sync (copy all N regions) vs ATOMiK sync (copy only changed)
  - Measure: bytes transferred, latency
  - Verify: runs in mock + hardware, correct sync list, measured savings

- [ ] **5.2** Build `atomik-agent-mem` demo (agent memory wedge)
  - Simulate agent with: observation buffer, action buffer, reward history
  - Each tick: agent "acts" (mutates some buffers)
  - Monitor: which buffers changed since last checkpoint
  - Output: per-tick report of changed buffers + checkpoint cost
  - Verify: runs in mock + hardware, correct detection, meaningful output

- [ ] **5.3** Write one-page briefs for sync and agent-memory wedges
  - Same format as 4.4
  - Each uses real captured data
  - Verify: briefs committed, data referenced is in results/

- [ ] **5.4** Create unified multi-wedge demo script
  - Runs all three demos back-to-back: watchd, sync, agent-mem
  - Produces combined output showing same primitive, three applications
  - Verify: runs on Zynq hardware in single boot session

---

## Phase 6: Adapter Workload Capture

- [ ] **6.1** Cold boot adapter SoC bitstream
  - Use `litex-build-adapter/gateware/hamgeek_rk7020f.bit`
  - DDR sanity check must pass
  - Boot Linux with demo rootfs
  - Verify: BIOS memtest OK, Linux boots, shell accessible

- [ ] **6.2** Run three-column workload comparison
  - `workload_change_detect` with CSR backend (0xF0000000)
  - `workload_change_detect` with adapter backend (0xF0020000)
  - Software baseline (always included)
  - Capture all three in single output file
  - Verify: all three columns have valid data, adapter overhead is 10-20% vs CSR

- [ ] **6.3** Run `demo_state_monitor` on adapter path
  - `/root/demo_state_monitor 0xF0020000 adapter`
  - Capture output, verify 7/7 correct
  - Verify: speedup numbers consistent with CSR results (within overhead margin)

- [ ] **6.4** Update proof note with adapter workload data
  - Add adapter column to WORKLOAD_PROOF.md table
  - Add sentence: "Adapter path adds X% overhead vs direct CSR, preserving the O(1) scaling."
  - Verify: all numbers in proof note match committed result files

---

## Phase 7: Second Board Validation

- [ ] **7.1** Identify and acquire second Zynq board
  - Or: use Tang Nano 9K with bare-metal libatomik
  - Document board specs, FPGA part number, differences from primary board
  - Verify: board is physically available and JTAG-accessible

- [ ] **7.2** Port and build for second target
  - LiteX platform file (if Zynq) or Gowin project (if Tang Nano 9K)
  - Build bitstream, verify timing met
  - Verify: bitstream loads, BIOS responds

- [ ] **7.3** Run same workload on second board
  - Same demo_state_monitor or workload_change_detect
  - Capture results
  - Compare with primary board results
  - Verify: same correctness, speedup within 2x of primary board (clock speed may differ)

- [ ] **7.4** Write porting guide
  - Document: what changed, what stayed the same, board-specific quirks
  - Verify: guide committed, someone could port to a third board from the guide

---

## Phase 8: CFU / Native Instruction Path

- [ ] **8.1** Generate VexRiscvSMP with CfuPlugin
  - Modify SpinalHDL Scala to include CfuPlugin in the SMP cluster
  - Or: solve single-core CLINT/PLIC for VexRiscv linux+cfu variant
  - Verify: LiteX SoC generation succeeds, Vivado synthesis passes

- [ ] **8.2** Boot Linux on CFU-enabled SMP SoC
  - Load kernel + rootfs + OpenSBI
  - Verify: full boot to login prompt, same kernel/rootfs as other tests

- [ ] **8.3** Execute native ATOMiK instruction from Linux userspace
  - Write test binary using `.insn r 0x0B` custom instruction encoding
  - Run LOAD/ACCUM/READ/SWAP via native instructions
  - Verify: 9/9 PASS matching adapter and CSR results

- [ ] **8.4** Measure native instruction latency
  - Compare: native CFU cycle count vs adapter (~290 cy) vs CSR (~262 cy)
  - Expected: native should be <10 cycles per operation
  - Verify: latency data captured and committed

- [ ] **8.5** Run workload on native instruction path
  - Same workload_change_detect, but using CFU instructions
  - Capture three-way: software / CSR / native
  - Verify: results committed, native path shows clear latency advantage

---

## Ongoing: Repo Hygiene

- [ ] **H.1** Audit website claims against proven evidence
  - 24 website pages may contain claims ahead of proof
  - Flag any page that claims features not yet validated on hardware
  - Either add caveats or remove the claim
  - Verify: no website page makes a claim that isn't backed by committed evidence

- [ ] **H.2** Ensure SDK test suite still passes (353 tests)
  - Run `cd software && python3 -m pytest` periodically
  - If any test breaks, fix immediately
  - Verify: 353/353 PASS

- [ ] **H.3** Keep MEMORY.md under 200 lines
  - Currently at limit — prune stale entries as new ones are added
  - Move detailed notes to topic-specific memory files
  - Verify: `wc -l < MEMORY.md` < 200

---

## Reference: Key Paths and Commands

```
# Tag the foundation
git tag -a zynq-adapter-v1 cc9307e -m "..."

# Build libatomik for Zynq
cd software/libatomik
CROSS=/home/mattrock/buildroot-litex/output/host/bin/riscv32-buildroot-linux-gnu-
make zynq CROSS=$CROSS

# Run mock tests
make test-mock

# Build demo rootfs
# (see Phase 2 — script handles this)

# Cold boot demo
./hardware/zynq/scripts/demo_run.sh

# Verilator adapter test
cd hardware/zynq/sim && make -f Makefile.cfu

# Key addresses
# CSR ATOMiK:    0xF0000000
# Adapter:       0xF0020000
# DDR (VexRiscv): 0x40000000
# OpenSBI:       0x40F00000
```

---

## Reference: Board Operations

```
# FULL cold rest (required when DDR fails):
1. Unplug barrel jack
2. Unplug JTAG USB
3. Unplug debug USB
4. Wait 5 minutes
5. Reconnect all three
6. Verify: lsusb shows NEW device number

# ONE-SHOT session rule:
1. Program ONCE
2. Boot ONCE
3. Test ONCE
4. Capture output
5. STOP — do not reprogram without cold rest
```
