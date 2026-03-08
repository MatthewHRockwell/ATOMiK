# Xilinx Zynq-7000 Reference Documentation

Quick reference for ATOMiK hardware development targeting ALINX AX7020 (XC7Z020).

## Target Hardware

| Specification | Value |
|---------------|-------|
| **Device** | XC7Z020-2CLG400I |
| **Family** | Zynq-7000 (Artix-7 fabric + dual Cortex-A9) |
| **Package** | CLG400 (400-ball BGA) |
| **Speed Grade** | -2 (industrial, -40 to +100C) |
| **LUT6** | 53,200 |
| **Flip-Flops** | 106,400 |
| **BRAM (36Kb)** | 140 (4.9 Mb total) |
| **DSP48E1** | 220 |
| **MMCM** | 4 |
| **PLL** | 2 |
| **PS DDR3** | 1 GB (32-bit, on-board) |
| **Board** | ALINX AX7020 |
| **PS Clock** | 33.333 MHz (PS_CLK input) |
| **PL Clock** | 50 MHz (from PS FCLK_CLK0, configurable) |

---

## Quick Reference Files

| File | Contents |
|------|----------|
| [AX7020_BOARD_REFERENCE.md](AX7020_BOARD_REFERENCE.md) | Board overview, I/O map, power, comparison to Tang Nano 9K |
| [ZYNQ_PS_CONFIGURATION.md](ZYNQ_PS_CONFIGURATION.md) | ARM PS architecture, DDR3, MIO, PS-PL interfaces, boot modes |
| [AXI_INTEGRATION_GUIDE.md](AXI_INTEGRATION_GUIDE.md) | AXI4-Lite wrapper for ATOMiK, register map, UIO driver |
| [CLOCK_REFERENCE.md](CLOCK_REFERENCE.md) | MMCM/PLL configuration, frequency planning, CDC |
| [RESOURCE_BUDGET_GUIDE.md](RESOURCE_BUDGET_GUIDE.md) | PL resource inventory, ATOMiK scaling projections, scenario planning |
| [VIVADO_BUILD_GUIDE.md](VIVADO_BUILD_GUIDE.md) | Vivado project setup, TCL flow, bitstream generation |
| [LINUX_SETUP_GUIDE.md](LINUX_SETUP_GUIDE.md) | PetaLinux, Ubuntu, device tree, UIO, cross-compilation |

---

## Official Xilinx Documentation Sources

| Topic | Document ID | Title | Key Sections |
|-------|-------------|-------|--------------|
| Zynq TRM | UG585 | Zynq-7000 SoC Technical Reference Manual | PS architecture, AXI, boot |
| Device Data | DS187 | Zynq-7000 SoC Data Sheet: Overview | Resource counts, speed grades |
| DC/AC Specs | DS191 | Zynq-7000 SoC Data Sheet: DC and AC Switching | Timing parameters |
| Clocking | UG472 | 7 Series FPGAs Clocking Resources User Guide | MMCM, PLL, BUFG |
| Memory | UG473 | 7 Series FPGAs Memory Resources User Guide | BRAM, FIFO |
| SelectIO | UG471 | 7 Series FPGAs SelectIO Resources User Guide | IOSTANDARD, LVDS, OSERDESE2 |
| Vivado Design | UG903 | Vivado Design Suite User Guide: Using Constraints | XDC syntax |
| Vivado TCL | UG894 | Vivado Design Suite User Guide: Using Tcl Scripting | TCL flow |
| PetaLinux | UG1144 | PetaLinux Tools Documentation Reference Guide | Linux build flow |
| IP Integrator | UG994 | Vivado Design Suite User Guide: Designing IP Subsystems Using IP Integrator | Block design |
| AXI Reference | -- | AMBA AXI and ACE Protocol Specification | AXI4-Lite protocol |

---

## ATOMiK-Specific Configuration

### Planned PL Architecture

ATOMiK operates as an AXI4-Lite peripheral in the PL fabric. The ARM Cortex-A9 PS runs Linux and accesses ATOMiK through memory-mapped I/O via the GP AXI port.

```
PS (ARM Cortex-A9)                    PL (Artix-7 Fabric)
 ┌──────────────┐                     ┌──────────────────────┐
 │  Linux       │                     │  ATOMiK Multi-Bank   │
 │  UIO Driver  │──── GP AXI M0 ────▶│  AXI4-Lite Wrapper   │
 │  Userspace   │                     │  MMCM Clock Gen      │
 └──────────────┘                     └──────────────────────┘
```

