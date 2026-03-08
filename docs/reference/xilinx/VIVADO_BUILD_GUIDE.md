# Vivado Build Guide -- Project Setup and Build Flow for AX7020

**Date:** March 7, 2026
**Status:** Pre-board (from Xilinx documentation and Vivado tooling reference)
**Target:** ALINX AX7020 (XC7Z020-2CLG400I)
**Audience:** Developers building ATOMiK bitstreams and Zynq PS+PL designs in Vivado

**Sources:** Xilinx UG894 (Vivado TCL Scripting), UG903 (Using Constraints), UG994 (IP Integrator), UG1157 (Power Analysis), Vivado 2023.2 Release Notes

---

## Table of Contents

1. [Vivado WebPACK Installation](#1-vivado-webpack-installation)
2. [Non-Project Mode TCL Flow](#2-non-project-mode-tcl-flow)
3. [Block Design (IPI) Flow](#3-block-design-ipi-flow)
4. [Bitstream Programming](#4-bitstream-programming)
5. [Resource and Timing Report Reading](#5-resource-and-timing-report-reading)
6. [Makefile Template](#6-makefile-template)
7. [Comparison: Gowin EDA vs Vivado](#7-comparison-gowin-eda-vs-vivado)
8. [POST: Hardware-Validated Data](#8-post-hardware-validated-data)

---

## 1. Vivado WebPACK Installation

### 1.1 Overview

Vivado WebPACK is the free edition of AMD/Xilinx's design suite. It supports the XC7Z020 without any license file or dongle. The WebPACK edition includes synthesis, implementation, IP Integrator, hardware manager, and the Vitis embedded development platform.

### 1.2 System Requirements

| Requirement | Minimum | Recommended |
|-------------|---------|-------------|
| **Disk Space** | ~30 GB (WebPACK only) | ~50 GB (with Vitis) |
| **RAM** | 8 GB | 16 GB+ |
| **OS** | Ubuntu 20.04/22.04 LTS (64-bit) | Ubuntu 22.04 LTS |
| **CPU** | x86_64, 4 cores | 8+ cores (synthesis is multi-threaded) |

### 1.3 Download and Install

1. Download from [AMD/Xilinx Downloads](https://www.xilinx.com/support/download.html)
   - Select "Vivado ML Edition" (2023.2 or newer recommended)
   - Choose "Vivado ML Standard" or "WebPACK" -- both are free for XC7Z020
   - Download the Linux Self Extracting Web Installer (~200 MB, downloads ~30 GB during install)

2. Run the installer:
   ```bash
   chmod +x Xilinx_Unified_2023.2_1013_2256_Lin64.bin
   sudo ./Xilinx_Unified_2023.2_1013_2256_Lin64.bin
   ```

3. During installation:
   - Select **Vivado** (not Vitis unless you need the embedded SDK)
   - Select **WebPACK** edition
   - Install to `/tools/Xilinx/Vivado/2023.2/` (default)
   - Uncheck device families you do not need (keep Zynq-7000 checked)

4. No license file is required for XC7Z020. The WebPACK edition automatically enables support for this device.

### 1.4 Environment Setup

Add the following to `~/.bashrc` or source it before use:

```bash
# Vivado environment
source /tools/Xilinx/Vivado/2023.2/settings64.sh
```

Verify the installation:

```bash
vivado -version
# Expected output: Vivado v2023.2 (64-bit)
```

### 1.5 JTAG Cable Drivers (Linux)

Vivado requires JTAG cable drivers for hardware programming. Install them after Vivado installation:

```bash
cd /tools/Xilinx/Vivado/2023.2/data/xicom/cable_drivers/lin64/install_script/install_drivers/
sudo ./install_drivers
```

Add udev rules for non-root access:

```bash
# /etc/udev/rules.d/52-xilinx-digilent-usb.rules
ATTR{idVendor}=="0403", ATTR{idProduct}=="6010", MODE="0666"
ATTR{idVendor}=="0403", ATTR{idProduct}=="6014", MODE="0666"
```

Reload udev:

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### 1.6 Vivado Invocation Modes

| Mode | Command | Use Case |
|------|---------|----------|
| **GUI** | `vivado` | Interactive design, block design, debugging |
| **Batch** | `vivado -mode batch -source build.tcl` | Automated builds, CI |
| **TCL Console** | `vivado -mode tcl` | Interactive scripting |
| **Journal/Log** | Auto-generated `vivado.jou` / `vivado.log` | Reproducibility, debugging |

---

## 2. Non-Project Mode TCL Flow

Non-project mode is preferred for CI/scripting and reproducible builds. It does not create a Vivado project directory structure; instead, it runs synthesis, implementation, and bitstream generation from a single TCL script.

### 2.1 Complete Build Script

Create `vivado/build.tcl`:

```tcl
# ==============================================================================
# ATOMiK Zynq Build Script (Non-Project Mode)
# Target: ALINX AX7020 (XC7Z020-2CLG400I)
# ==============================================================================

# Create output directories
file mkdir reports
file mkdir output

# ------------------------------------------------------------------------------
# Read sources
# ------------------------------------------------------------------------------

# ATOMiK core RTL (shared with v3)
read_verilog {
    ../../v3/rtl/atomik_v3_atomik.v
    ../../v3/rtl/atomik_v3_parallel.v
}

# Zynq-specific wrapper and top-level
read_verilog {
    rtl/atomik_axi4lite_wrapper.v
    rtl/atomik_zynq_top.v
}

# Constraints
read_xdc constraints/ax7020.xdc

# ------------------------------------------------------------------------------
# Synthesis
# ------------------------------------------------------------------------------

synth_design -top atomik_zynq_top -part xc7z020clg400-2

# Post-synthesis reports
report_utilization -file reports/post_synth_util.rpt
report_timing_summary -file reports/post_synth_timing.rpt

# Optional: save synthesis checkpoint
write_checkpoint -force output/post_synth.dcp

# ------------------------------------------------------------------------------
# Implementation
# ------------------------------------------------------------------------------

# Optimize: logic, power, and area
opt_design

# Place
place_design
report_utilization -file reports/post_place_util.rpt

# Physical optimization (optional, for timing closure)
# phys_opt_design

# Route
route_design

# Post-implementation reports
report_timing_summary -file reports/timing_summary.rpt
report_utilization -file reports/post_impl_util.rpt
report_power -file reports/power.rpt
report_clock_utilization -file reports/clock_util.rpt
report_methodology -file reports/methodology.rpt

# Save implementation checkpoint
write_checkpoint -force output/post_impl.dcp

# ------------------------------------------------------------------------------
# Bitstream
# ------------------------------------------------------------------------------

write_bitstream -force output/atomik_zynq.bit

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------

puts "============================================"
puts "Build complete."
puts "Bitstream:      output/atomik_zynq.bit"
puts "Timing report:  reports/timing_summary.rpt"
puts "Utilization:    reports/post_impl_util.rpt"
puts "============================================"

# Print timing summary to console
report_timing_summary -no_header -max_paths 5
```

### 2.2 Running the Build

```bash
vivado -mode batch -source vivado/build.tcl -tclargs 2>&1 | tee vivado_build.log
```

The `-mode batch` flag runs Vivado non-interactively and exits when the script completes.

### 2.3 Incremental Builds

To avoid re-running synthesis when only making implementation changes, save and reload checkpoints:

```tcl
# Load a previous synthesis checkpoint
open_checkpoint output/post_synth.dcp

# Re-run implementation only
opt_design
place_design
route_design

write_bitstream -force output/atomik_zynq.bit
```

### 2.4 Multi-Threaded Synthesis

Vivado supports multi-threaded synthesis and implementation. Set the thread count before running:

```tcl
# Use 4 threads (adjust to your CPU core count)
set_param general.maxThreads 4
```

Or from the command line:

```bash
vivado -mode batch -source vivado/build.tcl -tclargs -threads 4
```

---

## 3. Block Design (IPI) Flow

The Block Design (IP Integrator) flow is used to create a Zynq PS + ATOMiK PL design with graphical connectivity. This is the recommended approach for the initial bringup because it auto-generates the PS configuration, AXI interconnect, and reset infrastructure.

### 3.1 Create Block Design

This can be done interactively in the Vivado GUI or via TCL. The TCL approach is shown for reproducibility.

Create `vivado/create_bd.tcl`:

```tcl
# ==============================================================================
# ATOMiK Zynq Block Design Creation
# ==============================================================================

# Create project (project mode needed for block design)
create_project atomik_zynq vivado/atomik_zynq -part xc7z020clg400-2 -force

# Add ATOMiK RTL sources
add_files {
    rtl/atomik_axi4lite_wrapper.v
    ../../v3/rtl/atomik_v3_atomik.v
    ../../v3/rtl/atomik_v3_parallel.v
}

# Create block design
create_bd_design "atomik_system"

# ------------------------------------------------------------------------------
# Add and configure Zynq PS
# ------------------------------------------------------------------------------

# Add Zynq Processing System IP
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7

# Apply AX7020-specific PS configuration
# DDR3: 2x MT41K256M16 (1 GB total, 32-bit bus, 1066 MT/s)
set_property -dict [list \
    CONFIG.PCW_DDR_RAM_HIGHADDR {0x3FFFFFFF} \
    CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125} \
    CONFIG.PCW_UIPARAM_DDR_MEMORY_TYPE {DDR 3} \
    CONFIG.PCW_UIPARAM_DDR_DEVICE_CAPACITY {4096 MBits} \
    CONFIG.PCW_UIPARAM_DDR_BUS_WIDTH {32 Bit} \
    CONFIG.PCW_UIPARAM_DDR_FREQ_MHZ {533.333313} \
] [get_bd_cells ps7]

# Enable UART0 (MIO 46-47, connected to CP2102 USB-UART)
set_property -dict [list \
    CONFIG.PCW_UART0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_UART0_UART0_IO {MIO 46 .. 47} \
] [get_bd_cells ps7]

# Enable Ethernet (GEM0, MIO 16-27, RGMII to KSZ9031)
set_property -dict [list \
    CONFIG.PCW_ENET0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_ENET0_ENET0_IO {MIO 16 .. 27} \
    CONFIG.PCW_ENET0_GRP_MDIO_ENABLE {1} \
    CONFIG.PCW_ENET0_GRP_MDIO_IO {MIO 52 .. 53} \
] [get_bd_cells ps7]

# Enable USB0 (MIO 28-39, ULPI to USB3320)
set_property -dict [list \
    CONFIG.PCW_USB0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_USB0_USB0_IO {MIO 28 .. 39} \
] [get_bd_cells ps7]

# Enable SD0 (MIO 40-45, 4-bit SD)
set_property -dict [list \
    CONFIG.PCW_SD0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_SD0_SD0_IO {MIO 40 .. 45} \
] [get_bd_cells ps7]

# Enable QSPI (MIO 1-6, quad-SPI boot flash)
set_property -dict [list \
    CONFIG.PCW_QSPI_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_QSPI_GRP_SINGLE_SS_ENABLE {1} \
] [get_bd_cells ps7]

# Enable GP AXI Master 0 (for ATOMiK peripheral access)
set_property -dict [list \
    CONFIG.PCW_USE_M_AXI_GP0 {1} \
] [get_bd_cells ps7]

# Configure FCLK_CLK0 (PL clock from PS, 50 MHz default)
set_property -dict [list \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50} \
] [get_bd_cells ps7]

# ------------------------------------------------------------------------------
# Add ATOMiK AXI4-Lite Peripheral
# ------------------------------------------------------------------------------

# Package ATOMiK wrapper as an IP (or add as RTL module)
# Option A: Add as RTL module reference
create_bd_cell -type module -reference atomik_axi4lite_wrapper atomik_0

# ------------------------------------------------------------------------------
# Add AXI Interconnect
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0
set_property CONFIG.NUM_MI {1} [get_bd_cells axi_interconnect_0]

# ------------------------------------------------------------------------------
# Add Processor System Reset
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0

# ------------------------------------------------------------------------------
# Connect everything
# ------------------------------------------------------------------------------

# PS GP AXI Master -> AXI Interconnect -> ATOMiK
connect_bd_intf_net [get_bd_intf_pins ps7/M_AXI_GP0] \
                    [get_bd_intf_pins axi_interconnect_0/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_interconnect_0/M00_AXI] \
                    [get_bd_intf_pins atomik_0/S_AXI]

# Clocks
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] \
               [get_bd_pins axi_interconnect_0/ACLK] \
               [get_bd_pins axi_interconnect_0/S00_ACLK] \
               [get_bd_pins axi_interconnect_0/M00_ACLK] \
               [get_bd_pins atomik_0/S_AXI_ACLK] \
               [get_bd_pins proc_sys_reset_0/slowest_sync_clk]

# Resets
connect_bd_net [get_bd_pins ps7/FCLK_RESET0_N] \
               [get_bd_pins proc_sys_reset_0/ext_reset_in]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] \
               [get_bd_pins axi_interconnect_0/ARESETN] \
               [get_bd_pins axi_interconnect_0/S00_ARESETN] \
               [get_bd_pins axi_interconnect_0/M00_ARESETN] \
               [get_bd_pins atomik_0/S_AXI_ARESETN]

# PS fixed I/O and DDR (mandatory external connections)
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]

# ------------------------------------------------------------------------------
# Address assignment
# ------------------------------------------------------------------------------

# ATOMiK at 0x43C0_0000 (4 KB range)
assign_bd_address -target_address_space /ps7/Data \
    [get_bd_addr_segs atomik_0/S_AXI/reg0] \
    -range 4K -offset 0x43C00000

# ------------------------------------------------------------------------------
# Generate and wrap
# ------------------------------------------------------------------------------

# Validate
validate_bd_design

# Generate output products
generate_target all [get_files atomik_system.bd]

# Create HDL wrapper (Vivado-managed, auto-updated)
make_wrapper -files [get_files atomik_system.bd] -top
add_files -norecurse vivado/atomik_zynq/atomik_zynq.gen/sources_1/bd/atomik_system/hdl/atomik_system_wrapper.v

# Add constraints
add_files -fileset constrs_1 constraints/ax7020.xdc

# Run implementation
launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

# Export hardware definition (for PetaLinux / Vitis)
write_hw_platform -fixed -include_bit -force -file output/atomik_zynq.xsa

puts "Block design build complete."
puts "XSA file: output/atomik_zynq.xsa"
```

### 3.2 Key Concepts

**Zynq PS Configuration**: The PS must be configured to match the AX7020 board hardware -- DDR3 timing, MIO pin assignments, peripheral enables. The settings above match the AX7020 schematic. When the ALINX BSP is available, import their preset instead of manual configuration.

**M_AXI_GP0**: This is the PS General Purpose AXI Master port 0. It provides a 32-bit AXI interface from the ARM CPU to the PL fabric. ATOMiK registers are accessed through this port. The address space starts at 0x40000000 by default; ATOMiK is assigned at offset 0x43C00000.

**XSA Export**: The `.xsa` (Xilinx Support Archive) file contains the hardware description (address map, peripherals, clock frequencies) needed by PetaLinux and Vitis to generate device trees and BSPs.

### 3.3 PS Configuration via ALINX BSP

When the ALINX BSP becomes available, you can import their PS preset instead of manual configuration:

```tcl
# Import ALINX board preset (if available as a board file)
set_property board_part alinx.com:ax7020:part0:1.0 [current_project]

# Or import an existing .xsa / .hdf
# The ALINX BSP typically includes a Vivado project with PS pre-configured
```

---

## 4. Bitstream Programming

### 4.1 JTAG Programming (Volatile)

Loads the bitstream directly into PL configuration SRAM. Configuration is lost on power cycle. This is the fastest method for development iteration.

**GUI**: Vivado Hardware Manager -> Open Target -> Auto Connect -> Program Device

**TCL** (create `vivado/program.tcl`):

```tcl
# Connect to hardware
open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target

# Program the FPGA
set_property PROGRAM.FILE {output/atomik_zynq.bit} [current_hw_device]
program_hw_devices [current_hw_device]

puts "FPGA programmed (volatile)."
close_hw_manager
```

Run:

```bash
vivado -mode batch -source vivado/program.tcl
```

### 4.2 QSPI Flash Programming (Persistent)

Writes the bitstream to the on-board 32 MB QSPI flash. The FPGA loads the bitstream from flash on every power-on.

**Step 1**: Create the flash image:

```tcl
# Generate QSPI flash image
write_cfgmem -force -format bin -interface SPIx4 -size 32 \
    -loadbit "up 0x0 output/atomik_zynq.bit" \
    output/atomik_zynq_qspi.bin
```

**Step 2**: Program the flash via JTAG:

```tcl
# Connect to hardware
open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target

# Add configuration memory device (Zynq QSPI)
create_hw_cfgmem -hw_device [current_hw_device] \
    -mem_dev [lindex [get_cfgmem_parts {s25fl256sxxxxxx0-spi-x1_x2_x4}] 0]

# Configure and program
set_property PROGRAM.ADDRESS_RANGE {use_file} [current_hw_cfgmem]
set_property PROGRAM.FILES {output/atomik_zynq_qspi.bin} [current_hw_cfgmem]
set_property PROGRAM.PRM_FILE {} [current_hw_cfgmem]
set_property PROGRAM.UNUSED_PIN_PULLNONE {0} [current_hw_cfgmem]
set_property PROGRAM.BLANK_CHECK {0} [current_hw_cfgmem]
set_property PROGRAM.ERASE {1} [current_hw_cfgmem]
set_property PROGRAM.CFG_PROGRAM {1} [current_hw_cfgmem]
set_property PROGRAM.VERIFY {1} [current_hw_cfgmem]

# Execute
program_hw_cfgmem [current_hw_cfgmem]

puts "QSPI flash programmed. Power cycle to boot from flash."
close_hw_manager
```

**Note**: The exact flash part number may differ depending on the AX7020 board revision. Check the ALINX schematic for the specific QSPI flash IC. Common parts include S25FL256S (Spansion/Infineon) and IS25LP256 (ISSI).

### 4.3 SD Card Boot (BOOT.BIN)

For Linux boot, the Zynq loads a BOOT.BIN from the SD card FAT32 partition. This file contains the FSBL (First Stage Boot Loader), the PL bitstream, and U-Boot.

**Generate BOOT.BIN using bootgen**:

Create `vivado/boot.bif`:

```
the_ROM_image:
{
    [bootloader] output/zynq_fsbl.elf
    output/atomik_zynq.bit
    output/u-boot.elf
}
```

Run:

```bash
bootgen -image vivado/boot.bif -o output/BOOT.BIN -arch zynq -w on
```

The FSBL is generated by Vitis or PetaLinux. See [LINUX_SETUP_GUIDE.md](LINUX_SETUP_GUIDE.md) for the full SD card boot flow.

### 4.4 Programming Summary

| Method | Persistence | Speed | Use Case |
|--------|:-----------:|:-----:|----------|
| **JTAG (volatile)** | Power cycle clears | Fast (~2s) | Development iteration |
| **QSPI flash** | Persistent | Slow (~30s) | Standalone deployment |
| **SD card BOOT.BIN** | Persistent (removable) | N/A (boot media) | Linux deployment |

---

## 5. Resource and Timing Report Reading

After implementation, Vivado generates several reports. This section explains how to extract the key numbers for ATOMiK development.

### 5.1 Utilization Report (`report_utilization`)

The utilization report shows how many FPGA resources are consumed.

**Key sections to check**:

```
+-------------------------+------+-------+------------+-----------+-------+
| Site Type               | Used | Fixed | Prohibited | Available | Util% |
+-------------------------+------+-------+------------+-----------+-------+
| Slice LUTs              |  850 |     0 |          0 |     53200 |  1.60 |
|   LUT as Logic          |  850 |     0 |          0 |     53200 |  1.60 |
| Slice Registers         | 1200 |     0 |          0 |    106400 |  1.13 |
| Block RAM Tile          |    0 |     0 |          0 |       140 |  0.00 |
| DSPs                    |    0 |     0 |          0 |       220 |  0.00 |
+-------------------------+------+-------+------------+-----------+-------+
```

**What to look for**:
- **Slice LUTs**: ATOMiK core logic. Compare to Tang Nano 9K numbers (477 LUT4 for single-bank). Expect fewer LUT6s due to higher capability per LUT.
- **Slice Registers**: Flip-flops used. Should scale roughly linearly with bank count.
- **Block RAM**: ATOMiK does not use BRAM by default (state stored in registers). Should be 0 unless you add memory-mapped storage.
- **DSPs**: ATOMiK does not use DSP blocks (XOR-only datapath). Should be 0.

### 5.2 Timing Summary (`report_timing_summary`)

The timing summary shows whether all timing constraints are met.

**Key numbers**:

```
| Clock           | WNS (ns) | TNS (ns) | WHS (ns) | THS (ns) |
|-----------------|----------|----------|----------|----------|
| clk_fpga_0      |    2.456 |    0.000 |    0.089 |    0.000 |
| atomik_clk      |    1.234 |    0.000 |    0.045 |    0.000 |
```

| Metric | Meaning | Goal |
|--------|---------|------|
| **WNS** (Worst Negative Slack) | Slack on the worst setup timing path | > 0 (positive = met) |
| **TNS** (Total Negative Slack) | Sum of all negative slack | = 0.000 (no violations) |
| **WHS** (Worst Hold Slack) | Slack on the worst hold timing path | > 0 (positive = met) |
| **THS** (Total Hold Slack) | Sum of all hold violations | = 0.000 |

**Extracting Fmax**:

```
Fmax = 1 / (clock_period - WNS)
```

For example, if `clk_fpga_0` has a 10 ns period (100 MHz target) and WNS = 2.456 ns:

```
Fmax = 1 / (10.0 - 2.456) ns = 1 / 7.544 ns = 132.5 MHz
```

**If WNS is negative**, the design does not meet timing. Options:
1. Reduce the target clock frequency
2. Add pipeline stages to long combinational paths
3. Use `phys_opt_design` for physical optimization
4. Adjust placement constraints or floorplanning

### 5.3 Clock Utilization (`report_clock_utilization`)

Shows MMCM, PLL, and BUFG usage:

```
+------------+------+-----------+
| Type       | Used | Available |
+------------+------+-----------+
| BUFGCTRL   |    3 |        32 |
| MMCME2_ADV |    1 |         4 |
| PLLE2_ADV  |    0 |         2 |
+------------+------+-----------+
```

**ATOMiK clock plan**: One MMCM generates the ATOMiK high-speed clock (200+ MHz target) from FCLK_CLK0 (50 MHz). The AXI bus clock runs at FCLK_CLK0 speed. CDC (clock domain crossing) logic bridges the two domains.

### 5.4 Power Report (`report_power`)

Estimates power consumption by category:

```
+------------------+-------+
| On-Chip          | Power |
+------------------+-------+
| Clocks           | 0.012 |
| Logic            | 0.003 |
| Signals          | 0.004 |
| I/O              | 0.001 |
| PS7              | 1.523 |
| Static Power     | 0.142 |
+------------------+-------+
| Total            | 1.685 |
+------------------+-------+
```

The PS7 (ARM subsystem) dominates power consumption. ATOMiK PL logic typically consumes a small fraction of total power.

### 5.5 Extracting Key Numbers (Script)

Create `vivado/extract_results.tcl` to programmatically extract key metrics:

```tcl
# Extract post-implementation metrics
open_checkpoint output/post_impl.dcp

# Utilization
set lut_used [get_property SLICE_LUTS [get_design_summary]]
set ff_used [get_property SLICE_REGISTERS [get_design_summary]]
set bram_used [get_property BLOCK_RAMS [get_design_summary]]

# Timing
set wns [get_property SLACK [get_timing_paths -max_paths 1 -setup]]

puts "LUT: $lut_used / 53200"
puts "FF:  $ff_used / 106400"
puts "BRAM: $bram_used / 140"
puts "WNS: $wns ns"

close_design
```

---

## 6. Makefile Template

This Makefile wraps the Vivado TCL flow for command-line builds.

Create `hardware/zynq/Makefile`:

```makefile
# ==============================================================================
# ATOMiK Zynq Build Makefile
# Target: ALINX AX7020 (XC7Z020-2CLG400I)
# ==============================================================================

VIVADO = vivado -mode batch -source
BOOTGEN = bootgen

# RTL source directories
V3_RTL = ../../v3/rtl
ZYNQ_RTL = rtl

# Build outputs
BITSTREAM = output/atomik_zynq.bit
XSA = output/atomik_zynq.xsa
QSPI_BIN = output/atomik_zynq_qspi.bin
BOOT_BIN = output/BOOT.BIN

# ==============================================================================
# Targets
# ==============================================================================

.PHONY: bitstream program flash boot clean reports help

## Build bitstream (non-project mode)
bitstream:
	$(VIVADO) vivado/build.tcl

## Build block design (project mode, PS+PL)
blockdesign:
	$(VIVADO) vivado/create_bd.tcl

## Program FPGA via JTAG (volatile)
program: $(BITSTREAM)
	$(VIVADO) vivado/program.tcl

## Program QSPI flash (persistent)
flash: $(QSPI_BIN)
	$(VIVADO) vivado/flash.tcl

## Generate BOOT.BIN for SD card
boot: $(BITSTREAM)
	$(BOOTGEN) -image vivado/boot.bif -o $(BOOT_BIN) -arch zynq -w on

## Extract and print key metrics from implementation
reports:
	$(VIVADO) vivado/extract_results.tcl

## Clean all build artifacts
clean:
	rm -rf output/ reports/ .Xil/ vivado*.log vivado*.jou
	rm -rf vivado/atomik_zynq/

## Show available targets
help:
	@echo "ATOMiK Zynq Build Targets:"
	@echo "  make bitstream    - Synthesize and implement (non-project mode)"
	@echo "  make blockdesign  - Create Zynq PS+PL block design (project mode)"
	@echo "  make program      - Program FPGA via JTAG (volatile)"
	@echo "  make flash        - Program QSPI flash (persistent)"
	@echo "  make boot         - Generate SD card BOOT.BIN"
	@echo "  make reports      - Extract utilization and timing metrics"
	@echo "  make clean        - Remove all build artifacts"
```

### 6.1 Quick Build Commands

```bash
# Full build (synthesis + implementation + bitstream)
make bitstream

# Program FPGA via JTAG for quick testing
make program

# Program QSPI flash for persistent deployment
make flash

# Clean and rebuild
make clean && make bitstream

# Check results without rebuilding
make reports
```

---

## 7. Comparison: Gowin EDA vs Vivado

This comparison helps developers familiar with the Tang Nano 9K Gowin flow transition to the Vivado flow for the AX7020.

### 7.1 Tool Mapping

| Feature | Gowin gw_sh | Vivado | Notes |
|---------|-------------|--------|-------|
| **TCL Shell** | `gw_sh` | `vivado -mode tcl` | Both use TCL scripting |
| **Batch Mode** | `gw_sh script.tcl` | `vivado -mode batch -source script.tcl` | Functionally equivalent |
| **Synthesis** | `run_synthesis` | `synth_design` | Vivado has more options (strategy, retiming) |
| **Place & Route** | `run_pnr` | `opt_design; place_design; route_design` | Vivado separates into distinct steps |
| **Timing Analysis** | `report_timing` (limited) | `report_timing_summary` | Vivado timing analysis is far more detailed |
| **Utilization** | Post-synth summary | `report_utilization` | Vivado provides hierarchical breakdown |
| **Constraints** | `.cst` (physical) + `.sdc` (timing) | `.xdc` (unified) | XDC combines both physical and timing constraints |
| **Clock Definition** | `create_clock` in SDC | `create_clock` in XDC | Same SDC syntax |
| **Programming** | `openFPGALoader` | `program_hw_devices` | Vivado uses built-in HW Manager |
| **Flash Writing** | `openFPGALoader --external-flash` | `program_hw_cfgmem` | Vivado writes via JTAG to QSPI |
| **IP Cores** | Gowin IP GUI | IP Catalog / IP Integrator | Vivado IP ecosystem is much larger |
| **Block Design** | N/A | IP Integrator (IPI) | No equivalent in Gowin; Zynq PS requires IPI |
| **Debug** | N/A (limited) | ILA, VIO, JTAG-to-AXI | Vivado has comprehensive on-chip debug |
| **License** | Free (Education edition) | Free (WebPACK for XC7Z020) | Both free for our target devices |

### 7.2 Constraint File Differences

**Gowin (.cst physical + .sdc timing)**:

```
// Physical constraints (.cst)
IO_LOC "clk" 52;
IO_PORT "clk" IO_TYPE=LVCMOS33;

// Timing constraints (.sdc)
create_clock -name clk -period 12.346 [get_ports {clk}]
```

**Vivado (.xdc unified)**:

```tcl
# Physical + timing in one file (.xdc)
set_property PACKAGE_PIN <pin> [get_ports {clk}]
set_property IOSTANDARD LVCMOS33 [get_ports {clk}]
create_clock -name clk -period 20.000 [get_ports {clk}]
```

### 7.3 Key Differences to Remember

| Topic | Gowin | Vivado |
|-------|-------|--------|
| **Project files** | `.gprj` | `.xpr` (or non-project mode) |
| **Bitstream format** | `.fs` | `.bit` (volatile), `.bin` (flash) |
| **Close project** | `close_project` (not valid in gw_sh) | `close_project` (valid) |
| **Log files** | Custom output | `vivado.log` + `vivado.jou` (journal, replayable) |
| **Multi-clock CDC** | Manual | Vivado auto-detects clock domain crossings and reports them |
| **Synthesis strategy** | Single mode | Multiple strategies (default, area, performance, timing) |
| **Incremental builds** | Not supported | Supported via checkpoints (`.dcp`) |

### 7.4 ATOMiK RTL Portability

The ATOMiK core RTL (`atomik_v3_atomik.v`, `atomik_v3_parallel.v`) is vendor-neutral Verilog and synthesizes on both Gowin and Vivado without modification. The only vendor-specific code is the wrapper layer:

| Layer | Gowin (Tang Nano 9K) | Vivado (AX7020) |
|-------|---------------------|-----------------|
| **ATOMiK Core** | Same RTL | Same RTL |
| **Bus Wrapper** | Wishbone (`atomik_wb_wrapper.v`) | AXI4-Lite (`atomik_axi4lite_wrapper.v`) |
| **Clock Gen** | Gowin rPLL | Xilinx MMCM |
| **Top Level** | `picotiny.v` | `atomik_zynq_top.v` |
| **Constraints** | `.cst` + `.sdc` | `.xdc` |

---

## 8. POST: Hardware-Validated Data

The following sections contain placeholder data that will be updated when the AX7020 board arrives and builds are validated on hardware.

### 8.1 Actual Utilization Numbers

```
POST: To be populated after first successful build on AX7020 hardware.

Expected (pre-board estimate):
  - ATOMiK single-bank: ~300-400 LUT6 (vs 477 LUT4 on Gowin)
  - ATOMiK 16-bank: ~2,000-3,000 LUT6 (vs 1,779 LUT4 on Gowin)
  - AXI4-Lite wrapper: ~200-300 LUT6
  - Total with PS: dominated by PS (hard silicon, not counted in PL utilization)

Actual:
  - LUT:  _____ / 53,200 ( _____ %)
  - FF:   _____ / 106,400 ( _____ %)
  - BRAM: _____ / 140 ( _____ %)
  - DSP:  _____ / 220 ( _____ %)
```

### 8.2 Actual Timing Results

```
POST: To be populated after first successful timing closure.

Expected (pre-board estimate):
  - ATOMiK core Fmax: 200-300 MHz (vs 100.2 MHz on Gowin GW1NR-9)
  - AXI bus clock: 50-100 MHz (FCLK_CLK0)
  - CDC bridge latency: 2-4 cycles per domain

Actual:
  - ATOMiK clock target: _____ MHz
  - ATOMiK WNS: _____ ns (Fmax: _____ MHz)
  - AXI bus WNS: _____ ns
  - TNS: _____ ns
```

### 8.3 Build Time

```
POST: To be populated with actual build times on development machine.

Expected:
  - Synthesis: 2-5 minutes
  - Implementation: 5-15 minutes
  - Bitstream: 1-2 minutes
  - Total: 8-22 minutes

Actual (Ryzen 7 5700U, 8 GB RAM):
  - Synthesis: _____ minutes
  - Implementation: _____ minutes
  - Bitstream: _____ minutes
  - Total: _____ minutes
```

### 8.4 QSPI Flash Part Number

```
POST: Verify QSPI flash IC from AX7020 board markings or schematic.

Assumed: S25FL256S (256 Mbit, Spansion/Infineon)
Actual: _____
Vivado cfgmem part: _____
```

### 8.5 Vivado Version-Specific Notes

```
POST: Document any Vivado version-specific issues or workarounds encountered.

Vivado version used: _____
Any critical settings or patches: _____
```

---

## Appendix A: XDC Constraint Template

A minimal XDC constraint file for ATOMiK on the AX7020. Pin assignments are placeholders pending board schematic validation.

Create `hardware/zynq/constraints/ax7020.xdc`:

```tcl
# ==============================================================================
# ATOMiK Zynq Constraints -- ALINX AX7020
# Target: XC7Z020-2CLG400I
# ==============================================================================

# ------------------------------------------------------------------------------
# PL Clock (from PS FCLK_CLK0, directly connected in block design)
# No physical pin constraint needed -- FCLK_CLK0 is an internal PS-to-PL path
# ------------------------------------------------------------------------------

# ATOMiK clock constraint (if using MMCM-derived clock)
# create_clock -name atomik_clk -period 5.000 [get_pins mmcm_inst/CLKOUT0]
# 5.000 ns = 200 MHz target

# ------------------------------------------------------------------------------
# User LEDs (PL) -- Pin assignments TBD from schematic
# ------------------------------------------------------------------------------

# POST: Uncomment and fill in PACKAGE_PIN values from ALINX schematic
# set_property PACKAGE_PIN <pin> [get_ports {led[0]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {led[0]}]
# set_property PACKAGE_PIN <pin> [get_ports {led[1]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {led[1]}]
# set_property PACKAGE_PIN <pin> [get_ports {led[2]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {led[2]}]
# set_property PACKAGE_PIN <pin> [get_ports {led[3]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {led[3]}]

# ------------------------------------------------------------------------------
# User Buttons (PL) -- Pin assignments TBD from schematic
# ------------------------------------------------------------------------------

# POST: Uncomment and fill in PACKAGE_PIN values from ALINX schematic
# set_property PACKAGE_PIN <pin> [get_ports {btn[0]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {btn[0]}]
# set_property PACKAGE_PIN <pin> [get_ports {btn[1]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {btn[1]}]

# ------------------------------------------------------------------------------
# Timing Constraints
# ------------------------------------------------------------------------------

# AXI bus clock (FCLK_CLK0 from PS, auto-constrained by block design)
# If using non-project mode without block design, add:
# create_clock -name clk_fpga_0 -period 20.000 [get_ports {FCLK_CLK0}]

# CDC false paths (if ATOMiK runs on a separate clock domain)
# set_clock_groups -asynchronous \
#     -group [get_clocks clk_fpga_0] \
#     -group [get_clocks atomik_clk]

# ------------------------------------------------------------------------------
# Bitstream Configuration
# ------------------------------------------------------------------------------

set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.UNUSEDPIN PULLUP [current_design]
```

---

## Appendix B: Revision History

| Date | Change |
|------|--------|
| 2026-03-07 | Initial version. Pre-board reference from Vivado documentation and ATOMiK architecture decisions. |

---

*References: Xilinx UG894 (Vivado TCL Scripting), UG903 (Using Constraints), UG994 (IP Integrator), UG908 (Vivado Programming and Debugging), UG585 (Zynq-7000 TRM), ALINX AX7020 User Manual*
