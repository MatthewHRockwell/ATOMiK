# ATOMiK ASIC — Foundry Engagement Notes

## SRAM Compiler Swap Procedure

The ASIC RTL uses **behavioral SRAM models** that must be replaced with foundry-specific macros before tapeout.

### Files to Swap

| Behavioral Model | Usage | Size | Replacement |
|---|---|---|---|
| `rtl/atomik_sram_dp.v` | State table (256×64-bit dual-port) | 2 KB | Foundry dual-port SRAM macro |
| `rtl/atomik_sram_sp.v` | Reserved for future use | Parameterized | Foundry single-port SRAM macro |

### What to Hand the Foundry

When engaging Silicon Catalyst, Efabless, or a foundry shuttle (e.g., TSMC/GlobalFoundries MPW):

1. **This RTL package** (`hardware/asic/rtl/`) — all vendor-neutral, lint-clean
2. **SDC constraints** (`constraints/atomik_asic.sdc`) — portable across Synopsys DC, Cadence Genus, OpenSTA
3. **OpenLane config** (`openlane/config.json`) — ready for Sky130 trial synthesis
4. **BIST testbench** (`sim/tb_asic_top.v`) — 10/10 self-test derived from Lean4 proofs

Then: **swap `atomik_sram_dp` instantiation in `atomik_core_asic.v` for their SRAM compiler output.**

### Swap Steps

1. Run the foundry's SRAM compiler to generate a 256×64-bit dual-port macro (1R1W)
   - Port A: write-only (clk, we, waddr[7:0], wdata[63:0])
   - Port B: read-only (clk, re, raddr[7:0], rdata[63:0])
   - Read latency: 1 or 2 cycles (set `READ_LATENCY` parameter accordingly)

2. Replace the `atomik_sram_dp` instantiation in `atomik_core_asic.v` (line ~85) with the foundry macro name and port mapping

3. Add the foundry `.lib` / `.lef` / `.gds` files to the OpenLane config

4. Update `atomik_asic.sdc` timing constraints (Section 6) with actual SRAM Tco/Tsetup from the foundry datasheet

5. Re-run BIST testbench to verify — all 10 tests must still pass

### READ_LATENCY Parameter

The core's FSM adapts to SRAM read latency:
- **READ_LATENCY=1**: SRAM output available 1 cycle after `re` assertion (typical for register-file-style macros)
- **READ_LATENCY=2**: SRAM has output register (typical for compiled SRAMs with DOB_REG). Core adds an extra wait state automatically.

### SRAM Specifications for Foundry Request

```
Type:           Dual-port (1 read, 1 write), synchronous
Word depth:     256
Word width:     64 bits
Total capacity: 2 KB (16,384 bits)
Read ports:     1 (synchronous, registered output preferred)
Write ports:    1 (synchronous, byte-enable NOT required)
Clock:          Single clock domain
Access time:    Target < 2ns (for 200 MHz+ operation)
```

### Sky130 Trial (OpenLane)

For pre-tapeout validation without a commercial foundry:
- `make setup` installs OpenLane 2 + Sky130 PDK
- `make synth` runs full RTL-to-GDSII with behavioral SRAM (inferred as flip-flops — area will be large but functionally correct)
- Sky130 OpenRAM macros can replace behavioral SRAM for realistic area estimates

### Gate Count Reference

Yosys generic synthesis (excluding SRAM): ~2,800 logic cells
- Core FSM + datapath: ~1,200 cells
- BIST controller: ~800 cells
- DFT wrapper + JTAG TAP: ~500 cells
- Reset synchronizer + glue: ~300 cells

With foundry SRAM macro, total silicon area is dominated by the 2 KB SRAM, not logic.
