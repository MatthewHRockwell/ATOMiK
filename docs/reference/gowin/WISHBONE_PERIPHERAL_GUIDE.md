# Wishbone Peripheral Guide -- Adding Custom Peripherals to the ATOMiK SoC

**Date:** February 14, 2026
**Target:** Tang Nano 9K (GW1NR-LV9QN88PC6/I5) + PicoRV32 SoC
**Reference:** Gowin IPUG914 -- PicoRV32 Hardware Design Reference Manual
**Audience:** Developers adding new MMIO peripherals to the ATOMiK production SoC

---

## Table of Contents

1. [Overview: Gowin Bus Extension Options](#1-overview-gowin-bus-extension-options)
2. [Our Picotiny SoC Bus Architecture](#2-our-picotiny-soc-bus-architecture)
3. [Adding a New MMIO Peripheral (Step-by-Step)](#3-adding-a-new-mmio-peripheral-step-by-step)
4. [Address Space Allocation Strategy](#4-address-space-allocation-strategy)
5. [Interrupt Routing](#5-interrupt-routing)
6. [CDC Considerations for Multi-Clock Peripherals](#6-cdc-considerations-for-multi-clock-peripherals)
7. [Resource Budget Impact](#7-resource-budget-impact)

---

## 1. Overview: Gowin Bus Extension Options

The Gowin PicoRV32 IP (IPUG914) provides two bus extension points for attaching custom peripherals. These are relevant as reference material even though our SoC uses the open-source PicoRV32 directly.

### 1.1 Wishbone Bus Interface (OPEN WB INTERFACE)

The Wishbone subsystem is the primary peripheral bus in the Gowin PicoRV32 IP. The built-in peripherals on this bus include:

- UART
- I2C Master
- SPI Master
- SPI Slave
- GPIO
- ADV SPI-Flash controller

The OPEN WB INTERFACE exposes a standard Wishbone B4 slave port for custom peripherals:

| Signal | Direction | Width | Description |
|--------|-----------|-------|-------------|
| `slv_ext_stb_o` | Output | 1 | Strobe -- indicates valid transfer cycle |
| `slv_ext_we_o` | Output | 1 | Write enable (1 = write, 0 = read) |
| `slv_ext_cyc_o` | Output | 1 | Bus cycle active |
| `slv_ext_ack_i` | Input | 1 | Slave acknowledge |
| `slv_ext_adr_o` | Output | 32 | Address bus |
| `slv_ext_wdata_o` | Output | 32 | Write data |
| `slv_ext_rdata_i` | Input | 32 | Read data |
| `slv_ext_sel_o` | Output | 4 | Byte select (one bit per byte lane) |

Wishbone transactions follow the standard handshake: master asserts `stb` and `cyc`, slave responds with `ack` when data is ready. For single-cycle peripherals, `ack` can be combinationally derived from `stb & cyc`.

### 1.2 AHB Bus Interface (OPEN AHB INTERFACE)

The OPEN AHB INTERFACE provides an AMBA AHB-Lite slave port, primarily intended for high-bandwidth peripherals or memory-mapped regions.

| Signal | Direction | Width | Description |
|--------|-----------|-------|-------------|
| `haddr` | Output | 32 | AHB address bus |
| `hwdata` | Output | 32 | Write data |
| `hrdata` | Input | 32 | Read data |
| `hwrite` | Output | 1 | Transfer direction (1 = write) |
| `hsize` | Output | 3 | Transfer size (byte, halfword, word) |
| `hburst` | Output | 3 | Burst type |
| `htrans` | Output | 2 | Transfer type (IDLE, BUSY, NONSEQ, SEQ) |
| `hready` | Input | 1 | Transfer complete |
| `hresp` | Input | 2 | Transfer response (OKAY, ERROR) |
| `hsel` | Output | 1 | Slave select |

**AHB address range:** `0x8000_0000` to `0xFFFF_0000`.

### 1.3 Gowin IP Core Generator Settings

To use either extension interface in the Gowin IP flow:

1. Open the IP Core Generator for PicoRV32
2. Enable **"OPEN WB INTERFACE"** or **"OPEN AHB INTERFACE"** (or both)
3. Disable **"Use Gowin PicoRV32 as top module"** -- you must provide your own top-level wrapper
4. Instantiate the PicoRV32 IP and connect the extension port signals to your custom peripheral

### 1.4 External Interrupt Signals

Both bus interfaces share a common interrupt mechanism. The PicoRV32 exposes:

```
irq_in[31:20]  -- 12 external interrupt lines reserved for peripherals
```

These 12 lines are directly available for custom peripherals. Mapping is first-come, first-served. See [Section 5](#5-interrupt-routing) for routing details.

### 1.5 Why This Matters (and Why We Did Not Use It)

Our ATOMiK SoC is **not** based on the Gowin PicoRV32 IP core. We use the open-source picotiny reference design, which instantiates PicoRV32 directly as a soft-core. This gives us full control over the bus fabric, memory map, and peripheral integration -- at the cost of implementing our own address decoding and bus arbitration.

The Gowin IP's Wishbone and AHB interfaces are documented here for reference because:

- They define the standard approach if you were using the Gowin IP flow
- The signal naming conventions appear in many Gowin application notes
- Future integrations may bridge between our bus and Gowin IP peripherals

---

## 2. Our Picotiny SoC Bus Architecture

### 2.1 Bus Protocol: valid/ready (not Wishbone)

The picotiny SoC (and our ATOMiK production SoC) uses PicoRV32's native **valid/ready** handshake protocol, not Wishbone.

| Signal | Direction | Width | Description |
|--------|-----------|-------|-------------|
| `mem_valid` | CPU -> Bus | 1 | CPU asserts when it has a pending memory transaction |
| `mem_ready` | Bus -> CPU | 1 | Peripheral asserts for one cycle when transaction completes |
| `mem_addr` | CPU -> Bus | 32 | Address |
| `mem_wdata` | CPU -> Bus | 32 | Write data |
| `mem_wstrb` | CPU -> Bus | 4 | Byte write strobes (0 = read transaction) |
| `mem_rdata` | Bus -> CPU | 32 | Read data (valid when `mem_ready` is high) |

**Key differences from Wishbone:**

- No `stb`/`cyc` -- a single `mem_valid` signal indicates an active transaction
- Write vs. read is determined by `mem_wstrb` (non-zero = write), not a separate `we` signal
- The CPU stalls until `mem_ready` is asserted -- there is no bus timeout in hardware
- `mem_ready` must be a single-cycle pulse; holding it high causes undefined behavior

### 2.2 Address Decoding: 1:4 Mux

The picotiny SoC uses a 1:4 address decode mux based on `mem_addr[31:30]`:

```
mem_addr[31:30]    Slot    Peripheral Group
─────────────────────────────────────────────────
       2'b00       S0      SPI Flash XIP (0x0000_0000)
       2'b01       S1      SRAM (0x4000_0000)
       2'b10       S2      Boot ROM + Config Peripherals (0x8000_0000)
       2'b11       S3      ATOMiK Delta Accumulator (0xC000_0000)
```

Each slot produces its own `mem_ready` and `mem_rdata`. The top-level mux selects the active response:

```verilog
// Simplified address decode from picotiny
wire sel_s0 = (mem_addr[31:30] == 2'b00);  // Flash XIP
wire sel_s1 = (mem_addr[31:30] == 2'b01);  // SRAM
wire sel_s2 = (mem_addr[31:30] == 2'b10);  // Boot ROM + peripherals
wire sel_s3 = (mem_addr[31:30] == 2'b11);  // ATOMiK

assign mem_ready = sel_s0 ? s0_ready :
                   sel_s1 ? s1_ready :
                   sel_s2 ? s2_ready :
                            s3_ready ;

assign mem_rdata = sel_s0 ? s0_rdata :
                   sel_s1 ? s1_rdata :
                   sel_s2 ? s2_rdata :
                            s3_rdata ;
```

Slot S2 has its own sub-decoder using `mem_addr[27:24]` to distinguish Boot ROM (`0x80xx`), SPI Flash Config (`0x81xx`), GPIO (`0x82xx`), and UART (`0x83xx`).

### 2.3 Current Memory Map

| Address Range | Slot | Peripheral | Size |
|---------------|------|-----------|------|
| `0x0000_0000 - 0x007F_FFFF` | S0 | SPI Flash XIP | 8 MB |
| `0x4000_0000 - 0x4000_1FFF` | S1 | SRAM | 8 KB |
| `0x8000_0000 - 0x8000_1FFF` | S2 | Boot ROM (ISP flasher) | 2 KB |
| `0x8100_0000 - 0x8100_000F` | S2 | SPI Flash Config | 16 B |
| `0x8200_0000 - 0x8200_000F` | S2 | GPIO (7-bit I/O) | 16 B |
| `0x8300_0000 - 0x8300_000F` | S2 | UART (115200, 8N1) | 16 B |
| `0xC000_0000 - 0xC000_001F` | S3 | ATOMiK Delta Accumulator | 32 B |

### 2.4 How ATOMiK Connects

The ATOMiK accelerator occupies the entire S3 slot (`mem_addr[31:30] == 2'b11`). The connection chain is:

```
PicoRV32 CPU  --(valid/ready)-->  atomik_bus_wrapper
                                        |
                                  atomik_cdc_bridge  (25.2 MHz <-> 81 MHz)
                                        |
                                  atomik_parallel_acc (core @ 81 MHz)
```

The bus wrapper (`atomik_bus_wrapper.v`) receives `mem_valid`, `mem_addr`, `mem_wdata`, `mem_wstrb` and drives `mem_ready`, `mem_rdata`. Internally it:

1. Passes the bus signals to `atomik_cdc_bridge` (bus-side clock domain)
2. The CDC bridge transfers the request to the core clock domain via toggle-handshake
3. The core-side logic decodes `mem_addr[4:2]` (register select within the 32-byte window) and executes the operation
4. Results propagate back through the CDC bridge, which asserts `mem_ready` with the response data

**Latency:** approximately 3 bus cycles (~120 ns at 25.2 MHz) per MMIO access due to CDC synchronizer chain.

---

## 3. Adding a New MMIO Peripheral (Step-by-Step)

This section walks through adding a new peripheral to the SoC. The process depends on where you place the peripheral in the address map.

### 3.1 Strategy A: Add to the S2 Sub-Decoder (Simple, Same Clock)

If your peripheral runs at the bus clock (25.2 MHz) and needs a small register window, the easiest approach is to add a new sub-slot in the S2 range.

**Step 1: Choose an address.** The S2 sub-decoder uses `mem_addr[27:24]` to route peripherals. Current allocation:

| `mem_addr[27:24]` | Address Prefix | Peripheral |
|-------------------|---------------|-----------|
| `4'h0` | `0x80xx_xxxx` | Boot ROM |
| `4'h1` | `0x81xx_xxxx` | SPI Flash Config |
| `4'h2` | `0x82xx_xxxx` | GPIO |
| `4'h3` | `0x83xx_xxxx` | UART |
| `4'h4` - `4'hF` | `0x84xx` - `0x8Fxx` | **Available** |

Pick the next unused slot. For example, a new timer peripheral would go at `0x8400_0000`.

**Step 2: Write the peripheral module.** Follow this Verilog template:

```verilog
// =============================================================================
// Minimal Bus Peripheral Template -- PicoRV32 valid/ready Protocol
//
// Drop-in template for adding MMIO peripherals to the ATOMiK SoC.
// Runs on the bus clock (25.2 MHz). For peripherals on a different clock
// domain, see Section 6 (CDC) and use atomik_bus_wrapper.v as the pattern.
//
// Example: 4-register peripheral with read/write support.
// =============================================================================

`timescale 1ns / 1ps

module my_peripheral (
    input  wire        clk,           // Bus clock (25.2 MHz)
    input  wire        resetn,        // Active-low reset

    // PicoRV32 Bus Interface
    input  wire        mem_valid,     // Transaction pending
    input  wire [31:0] mem_addr,      // Address
    input  wire [31:0] mem_wdata,     // Write data
    input  wire [3:0]  mem_wstrb,     // Byte write strobes (0 = read)
    output reg         mem_ready,     // Transaction complete (single-cycle pulse)
    output reg  [31:0] mem_rdata      // Read data
);

    // =========================================================================
    // Register File
    // =========================================================================

    reg [31:0] reg_ctrl;       // Offset 0x00 -- Control register (RW)
    reg [31:0] reg_status;     // Offset 0x04 -- Status register (RO)
    reg [31:0] reg_data_in;    // Offset 0x08 -- Data input (WO)
    reg [31:0] reg_data_out;   // Offset 0x0C -- Data output (RO)

    // Register select: use addr[3:2] for 4 registers (16-byte window)
    wire [1:0] reg_sel = mem_addr[3:2];

    // Write strobe: transaction is a write when any byte strobe is set
    wire is_write = |mem_wstrb;

    // =========================================================================
    // Bus Transaction Handling
    // =========================================================================

    // Prevent double-acknowledge: track whether we already responded
    // to the current valid assertion.
    reg ack_sent;

    always @(posedge clk or negedge resetn) begin
        if (!resetn) begin
            mem_ready    <= 1'b0;
            mem_rdata    <= 32'b0;
            ack_sent     <= 1'b0;
            reg_ctrl     <= 32'b0;
            reg_status   <= 32'b0;
            reg_data_in  <= 32'b0;
            reg_data_out <= 32'b0;
        end else begin
            mem_ready <= 1'b0;  // Default: not ready (single-cycle pulse)

            if (mem_valid && !ack_sent) begin
                mem_ready <= 1'b1;
                ack_sent  <= 1'b1;

                if (is_write) begin
                    // ---- WRITE ----
                    case (reg_sel)
                        2'b00: reg_ctrl    <= mem_wdata;  // 0x00: Control
                        2'b10: reg_data_in <= mem_wdata;  // 0x08: Data In
                        // 0x04 and 0x0C are read-only: writes are silently ignored
                        default: ;
                    endcase
                end else begin
                    // ---- READ ----
                    case (reg_sel)
                        2'b00: mem_rdata <= reg_ctrl;
                        2'b01: mem_rdata <= reg_status;
                        2'b10: mem_rdata <= reg_data_in;
                        2'b11: mem_rdata <= reg_data_out;
                    endcase
                end
            end

            // Reset ack_sent when valid deasserts (CPU has consumed the response)
            if (!mem_valid)
                ack_sent <= 1'b0;
        end
    end

endmodule
```

**Step 3: Wire it into the S2 sub-decoder.** In your top-level SoC file, add address decode logic and mux the new peripheral's ready/rdata into the S2 response:

```verilog
// New peripheral instance
wire        my_periph_ready;
wire [31:0] my_periph_rdata;

wire sel_my_periph = (mem_addr[31:24] == 8'h84);  // 0x84xx_xxxx

my_peripheral u_my_periph (
    .clk       (clk),
    .resetn    (resetn),
    .mem_valid (mem_valid & sel_my_periph),
    .mem_addr  (mem_addr),
    .mem_wdata (mem_wdata),
    .mem_wstrb (mem_wstrb),
    .mem_ready (my_periph_ready),
    .mem_rdata (my_periph_rdata)
);

// Update S2 ready/rdata mux to include the new peripheral
// (add alongside existing Boot ROM, SPI Config, GPIO, UART terms)
```

**Step 4: Add a C header.** Follow the pattern in `hardware/picorv32/firmware/atomik.h`:

```c
#define MY_PERIPH_BASE    0x84000000
#define MY_PERIPH_CTRL    (*(volatile uint32_t*)(MY_PERIPH_BASE + 0x00))
#define MY_PERIPH_STATUS  (*(volatile uint32_t*)(MY_PERIPH_BASE + 0x04))
#define MY_PERIPH_DATA_IN (*(volatile uint32_t*)(MY_PERIPH_BASE + 0x08))
#define MY_PERIPH_DATA_OUT (*(volatile uint32_t*)(MY_PERIPH_BASE + 0x0C))
```

**Step 5: Synthesize and verify.** Re-run Gowin synthesis and check timing reports. Ensure the new peripheral does not introduce combinational loops or degrade Fmax on the bus clock domain.

### 3.2 Strategy B: Add a New Top-Level Slot (Separate Address Space)

If your peripheral needs a large address window or runs on a different clock domain, consider adding a new top-level slot. However, the current 2-bit decode (`mem_addr[31:30]`) uses all 4 slots. To add a 5th top-level slot, you must either:

- **Sub-partition an existing slot** (e.g., split S3 so that `0xC0xx` is ATOMiK and `0xD0xx`+ is the new peripheral)
- **Expand the mux** to 3-bit decode using `mem_addr[31:29]`, which requires reworking existing address assignments

For most peripherals, Strategy A (S2 sub-slot) is strongly preferred.

### 3.3 Strategy C: CDC Wrapper for Cross-Domain Peripherals

If the peripheral operates on a clock other than 25.2 MHz, follow the pattern established by ATOMiK:

1. Write the peripheral's core logic in its own clock domain
2. Write a CDC bridge module (see `atomik_cdc_bridge.v` for the toggle-handshake pattern)
3. Write a bus wrapper that combines the CDC bridge with the core logic (see `atomik_bus_wrapper.v`)
4. Instantiate the wrapper in the top-level mux, providing both `clk` (bus) and `clk_core` (peripheral)

See [Section 6](#6-cdc-considerations-for-multi-clock-peripherals) for detailed CDC guidance.

---

## 4. Address Space Allocation Strategy

### 4.1 Planned Peripheral Assignments

The following address assignments are reserved for upcoming peripherals:

| Address | `mem_addr[27:24]` | Peripheral | Clock | Notes |
|---------|-------------------|-----------|-------|-------|
| `0x8400_0000` | `4'h4` | HDMI Framebuffer Control | 25.2 MHz | Register interface for framebuffer base address, resolution, enable. Framebuffer data path is separate (DMA from SRAM or flash). |
| `0x8500_0000` | `4'h5` | SPI Master for MAX3421E | 25.2 MHz | USB Host via MAX3421E. SPI clock, MOSI, MISO, CS, plus interrupt status. Enables USB keyboard/mouse input. |
| `0x8600_0000` | `4'h6` | Inter-Board Communication | 25.2 MHz | SPI or UART link between multiple Tang Nano 9K boards for distributed ATOMiK arrays. |
| `0x8700_0000` | `4'h7` | Timer / Watchdog | 25.2 MHz | Cycle counter, compare registers, interrupt generation. |
| `0x8800_0000` | `4'h8` | DMA Controller | 25.2 MHz | Bulk data transfer between SRAM and peripherals without CPU involvement. |
| `0x8900_0000` - `0x8F00_0000` | `4'h9` - `4'hF` | Reserved | -- | Available for future expansion. |

### 4.2 Address Map Design Rules

1. **Each S2 sub-peripheral gets a full 16 MB window** (`mem_addr[27:24]` decode), even though most use only 16-64 bytes. This simplifies decode logic (no narrow range comparisons).

2. **Register offsets within a peripheral use `mem_addr[N:2]`** where N depends on the number of registers. For a 4-register peripheral, N=3 (16-byte window). For ATOMiK's 7 registers, N=4 (32-byte window).

3. **Unused address bits are ignored** -- a read to `0x8300_1000` hits the same UART register as `0x8300_0000` (only the low bits matter). This is by design; adding full address decode for unused bits wastes LUTs.

4. **The S3 range (`0xC000_0000` - `0xFFFF_FFFF`) is entirely ATOMiK's** currently. If multi-bank support requires per-bank address windows, use `mem_addr[7:5]` as a bank select (stride of 32 bytes per bank), keeping everything within S3.

### 4.3 Register Naming Convention

Follow the ATOMiK register naming convention from `memory_map.md`:

- Short uppercase names: `CTRL`, `STATUS`, `DATA`, `CONFIG`
- Offsets in multiples of 4 (word-aligned)
- Document R/W/RO/WO access in the register map table
- Provide `#define` macros in a `.h` header file

---

## 5. Interrupt Routing

### 5.1 PicoRV32 Interrupt Architecture

PicoRV32 supports 32 interrupt lines via the `irq` input port. In the Gowin IP flow, the upper 12 bits are reserved for external peripherals:

```
irq[31:20]  -- 12 lines available for custom peripherals
irq[19:0]   -- Reserved for internal use (timer, software, etc.)
```

In our picotiny SoC, the IRQ mechanism is simpler. PicoRV32 must be instantiated with `ENABLE_IRQ = 1` and the `irq` port connected:

```verilog
picorv32 #(
    .ENABLE_IRQ(1),
    .ENABLE_IRQ_QREGS(1),
    .ENABLE_IRQ_TIMER(1)
) cpu (
    // ...
    .irq (irq_bus),
    .eoi (eoi_bus)
    // ...
);
```

### 5.2 Assigning Interrupts to Peripherals

Each peripheral drives a single bit of the `irq` bus. The recommended allocation:

| IRQ Bit | Peripheral | Trigger |
|---------|-----------|---------|
| `irq[20]` | UART RX Ready | Level (high while RX FIFO non-empty) |
| `irq[21]` | Timer Compare Match | Edge (pulses for 1 cycle) |
| `irq[22]` | MAX3421E SPI (USB) | Level (mirrors MAX3421E INT pin) |
| `irq[23]` | Inter-Board RX | Level (high while RX buffer non-empty) |
| `irq[24]` - `irq[31]` | Reserved | -- |

### 5.3 Interrupt Handling in Firmware

PicoRV32 uses custom instructions (`getq`, `setq`, `retirq`) for interrupt management. The interrupt handler in firmware:

```c
// In the IRQ handler (linked at the IRQ vector address)
void __attribute__((interrupt)) irq_handler(void) {
    uint32_t irqs = picorv32_getq(0);   // Read pending IRQs

    if (irqs & (1 << 20)) {
        // Handle UART RX
        uart_rx_handler();
    }
    if (irqs & (1 << 21)) {
        // Handle timer
        timer_handler();
    }

    picorv32_setq(0, 0);               // Clear handled IRQs
}
```

### 5.4 Current Status

The ATOMiK production SoC does **not** currently use interrupts. The CPU polls peripherals via MMIO reads. Interrupt support requires:

1. Enabling `ENABLE_IRQ` in the PicoRV32 instantiation
2. Adding an IRQ vector to the linker script
3. Wiring the `irq` bus to peripheral interrupt outputs
4. Writing the interrupt handler in firmware

This is a planned enhancement. Polling works for current workloads since ATOMiK MMIO access is the critical path and completes in ~3 bus cycles.

---

## 6. CDC Considerations for Multi-Clock Peripherals

### 6.1 When CDC Is Required

Any peripheral running on a clock other than the bus clock (25.2 MHz) requires clock domain crossing. In our SoC, the ATOMiK accelerator at 81 MHz is the only cross-domain peripheral, but future additions may include:

- High-speed SPI (running off a faster PLL output)
- Video pipeline (126 MHz HDMI serializer domain)
- Additional ATOMiK banks on a different PLL

### 6.2 Toggle-Handshake Protocol (Recommended)

The ATOMiK SoC uses a toggle-handshake CDC protocol, implemented in `atomik_cdc_bridge.v`. This is the recommended pattern for all cross-domain MMIO peripherals.

**How it works:**

```
Bus Domain (25.2 MHz)                   Core Domain (81 MHz)
─────────────────────                   ─────────────────────
1. Latch addr/wdata/wstrb
2. Toggle req_toggle ──────2FF sync────> Detect req_edge
                                         3. Execute operation
                                         4. Latch result
                           <───2FF sync── Toggle ack_toggle
5. Detect ack_edge
6. Drive mem_ready + mem_rdata
```

**Key properties:**

- Only 1 bit crosses in each direction (req_toggle, ack_toggle)
- Data is latched and stable *before* the toggle -- no multi-bit synchronization hazard
- 2FF synchronizer chain per direction (Gowin flip-flops with `DFFRE` cells)
- Latency: ~3 bus cycles per MMIO access

### 6.3 CDC Reset Synchronization

The core domain reset must be synchronized separately. Pattern from `atomik_bus_wrapper.v`:

```verilog
// 3-stage reset synchronizer into core clock domain
wire core_rst_base_n = resetn & core_pll_lock;

reg core_rst_sync_0, core_rst_sync_1, core_rst_sync_2;
always @(posedge clk_core or negedge core_rst_base_n) begin
    if (!core_rst_base_n) begin
        core_rst_sync_0 <= 1'b0;
        core_rst_sync_1 <= 1'b0;
        core_rst_sync_2 <= 1'b0;
    end else begin
        core_rst_sync_0 <= 1'b1;
        core_rst_sync_1 <= core_rst_sync_0;
        core_rst_sync_2 <= core_rst_sync_1;
    end
end

wire core_rst_n = core_rst_sync_2;
```

**Critical lesson from ATOMiK bringup:** Do NOT gate the bus-domain CPU reset on a secondary PLL lock signal. If the ATOMiK PLL fails to lock, gating the CPU reset on `atomik_pll_lock` prevents the CPU from booting at all. Keep secondary PLL resets independent -- gate only the *core-domain* reset, not the bus-domain reset.

### 6.4 Soft Reset Stretching

The CDC bridge delivers a soft-reset pulse that is 1 core clock cycle wide. Since the core clock (81 MHz) is faster than the bus clock (25.2 MHz), this pulse might be too narrow for some peripherals. The bus wrapper stretches it:

```verilog
reg [1:0] soft_reset_stretch;
always @(posedge clk_core or negedge core_rst_base_n) begin
    if (!core_rst_base_n)
        soft_reset_stretch <= 2'b0;
    else if (cdc_soft_reset)
        soft_reset_stretch <= 2'b11;
    else
        soft_reset_stretch <= {1'b0, soft_reset_stretch[1]};
end

wire core_rst_n = core_rst_sync_2 & ~soft_reset_stretch[0];
```

This produces a 2-cycle reset pulse in the core domain, which is sufficient for single-stage register initialization.

### 6.5 What NOT to Do

- **Do NOT pass multi-bit data through 2FF synchronizers.** Synchronize only 1-bit toggle signals. Data must be latched and stable before the toggle transitions.
- **Do NOT use asynchronous FIFOs for single-register MMIO.** Async FIFOs add ~100 LUT overhead and are only justified for streaming data paths (e.g., DMA).
- **Do NOT assume clock ratios are exact.** The 81 MHz and 25.2 MHz clocks come from independent PLLs. There is no fixed phase relationship. The toggle-handshake protocol is frequency-independent by design.

---

## 7. Resource Budget Impact

### 7.1 Current SoC Resource Usage

The production ATOMiK SoC on Tang Nano 9K (GW1NR-9, 8640 LUT4):

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| LUT4 | 3,838 | 8,640 | 44.4% |
| ALU (carry chain) | 707 | 6,693 | 10.6% |
| CLS (logic slices) | 3,103 | 4,320 | 71.8% |
| FF (flip-flops) | ~2,500 | 6,693 | ~37% |
| PLL | 2 | 2 | 100% |
| BSRAM (block RAM) | Varies | 26 | Varies |

### 7.2 Budget Remaining

| Resource | Remaining | Notes |
|----------|-----------|-------|
| LUT4 | ~4,800 | 55.6% free |
| CLS | ~1,200 | 28.2% free -- this is the **binding constraint** |
| PLL | 0 | Both PLLs are used. New clock domains require CLKDIV from existing PLLs. |

**CLS is the bottleneck.** Each CLS (Configurable Logic Slice) contains 2 LUT4 + 2 FF. Even though LUT4 utilization is only 44%, CLS utilization is 72% because the place-and-route tool cannot always pack two unrelated LUTs into the same slice.

### 7.3 Estimated Cost per Peripheral Type

| Peripheral | LUT4 | FF | CLS | BSRAM | Notes |
|-----------|------|-----|-----|-------|-------|
| Minimal MMIO (4 regs) | 30-50 | 40-60 | 30-40 | 0 | Template from Section 3 |
| UART (TX+RX+FIFO) | 80-120 | 60-80 | 60-80 | 0 | Already in SoC |
| SPI Master | 100-150 | 80-100 | 80-100 | 0 | For MAX3421E USB host |
| Timer (32-bit, 2 compare) | 60-100 | 70-90 | 50-70 | 0 | Counter + comparators |
| CDC bridge (toggle) | 40-60 | 30-40 | 30-40 | 0 | Per `atomik_cdc_bridge.v` |
| HDMI framebuffer ctrl | 50-80 | 40-60 | 40-60 | 0 | Registers only; DMA separate |
| DMA controller (1-ch) | 200-300 | 150-200 | 150-200 | 0 | Address gen + handshake |
| 1 KB dual-port BRAM | 10-20 | 10-20 | 10-15 | 1 | Uses block RAM, minimal LUT |

### 7.4 What Fits

With ~1,200 CLS remaining, realistic additions before exhausting resources:

| Scenario | CLS Cost | Running Total | Fits? |
|----------|----------|---------------|-------|
| + SPI Master (MAX3421E) | ~90 | 3,193 | Yes |
| + Timer/Watchdog | ~60 | 3,253 | Yes |
| + HDMI Framebuffer Ctrl | ~50 | 3,303 | Yes |
| + Inter-Board SPI | ~90 | 3,393 | Yes |
| + DMA (1 channel) | ~175 | 3,568 | Yes |
| **Total** | **~465** | **3,568** | **82.6%** |

This leaves ~750 CLS (17.4%) as margin, which is tight but workable for Gowin's place-and-route. Designs above 85% CLS utilization risk timing closure failures on GW1NR-9.

### 7.5 If Resources Run Out

Options for expanding beyond the Tang Nano 9K's capacity:

1. **Remove unused peripherals.** If HDMI output is not needed, dropping the HDMI serializer frees significant LUT/FF.
2. **Move to Tang Nano 20K.** The GW2AR-18 has 20,736 LUT4 (2.4x more than GW1NR-9), same form factor, similar price.
3. **Multi-board architecture.** Use the inter-board communication peripheral to distribute workloads across multiple Tang Nano 9K boards, each with its own ATOMiK accelerator.

---

## Appendix A: Quick Reference -- File Locations

| File | Path | Description |
|------|------|-------------|
| ATOMiK bus wrapper | `hardware/picorv32/atomik_bus_wrapper.v` | CDC wrapper + parallel accumulator |
| ATOMiK CDC bridge | `hardware/picorv32/atomik_cdc_bridge.v` | Toggle-handshake CDC |
| ATOMiK C header | `hardware/picorv32/firmware/atomik.h` | Register macros + inline API |
| Memory map | `hardware/picorv32/memory_map.md` | SoC address map and register definitions |
| Production deployment | `docs/PRODUCTION_DEPLOYMENT.md` | Synthesis results, timing, test status |
| Hardware synthesis | `docs/HARDWARE_SYNTHESIS.md` | Parallel bank sweep results |

## Appendix B: Wishbone-to-valid/ready Shim

If you have an existing Wishbone peripheral and want to connect it to our SoC without rewriting it, use this shim:

```verilog
// =============================================================================
// Wishbone-to-PicoRV32 valid/ready Shim
//
// Translates PicoRV32's native valid/ready protocol to Wishbone B4 slave.
// Use this to integrate Gowin IP or third-party Wishbone peripherals.
// =============================================================================

module wb_shim (
    input  wire        clk,
    input  wire        resetn,

    // PicoRV32 side (valid/ready)
    input  wire        mem_valid,
    input  wire [31:0] mem_addr,
    input  wire [31:0] mem_wdata,
    input  wire [3:0]  mem_wstrb,
    output reg         mem_ready,
    output reg  [31:0] mem_rdata,

    // Wishbone side (B4 slave)
    output reg         wb_cyc_o,
    output reg         wb_stb_o,
    output reg         wb_we_o,
    output reg  [31:0] wb_adr_o,
    output reg  [31:0] wb_dat_o,
    output reg  [3:0]  wb_sel_o,
    input  wire        wb_ack_i,
    input  wire [31:0] wb_dat_i
);

    reg ack_sent;

    always @(posedge clk or negedge resetn) begin
        if (!resetn) begin
            wb_cyc_o  <= 1'b0;
            wb_stb_o  <= 1'b0;
            wb_we_o   <= 1'b0;
            wb_adr_o  <= 32'b0;
            wb_dat_o  <= 32'b0;
            wb_sel_o  <= 4'b0;
            mem_ready <= 1'b0;
            mem_rdata <= 32'b0;
            ack_sent  <= 1'b0;
        end else begin
            mem_ready <= 1'b0;

            if (mem_valid && !ack_sent) begin
                // Drive Wishbone signals
                wb_cyc_o <= 1'b1;
                wb_stb_o <= 1'b1;
                wb_we_o  <= |mem_wstrb;
                wb_adr_o <= mem_addr;
                wb_dat_o <= mem_wdata;
                wb_sel_o <= (|mem_wstrb) ? mem_wstrb : 4'b1111;

                if (wb_ack_i) begin
                    // Wishbone slave acknowledged
                    mem_ready <= 1'b1;
                    mem_rdata <= wb_dat_i;
                    wb_cyc_o  <= 1'b0;
                    wb_stb_o  <= 1'b0;
                    ack_sent  <= 1'b1;
                end
            end else begin
                wb_cyc_o <= 1'b0;
                wb_stb_o <= 1'b0;
            end

            if (!mem_valid)
                ack_sent <= 1'b0;
        end
    end

endmodule
```

This shim adds 1 cycle of latency (Wishbone ack arrives 1 cycle after stb, which is typical for registered slaves). For combinational-ack Wishbone slaves, the response completes in the same cycle as the request.
