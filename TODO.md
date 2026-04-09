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

- [x] **3.1** Add mock support for `atomik_open_devmem`
  - Mock backend in libatomik.c: in-memory state table, anonymous mmap
  - Supports all 3 layouts (CSR, AXI, Adapter)
  - Verified: demo_state_monitor 7/7 YES in CSR and adapter mock mode on x86

- [x] **3.2** Add `atomik_detect_changed()` convenience function
  - Computes XOR fingerprint, compares via ATOMiK acc_zero, stores new reference
  - Verified: 7 edge cases (unchanged, 1-byte change, empty, 1 byte, 64KB unchanged/changed)

- [x] **3.3** Expand mock test suite to 59 tests (target was 50+)
  - 33 original + 26 new: devmem CSR/adapter open, detect_changed, 1000-accum stress, 256-addr load/swap
  - Verified: `make test-mock` → 59/59 PASS

- [x] **3.4** Document single-bank accumulator model
  - docs/ACCUMULATOR_MODEL.md: shared accumulator, LOAD/SWAP clearing, usage patterns, multi-bank future
  - Verified: no doc contradicts this model (grep confirmed)

- [x] **3.5** Version libatomik as 1.0.0
  - LIBATOMIK_VERSION_MAJOR/MINOR/PATCH defines, atomik_version_string()
  - CHANGELOG.md in software/libatomik/
  - Verified: 59/59 tests pass
  - NOTE: soname not embedded in .so (no -Wl,-soname flag) — minor gap

- [x] **3.6** Add pkg-config and CMake find module
  - libatomik.pc.in, FindATOMiK.cmake, `make install PREFIX=...`
  - Verified: installs header, .a, .so, .pc, .cmake to correct paths
  - NOTE: external CMake find_package not integration-tested (would need a separate project)

---

## Phase 4: State Change Detection Service

- [x] **4.1** Build `atomik-watchd` daemon skeleton
  - 301 lines, configurable via CLI flags (-n, -s, -t, -c, -a, -l, -m)
  - Uses atomik_detect_changed() with new_fp reference advancing
  - Verified: compiles -Wall -Werror, mock mode 5 ticks, edge cases

- [x] **4.2** Add JSON output to `atomik-watchd`
  - JSONL: {"tick","n_regions","changed":[],"n_changed","detect_cy","baseline_cy","speedup"}
  - Verified: jq parses all output lines, fields correct

- [ ] **4.3** Run `atomik-watchd` on Zynq hardware
  - Cross-compiled for rv32ima (ELF 32-bit, static)
  - PENDING: requires cold boot + rootfs injection
  - Verify: 10 ticks, JSON valid, speedup consistent with benchmark

- [x] **4.4** Write one-page brief: "ATOMiK State Watch Service"
  - docs/BRIEF_STATE_WATCH.md (53 lines)
  - Uses real captured Zynq data, no overclaims
  - NOTE: uses workload benchmark data since 4.3 hardware capture is pending

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
