# Zynq PS Configuration -- ARM Processing System on AX7020

**Date:** March 7, 2026
**Status:** Pre-board (from Xilinx UG585 TRM and design decisions)
**Target:** ALINX AX7020 (XC7Z020-2CLG400I)
**Audience:** Developers configuring the PS subsystem for ATOMiK integration

---

## Table of Contents

1. [PS Architecture Overview](#1-ps-architecture-overview)
2. [DDR3 Configuration](#2-ddr3-configuration)
3. [PS MIO Allocation Table](#3-ps-mio-allocation-table)
4. [PS Clock Configuration](#4-ps-clock-configuration)
5. [PS-PL Interfaces](#5-ps-pl-interfaces)
6. [Boot Mode Selection](#6-boot-mode-selection)
7. [ATOMiK-Relevant Decisions](#7-atomik-relevant-decisions)

---

## 1. PS Architecture Overview

The Zynq-7020 Processing System (PS) contains a dual-core ARM Cortex-A9 MPCore with a comprehensive set of hard peripherals. Unlike the Tang Nano 9K's soft PicoRV32, the PS is hardened silicon -- it does not consume any PL fabric resources.

### 1.1 CPU Complex

| Component | Specification |
|-----------|---------------|
| **Cores** | 2x ARM Cortex-A9 (ARMv7-A) |
| **Max Frequency** | 667 MHz (-2 speed grade) |
| **L1 I-Cache** | 32 KB per core (4-way set associative) |
| **L1 D-Cache** | 32 KB per core (4-way set associative) |
| **L2 Cache** | 512 KB shared (8-way set associative, unified) |
| **SCU** | Snoop Control Unit (maintains L1 coherency between cores) |
| **FPU** | VFPv3-D16 per core (hardware single/double precision) |
| **NEON** | SIMD coprocessor per core |

### 1.2 Interrupt Controller

The PS uses a standard ARM Generic Interrupt Controller (GIC, PL390):

| Feature | Value |
|---------|-------|
| **Shared Peripheral Interrupts (SPI)** | 60 (IRQ IDs 32-91) |
| **Private Peripheral Interrupts (PPI)** | 5 per core |
| **Software Generated Interrupts (SGI)** | 16 (IRQ IDs 0-15) |
| **PL-to-PS Interrupt Lines** | 16 (directly wired from PL fabric) |
| **Priority Levels** | 32 |

The 16 PL-to-PS interrupt lines (IRQ_F2P[15:0]) are the mechanism for ATOMiK or other PL peripherals to signal the ARM cores. These connect to SPI IDs 61-68 and 84-91 in the GIC.

### 1.3 DMA Controller

| Feature | Value |
|---------|-------|
| **Controller** | ARM DMA-330 (PL330) |
| **Channels** | 8 |
| **Bus Width** | 64-bit AXI |
| **Accessible from PL** | Yes (via S_AXI_GP or S_AXI_HP) |

The DMA controller can transfer data between PS peripherals and DDR memory without CPU involvement. For ATOMiK, DMA is not needed initially (register-level MMIO is sufficient), but it becomes relevant for bulk state transfer workloads.

### 1.4 PS Architecture Block Diagram

```
                    Zynq-7020 Processing System (PS)
 ┌──────────────────────────────────────────────────────────────────┐
 │                                                                  │
 │  ┌──────────┐  ┌──────────┐                                     │
 │  │ Cortex-A9│  │ Cortex-A9│        ┌─────────┐                  │
 │  │  Core 0  │  │  Core 1  │        │   GIC   │                  │
 │  │ 32K I+D  │  │ 32K I+D  │        │ PL390   │◄── IRQ_F2P[15:0]│
 │  └────┬─────┘  └────┬─────┘        └─────────┘      (from PL)  │
 │       │              │                                           │
 │  ┌────┴──────────────┴────┐                                     │
 │  │   Snoop Control Unit   │                                     │
 │  │        (SCU)           │                                     │
 │  └────────────┬───────────┘                                     │
 │               │                                                  │
 │  ┌────────────┴───────────┐    ┌─────────┐                      │
 │  │  512 KB L2 Cache       │    │  DMA330  │                      │
 │  │  (PL310, 8-way)        │    │  8-ch    │                      │
 │  └────────────┬───────────┘    └────┬────┘                      │
 │               │                     │                            │
 │  ┌────────────┴─────────────────────┴──────────────────┐        │
 │  │              Central Interconnect                    │        │
 │  │           (AXI 64-bit, 3x3 switch)                  │        │
 │  └──┬──────┬──────┬──────┬──────┬──────┬───────────────┘        │
 │     │      │      │      │      │      │                         │
 │   DDR   QSPI   MIO    USB   GigE   SD/SDIO                     │
 │   Ctrl  Flash  Periph  2x    1x     2x                          │
 │     │                                                            │
 │  ┌──┴──┐                                                         │
 │  │1 GB │                PS-PL Interfaces                         │
 │  │DDR3 │  ┌──────┬──────┬──────┬──────┬──────┬──────┐           │
 │  └─────┘  │GP M0 │GP M1 │GP S0 │GP S1 │HP0-3 │ ACP  │          │
 │           │32-bit│32-bit│32-bit│32-bit│64-bit│64-bit│           │
 │           └──┬───┴──┬───┴──┬───┴──┬───┴──┬───┴──┬───┘           │
 └──────────────┼──────┼──────┼──────┼──────┼──────┼───────────────┘
        to PL ──┘      │      │      │      │      └── to PL
                       ...   ...    ...    ...
```

---

## 2. DDR3 Configuration

### 2.1 On-Board DDR3 Specification

The AX7020 provides 1 GB of DDR3 SDRAM connected to the PS DDR controller:

| Parameter | Value |
|-----------|-------|
| **Total Capacity** | 1 GB (8 Gbit) |
| **Organization** | 2x 256M x 16 (two 16-bit devices = 32-bit bus) |
| **DDR Clock** | 533 MHz (1066 MT/s) |
| **Data Rate** | DDR3-1066 |
| **Peak Bandwidth** | 4,264 MB/s (32-bit x 1066 MT/s) |
| **Row Address** | 15 bits |
| **Column Address** | 10 bits |
| **Bank Address** | 3 bits (8 banks per device) |
| **DRAM Voltage** | 1.5V |
| **Termination** | On-die (ODT), board-level matched routing |

### 2.2 PS DDR Controller

The Zynq PS includes a hardened DDR memory controller. Configuration is set through Vivado's PS Configuration GUI (or TCL):

| Parameter | Setting |
|-----------|---------|
| **Controller** | PS hard DDR3 controller |
| **Data Width** | 32-bit (matches 2x16 on board) |
| **ECC** | Not available (requires 40-bit or 72-bit bus) |
| **Refresh** | Automatic (controller manages tREFI/tRFC) |
| **Timing** | Auto-calculated from speed grade + DRAM part number |
| **Address Mapping** | Bank-Row-Column (default, optimized for sequential access) |

### 2.3 DDR Address Space

| Address Range | Size | Description |
|---------------|------|-------------|
| `0x00000000` - `0x3FFFFFFF` | 1 GB | DDR3 SDRAM (PS view) |
| `0x00100000` - `0x3FFFFFFF` | ~1023 MB | Linux usable (first 1 MB reserved by bootloader) |

The PS DDR address space is accessible from both PS cores and from PL via the S_AXI_HP and S_AXI_ACP ports. The S_AXI_GP slave ports can also reach DDR but with lower bandwidth (32-bit path vs. 64-bit for HP).

### 2.4 Comparison to Tang Nano 9K Memory

| Parameter | Tang Nano 9K | AX7020 | Ratio |
|-----------|:----------:|:------:|:-----:|
| **Type** | SRAM (on-chip) | DDR3 (external) | -- |
| **Capacity** | 8 KB | 1 GB | **131,072x** |
| **Data Width** | 32-bit | 32-bit | 1x |
| **Latency** | 1 cycle (deterministic) | ~10-20 ns (variable) | Higher |
| **Bandwidth** | ~100 MB/s @ 25.2 MHz | 4,264 MB/s peak | **~43x** |

The move from 8 KB SRAM to 1 GB DDR3 eliminates the memory constraint that currently limits ATOMiK workload sizes on the Tang Nano 9K.

---

## 3. PS MIO Allocation Table

The Zynq PS has 54 multiplexed I/O pins (MIO[0:53]) that connect PS hard peripherals directly, without using PL fabric. MIO pins are configured during PS initialization (FSBL) based on the Vivado PS configuration.

### 3.1 AX7020 MIO Pin Assignments

| MIO Pin(s) | Peripheral | Signal(s) | Direction | IOSTANDARD | Notes |
|------------|-----------|-----------|-----------|------------|-------|
| MIO 0 | System | Reserved | -- | -- | Boot mode strapping |
| MIO 1 | QSPI | CS_B | Output | LVCMOS33 | QSPI flash chip select |
| MIO 2 | QSPI | IO[0] (MOSI) | Bidir | LVCMOS33 | QSPI data |
| MIO 3 | QSPI | IO[1] (MISO) | Bidir | LVCMOS33 | QSPI data |
| MIO 4 | QSPI | IO[2] | Bidir | LVCMOS33 | QSPI data |
| MIO 5 | QSPI | IO[3] | Bidir | LVCMOS33 | QSPI data |
| MIO 6 | QSPI | CLK | Output | LVCMOS33 | QSPI clock |
| MIO 7-8 | USB 0 | Reset, Direction | Output | LVCMOS18 | USB PHY control |
| MIO 9-15 | Ethernet 0 | MDIO, MDC | Bidir | LVCMOS18 | RGMII management |
| MIO 16-21 | Ethernet 0 | RGMII TX/RX | Bidir | LVCMOS18 | RGMII data path |
| MIO 22-27 | Ethernet 0 | RGMII TX/RX | Bidir | LVCMOS18 | RGMII data path (cont.) |
| MIO 28-39 | USB 0 | Data[0:7], CLK, STP, NXT, DIR | Bidir | LVCMOS18 | ULPI USB interface |
| MIO 40-45 | SD 0 | CLK, CMD, DATA[0:3] | Bidir | LVCMOS18 | SD card interface |
| MIO 46 | SD 0 | CD | Input | LVCMOS18 | Card detect |
| MIO 47 | SD 0 | WP | Input | LVCMOS18 | Write protect |
| MIO 48 | UART 0 | TX | Output | LVCMOS18 | Serial console transmit |
| MIO 49 | UART 0 | RX | Input | LVCMOS18 | Serial console receive |
| MIO 50-51 | I2C 0 | SDA, SCL | Bidir | LVCMOS18 | I2C bus |
| MIO 52-53 | GPIO | User GPIO | Bidir | LVCMOS18 | Board LEDs/buttons |

### 3.2 IOSTANDARD Voltage Banks

The PS MIO pins are organized into two voltage banks:

| Bank | MIO Range | Voltage | Peripherals |
|------|-----------|---------|-------------|
| 500 | MIO 0-15 | 3.3V (LVCMOS33) | QSPI, Ethernet MDIO |
| 501 | MIO 16-53 | 1.8V (LVCMOS18) | Ethernet RGMII, USB, SD, UART, I2C, GPIO |

### 3.3 EMIO Extensions

PS peripherals that are not connected to MIO pins can be routed through PL fabric using Extended MIO (EMIO). EMIO signals appear as PL ports and must be constrained to PL I/O pins in the XDC file.

Common EMIO usage:
- **UART1** via EMIO: for a second serial port routed through PL pins
- **SPI0/SPI1** via EMIO: for SPI peripherals on PL-connected headers
- **GPIO** via EMIO: up to 64 additional GPIO pins through PL fabric

ATOMiK does not currently require EMIO -- all PS peripherals fit within MIO, and PL communication uses AXI.

---

## 4. PS Clock Configuration

### 4.1 Clock Sources

The Zynq PS generates all internal clocks from a single external reference:

| Parameter | Value |
|-----------|-------|
| **PS_CLK Input** | 33.333 MHz (on-board oscillator) |
| **ARM PLL** | Generates CPU clock |
| **DDR PLL** | Generates DDR controller clock |
| **IO PLL** | Generates peripheral clocks and FCLK outputs to PL |

### 4.2 PLL Configuration

| PLL | Output | Multiplier | Divisor | Result |
|-----|--------|:----------:|:-------:|:------:|
| **ARM PLL** | CPU_6x4x | 40 | 2 | 667 MHz |
| **DDR PLL** | DDR_3x | 32 | 1 | 533 MHz (1066 MT/s) |
| **IO PLL** | Configurable | Varies | Varies | See FCLK table below |

### 4.3 ARM CPU Clock Tree

```
PS_CLK (33.333 MHz)
  └─► ARM PLL (x40 = 1333.33 MHz)
        ├─► /2 = 666.67 MHz ──► CPU_6x4x (ARM core clock)
        ├─► /4 = 333.33 MHz ──► CPU_3x2x (L2 cache, SCU)
        ├─► /6 = 222.22 MHz ──► CPU_2x (APB peripheral clock)
        └─► /6 = 222.22 MHz ──► CPU_1x (debug, trace)
```

### 4.4 FCLK Outputs to PL

The IO PLL generates up to 4 configurable clock outputs (FCLK_CLK0-3) that are available to PL fabric. These are the primary mechanism for providing clocks to PL logic without using MMCM/PLL resources in the PL.

| FCLK Output | Planned Frequency | Purpose |
|-------------|:-----------------:|---------|
| **FCLK_CLK0** | 100 MHz | AXI bus clock (PS-PL interface clock) |
| **FCLK_CLK1** | 50 MHz | General PL logic / ATOMiK initial clock |
| **FCLK_CLK2** | 200 MHz | High-speed logic (if needed) |
| **FCLK_CLK3** | Reserved | Available for future use |

FCLK frequencies are configured in Vivado's PS Configuration wizard under Clock Configuration. The IO PLL output is divided down to each FCLK independently.

### 4.5 Comparison to Tang Nano 9K Clocking

| Parameter | Tang Nano 9K | AX7020 |
|-----------|:----------:|:------:|
| **Reference Clock** | 27 MHz crystal | 33.333 MHz oscillator |
| **CPU Clock** | 25.2 MHz (PicoRV32) | 667 MHz (Cortex-A9) |
| **PL Clock Sources** | 2 PLL (Gowin rPLL) | 4 FCLK + 4 MMCM + 2 PLL |
| **ATOMiK Clock** | 81 MHz (dedicated PLL) | 100-200 MHz (FCLK or MMCM) |
| **Clock Flexibility** | Very limited (2 PLL, both used) | Extensive (FCLK + PL PLLs) |

The XC7Z020 has vastly more clocking resources. On the Tang Nano 9K, both PLLs are consumed (HDMI + ATOMiK). On the AX7020, FCLK outputs from the PS provide clocks to PL without using any PL PLL/MMCM, leaving all 4 MMCM and 2 PLL free for high-frequency clock generation.

---

## 5. PS-PL Interfaces

The PS-PL boundary defines how the ARM cores communicate with PL fabric. This is the most architecturally important section for ATOMiK integration.

### 5.1 Interface Summary

| Interface | Width | Direction | Count | Bandwidth (Peak) | Use Case |
|-----------|:-----:|:---------:|:-----:|:-----------------:|----------|
| **M_AXI_GP0** | 32-bit | PS -> PL | 1 | 400 MB/s | PS writes to PL peripherals |
| **M_AXI_GP1** | 32-bit | PS -> PL | 1 | 400 MB/s | PS writes to PL peripherals |
| **S_AXI_GP0** | 32-bit | PL -> PS | 1 | 400 MB/s | PL reads PS memory (low BW) |
| **S_AXI_GP1** | 32-bit | PL -> PS | 1 | 400 MB/s | PL reads PS memory (low BW) |
| **S_AXI_HP0** | 64-bit | PL -> PS | 1 | 1,200 MB/s | PL DMA to DDR (high BW) |
| **S_AXI_HP1** | 64-bit | PL -> PS | 1 | 1,200 MB/s | PL DMA to DDR (high BW) |
| **S_AXI_HP2** | 64-bit | PL -> PS | 1 | 1,200 MB/s | PL DMA to DDR (high BW) |
| **S_AXI_HP3** | 64-bit | PL -> PS | 1 | 1,200 MB/s | PL DMA to DDR (high BW) |
| **S_AXI_ACP** | 64-bit | PL -> PS | 1 | 1,200 MB/s | PL access to L2 (cache-coherent) |
| **FCLK_CLK0-3** | 1-bit | PS -> PL | 4 | -- | Configurable PL clocks |
| **FCLK_RESET0-3** | 1-bit | PS -> PL | 4 | -- | Synchronized resets for FCLK domains |
| **IRQ_F2P** | 1-bit | PL -> PS | 16 | -- | PL-to-PS interrupt lines |
| **EMIO** | Varies | Bidir | -- | -- | PS peripheral signals through PL |

### 5.2 M_AXI_GP: General Purpose Master Ports

The M_AXI_GP ports are 32-bit AXI3 master interfaces where the PS initiates transactions to PL slave peripherals. This is the primary mechanism for CPU-controlled register access.

| Parameter | Value |
|-----------|-------|
| **Data Width** | 32 bits |
| **Address Width** | 32 bits |
| **Protocol** | AXI3 (compatible with AXI4-Lite slaves) |
| **Burst Length** | Up to 16 beats (AXI3) |
| **Outstanding Transactions** | Up to 8 |
| **Clock** | FCLK_CLK0 (or any FCLK) |

**GP0 Address Range:** `0x40000000` - `0x7FFFFFFF` (1 GB)
**GP1 Address Range:** `0x80000000` - `0xBFFFFFFF` (1 GB)

ATOMiK registers will be placed at `0x43C00000` within the GP0 address range. The Vivado Address Editor assigns this automatically when the IP is connected to M_AXI_GP0.

### 5.3 S_AXI_GP: General Purpose Slave Ports

The S_AXI_GP ports allow PL masters to access PS address space (DDR, OCM, peripherals) through a 32-bit path. These are lower bandwidth than HP ports and share the central interconnect with CPU traffic.

Not currently planned for ATOMiK use. Relevant only if a PL-side DMA engine needs to read from PS memory.

### 5.4 S_AXI_HP: High Performance Slave Ports

The S_AXI_HP ports provide high-bandwidth paths from PL to PS DDR memory. They bypass the CPU's cache hierarchy and connect directly to the DDR controller through dedicated FIFO buffers.

| Parameter | Value |
|-----------|-------|
| **Data Width** | 32-bit or 64-bit (configurable) |
| **FIFO Depth** | 1 KB per port (read and write FIFOs) |
| **Clock** | Independent per port (any FCLK or PL-generated clock) |
| **Outstanding Transactions** | Up to 8 per port |

**Future ATOMiK use:** For streaming state transfer workloads, an HP port with a DMA engine in PL could move bulk data between DDR and ATOMiK at up to 1,200 MB/s per port. With all 4 HP ports active, the theoretical aggregate bandwidth is 4,800 MB/s.

### 5.5 S_AXI_ACP: Accelerator Coherency Port

The ACP provides cache-coherent access from PL to the PS memory system. Transactions through ACP are snooped by the SCU, ensuring L1/L2 cache coherency without software cache management.

| Parameter | Value |
|-----------|-------|
| **Data Width** | 64 bits |
| **Cache Coherency** | Full (snooped by SCU) |
| **Clock** | CPU_6x4x / 3 (synchronized to CPU interconnect) |

**ATOMiK relevance:** The ACP is useful if ATOMiK's PL logic needs to read/write data that the CPU has in cache. Without ACP, the CPU must manually flush/invalidate cache lines before/after DMA transfers. With ACP, cache coherency is automatic but at the cost of snoop overhead.

For initial ATOMiK integration, ACP is not needed. M_AXI_GP0 is sufficient for register access, and any future DMA path through HP ports can use software cache management.

### 5.6 FCLK and FCLK_RESET

Each FCLK output has a corresponding reset signal (FCLK_RESET_N) that is synchronized to the FCLK domain. Use these resets for PL logic clocked by FCLK:

```
FCLK_CLK0    ──► AXI bus logic, AXI interconnect, AXI peripherals
FCLK_RESET0_N ──► Synchronized reset for FCLK_CLK0 domain

FCLK_CLK1    ──► ATOMiK clock (or input to MMCM for higher frequency)
FCLK_RESET1_N ──► Synchronized reset for FCLK_CLK1 domain
```

### 5.7 IRQ_F2P: PL-to-PS Interrupts

The 16 PL-to-PS interrupt lines connect to the GIC as Shared Peripheral Interrupts. They can be configured as level-sensitive or edge-triggered in the GIC.

| IRQ_F2P Bit | GIC IRQ ID | Planned Use |
|:-----------:|:----------:|-------------|
| 0 | 61 | ATOMiK operation complete (future) |
| 1-15 | 62-68, 84-91 | Available for other PL peripherals |

For initial ATOMiK integration, interrupts are not needed. The CPU polls the STATUS register via MMIO. Interrupt-driven operation is a future optimization for reducing CPU busy-wait overhead.

---

## 6. Boot Mode Selection

### 6.1 Boot Modes

The Zynq boot mode is set by MIO[5:2] pin strapping at power-up:

| Mode | MIO[5:2] | Description | Use Case |
|------|:--------:|-------------|----------|
| **QSPI** | `0b0001` | Boot from on-board 32 MB QSPI flash | Production deployment |
| **SD Card** | `0b0110` | Boot from SD card FAT partition | Development, Linux images |
| **JTAG** | `0b0000` | Boot from JTAG debugger | Development, no persistent boot |

### 6.2 Boot Sequence

The Zynq boot process is a multi-stage chain:

```
Power-On
  │
  ▼
BootROM (hard-coded in PS silicon)
  │  - Reads boot mode pins
  │  - Loads FSBL from QSPI/SD/JTAG
  │  - Authenticates FSBL (if secure boot enabled)
  ▼
FSBL (First Stage Boot Loader)
  │  - Initializes PS clocks (ARM PLL, DDR PLL, IO PLL)
  │  - Initializes DDR3 controller and trains memory interface
  │  - Programs PL bitstream (if included in boot image)
  │  - Loads next stage (U-Boot or bare-metal application)
  ▼
U-Boot (Second Stage Boot Loader)
  │  - Initializes Ethernet, USB, SD
  │  - Loads Linux kernel + device tree + rootfs
  │  - Or: boots bare-metal application directly
  ▼
Linux Kernel
  │  - Boots with device tree describing hardware
  │  - Loads UIO driver for ATOMiK access
  │  - Mounts rootfs (from SD or initramfs)
  ▼
Userspace Application
     - Opens /dev/uio0 for ATOMiK MMIO
     - SSH access via Ethernet
```

### 6.3 Boot Image Creation

The boot image (`BOOT.bin`) is created with the Xilinx `bootgen` tool:

```
bootgen -image boot.bif -o BOOT.bin -w on
```

The BIF (Boot Image Format) file specifies the components:

```
the_ROM_image:
{
    [bootloader]    fsbl.elf
    [destination_device = pl] bitstream.bit
                    u-boot.elf
}
```

For QSPI boot, `BOOT.bin` is programmed to the on-board 32 MB flash using Vivado's Hardware Manager or the `program_flash` command.

### 6.4 Development vs. Production Boot

| Phase | Boot Mode | Boot Source | PL Programming |
|-------|-----------|-------------|---------------|
| **Bring-up** | JTAG | Vivado Hardware Manager | JTAG download (volatile) |
| **Development** | SD Card | `BOOT.bin` + `image.ub` on SD | Included in BOOT.bin |
| **Production** | QSPI | `BOOT.bin` in QSPI flash | Included in BOOT.bin |

For ATOMiK development, SD card boot is recommended: modify `BOOT.bin` on the SD card, power cycle, and the new bitstream/firmware loads automatically. JTAG boot requires a connected host PC and is useful only for interactive debugging.

---

## 7. ATOMiK-Relevant Decisions

### 7.1 PS-PL Interface Selection

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **ATOMiK access port** | M_AXI_GP0 | Simplest path; 32-bit is sufficient for register access; no DMA needed initially |
| **ATOMiK base address** | `0x43C00000` | Standard Vivado AXI address in GP0 range (`0x40000000`-`0x7FFFFFFF`) |
| **PL bus clock** | FCLK_CLK0 at 100 MHz | Standard AXI clock; 4x faster than Tang Nano 9K bus clock |
| **ATOMiK core clock** | FCLK_CLK1 at 50 MHz (initial) | Start simple; derive higher clocks from MMCM later if needed |
| **Serial console** | UART0 on MIO 48-49 | Standard PS UART, no PL resources needed |
| **Network access** | Ethernet 0 on MIO | SSH access for development and data transfer |

### 7.2 Clock Strategy

**Phase 1 (Bring-up):** Run ATOMiK on FCLK_CLK0 (100 MHz), same clock as AXI bus. No CDC required. Simple, verifiable.

**Phase 2 (Optimization):** Use a PL MMCM to generate a higher ATOMiK clock (200+ MHz) from FCLK_CLK1. Add toggle-handshake CDC bridge between AXI domain (100 MHz) and ATOMiK domain (200 MHz). This mirrors the proven Gowin architecture (25.2 MHz bus / 81 MHz core).

### 7.3 Software Access Path

```
Linux Userspace                          PL Fabric
┌─────────────────┐                   ┌──────────────────┐
│ ATOMiK App      │                   │                  │
│  mmap(/dev/uio0)│                   │  ATOMiK Core     │
│  *(base+0x0C) = │──► UIO Driver ──► │  AXI4-Lite       │
│     delta_lo    │    (kernel)       │  Wrapper         │
│  *(base+0x10) = │    ┌────────┐    │                  │
│     delta_hi    │    │ GP AXI │    │  Register Decode │
│                 │    │  M0    │───►│  CDC Bridge      │
│  state = read   │    └────────┘    │  Delta Acc Core  │
│   (base+0x14)   │                   │                  │
└─────────────────┘                   └──────────────────┘
```

Compared to the Tang Nano 9K path (bare-metal `volatile uint32_t*` over PicoRV32 bus), the Zynq path adds:
- A Linux kernel layer (UIO driver + mmap)
- AXI interconnect (instead of direct valid/ready)
- Cache effects (L1/L2 -- mitigated by mapping ATOMiK region as device memory)

The additional layers add microseconds of initial setup but negligible per-access overhead once the mmap region is established. The `volatile` access pattern through mmap produces the same single-word uncached reads/writes as bare-metal MMIO.

### 7.4 Key Differences from Tang Nano 9K Integration

| Aspect | Tang Nano 9K | AX7020 (Zynq) |
|--------|:----------:|:-------------:|
| **Bus Protocol** | PicoRV32 valid/ready | AXI4-Lite |
| **Bus Width** | 32-bit | 32-bit (GP port) |
| **Bus Clock** | 25.2 MHz | 100 MHz |
| **CPU** | PicoRV32 (soft, 25.2 MHz) | Cortex-A9 (hard, 667 MHz) |
| **OS** | Bare-metal | Linux |
| **Access Method** | `volatile uint32_t*` | `mmap` + `volatile uint32_t*` |
| **Address** | `0xC0000000` (S3 slot) | `0x43C00000` (GP0 range) |
| **CDC** | Toggle-handshake (25.2/81 MHz) | None initially (single clock) |
| **Wrapper** | `atomik_bus_wrapper.v` | `atomik_axi4lite_wrapper.v` |

---

## Appendix A: PS Configuration Checklist

Before generating the Vivado block design, verify these PS settings:

- [ ] PS_CLK frequency: 33.333 MHz
- [ ] ARM PLL: 667 MHz (CPU_6x4x)
- [ ] DDR PLL: 533 MHz (DDR3-1066)
- [ ] DDR3 part number matches board BOM
- [ ] DDR3 data width: 32-bit
- [ ] FCLK_CLK0: 100 MHz (enabled)
- [ ] FCLK_CLK1: 50 MHz (enabled)
- [ ] M_AXI_GP0: enabled
- [ ] UART0 on MIO 48-49: enabled
- [ ] Ethernet 0 on MIO: enabled
- [ ] SD 0 on MIO: enabled
- [ ] QSPI on MIO 1-6: enabled
- [ ] USB 0 on MIO: enabled (if USB needed)
- [ ] I2C 0 on MIO 50-51: enabled (if I2C needed)

## Appendix B: Key UG585 TRM References

| Section | Topic | Relevance |
|---------|-------|-----------|
| Ch. 3 | Application Processing Unit | CPU clocks, caches, SCU |
| Ch. 4 | Interrupts | GIC configuration, IRQ_F2P |
| Ch. 10 | DDR Memory Controller | DDR3 timing, training |
| Ch. 17 | PS-PL AXI Interfaces | GP, HP, ACP port details |
| Ch. 18 | PL Configuration | PCAP, bitstream loading |
| Ch. 25 | UART Controller | UART0 register map |
| Ch. 26 | Boot and Configuration | Boot sequence, FSBL |
| Appendix B | PS MIO Signal List | MIO multiplexing options |

---

*Last Updated: March 7, 2026*