### Register Map

| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| 0x00 | LOAD_ADDR | W | Set active address (addr[7:0]) |
| 0x04 | LOAD_DATA_LO | W | Initial state bits [31:0] |
| 0x08 | LOAD_DATA_HI | W | Initial state bits [63:32], triggers LOAD |
| 0x0C | ACCUM_LO | W | Delta bits [31:0] |
| 0x10 | ACCUM_HI | W | Delta bits [63:32], triggers ACCUM |
| 0x14 | STATE_LO | R | Current state bits [31:0] |
| 0x18 | STATE_HI | R | Current state bits [63:32] |
| 0x1C | STATUS | R | {acc_zero, n_banks[7:0], version[7:0]} |
| 0x20 | SWAP_ADDR | W | Set swap address, triggers SWAP |
| 0x24 | CONFIG | R/W | {enable, bank_select[7:0]} |

Full protocol details: [AXI_INTEGRATION_GUIDE.md](AXI_INTEGRATION_GUIDE.md)

### Constraints File

| File | Purpose |
|------|---------|
| `hardware/zynq/constraints/ax7020.xdc` | Physical pin assignments + timing constraints |

---

## Tool Requirements

| Tool | Version | Purpose |
|------|---------|---------|
| Vivado | 2023.2+ (WebPACK) | Synthesis, implementation, programming (free for XC7Z020) |
| PetaLinux | 2023.2+ | Linux BSP, device tree, kernel build |
| arm-linux-gnueabihf-gcc | 9+ | Cross-compilation for ARM Cortex-A9 |
| Icarus Verilog | Any recent | PL simulation |
| Verilator | Any recent | PL linting |
| GTKWave | Any recent | Waveform viewing |

### Vivado WebPACK Installation

1. Download from [AMD/Xilinx Downloads](https://www.xilinx.com/support/download.html)
2. Install WebPACK edition (free, no license file required for XC7Z020)
3. Source environment: `source /tools/Xilinx/Vivado/2023.2/settings64.sh`
4. Verify: `vivado -version`

---

## Comparison: Tang Nano 9K vs. ALINX AX7020

| Resource | Tang Nano 9K (GW1NR-9) | ALINX AX7020 (XC7Z020) | Ratio |
|----------|:---------------------:|:----------------------:|:-----:|
| **LUT** | 8,640 (LUT4) | 53,200 (LUT6) | **6.2x** (effective ~12x) |
| **Flip-Flops** | 6,480 | 106,400 | **16.4x** |
| **BRAM** | 26 x 18Kb (468 Kb) | 140 x 36Kb (4,900 Kb) | **10.5x** |
| **DSP** | 20 (pMAC18) | 220 (DSP48E1) | **11x** |
| **PLL/MMCM** | 2 PLL | 4 MMCM + 2 PLL | **3x** |
| **CPU** | None (soft PicoRV32) | Dual Cortex-A9 @ 667 MHz | Hard CPU |
| **Memory** | 8 KB SRAM | 1 GB DDR3 | **131,072x** |
| **OS** | Bare-metal | Linux (Ubuntu) | Full OS |
| **Bus** | Wishbone | AXI4 | Industry standard |
| **Price** | $13.50 | $195 | 14.4x |

The LUT6 on Artix-7 can implement any function of 6 inputs (vs. 4 on Gowin), making each LUT effectively ~2x more capable. Combined with 6.2x more LUT count, the effective logic capacity is roughly 12x greater.

---

## Related Project Documentation

| Document | Location |
|----------|----------|
| Zynq Port Architecture Spec | `specs/zynq_port.md` |
| Zynq Port Task List | `specs/zynq_port_tasks.md` |
| ATOMiK v3 Architecture Spec | `specs/atomik_v3.md` |
| ATOMiK v3 Core RTL | `hardware/v3/rtl/atomik_v3_atomik.v` |
| Parallel Banks RTL | `hardware/v3/rtl/atomik_v3_parallel.v` |
| Zynq Hardware Directory | `hardware/zynq/` |
| Gowin Reference Docs | `docs/reference/gowin/` |
| Project Roadmap | `ROADMAP.md` (Section 15) |

---

*Last Updated: March 7, 2026*
