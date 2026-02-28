# Phase 3D: Complete Firmware Port & Flash Deployment

**Date:** February 24, 2026  
**Status:** Ready to start - Phase 3C complete (UART stable at 115200 baud)  
**Dependencies:** Phase 3C ✅ (RV64I CPU + stable UART communication)

---

## Current Position

### What We Have ✅
- **Hardware:** 100% stable v3 SoC at 115200 baud (27 MHz crystal direct)
- **Boot ROM:** BRINGUP firmware working (continuous 'T' transmission test)
- **UART:** Fully functional, 20/20 tests passed
- **Synthesis:** 5,594 LUT (65%), 16 BSRAM (62%), zero TNS, Fmax 25.3 MHz
- **Deployment:** SRAM bitstream loading works perfectly

### What We Need ✅
- **Complete ISP flasher** - Boot ROM firmware with UART flash programming
- **Flash deployment** - Persistent bitstream + firmware to SPI flash
- **Full v2 firmware port** - UART menu + ATOMiK test suite in RV64I
- **Hardware validation** - All tests passing on real FPGA

---

## Phase 3D Tasks (From specs/atomik_v3_tasks.md Section 3.9)

### Task 1: Complete ISP Flasher (Boot ROM)
**Goal:** Port v2's ISP flasher to RV64I and restore Boot ROM boot flow

**Current state:**
- `hardware/v3/soc/firmware/fw-brom/isp_flasher.c` exists
- Has BRINGUP_MODE (working) and ISP mode (needs completion)
- RV64I build system working (Makefile with -DBRINGUP_MODE)

**Steps:**
1. **Review v2 ISP flasher protocol:**
   - Location: `/home/mattrock/Projects/TangNano-9K-example/picotiny/brom/isp_flasher.c`
   - Protocol: Receive firmware over UART, write to SPI flash via MMIO
   - Format: Simple protocol with start marker, length, data, checksum

2. **Port ISP protocol to RV64I:**
   - Update for 64-bit pointers (`uint64_t` addresses)
   - Keep 32-bit SPI flash MMIO (unchanged from v2)
   - Port `pico-programmer.py` client for RV64I address format
   
3. **Remove BRINGUP_MODE, enable ISP mode:**
   - Remove `-DBRINGUP_MODE` from Makefile
   - Restore full ISP flasher main loop
   - Boot sequence: Reset → Boot ROM @ 0x80000000 → Wait for ISP or timeout → Jump to flash @ 0x00000000

4. **Test ISP flasher in simulation:**
   - Verilator testbench: simulate UART RX, verify flash write
   - Verify timeout logic works (jumps to flash after no ISP command)

**Exit criteria:** Boot ROM can receive firmware over UART and write to SPI flash. Timeout jumps to flash correctly.

---

### Task 2: Port Flash Firmware (Main Application)
**Goal:** Port v2 UART menu + ATOMiK tests to RV64I

**Current state:**
- `hardware/v3/soc/firmware/fw-flash/` exists but incomplete
- v2 reference: `/home/mattrock/Projects/TangNano-9K-example/picotiny/fw-flash/`

**Steps:**
1. **Port firmware infrastructure:**
   - `crt_flash.S`: 64-bit startup code (sd/ld, stack setup, BSS clear)
   - `linker_flash.ld`: elf64-littleriscv, flash @ 0x00000000, RAM @ 0x40000000
   - `printf.c`: 64-bit print_hex (16 digits), print_dec (no multiply)
   - `atomik_v3.h`: Custom instruction wrappers (`.insn r 0x0B, funct3, rd, rs1, rs2`)

2. **Port UART menu system:**
   - Menu loop: Read command character, dispatch to test function
   - Commands: 'H' (help), 'X' (ATOMiK test), 'M' (memory test), 'B' (benchmark), etc.
   - Banner: Display on boot (board name, clock freq, build date)

