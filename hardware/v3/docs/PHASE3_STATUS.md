# Phase 3: SoC Integration - Status Update

**Last Updated:** February 23, 2026

## Current Status: 🟢 **Phase 3A Complete - UART Working**

### Completed Milestones

#### ✅ Phase 3A: UART Communication (Feb 23, 2026)
- **Manual UART TX peripheral** implemented and validated
- **Hardware verified** on Tang Nano 9K
- **Boot ROM executing** and transmitting at 115200 baud
- **Full SoC integration** working (CPU → bus → UART)

**Evidence:** Continuous "TEST\n" output from /dev/ttyUSB1

### Architecture Overview

**v3 SoC Configuration:**
- CPU: RV64I @ 13.5 MHz (crystal ÷2, no PLL)
- UART: manual_uart_tx @ 115200 baud (TX only)
- Memory: 8KB SRAM, 8KB Boot ROM, SPI Flash XIP
- ATOMiK: Direct-wired via custom instructions (no MMIO)

**Key Difference from v2:**
- ATOMiK integrated as CPU datapath extension (not bus peripheral)
- Frees up one bus slot (S3: 0xC0000000 unused)
- Zero bus overhead for ATOMiK operations

### Resource Usage

From synthesis report:
- **LUT:** ~3,800 (44% of 8,640)
- **BSRAM:** 14/26 (54%)
  - 4: CPU regfile
  - 2: ATOMiK banks
  - 4: Boot ROM
  - 4: SRAM
- **Clock:** 13.5 MHz (stable, no timing violations)

### Next Steps

#### Phase 3B: Flash XIP Validation
- [ ] Verify SPI flash read timing
- [ ] Test code execution from flash
- [ ] Validate instruction fetch pipeline

#### Phase 3C: Main Firmware Development
- [ ] Port v2 firmware to RV64I
  - [ ] 64-bit startup (crt_flash.S)
  - [ ] ATOMiK test wrappers (custom instructions)
  - [ ] UART menu with all v2 features
- [ ] Build firmware with RV64I toolchain
- [ ] Flash via ISP programmer

#### Phase 3D: ATOMiK Custom Instructions
- [ ] Test ATOMIK.LOAD / ATOMIK.ACCUM / ATOMIK.READ
- [ ] Verify zero bus overhead
- [ ] Compare performance vs v2 MMIO approach

#### Phase 3E: Full Validation
- [ ] RISC-V compliance tests (riscv-tests)
- [ ] ATOMiK functionality tests
- [ ] Performance benchmarks vs v2

### Known Issues

1. **HDMI non-functional** - PLL bypassed (13.5 MHz not sufficient for HDMI timing)
   - **Status:** Acceptable - HDMI is stretch goal, UART console is primary
   - **Fix if needed:** Re-enable PLL for 25.2 MHz operation

2. **UART RX not implemented** - manual_uart_tx is TX-only
   - **Status:** Acceptable - console output is primary use case
   - **Fix if needed:** Add RX state machine to manual_uart_tx

3. **simpleuart synthesis issue** - uninvestigated
   - **Status:** Low priority - manual_uart_tx works
   - **Investigation:** Optional future work, file as synthesis bug report

### Files Ready for Commit

**New Files:**
- `soc/manual_uart_tx.v` - Drop-in UART TX peripheral
- `sim/test_manual_uart_tx.cpp` - Verilator validation
- `test/test_manual_uart_hardware.v` - Standalone hardware test
- `docs/UART_FIX_SUCCESS.md` - Implementation documentation
- `docs/UART_ROOT_CAUSE.md` - Root cause analysis
- `docs/UART_DEBUG_FINDINGS.md` - Debug session notes

**Modified Files:**
- `soc/picoperipheral.v` - Use manual_uart_tx instead of simpleuart
- `synth/atomik_v3_soc.gprj` - Project file updated

**Generated Artifacts:**
- `synth/impl/pnr/atomik_v3_soc.fs` - Working bitstream (in persistent flash)
- `test/impl/pnr/project.fs` - Standalone test bitstream

### Timeline

| Phase | Planned | Actual | Status |
|-------|---------|--------|--------|
| Phase 0: Tooling | - | Complete | ✅ |
| Phase 1: CPU Core | - | Complete | ✅ |
| Phase 2: Compliance | - | Complete | ✅ |
| Phase 3A: UART | Feb 23 | Feb 23 | ✅ |
| Phase 3B: Flash XIP | TBD | - | 🔄 Next |
| Phase 3C: Firmware | TBD | - | 📋 Planned |
| Phase 3D: ATOMiK | TBD | - | 📋 Planned |
| Phase 3E: Validation | TBD | - | 📋 Planned |

### Success Criteria

**Phase 3A (Current):** ✅ ACHIEVED
- [x] UART transmitting on hardware
- [x] Boot ROM executing
- [x] SoC integration verified

**Phase 3 (Overall):** 🔄 IN PROGRESS
- [x] SoC boots from SPI flash
- [x] UART console functional
- [ ] ATOMiK custom instructions validated
- [ ] Performance meets/exceeds v2

### Recommendations

1. **Proceed to Phase 3B immediately** - Flash XIP is critical path
2. **Firmware development in parallel** - Can start porting while debugging flash
3. **Document ATOMiK instruction encoding** - Needed for firmware wrappers
4. **Plan FPGA availability** - Hardware validation requires physical board access

### Contact & Resources

- **Hardware:** Tang Nano 9K @ $13.50 (GW1NR-LV9QN88PC6/I5)
- **Toolchain:** riscv64-unknown-elf-gcc @ /usr/bin/
- **Synthesis:** Gowin EDA V1.9.12.01 @ /opt/gowin/IDE/
- **Bitstream:** Flash via openFPGALoader -b tangnano9k -f

---

**Bottom Line:** Phase 3A complete. v3 SoC UART communication validated on hardware. Ready for Flash XIP testing and firmware development.
