# AXI Integration Guide -- AXI4-Lite Wrapper for ATOMiK

**Date:** March 7, 2026
**Status:** Pre-board (design specification from UG585, AMBA AXI spec, and v3 LSU design)
**Target:** ALINX AX7020 (XC7Z020-2CLG400I)
**Audience:** Developers implementing the ATOMiK AXI4-Lite wrapper and Vivado IP integration
**Mirror of:** [WISHBONE_PERIPHERAL_GUIDE.md](../gowin/WISHBONE_PERIPHERAL_GUIDE.md) (Gowin/PicoRV32 equivalent)

---

## Table of Contents

1. [AXI4-Lite Protocol Summary](#1-axi4-lite-protocol-summary)
2. [Wishbone vs AXI4-Lite Comparison](#2-wishbone-vs-axi4-lite-comparison)
3. [ATOMiK Register Map Design](#3-atomik-register-map-design)
4. [32-bit AXI to 64-bit Datapath](#4-32-bit-axi-to-64-bit-datapath)
5. [Verilog Wrapper Template](#5-verilog-wrapper-template)
6. [Vivado IP Packaging](#6-vivado-ip-packaging)
7. [UIO Driver Mapping](#7-uio-driver-mapping)
8. [CDC Considerations](#8-cdc-considerations)
9. [Performance Considerations](#9-performance-considerations)

---

## 1. AXI4-Lite Protocol Summary

AXI4-Lite is a simplified subset of the AMBA AXI4 protocol designed for low-throughput, register-level access to peripherals. It is the standard bus interface for custom IP in Xilinx Vivado designs.

### 1.1 Channel Architecture

AXI4-Lite uses 5 independent channels. Each channel has its own valid/ready handshake:

```
                    AXI4-Lite Channels
                    ==================

  Master (PS)                              Slave (ATOMiK)
  ┌──────────┐                             ┌──────────┐
  │          │──── Write Address (AW) ────►│          │
  │          │──── Write Data    (W)  ────►│          │
  │          │◄─── Write Response (B) ─────│          │
  │          │                             │          │
  │          │──── Read Address  (AR) ────►│          │
  │          │◄─── Read Data     (R)  ─────│          │
  └──────────┘                             └──────────┘
```

### 1.2 Write Transaction

A write transaction uses three channels in sequence:

```
         AW Channel          W Channel           B Channel
         ──────────          ─────────           ─────────
  ACLK   ─┐ ┌─┐ ┌─┐ ┌─    ─┐ ┌─┐ ┌─┐ ┌─    ─┐ ┌─┐ ┌─┐ ┌─
          └─┘ └─┘ └─┘      └─┘ └─┘ └─┘      └─┘ └─┘ └─┘

  AWVALID ─────┐
               └──────     (address accepted)
  AWREADY ───────┐
                 └────
  AWADDR  ═══╤═══╗
             │REG│
             ╘═══╝

  WVALID              ─────┐
                            └──
  WREADY              ───────┐
                             └──
  WDATA               ═══╤═══╗
                         │DAT│
                         ╘═══╝
  WSTRB               ═══╤═══╗
                         │SEL│
                         ╘═══╝

  BVALID                          ─────┐
                                       └──
  BREADY                          ───────┐
                                         └──
  BRESP                           ═══╤═══╗
                                     │OK │
                                     ╘═══╝
```

**Write channel signals:**

| Signal | Width | Direction | Description |
|--------|:-----:|:---------:|-------------|
| AWADDR | 8+ | Master -> Slave | Write address |
| AWVALID | 1 | Master -> Slave | Write address valid |
| AWREADY | 1 | Slave -> Master | Write address ready |
| AWPROT | 3 | Master -> Slave | Protection type (usually ignored) |
| WDATA | 32 | Master -> Slave | Write data |
| WSTRB | 4 | Master -> Slave | Write byte strobes |
| WVALID | 1 | Master -> Slave | Write data valid |
| WREADY | 1 | Slave -> Master | Write data ready |
| BRESP | 2 | Slave -> Master | Write response (OKAY=2'b00, SLVERR=2'b10) |
| BVALID | 1 | Slave -> Master | Write response valid |
| BREADY | 1 | Master -> Slave | Write response ready |

### 1.3 Read Transaction

A read transaction uses two channels:

| Signal | Width | Direction | Description |
|--------|:-----:|:---------:|-------------|
| ARADDR | 8+ | Master -> Slave | Read address |
| ARVALID | 1 | Master -> Slave | Read address valid |
| ARREADY | 1 | Slave -> Master | Read address ready |
| ARPROT | 3 | Master -> Slave | Protection type (usually ignored) |
| RDATA | 32 | Slave -> Master | Read data |
| RRESP | 2 | Slave -> Master | Read response (OKAY=2'b00, SLVERR=2'b10) |
| RVALID | 1 | Slave -> Master | Read data valid |
| RREADY | 1 | Master -> Slave | Read data ready |

### 1.4 Key Protocol Properties

- **All transfers are single-beat** -- no burst support (unlike full AXI4)
- **Each channel has independent valid/ready handshake** -- transfer occurs when both valid AND ready are high on the rising clock edge
- **Write address and data can arrive in any order** -- the slave must handle AW before W, W before AW, or simultaneous
- **Response is mandatory** -- every write gets a BRESP, every read gets RRESP+RDATA
- **AWPROT/ARPROT** are typically tied to 3'b000 and ignored by the slave

---

## 2. Wishbone vs AXI4-Lite Comparison

This table compares the two bus protocols used across ATOMiK platforms:

| Feature | Wishbone B4 (PicoRV32) | AXI4-Lite (Zynq) |
|---------|:----------------------:|:-----------------:|
| **Channels** | 1 (shared R/W) | 5 (AW, W, B, AR, R) |
| **Data Width** | 32-bit | 32-bit |
| **Address Width** | 32-bit | Configurable (8-64 bit) |
| **Burst Support** | No | No |
| **Handshake** | `stb`+`cyc` / `ack` | `valid` / `ready` per channel |
| **Write vs Read** | `we` signal | Separate channels |
| **Response Channel** | None (ack only) | Explicit BRESP/RRESP |
| **Byte Strobes** | `sel[3:0]` | `wstrb[3:0]` |
| **Error Reporting** | `err` (optional) | SLVERR in BRESP/RRESP |
| **Complexity** | Low (~10 signals) | Medium (~25 signals) |
| **Tooling** | Manual instantiation | Vivado IP Integrator auto-connection |
| **Pipeline** | No | Yes (address and data can overlap) |
| **Used By** | Tang Nano 9K (Gowin SoC) | AX7020 (Zynq SoC) |

**Practical impact for ATOMiK:** The wrapper module is larger for AXI4-Lite (~150 lines vs. ~50 lines for Wishbone) due to the separate channel state machines. However, the core ATOMiK logic is identical -- only the bus interface changes.

---

## 3. ATOMiK Register Map Design

### 3.1 Base Address

| Parameter | Value |
|-----------|-------|
| **Base Address** | `0x43C00000` |
| **Address Range** | 256 bytes (8-bit address, only 40 bytes used) |
| **AXI GP Port** | M_AXI_GP0 |

The base address `0x43C00000` is within the GP0 address range (`0x40000000`-`0x7FFFFFFF`) and is assigned by the Vivado Address Editor. This is a standard Vivado address for custom AXI peripherals.

### 3.2 Register Map

| Offset | Name | Access | Width | Description |
|:------:|------|:------:|:-----:|-------------|
| `0x00` | LOAD_ADDR | W | 32 | Set active address: `addr[7:0]` in bits [7:0], upper bits ignored |
| `0x04` | LOAD_DATA_LO | W | 32 | Initial state bits [31:0] (buffered, no operation triggered) |
| `0x08` | LOAD_DATA_HI | W | 32 | Initial state bits [63:32]; **writing triggers LOAD operation** |
| `0x0C` | ACCUM_LO | W | 32 | Delta bits [31:0] (buffered, no operation triggered) |
| `0x10` | ACCUM_HI | W | 32 | Delta bits [63:32]; **writing triggers ACCUM operation** |
| `0x14` | STATE_LO | R | 32 | Current state bits [31:0] (snapshot latched at read time) |
| `0x18` | STATE_HI | R | 32 | Current state bits [63:32] (latched when STATE_LO is read) |
| `0x1C` | STATUS | R | 32 | Status and identification register (see bit fields below) |
| `0x20` | SWAP_ADDR | W | 32 | `addr[7:0]` in bits [7:0]; **writing triggers SWAP operation** |
| `0x24` | CONFIG | R/W | 32 | Configuration register (see bit fields below) |

### 3.3 STATUS Register Bit Fields (Offset `0x1C`, Read-Only)

| Bits | Field | Description |
|:----:|-------|-------------|
| [0] | `acc_zero` | 1 if accumulator is all zeros (no deltas accumulated since last LOAD) |
| [7:1] | Reserved | Read as zero |
| [15:8] | `n_banks` | Number of ATOMiK banks instantiated (1-16) |
| [23:16] | `version` | Hardware version identifier (`0x01` for initial release) |
| [31:24] | Reserved | Read as zero |

### 3.4 CONFIG Register Bit Fields (Offset `0x24`, Read/Write)

| Bits | Field | Description |
|:----:|-------|-------------|
| [0] | `enable` | 1 = ATOMiK core enabled, 0 = held in reset |
| [7:1] | Reserved | Write as zero |
| [15:8] | `bank_select` | Active bank index for multi-bank configurations (0-indexed) |
| [31:16] | Reserved | Write as zero |

### 3.5 Register Map Compatibility

This register map is identical to the one used on the Tang Nano 9K (see [WISHBONE_PERIPHERAL_GUIDE.md](../gowin/WISHBONE_PERIPHERAL_GUIDE.md)), with only the base address changed:

| Platform | Base Address | Bus Protocol | Register Offsets |
|----------|:----------:|:------------:|:----------------:|
| Tang Nano 9K | `0xC0000000` | PicoRV32 valid/ready | Same (0x00-0x24) |
| AX7020 Zynq | `0x43C00000` | AXI4-Lite | Same (0x00-0x24) |

This means the ATOMiK software API (C header, register access patterns) is portable between platforms. Only the base address macro changes.

---

## 4. 32-bit AXI to 64-bit Datapath

### 4.1 The Width Mismatch Problem

The ATOMiK core operates on 64-bit values (initial_state, delta, current_state), but the AXI4-Lite GP port is 32 bits wide. This requires a two-register access pattern for every 64-bit value.

### 4.2 Write Pattern: LO First, HI Triggers

For 64-bit write operations (LOAD and ACCUM), the software writes the low 32 bits first, which are buffered in a holding register. Writing the high 32 bits assembles the full 64-bit value and triggers the operation:

```
   Software                         Hardware (AXI Wrapper)
   ────────                         ─────────────────────
   Write ACCUM_LO (0x0C)  ────►    delta_buf[31:0] = WDATA
                                    (no operation triggered)

   Write ACCUM_HI (0x10)  ────►    delta[63:32] = WDATA
                                    delta[31:0]  = delta_buf[31:0]
                                    ──► TRIGGER ACCUM operation
```

This is the same pattern used by the v3 LSU 64-to-32 adapter (`atomik_v3_lsu.v`), proven in simulation and hardware.

### 4.3 Read Pattern: LO Latches, HI Returns Latched

For 64-bit read operations (STATE), reading the low register latches both halves of the current state atomically. Reading the high register returns the latched upper half:

```
   Software                         Hardware (AXI Wrapper)
   ────────                         ─────────────────────
   Read STATE_LO (0x14)   ────►    snapshot = current_state (atomic 64-bit latch)
                           ◄────    RDATA = snapshot[31:0]

   Read STATE_HI (0x18)   ────►    (no new latch)
                           ◄────    RDATA = snapshot[63:32]
```

This ensures the two 32-bit reads return a consistent 64-bit value, even if the ATOMiK core updates the state between the two reads.

### 4.4 C Access Example

```c
#include <stdint.h>

#define ATOMIK_BASE       0x43C00000

#define ATOMIK_LOAD_ADDR     (*(volatile uint32_t*)(ATOMIK_BASE + 0x00))
#define ATOMIK_LOAD_DATA_LO  (*(volatile uint32_t*)(ATOMIK_BASE + 0x04))
#define ATOMIK_LOAD_DATA_HI  (*(volatile uint32_t*)(ATOMIK_BASE + 0x08))
#define ATOMIK_ACCUM_LO      (*(volatile uint32_t*)(ATOMIK_BASE + 0x0C))
#define ATOMIK_ACCUM_HI      (*(volatile uint32_t*)(ATOMIK_BASE + 0x10))
#define ATOMIK_STATE_LO      (*(volatile uint32_t*)(ATOMIK_BASE + 0x14))
#define ATOMIK_STATE_HI      (*(volatile uint32_t*)(ATOMIK_BASE + 0x18))
#define ATOMIK_STATUS        (*(volatile uint32_t*)(ATOMIK_BASE + 0x1C))
#define ATOMIK_SWAP_ADDR     (*(volatile uint32_t*)(ATOMIK_BASE + 0x20))
#define ATOMIK_CONFIG        (*(volatile uint32_t*)(ATOMIK_BASE + 0x24))

// ── LOAD operation: set initial state at address 5 ──
static inline void atomik_load(uint8_t addr, uint64_t initial_state) {
    ATOMIK_LOAD_ADDR    = addr;
    ATOMIK_LOAD_DATA_LO = (uint32_t)(initial_state);
    ATOMIK_LOAD_DATA_HI = (uint32_t)(initial_state >> 32);  // triggers LOAD
}

// ── ACCUM operation: accumulate delta ──
static inline void atomik_accum(uint64_t delta) {
    ATOMIK_ACCUM_LO = (uint32_t)(delta);
    ATOMIK_ACCUM_HI = (uint32_t)(delta >> 32);              // triggers ACCUM
}

// ── READ state ──
static inline uint64_t atomik_read_state(void) {
    uint32_t lo = ATOMIK_STATE_LO;                           // latches snapshot
    uint32_t hi = ATOMIK_STATE_HI;                           // returns latched HI
    return ((uint64_t)hi << 32) | lo;
}

// ── SWAP operation: swap reference state at address ──
static inline void atomik_swap(uint8_t addr) {
    ATOMIK_SWAP_ADDR = addr;                                 // triggers SWAP
}

// ── STATUS read ──
static inline int atomik_acc_zero(void)   { return ATOMIK_STATUS & 0x1; }
static inline int atomik_n_banks(void)    { return (ATOMIK_STATUS >> 8) & 0xFF; }
static inline int atomik_version(void)    { return (ATOMIK_STATUS >> 16) & 0xFF; }
```

For Linux userspace, replace the `ATOMIK_BASE` with the mmap'd pointer (see [Section 7](#7-uio-driver-mapping)).

### 4.5 Write Ordering Guarantee

On AXI4-Lite, the PS AXI interconnect guarantees that writes from a single master to the same slave arrive in order. This means:
- Write to ACCUM_LO (0x0C) is **guaranteed** to complete before write to ACCUM_HI (0x10)
- No memory barrier or fence instruction is needed between the two writes
- The `volatile` qualifier prevents the compiler from reordering

This is stronger than the Tang Nano 9K, where PicoRV32's in-order execution provides the same guarantee by construction (no out-of-order memory system to worry about).

---

## 5. Verilog Wrapper Template

### 5.1 Module Port List

```verilog
// =============================================================================
// ATOMiK AXI4-Lite Wrapper
//
// Wraps the ATOMiK delta accumulator core with an AXI4-Lite slave interface
// for integration into Zynq PS-PL designs via M_AXI_GP0.
//
// Two clock domains are supported:
//   - S_AXI_ACLK: AXI bus clock (typically FCLK_CLK0, 100 MHz)
//   - atomik_clk:  ATOMiK core clock (FCLK_CLK1 or MMCM output)
//
// If atomik_clk == S_AXI_ACLK (single-clock mode), the CDC bridge
// is bypassed and the core runs directly on the AXI clock.
// =============================================================================

`timescale 1ns / 1ps

module atomik_axi4lite_wrapper #(
    parameter C_S_AXI_DATA_WIDTH = 32,
    parameter C_S_AXI_ADDR_WIDTH = 8,
    parameter N_BANKS            = 1,
    parameter SINGLE_CLOCK       = 1    // 1 = no CDC, 0 = separate atomik_clk
)(
    // =========================================================================
    // AXI4-Lite Slave Interface
    // =========================================================================
    input  wire                                S_AXI_ACLK,
    input  wire                                S_AXI_ARESETN,

    // Write Address Channel
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]       S_AXI_AWADDR,
    input  wire [2:0]                          S_AXI_AWPROT,
    input  wire                                S_AXI_AWVALID,
    output wire                                S_AXI_AWREADY,

    // Write Data Channel
    input  wire [C_S_AXI_DATA_WIDTH-1:0]       S_AXI_WDATA,
    input  wire [(C_S_AXI_DATA_WIDTH/8)-1:0]   S_AXI_WSTRB,
    input  wire                                S_AXI_WVALID,
    output wire                                S_AXI_WREADY,

    // Write Response Channel
    output wire [1:0]                          S_AXI_BRESP,
    output wire                                S_AXI_BVALID,
    input  wire                                S_AXI_BREADY,

    // Read Address Channel
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]       S_AXI_ARADDR,
    input  wire [2:0]                          S_AXI_ARPROT,
    input  wire                                S_AXI_ARVALID,
    output wire                                S_AXI_ARREADY,

    // Read Data Channel
    output wire [C_S_AXI_DATA_WIDTH-1:0]       S_AXI_RDATA,
    output wire [1:0]                          S_AXI_RRESP,
    output wire                                S_AXI_RVALID,
    input  wire                                S_AXI_RREADY,

    // =========================================================================
    // ATOMiK Core Clock Domain (ignored if SINGLE_CLOCK=1)
    // =========================================================================
    input  wire                                atomik_clk,
    input  wire                                atomik_rstn
);

    // ... (implementation: AXI state machine, register decode, CDC bridge,
    //      ATOMiK core instantiation -- see Section 5.2 for architecture)

endmodule
```

### 5.2 Internal Architecture

```
                 atomik_axi4lite_wrapper
┌──────────────────────────────────────────────────────────┐
│                                                          │
│  ┌──────────────┐    ┌──────────┐    ┌────────────────┐  │
│  │  AXI4-Lite   │    │  64-bit  │    │                │  │
│  │  State       │───►│  Assemble│───►│  CDC Bridge    │  │
│  │  Machine     │    │  Buffer  │    │  (toggle-      │  │
│  │              │◄───│  (LO/HI  │◄───│   handshake)   │  │
│  │  AW/W/B      │    │   split) │    │                │  │
│  │  AR/R        │    │          │    │  ── or ──      │  │
│  │              │    │          │    │  Direct (if     │  │
│  └──────────────┘    └──────────┘    │  SINGLE_CLOCK) │  │
│   S_AXI_ACLK domain                 └───────┬────────┘  │
│                                              │           │
│                                     ┌────────┴────────┐  │
│                                     │  ATOMiK Core    │  │
│                                     │  (delta_acc,    │  │
│                                     │   state_rec,    │  │
│                                     │   N_BANKS)      │  │
│                                     └─────────────────┘  │
│                                      atomik_clk domain   │
└──────────────────────────────────────────────────────────┘
```

### 5.3 AXI State Machine Overview

The AXI4-Lite slave state machine handles the five channels. A simplified flow:

```verilog
// ── Write address/data capture ──
// Accept AW and W independently (they may arrive in either order).
// When both are received, execute the write and issue B response.

reg aw_received, w_received;
reg [C_S_AXI_ADDR_WIDTH-1:0] aw_addr_reg;
reg [C_S_AXI_DATA_WIDTH-1:0] w_data_reg;
reg [(C_S_AXI_DATA_WIDTH/8)-1:0] w_strb_reg;

// AWREADY: accept address when we don't already have one pending
assign S_AXI_AWREADY = ~aw_received;

// WREADY: accept data when we don't already have it pending
assign S_AXI_WREADY = ~w_received;

always @(posedge S_AXI_ACLK) begin
    if (!S_AXI_ARESETN) begin
        aw_received <= 1'b0;
        w_received  <= 1'b0;
    end else begin
        // Capture write address
        if (S_AXI_AWVALID && S_AXI_AWREADY) begin
            aw_addr_reg <= S_AXI_AWADDR;
            aw_received <= 1'b1;
        end
        // Capture write data
        if (S_AXI_WVALID && S_AXI_WREADY) begin
            w_data_reg <= S_AXI_WDATA;
            w_strb_reg <= S_AXI_WSTRB;
            w_received <= 1'b1;
        end
        // When both received: execute write, prepare BRESP
        if (aw_received && w_received) begin
            // Execute register write (decode aw_addr_reg, apply w_data_reg)
            // Assert BVALID...
            aw_received <= 1'b0;
            w_received  <= 1'b0;
        end
    end
end
```

The read path is simpler: accept ARADDR, decode the register, and return RDATA with RVALID.

---

## 6. Vivado IP Packaging

### 6.1 Option A: Create AXI4 Peripheral (Vivado Wizard)

Vivado provides a wizard that generates a complete AXI4-Lite slave template:

1. **Tools -> Create and Package New IP**
2. Select **Create AXI4 Peripheral**
3. Configure:
   - Name: `atomik_core`
   - Number of Registers: 10 (covers offsets 0x00-0x24)
   - Data Width: 32
   - Slave interface only
4. Select **Edit IP** to modify the generated template
5. Replace the auto-generated register logic with ATOMiK-specific decode
6. Add the ATOMiK core instantiation and CDC bridge inside the wrapper
7. Re-package the IP

This approach generates the boilerplate AXI state machine and lets you focus on the ATOMiK-specific logic.

### 6.2 Option B: Manual IP-XACT Packaging

For full control, package the wrapper manually:

```
atomik_ip/
├── component.xml           # IP-XACT descriptor
├── xgui/
│   └── atomik_core_v1_0.tcl  # Vivado GUI customization
├── hdl/
│   ├── atomik_axi4lite_wrapper.v
│   ├── atomik_cdc_bridge.v
│   ├── atomik_delta_acc.v
│   ├── atomik_state_rec.v
│   └── atomik_core.v
└── drivers/
    └── atomik_core_v1_0/
        └── src/
            ├── atomik_core.h     # Register macros
            └── atomik_core.c     # Bare-metal driver (optional)
```

Package steps:

1. **Tools -> Create and Package New IP -> Package a specified directory**
2. Point to the `atomik_ip/` directory
3. In the Package IP wizard:
   - **Identification:** Set vendor, library, name, version
   - **File Groups:** Add all HDL sources
   - **Ports and Interfaces:** Associate AXI4-Lite ports with a bus interface definition
   - **Addressing:** Set address range to 256 bytes
   - **Parameters:** Expose `N_BANKS` and `SINGLE_CLOCK` as GUI parameters
4. **Review and Package**

### 6.3 Block Design Integration

Once packaged, the IP appears in Vivado's IP Catalog. In a block design:

```
┌──────────────┐       ┌──────────────────┐       ┌──────────────┐
│  Zynq PS     │       │ AXI Interconnect │       │ ATOMiK Core  │
│              │       │                  │       │              │
│  M_AXI_GP0  ├──────►│  S00_AXI         │       │  S_AXI       │
│              │       │            M00 ──├──────►│              │
│  FCLK_CLK0  ├──────►│  ACLK            │       │  S_AXI_ACLK  │
│  FCLK_RST0  ├──────►│  ARESETN         │       │  S_AXI_ARSTN │
│              │       │                  │       │              │
│  FCLK_CLK1  ├───────┼──────────────────┼──────►│  atomik_clk  │
│  FCLK_RST1  ├───────┼──────────────────┼──────►│  atomik_rstn │
└──────────────┘       └──────────────────┘       └──────────────┘
```

The Vivado Address Editor will assign the base address (typically `0x43C00000` for the first custom peripheral on GP0). This address must match the device tree and UIO driver configuration.

### 6.4 Vivado TCL Automation

For scripted (non-GUI) builds:

```tcl
# Create block design
create_bd_design "atomik_system"

# Add Zynq PS
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7
# Apply board preset (sets DDR3, MIO, clocks automatically)
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]

# Enable M_AXI_GP0 and FCLK
set_property -dict [list \
    CONFIG.PCW_USE_M_AXI_GP0 {1} \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100} \
    CONFIG.PCW_FPGA1_PERIPHERAL_FREQMHZ {50} \
    CONFIG.PCW_EN_CLK1_PORT {1} \
] [get_bd_cells ps7]

# Add ATOMiK IP
create_bd_cell -type ip -vlnv user.org:user:atomik_core:1.0 atomik_0
set_property -dict [list CONFIG.N_BANKS {1} CONFIG.SINGLE_CLOCK {1}] \
    [get_bd_cells atomik_0]

# Connect AXI
apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config {Master "/ps7/M_AXI_GP0" Clk "Auto"} \
    [get_bd_intf_pins atomik_0/S_AXI]

# Validate and save
validate_bd_design
save_bd_design
```

---

## 7. UIO Driver Mapping

### 7.1 Device Tree Entry

The ATOMiK peripheral is exposed to Linux userspace through the UIO (Userspace I/O) framework. Add this entry to the device tree:

```dts
/ {
    amba {
        atomik@43c00000 {
            compatible = "generic-uio";
            reg = <0x43c00000 0x1000>;
            interrupt-parent = <&intc>;
            /* interrupts = <0 29 4>;  -- uncomment when IRQ is implemented */
        };
    };
};
```

| Field | Value | Notes |
|-------|-------|-------|
| `compatible` | `"generic-uio"` | Uses kernel's generic UIO driver (no custom kernel module needed) |
| `reg` | `<0x43c00000 0x1000>` | Base address and size (4 KB page, minimum mappable unit) |
| `interrupt-parent` | `<&intc>` | GIC interrupt controller |

### 7.2 Kernel Configuration

Ensure UIO is enabled in the Linux kernel:

```
CONFIG_UIO=y
CONFIG_UIO_PDRV_GENIRQ=y
```

For PetaLinux, these are typically enabled by default. For custom kernel builds, enable under `Device Drivers -> Userspace I/O drivers`.

### 7.3 Userspace Access

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define ATOMIK_MAP_SIZE  4096   // One page (minimum mmap granularity)

// Register offsets (same as bare-metal, relative to base)
#define REG_LOAD_ADDR     0x00
#define REG_LOAD_DATA_LO  0x04
#define REG_LOAD_DATA_HI  0x08
#define REG_ACCUM_LO      0x0C
#define REG_ACCUM_HI      0x10
#define REG_STATE_LO      0x14
#define REG_STATE_HI      0x18
#define REG_STATUS         0x1C
#define REG_SWAP_ADDR      0x20
#define REG_CONFIG         0x24

static volatile uint32_t *atomik_base;

int atomik_init(void) {
    int fd = open("/dev/uio0", O_RDWR);
    if (fd < 0) {
        perror("open /dev/uio0");
        return -1;
    }

    atomik_base = (volatile uint32_t *)mmap(
        NULL,
        ATOMIK_MAP_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0       // offset 0 = first memory region in device tree
    );

    if (atomik_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    // Verify ATOMiK is present
    uint32_t status = atomik_base[REG_STATUS / 4];
    int version = (status >> 16) & 0xFF;
    int n_banks = (status >> 8) & 0xFF;
    printf("ATOMiK v%d detected: %d bank(s)\n", version, n_banks);

    return fd;
}

void atomik_load(uint8_t addr, uint64_t initial_state) {
    atomik_base[REG_LOAD_ADDR / 4]    = addr;
    atomik_base[REG_LOAD_DATA_LO / 4] = (uint32_t)(initial_state);
    atomik_base[REG_LOAD_DATA_HI / 4] = (uint32_t)(initial_state >> 32);
}

void atomik_accum(uint64_t delta) {
    atomik_base[REG_ACCUM_LO / 4] = (uint32_t)(delta);
    atomik_base[REG_ACCUM_HI / 4] = (uint32_t)(delta >> 32);
}

uint64_t atomik_read_state(void) {
    uint32_t lo = atomik_base[REG_STATE_LO / 4];
    uint32_t hi = atomik_base[REG_STATE_HI / 4];
    return ((uint64_t)hi << 32) | lo;
}

void atomik_cleanup(int fd) {
    munmap((void *)atomik_base, ATOMIK_MAP_SIZE);
    close(fd);
}
```

### 7.4 Access Permissions

By default, `/dev/uio0` is accessible only to root. For non-root access, add a udev rule:

```
# /etc/udev/rules.d/99-atomik.rules
SUBSYSTEM=="uio", ATTR{name}=="atomik", MODE="0666"
```

Or run the application with `sudo`.

### 7.5 Comparison to Tang Nano 9K Access

| Aspect | Tang Nano 9K | AX7020 (Linux/UIO) |
|--------|:----------:|:------------------:|
| **Init** | None (bare-metal) | `open()` + `mmap()` |
| **Read/Write** | `*(volatile uint32_t*)addr` | `base[offset/4]` (via mmap) |
| **Latency** | ~120 ns (3 bus cycles @ 25.2 MHz) | ~50-100 ns (AXI + interconnect) |
| **Concurrency** | None (single core) | Multi-threaded (dual core) |
| **Cleanup** | None | `munmap()` + `close()` |

---

## 8. CDC Considerations

### 8.1 When CDC Is Needed

| Configuration | CDC Required? | Rationale |
|---------------|:------------:|-----------|
| ATOMiK on FCLK_CLK0 (100 MHz) | **No** | Same clock as AXI bus |
| ATOMiK on FCLK_CLK1 (50 MHz) | **Yes** | Different clock from AXI bus |
| ATOMiK on MMCM output (200+ MHz) | **Yes** | Independent PL clock |

### 8.2 Recommendation: Start Without CDC

For initial bring-up, use `SINGLE_CLOCK=1` and run ATOMiK on `S_AXI_ACLK` (FCLK_CLK0, 100 MHz). This eliminates CDC complexity entirely:

```
FCLK_CLK0 (100 MHz) ──► S_AXI_ACLK ──► AXI State Machine ──► ATOMiK Core
                                        (all in one clock domain)
```

At 100 MHz with N_BANKS=1, the ATOMiK core achieves 100 Mops/s -- already comparable to the Tang Nano 9K configuration (94.5 MHz validated). This is a safe starting point.

### 8.3 Adding CDC Later

When higher ATOMiK clock frequencies are needed, set `SINGLE_CLOCK=0` and provide a separate `atomik_clk` from a PL MMCM:

```
FCLK_CLK0 (100 MHz) ──► S_AXI_ACLK ──► AXI State Machine
                                               │
                                         CDC Bridge (toggle-handshake)
                                               │
FCLK_CLK1 ──► MMCM ──► 200 MHz ──► atomik_clk ──► ATOMiK Core
```

The toggle-handshake CDC bridge is the same architecture used on the Tang Nano 9K (`atomik_cdc_bridge.v`). The Verilog implementation is portable -- only the clock frequencies change.

### 8.4 CDC Bridge Architecture

The toggle-handshake protocol (identical to Gowin implementation):

```
AXI Domain (100 MHz)                    ATOMiK Domain (200 MHz)
────────────────────                    ──────────────────────
1. Latch addr/wdata/wstrb
2. Toggle req_toggle ──────2FF sync────► Detect req_edge
                                         3. Execute operation
                                         4. Latch result
                           ◄───2FF sync── Toggle ack_toggle
5. Detect ack_edge
6. Drive AXI RDATA/BRESP
```

**Key properties (same as Gowin CDC):**
- Only 1 bit crosses per direction (req_toggle, ack_toggle)
- Data is latched and stable before toggle transitions
- 2FF synchronizer chain per direction (Xilinx uses `FDRE` cells)
- Latency: ~4-6 AXI clock cycles per MMIO access (vs. ~3 bus cycles on Tang Nano 9K)
- Frequency-independent: works regardless of clock ratio

### 8.5 Reset Synchronization

The ATOMiK core domain reset must be synchronized independently:

```verilog
// 3-stage reset synchronizer into ATOMiK clock domain
wire atomik_rst_base_n = S_AXI_ARESETN & atomik_rstn;

reg [2:0] atomik_rst_sync;
always @(posedge atomik_clk or negedge atomik_rst_base_n) begin
    if (!atomik_rst_base_n)
        atomik_rst_sync <= 3'b000;
    else
        atomik_rst_sync <= {atomik_rst_sync[1:0], 1'b1};
end

wire atomik_rst_n = atomik_rst_sync[2];
```

**Critical lesson (from Tang Nano 9K bringup):** Do NOT gate `S_AXI_ARESETN` on the ATOMiK MMCM lock signal. If the MMCM fails to lock, gating the AXI reset prevents the entire PL bus from operating. Gate only the ATOMiK-domain reset on MMCM lock, not the AXI-domain reset.

---

## 9. Performance Considerations

### 9.1 GP Port Bandwidth

| Parameter | Value |
|-----------|-------|
| **Data Width** | 32 bits (4 bytes) |
| **Clock** | 100 MHz (FCLK_CLK0) |
| **Peak Bandwidth** | 400 MB/s |
| **Shared With** | Other PL peripherals on GP0 |
| **Protocol Overhead** | ~2-3 cycles per transfer (handshake) |
| **Effective Bandwidth** | ~200 MB/s (typical, with handshake overhead) |

### 9.2 ATOMiK Operation Throughput

Each ATOMiK operation (LOAD, ACCUM, SWAP) requires either 2 or 3 AXI write transactions (32-bit each). At 100 MHz AXI clock:

| Operation | AXI Writes | Cycles (est.) | Time (100 MHz) | Effective Rate |
|-----------|:----------:|:-------------:|:---------------:|:--------------:|
| LOAD | 3 (ADDR + LO + HI) | ~9-12 | 90-120 ns | ~8-11 M ops/s |
| ACCUM | 2 (LO + HI) | ~6-8 | 60-80 ns | ~12-17 M ops/s |
| READ | 2 (LO + HI) | ~6-8 | 60-80 ns | ~12-17 M ops/s |
| SWAP | 1 (ADDR) | ~3-4 | 30-40 ns | ~25-33 M ops/s |

These estimates assume single-clock mode (no CDC latency). With CDC, add ~2-4 cycles per operation.

### 9.3 Comparison to Tang Nano 9K

| Metric | Tang Nano 9K | AX7020 (est.) | Improvement |
|--------|:----------:|:-------------:|:-----------:|
| **Bus clock** | 25.2 MHz | 100 MHz | 4x |
| **ACCUM latency** | ~70 cycles (~2.8 us) | ~8 cycles (~80 ns) | ~35x |
| **CPU overhead** | ~285 cy roundtrip | ~15 cy roundtrip (est.) | ~19x |
| **Software speed** | 25.2 MHz RV32I | 667 MHz ARMv7-A | ~100x (with caches) |

The combined effect of a faster bus, faster CPU, and caches makes the Zynq platform orders of magnitude faster for ATOMiK workloads that involve significant software processing between hardware operations.

### 9.4 Future: HP Port with DMA

For bulk state transfer workloads (e.g., initializing thousands of addresses), the GP port becomes a bottleneck. A future optimization path:

1. **DMA engine in PL** reads a command buffer from DDR via S_AXI_HP0
2. Commands are (addr, initial_state) pairs packed in DDR
3. DMA engine feeds ATOMiK's LOAD interface at wire speed
4. CPU sets up the command buffer, kicks DMA, then polls for completion

This would achieve near-peak HP port bandwidth (~1,200 MB/s per port) for bulk initialization, compared to ~200 MB/s through GP MMIO. This is a Phase 2+ optimization and not required for initial bring-up.

---

## Appendix A: Quick Reference -- File Locations

| File | Path | Description |
|------|------|-------------|
| AXI wrapper (planned) | `hardware/zynq/rtl/atomik_axi4lite_wrapper.v` | AXI4-Lite slave + ATOMiK core |
| CDC bridge (portable) | `hardware/zynq/rtl/atomik_cdc_bridge.v` | Toggle-handshake CDC (adapted from Gowin) |
| C header (userspace) | `hardware/zynq/sw/atomik.h` | Register macros + inline API |
| Device tree fragment | `hardware/zynq/dts/atomik.dtsi` | UIO device tree entry |
| Vivado block design | `hardware/zynq/bd/atomik_system.tcl` | TCL script for block design |
| Zynq PS configuration | [ZYNQ_PS_CONFIGURATION.md](ZYNQ_PS_CONFIGURATION.md) | PS architecture, DDR3, MIO, clocks |
| Gowin equivalent | [../gowin/WISHBONE_PERIPHERAL_GUIDE.md](../gowin/WISHBONE_PERIPHERAL_GUIDE.md) | Wishbone bus peripheral guide (Tang Nano 9K) |

## Appendix B: AXI4-Lite vs AXI4 Full vs AXI3

| Feature | AXI3 | AXI4 Full | AXI4-Lite |
|---------|:----:|:---------:|:---------:|
| Burst length | 1-16 | 1-256 | 1 (fixed) |
| Burst type | FIXED, INCR, WRAP | FIXED, INCR, WRAP | N/A |
| Data width | 8-1024 | 8-1024 | 32 or 64 |
| Write interleaving | Yes | No | N/A |
| Exclusive access | Yes | Yes | No |
| QoS | No | Yes | No |
| Regions | No | Yes | No |
| Complexity | High | High | Low |
| **Zynq GP port** | **AXI3 master** | -- | **Compatible slave** |

The Zynq M_AXI_GP ports are AXI3 masters, but AXI4-Lite slaves are backward-compatible with AXI3 masters. The Vivado AXI Interconnect IP handles protocol bridging automatically.

---

*Last Updated: March 7, 2026*