3. **Port ATOMiK tests (9 tests from v2):**
   - Test 1: LOAD → verify acc cleared, addr set
   - Test 2: ACCUM → verify acc updated
   - Test 3: READ → verify state reconstruction
   - Test 4: XOR cancel → delta ⊕ delta = 0
   - Test 5: Multi-delta → multiple ACCUM operations
   - Test 6: 64-bit patterns → test all 64 bits
   - Test 7: SWAP → context switch test
   - Test 8: Post-swap ACCUM → verify acc preserved
   - Test 9: Performance → cycle count measurement
   
   Use custom instruction wrappers:
   ```c
   static inline uint64_t atomik_read(void) {
       uint64_t result;
       asm volatile (".insn r 0x0B, 0x2, %0, x0, x0" : "=r"(result));
       return result;
   }
   ```

4. **Build and verify size:**
   - Target: ≤16 KB (fits in flash easily)
   - Check: `.text` section size, verify no bloat from 64-bit

**Exit criteria:** Flash firmware builds, links correctly, all ATOMiK tests compile with custom instructions.

---

### Task 3: Flash Deployment (Persistent Storage)
**Goal:** Deploy bitstream + firmware to SPI flash, verify persistent boot

**Hardware required:** Tang Nano 9K with USB connection

**Steps:**
1. **Flash bitstream to persistent storage:**
   ```bash
   cd hardware/v3/deploy
   openFPGALoader -b tangnano9k -f atomik_v3_soc_direct_clk.fs
   ```
   **Note:** This writes to the same SPI flash that holds firmware!
   **Critical:** After bitstream flash, must re-flash firmware (bitstream erase wipes flash)

2. **Flash firmware via ISP:**
   ```bash
   cd hardware/v3/soc/firmware/fw-flash/build
   python3 ../../scripts/pico-programmer.py fw-flash.v /dev/ttyUSB1
   ```
   **Protocol:** 
   - Reset board (enter Boot ROM ISP mode)
   - Programmer sends firmware over UART
   - Boot ROM writes to flash
   - Boot ROM jumps to flash on completion

3. **Verify persistent boot:**
   - Power cycle board (unplug USB, wait, replug)
   - Boot ROM should timeout (no ISP command) and jump to flash
   - Flash firmware should print banner over UART
   - UART menu should be interactive

4. **Test menu commands:**
   - 'H': Help - verify menu displays
   - 'X': ATOMiK tests - run all 9 tests, verify PASS
   - Check for any failures or hangs

**Exit criteria:** Power cycle → banner prints → menu works → tests pass. No manual reload needed.

---

### Task 4: Hardware Validation
**Goal:** Verify all v2 functionality ported correctly to v3 on real hardware

**Test matrix:**

| Test | Description | Pass Criteria |
|------|-------------|---------------|
| Boot | Power-on → Banner | Banner prints within 2 seconds |
| Menu | 'H' command | Help text displays |
| ATOMiK LOAD | 'X' test 1 | "PASS" printed |
| ATOMiK ACCUM | 'X' test 2 | "PASS" printed |
| ATOMiK READ | 'X' test 3 | "PASS" printed |
| XOR cancel | 'X' test 4 | "PASS" printed |
| Multi-delta | 'X' test 5 | "PASS" printed |
| 64-bit | 'X' test 6 | "PASS" printed |
| SWAP | 'X' test 7 | "PASS" printed |
| Post-swap | 'X' test 8 | "PASS" printed |
| Performance | 'X' test 9 | Cycle count reported |
| GPIO | LED blink test | LEDs toggle |
| Persistence | Power cycle test | All tests pass after reboot |

**Debugging procedure:**
- If test fails: capture UART output, check for assertion messages
- If hang: check for infinite loop, bus timeout, or flash corruption
- If boot fails: verify bitstream + firmware flash order (bitstream first, then firmware)

**Exit criteria:** 12/12 tests pass on hardware. Power cycle test confirms persistence.

---

## Risk Mitigation

### Risk 1: Flash Corruption
**Symptom:** Bitstream flash erases firmware  
**Mitigation:** Always re-flash firmware after bitstream flash  
**Recovery:** Re-run ISP programmer, board recovers

### Risk 2: ISP Protocol Mismatch
**Symptom:** pico-programmer.py can't communicate with Boot ROM  
**Mitigation:** Test ISP in Verilator first, verify protocol byte-for-byte  
**Recovery:** Fix Boot ROM ISP code, re-synthesize, re-flash bitstream

### Risk 3: Custom Instruction Encoding Error
**Symptom:** ATOMiK tests hang or produce wrong results  
**Mitigation:** Cross-check `.insn r 0x0B` encoding against v3 spec  
**Recovery:** Fix atomik_v3.h wrappers, rebuild firmware, re-flash

### Risk 4: RV64I Porting Bugs
**Symptom:** Firmware crashes, hangs, or prints garbage  
**Mitigation:** Test each component in Verilator before hardware  
**Recovery:** Debug with printf, check for 32-bit/64-bit type mismatches

---

## Success Metrics

### Phase 3D Complete When:
- [x] ISP flasher protocol ported and working
- [x] Flash firmware ported (UART menu + 9 ATOMiK tests)
- [x] Bitstream + firmware flashed to persistent storage
- [x] Power cycle test: boots without manual reload
- [x] All 9 ATOMiK tests pass on real hardware
- [x] GPIO tests pass (LED blink)
- [x] Documentation updated (KNOWN_ISSUES.md, Phase Context)

### Then Phase 3 is COMPLETE:
- **specs/atomik_v3_tasks.md** Section 3.9 checkboxes all marked
- **docs/ATOMiK_v3_Phase_Context_Template.md** updated for Phase 4
- **Git tag:** `v3-phase3-complete`
- **Ready for:** Phase 4 (Display Pipeline) or Phase 5 (I/O Streaming)

---

## Timeline Estimate

| Task | Estimated Time | Complexity |
|------|----------------|------------|
| Task 1: ISP flasher port | 2-4 hours | Medium (protocol straightforward) |
| Task 2: Flash firmware port | 3-6 hours | Medium (mostly copy-paste + custom inst wrappers) |
| Task 3: Flash deployment | 1-2 hours | Low (hardware access required) |
| Task 4: Hardware validation | 1-2 hours | Low (test execution + debugging) |
| **Total** | **7-14 hours** | **Phased over 1-2 days** |

**Parallel opportunities:** Task 1 and Task 2 can be worked on concurrently (different source files).

---

## Next Steps (Immediate)

1. **Review v2 ISP flasher** - Read `/home/mattrock/Projects/TangNano-9K-example/picotiny/brom/isp_flasher.c`
2. **Start Task 1** - Port ISP protocol to RV64I in `hardware/v3/soc/firmware/fw-brom/isp_flasher.c`
3. **Test in Verilator** - Verify ISP protocol before touching hardware
4. **Proceed to Task 2** - Once ISP works in sim, start flash firmware port

**Recommendation:** Start with Task 1 (ISP flasher), get it working in Verilator, then move to hardware testing.

---

## References

- **v2 ISP flasher:** `/home/mattrock/Projects/TangNano-9K-example/picotiny/brom/isp_flasher.c`
- **v2 flash firmware:** `/home/mattrock/Projects/TangNano-9K-example/picotiny/fw-flash/`
- **v2 pico-programmer:** `/home/mattrock/Projects/TangNano-9K-example/picotiny/pico-programmer.py`
- **v3 task list:** `specs/atomik_v3_tasks.md` Section 3.9
- **Phase context:** `docs/ATOMiK_v3_Phase_Context_Template.md`
- **Phase 3C success:** `hardware/v3/deploy/PHASE3C_SUCCESS.md`

---

## Dependencies

**Required before starting:**
- [x] Phase 3C complete (UART stable) ✅
- [x] v3 SoC synthesis working ✅
- [x] BRINGUP firmware working ✅
- [x] Bitstream deployment to SRAM working ✅

**Required for completion:**
- [ ] Physical Tang Nano 9K board with USB connection
- [ ] `pico-programmer.py` ported for RV64I
- [ ] All v2 test code ported to custom instructions

**Blocked by:** None - all prerequisites met!

---

## Status: READY TO START

Phase 3C gave us a stable foundation. Now we complete the firmware port and deploy to persistent flash.

**Recommended approach:** Start Task 1 (ISP flasher) immediately, test in Verilator, then proceed to hardware.
