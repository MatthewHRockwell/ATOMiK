# HDMI Display Output Reference — Tang Nano 9K

Implementation guide for HDMI text-mode display on the ATOMiK SoC.

**Source**: Tang Nano 9K schematics, Sipeed pinout reference, VESA timing standards, Gowin UG286-1.9.1E
**Target**: ATOMiK production SoC on GW1NR-LV9QN88PC6/I5
**Status**: Design reference for Roadmap Section 5 (Task 4.1.1, 4.1.3)

---

## Table of Contents

1. [HDMI/DVI-D Overview](#1-hdmidvi-d-overview)
2. [Tang Nano 9K HDMI Hardware](#2-tang-nano-9k-hdmi-hardware)
3. [PLL Configuration for Video](#3-pll-configuration-for-video)
4. [Video Timing Parameters](#4-video-timing-parameters)
5. [TMDS Encoder Architecture](#5-tmds-encoder-architecture)
6. [Text Framebuffer Architecture](#6-text-framebuffer-architecture)
7. [Memory-Mapped Registers](#7-memory-mapped-registers)
8. [Verilog Module Skeleton](#8-verilog-module-skeleton)
9. [SoC Integration](#9-soc-integration)
10. [Resource Budget](#10-resource-budget)

---

## 1. HDMI/DVI-D Overview

### Why DVI-D Mode (Not Full HDMI)

The Tang Nano 9K outputs a DVI-D signal over the HDMI connector. This is the correct approach for FPGA-based designs:

- **DVI-D** transmits uncompressed digital video using TMDS encoding, identical to HDMI video, but without the audio, CEC, or HDCP overhead.
- **All HDMI monitors accept DVI-D** signals natively. The HDMI connector is physically compatible with DVI-D — the electrical signaling is the same.
- **No HDCP license required.** HDCP (High-bandwidth Digital Content Protection) requires a paid license and encrypted key exchange. DVI-D mode avoids this entirely.
- **No audio IP core required.** HDMI audio requires an audio clock recovery PLL and I2S/SPDIF encoder, consuming resources we do not have to spare.
- **Simpler implementation.** DVI-D requires only TMDS encoding and serialization — no info frames, no AVI packets, no audio sample packets.

### TMDS Encoding Summary

Transition-Minimized Differential Signaling (TMDS) is the physical layer for both DVI and HDMI:

- **4 differential pairs**: 3 data channels (D0, D1, D2) + 1 clock channel
- **8b/10b encoding**: Each 8-bit pixel component is encoded into a 10-bit TMDS symbol
- **Serialization ratio**: 10:1 (10 bits per pixel clock period per channel)
- **Serializer clock**: 5x the pixel clock (DDR) or 10x the pixel clock (SDR)
- **Data channels carry**:
  - D0: Blue + HSync + VSync (during blanking)
  - D1: Green (during blanking: CTL0/CTL1)
  - D2: Red (during blanking: CTL2/CTL3)

### Signal Flow

```
Pixel Data (R,G,B)         TMDS Encoding          Serializer          FPGA Pins
    8-bit each        -->   10-bit symbols   -->   10:1 serial   -->  Differential
                                                                       pairs
    @ pixel_clk            @ pixel_clk            @ 5x pixel_clk      to HDMI
                                                    (DDR)              connector
```

---

## 2. Tang Nano 9K HDMI Hardware

### Pin Assignments (Bank 1)

All HDMI signals are on Bank 1 (LVCMOS18 capable, true LVDS output).

| Signal | FPGA Pin | Pair | HDMI Connector | Schematic Net |
|--------|----------|------|----------------|---------------|
| `tmds_clk_p` | 69 | Clock + | CK_P (Pin 10) | HDMLTXC_P |
| `tmds_clk_n` | 68 | Clock - | CK_N (Pin 12) | HDMLTXC_N |
| `tmds_d0_p` | 71 | Data 0 + | D0_P (Pin 7) | HDMLTX0_P |
| `tmds_d0_n` | 70 | Data 0 - | D0_N (Pin 9) | HDMLTX0_N |
| `tmds_d1_p` | 73 | Data 1 + | D1_P (Pin 4) | HDMLTX1_P |
| `tmds_d1_n` | 72 | Data 1 - | D1_N (Pin 6) | HDMLTX1_N |
| `tmds_d2_p` | 75 | Data 2 + | D2_P (Pin 1) | HDMLTX2_P |
| `tmds_d2_n` | 74 | Data 2 - | D2_N (Pin 3) | HDMLTX2_N |

### CST Constraints

```
// HDMI TMDS output (Bank 1)
IO_LOC "tmds_clk_p" 69;
IO_LOC "tmds_clk_n" 68;
IO_LOC "tmds_d0_p"  71;
IO_LOC "tmds_d0_n"  70;
IO_LOC "tmds_d1_p"  73;
IO_LOC "tmds_d1_n"  72;
IO_LOC "tmds_d2_p"  75;
IO_LOC "tmds_d2_n"  74;
IO_PORT "tmds_clk_p" IO_TYPE=LVCMOS33D DRIVE=8;
IO_PORT "tmds_clk_n" IO_TYPE=LVCMOS33D DRIVE=8;
IO_PORT "tmds_d0_p"  IO_TYPE=LVCMOS33D DRIVE=8;
IO_PORT "tmds_d0_n"  IO_TYPE=LVCMOS33D DRIVE=8;
IO_PORT "tmds_d1_p"  IO_TYPE=LVCMOS33D DRIVE=8;
IO_PORT "tmds_d1_n"  IO_TYPE=LVCMOS33D DRIVE=8;
IO_PORT "tmds_d2_p"  IO_TYPE=LVCMOS33D DRIVE=8;
IO_PORT "tmds_d2_n"  IO_TYPE=LVCMOS33D DRIVE=8;
```

**Note**: `LVCMOS33D` is the Gowin differential output type. The Gowin EDA toolchain handles the differential signaling automatically when using the ELVDS_OBUF or TLVDS_OBUF primitives. The actual CST type used may vary depending on how the serializer is instantiated — see Section 5 for the OSER10 approach.

### ESD Protection

The schematic shows PCLAMP0524P ESD protection ICs on all TMDS pairs:

- Each differential pair has 49.9-ohm series termination resistors to 3.3V
- D2: R14/R15 (49.9 ohm)
- D1: R16/R49 (49.9 ohm)
- D0: R50/R51 (49.9 ohm)
- Clock: R52/R53 (49.9 ohm)

These resistors provide source termination for the TMDS signals. No external components are needed beyond what is on the board.

### DDC/I2C (EDID)

The HDMI connector includes I2C lines for DDC (Display Data Channel) / EDID:

| Signal | HDMI Pin | Pull-Up | Notes |
|--------|----------|---------|-------|
| HDMI_SCL | 15 | R12 = 1.5K | I2C clock for EDID |
| HDMI_SDA | 16 | R13 = 1.5K | I2C data for EDID |
| HCEC | 13 | R11 = 27K | Consumer Electronics Control |
| HPD | 14/18 | — | Hot Plug Detect / Utility |

**For our use case, DDC/EDID is not required.** We output a fixed resolution (640x480@60Hz) that every HDMI monitor supports. EDID read-back would only be needed for auto-detecting the monitor's preferred resolution.

### HDMI 5V Supply

The schematic routes +5VD through diode D1 to the HDMI connector's 5V pin (pin 18). This is sourced from the USB-C power input. The monitor detects this 5V as a "source connected" indication. No FPGA logic is required for this.

---

## 3. PLL Configuration for Video

### Current Production PLL Configuration

Our production SoC already uses the HDMI PLL:

| Clock | Source | Frequency | Purpose |
|-------|--------|-----------|---------|
| `clk_hdmi_ser` | HDMI PLL CLKOUT | 126.0 MHz | TMDS serializer (5x pixel clock, DDR) |
| `clk_p` | HDMI PLL CLKOUTD (div 5) | 25.2 MHz | Pixel clock + CPU clock |
| `clk_atomik` | ATOMiK PLL CLKOUT | 81.0 MHz | ATOMiK core clock |

**Important**: The CPU (PicoRV32) shares the pixel clock (25.2 MHz) via `clk_p` from the HDMI PLL divider. This means the pixel clock is already generated and available. No additional PLL is needed for video output.

### HDMI PLL Parameters (27 MHz Input)

```
fCLKOUT  = 27 MHz x (FBDIV_SEL + 1) / (IDIV_SEL + 1)
fVCO     = fCLKOUT x ODIV_SEL
fCLKOUTD = fCLKOUT / DYN_SDIV_SEL
```

#### 640x480 @ 60 Hz (Current Configuration)

| Parameter | Value | Notes |
|-----------|-------|-------|
| Target Pixel Clock | 25.175 MHz | VESA standard |
| Actual Pixel Clock | 25.2 MHz | 27 x 14 / 15 = 25.2 (0.1% over, acceptable) |
| Serializer Clock | 126.0 MHz | 25.2 x 5 (DDR mode) |
| IDIV_SEL | 14 | IDIV = 15 |
| FBDIV_SEL | 13 | FBDIV = 14 |
| ODIV_SEL | 4 | ODIV = 4 |
| DYN_SDIV_SEL | 5 | CLKOUTD divider for pixel clock |
| fVCO | 504 MHz | 126 x 4 = 504 (within 400-1200 MHz) |

**Instantiation**:

```verilog
rPLL hdmi_pll_inst (
    .CLKIN    (sys_clk),          // 27 MHz
    .CLKOUT   (clk_hdmi_ser),     // 126 MHz (TMDS serializer)
    .CLKOUTD  (clk_p),            // 25.2 MHz (pixel + CPU)
    .LOCK     (hdmi_pll_lock),
    .RESET    (~sys_rst_n),
    .RESET_P  (1'b0),
    .CLKFB    (1'b0),
    .FBDSEL   (6'b0),
    .IDSEL    (6'b0),
    .ODSEL    (6'b0),
    .PSDA     (4'b0),
    .DUTYDA   (4'b0),
    .FDLY     (4'b0)
);

defparam hdmi_pll_inst.FCLKIN        = "27";
defparam hdmi_pll_inst.IDIV_SEL      = 14;
defparam hdmi_pll_inst.FBDIV_SEL     = 13;
defparam hdmi_pll_inst.ODIV_SEL      = 4;
defparam hdmi_pll_inst.DYN_SDIV_SEL  = 5;
defparam hdmi_pll_inst.CLKOUTD_SRC   = "CLKOUT";
defparam hdmi_pll_inst.CLKFB_SEL     = "internal";
defparam hdmi_pll_inst.DEVICE        = "GW1NR-9C";
```

#### Alternative Resolutions

| Resolution | Pixel Clock (VESA) | Achievable | Serializer | fVCO | IDIV | FBDIV | ODIV | Feasible? |
|---|---|---|---|---|---|---|---|---|
| 640x480 @ 60 Hz | 25.175 MHz | 25.2 MHz | 126.0 MHz | 504 MHz | 15 | 14 | 4 | Yes (current) |
| 720x480 @ 60 Hz | 27.0 MHz | 27.0 MHz | 135.0 MHz | 540 MHz | 1 | 5 | 4 | Yes |
| 800x600 @ 60 Hz | 40.0 MHz | 40.5 MHz | 202.5 MHz | 810 MHz | 1 | 15 | 4 | Marginal |
| 1024x768 @ 60 Hz | 65.0 MHz | 64.8 MHz | 324.0 MHz | 648 MHz | 1 | 12 | 2 | Unlikely (timing) |

**Recommendation**: Stay with 640x480 @ 60 Hz. It is the baseline resolution supported by every HDMI monitor, uses the lowest serializer frequency (easiest timing closure), and provides 80x30 characters in 8x16 font — sufficient for a text terminal.

**Important for 800x600**: The 202.5 MHz serializer clock approaches the OSER10 timing limits on the GW1NR-9 at the C6/I5 speed grade. Use this only if 640x480 proves insufficient and test timing closure carefully.

---

## 4. Video Timing Parameters

### How Video Timing Works

Each frame consists of visible pixels surrounded by blanking intervals. The blanking intervals include front porch, sync pulse, and back porch regions.

```
Horizontal (per line):
  ┌──────────┬──────────┬───────────┬──────────────────────────┐
  │ H_SYNC   │ H_BACK   │  VISIBLE  │ H_FRONT                 │
  │ (pulse)  │ (porch)  │  (pixels) │ (porch)                 │
  └──────────┴──────────┴───────────┴──────────────────────────┘

Vertical (per frame):
  ┌──────────┬──────────┬───────────┬──────────────────────────┐
  │ V_SYNC   │ V_BACK   │  VISIBLE  │ V_FRONT                 │
  │ (pulse)  │ (porch)  │  (lines)  │ (porch)                 │
  └──────────┴──────────┴───────────┴──────────────────────────┘
```

Total pixels per line = H_VISIBLE + H_FRONT + H_SYNC + H_BACK
Total lines per frame = V_VISIBLE + V_FRONT + V_SYNC + V_BACK
Frame rate = pixel_clock / (total_pixels_per_line x total_lines_per_frame)

### 640x480 @ 60 Hz Timing (Primary Target)

| Parameter | Value | Unit |
|-----------|-------|------|
| **Pixel Clock** | 25.175 MHz (use 25.2) | MHz |
| **H Visible** | 640 | pixels |
| **H Front Porch** | 16 | pixels |
| **H Sync Pulse** | 96 | pixels |
| **H Back Porch** | 48 | pixels |
| **H Total** | 800 | pixels |
| **H Sync Polarity** | Negative | |
| **V Visible** | 480 | lines |
| **V Front Porch** | 10 | lines |
| **V Sync Pulse** | 2 | lines |
| **V Back Porch** | 33 | lines |
| **V Total** | 525 | lines |
| **V Sync Polarity** | Negative | |
| **Frame Rate** | 25,200,000 / (800 x 525) = **60.0 Hz** | Hz |

### 720x480 @ 60 Hz (CEA-861, Optional)

| Parameter | Value | Unit |
|-----------|-------|------|
| **Pixel Clock** | 27.0 MHz | MHz |
| **H Visible** | 720 | pixels |
| **H Front Porch** | 16 | pixels |
| **H Sync Pulse** | 62 | pixels |
| **H Back Porch** | 60 | pixels |
| **H Total** | 858 | pixels |
| **H Sync Polarity** | Negative | |
| **V Visible** | 480 | lines |
| **V Front Porch** | 9 | lines |
| **V Sync Pulse** | 6 | lines |
| **V Back Porch** | 30 | lines |
| **V Total** | 525 | lines |
| **V Sync Polarity** | Negative | |

### 800x600 @ 60 Hz (VESA DMT, Stretch Goal)

| Parameter | Value | Unit |
|-----------|-------|------|
| **Pixel Clock** | 40.0 MHz | MHz |
| **H Visible** | 800 | pixels |
| **H Front Porch** | 40 | pixels |
| **H Sync Pulse** | 128 | pixels |
| **H Back Porch** | 88 | pixels |
| **H Total** | 1056 | pixels |
| **H Sync Polarity** | Positive | |
| **V Visible** | 600 | lines |
| **V Front Porch** | 1 | lines |
| **V Sync Pulse** | 4 | lines |
| **V Back Porch** | 23 | lines |
| **V Total** | 628 | lines |
| **V Sync Polarity** | Positive | |

### Verilog Timing Parameters (640x480 @ 60 Hz)

```verilog
// 640x480 @ 60 Hz timing constants
localparam H_VISIBLE    = 640;
localparam H_FRONT      = 16;
localparam H_SYNC       = 96;
localparam H_BACK       = 48;
localparam H_TOTAL      = 800;   // 640 + 16 + 96 + 48

localparam V_VISIBLE    = 480;
localparam V_FRONT      = 10;
localparam V_SYNC       = 2;
localparam V_BACK       = 33;
localparam V_TOTAL      = 525;   // 480 + 10 + 2 + 33

// Sync polarity: both negative for 640x480
localparam H_SYNC_POL   = 1'b0;  // Active-low
localparam V_SYNC_POL   = 1'b0;  // Active-low
```

---

## 5. TMDS Encoder Architecture

### 8b/10b TMDS Encoding

Each pixel clock cycle, three 8-bit color values (R, G, B) are encoded into three 10-bit TMDS symbols. The encoding minimizes transitions on the differential pairs to reduce EMI.

**During active video:**
- Input: 8-bit pixel data
- Output: 10-bit TMDS symbol (bit 9 = inversion flag, bit 8 = XOR/XNOR select, bits 7:0 = encoded data)
- DC balance tracking: running disparity counter adjusts encoding to maintain near-zero DC offset

**During blanking:**
- Data channels carry control tokens instead of pixel data
- D0 carries HSync and VSync as 2-bit control
- D1 and D2 carry CTL signals (typically 2'b00)
- Control tokens are fixed 10-bit patterns with guaranteed transitions for clock recovery

### TMDS Control Tokens

| Control Input (2-bit) | 10-bit Token | Description |
|------------------------|--------------|-------------|
| 2'b00 | 10'b1101010100 | D0: no sync |
| 2'b01 | 10'b0010101011 | D0: HSync only |
| 2'b10 | 10'b0101010100 | D0: VSync only |
| 2'b11 | 10'b1010101011 | D0: HSync + VSync |

### TMDS Encoder Module

```verilog
// =============================================================================
// TMDS 8b/10b Encoder
// Implements DVI 1.0 / HDMI 1.4 TMDS encoding with DC balance
// =============================================================================

module tmds_encoder (
    input  wire       clk,        // Pixel clock
    input  wire       rst_n,
    input  wire [7:0] data_in,    // 8-bit pixel data
    input  wire [1:0] ctrl_in,    // 2-bit control (sync signals)
    input  wire       data_en,    // 1 = active video, 0 = blanking
    output reg  [9:0] tmds_out    // 10-bit TMDS symbol
);

    // Count number of 1s in input data
    wire [3:0] n_ones = data_in[0] + data_in[1] + data_in[2] + data_in[3]
                      + data_in[4] + data_in[5] + data_in[6] + data_in[7];

    // Step 1: Minimize transitions (XOR or XNOR chain)
    wire use_xnor = (n_ones > 4) || (n_ones == 4 && data_in[0] == 1'b0);

    wire [8:0] q_m;
    assign q_m[0] = data_in[0];
    assign q_m[1] = use_xnor ? ~(q_m[0] ^ data_in[1]) : (q_m[0] ^ data_in[1]);
    assign q_m[2] = use_xnor ? ~(q_m[1] ^ data_in[2]) : (q_m[1] ^ data_in[2]);
    assign q_m[3] = use_xnor ? ~(q_m[2] ^ data_in[3]) : (q_m[2] ^ data_in[3]);
    assign q_m[4] = use_xnor ? ~(q_m[3] ^ data_in[4]) : (q_m[3] ^ data_in[4]);
    assign q_m[5] = use_xnor ? ~(q_m[4] ^ data_in[5]) : (q_m[4] ^ data_in[5]);
    assign q_m[6] = use_xnor ? ~(q_m[5] ^ data_in[6]) : (q_m[5] ^ data_in[6]);
    assign q_m[7] = use_xnor ? ~(q_m[6] ^ data_in[7]) : (q_m[6] ^ data_in[7]);
    assign q_m[8] = use_xnor ? 1'b0 : 1'b1;

    // Count 1s and 0s in q_m[7:0]
    wire [3:0] n_ones_qm = q_m[0] + q_m[1] + q_m[2] + q_m[3]
                          + q_m[4] + q_m[5] + q_m[6] + q_m[7];
    wire [3:0] n_zeros_qm = 4'd8 - n_ones_qm;

    // Step 2: DC balance with running disparity
    reg signed [4:0] disparity;  // Running DC balance counter

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            tmds_out  <= 10'b1101010100;  // Control token for 2'b00
            disparity <= 5'sd0;
        end else begin
            if (!data_en) begin
                // Blanking period: output control tokens
                disparity <= 5'sd0;
                case (ctrl_in)
                    2'b00: tmds_out <= 10'b1101010100;
                    2'b01: tmds_out <= 10'b0010101011;
                    2'b10: tmds_out <= 10'b0101010100;
                    2'b11: tmds_out <= 10'b1010101011;
                endcase
            end else begin
                // Active video: encode with DC balance
                if (disparity == 5'sd0 || n_ones_qm == n_zeros_qm) begin
                    // No disparity correction needed
                    tmds_out[9]   <= ~q_m[8];
                    tmds_out[8]   <= q_m[8];
                    tmds_out[7:0] <= q_m[8] ? q_m[7:0] : ~q_m[7:0];
                    if (q_m[8] == 1'b0)
                        disparity <= disparity + (n_zeros_qm - n_ones_qm);
                    else
                        disparity <= disparity + (n_ones_qm - n_zeros_qm);
                end else begin
                    if ((disparity > 0 && n_ones_qm > n_zeros_qm) ||
                        (disparity < 0 && n_zeros_qm > n_ones_qm)) begin
                        // Invert to reduce disparity
                        tmds_out[9]   <= 1'b1;
                        tmds_out[8]   <= q_m[8];
                        tmds_out[7:0] <= ~q_m[7:0];
                        disparity <= disparity + {q_m[8], 1'b0}
                                   + (n_zeros_qm - n_ones_qm);
                    end else begin
                        // No inversion
                        tmds_out[9]   <= 1'b0;
                        tmds_out[8]   <= q_m[8];
                        tmds_out[7:0] <= q_m[7:0];
                        disparity <= disparity - {~q_m[8], 1'b0}
                                   + (n_ones_qm - n_zeros_qm);
                    end
                end
            end
        end
    end

endmodule
```

### 10:1 Serializer (Gowin OSER10)

The Gowin GW1NR-9 provides a dedicated OSER10 primitive for 10:1 serialization in DDR mode. This converts the 10-bit TMDS symbol (at pixel clock rate) into a serial bitstream (at 5x pixel clock rate, DDR).

```verilog
// =============================================================================
// TMDS Serializer using Gowin OSER10 + ELVDS_OBUF
// One instance per TMDS channel (3 data + 1 clock = 4 total)
// =============================================================================

module tmds_serializer (
    input  wire       clk_pixel,    // 25.2 MHz pixel clock
    input  wire       clk_5x,       // 126 MHz serializer clock (5x pixel)
    input  wire       rst_n,
    input  wire [9:0] tmds_data,    // 10-bit TMDS symbol (from encoder)
    output wire       tmds_p,       // Differential output +
    output wire       tmds_n        // Differential output -
);

    wire serial_out;

    // Gowin OSER10: 10:1 serializer (DDR, 5x clock)
    OSER10 oser10_inst (
        .Q     (serial_out),
        .D0    (tmds_data[0]),
        .D1    (tmds_data[1]),
        .D2    (tmds_data[2]),
        .D3    (tmds_data[3]),
        .D4    (tmds_data[4]),
        .D5    (tmds_data[5]),
        .D6    (tmds_data[6]),
        .D7    (tmds_data[7]),
        .D8    (tmds_data[8]),
        .D9    (tmds_data[9]),
        .PCLK  (clk_pixel),
        .FCLK  (clk_5x),
        .RESET (~rst_n)
    );
    defparam oser10_inst.GSREN  = "false";
    defparam oser10_inst.LSREN  = "true";

    // ELVDS output buffer (emulated LVDS)
    ELVDS_OBUF elvds_inst (
        .O  (tmds_p),
        .OB (tmds_n),
        .I  (serial_out)
    );

endmodule
```

### Clock Channel

The TMDS clock channel transmits the pixel clock as a simple 10-bit repeating pattern `10'b0000011111` (5 zeros followed by 5 ones), which the monitor recovers as the pixel clock reference.

```verilog
// Clock channel: constant pattern
wire [9:0] tmds_clk_pattern = 10'b0000011111;

tmds_serializer u_tmds_clk (
    .clk_pixel (clk_pixel),
    .clk_5x    (clk_5x),
    .rst_n     (rst_n),
    .tmds_data (tmds_clk_pattern),
    .tmds_p    (tmds_clk_p),
    .tmds_n    (tmds_clk_n)
);
```

---

## 6. Text Framebuffer Architecture

### Design Overview

The text framebuffer renders an 80-column by 30-row character display. Each character cell is 8 pixels wide by 16 pixels tall, yielding 640x480 visible pixels.

```
80 columns x 8 pixels = 640 pixels (matches H_VISIBLE)
30 rows    x 16 pixels = 480 pixels (matches V_VISIBLE)
```

### Architecture Block Diagram

```
CPU Bus (25.2 MHz)                          Pixel Pipeline (25.2 MHz)
┌─────────────┐                            ┌──────────────────────────────────────┐
│  PicoRV32   │      MMIO Write            │                                      │
│             │────────────────────┐        │  H/V Counters                        │
│ framebuffer │                    │        │       │                               │
│   writes    │                    v        │       v                               │
│             │              ┌──────────┐   │  ┌──────────┐    ┌──────────┐        │
└─────────────┘              │ Character │   │  │ Character │───>│ Font ROM │        │
                             │  Buffer   │   │  │  Lookup   │    │ (BSRAM)  │        │
                             │ (BSRAM)   │<──┘  │ char_x,y  │    │ 8x16 px  │        │
                             │ 80x30     │      └──────────┘    └────┬─────┘        │
                             │ = 2,400 B │                           │               │
                             └──────────┘                           v               │
                                                              ┌──────────┐          │
                                                              │ Pixel    │          │
                                                              │ Select   │──> R,G,B │
                                                              │ (fg/bg)  │          │
                                                              └──────────┘          │
                                                                    │               │
                                                                    v               │
                                                              ┌──────────┐          │
                                                              │ TMDS     │          │
                                                              │ Encode   │──> HDMI  │
                                                              └──────────┘          │
                                                                                    │
                                                                    @ 126 MHz       │
                                                              ┌──────────┐          │
                                                              │ OSER10   │──> Pins  │
                                                              └──────────┘          │
                                                                                    │
                                                                                    │
└──────────────────────────────────────────────────────────────────────────────────┘
```

### Character Buffer (BSRAM)

The character buffer stores one byte per character cell:

| Parameter | Value |
|-----------|-------|
| Columns | 80 |
| Rows | 30 |
| Total Characters | 2,400 |
| Bytes per Character | 1 (ASCII code) or 2 (ASCII + attribute) |
| Total Memory | 2,400 bytes (1-byte mode) or 4,800 bytes (2-byte mode) |
| BSRAM Blocks | 1 (1-byte mode, fits in one 2KB pSRAM + partial second) |

**BSRAM Configuration**: Gowin GW1NR-9 BSRAM blocks are 2KB (16Kbit) each. In dual-port mode, one port is connected to the CPU bus for writes, and the other port is connected to the video scan logic for reads.

```verilog
// Character buffer: dual-port BSRAM
// Port A: CPU write (25.2 MHz bus clock)
// Port B: Video read (25.2 MHz pixel clock — same clock domain)

// Addressing: linear index = row * 80 + col
// With 80x30 = 2,400 entries, need 12-bit address (2^12 = 4,096 > 2,400)

// 1-byte mode: 8-bit ASCII code per cell
// 2-byte mode: 8-bit ASCII + 8-bit attribute (4-bit fg color, 4-bit bg color)
```

**Recommended approach**: Use 2-byte mode (character + attribute). This costs 2 BSRAM blocks instead of 1 but enables foreground/background color per character, which makes the terminal far more usable (syntax highlighting, status bars, error messages in red).

#### Attribute Byte Format

```
Bit 7   | Bit 6:4       | Bit 3   | Bit 2:0
--------|---------------|---------|--------
Blink   | Background    | Bright  | Foreground
        | (3-bit color) |         | (3-bit color)
```

Standard 8-color palette (CGA/VGA compatible):

| Code | Color |
|------|-------|
| 000 | Black |
| 001 | Blue |
| 010 | Green |
| 011 | Cyan |
| 100 | Red |
| 101 | Magenta |
| 110 | Yellow/Brown |
| 111 | White/Light Gray |

With the bright bit, this gives 16 foreground colors and 8 background colors — the classic VGA text mode color scheme.

### Font ROM (BSRAM)

The font ROM stores an 8x16 bitmap for each of the 256 ASCII characters:

| Parameter | Value |
|-----------|-------|
| Characters | 256 |
| Glyph Size | 8 pixels wide x 16 pixels tall |
| Bytes per Glyph | 16 (one byte per row, 8 bits = 8 pixels) |
| Total Memory | 256 x 16 = 4,096 bytes = 4 KB |
| BSRAM Blocks | 2 (each block is 2 KB) |

**Font ROM addressing**: `addr = {char_code[7:0], glyph_row[3:0]}` (12-bit address, 8-bit data output)

The font data can be initialized at synthesis time using Gowin's BSRAM initialization (`.mi` files) or loaded from SPI flash at boot.

### Pixel Pipeline Timing

The pixel pipeline must produce one RGB pixel per pixel clock cycle. Here is the 2-stage pipeline:

```
Stage 1 (clock cycle N):
  - H/V counters determine character column (pixel_x / 8) and row (pixel_y / 16)
  - Character buffer read: addr = char_row * 80 + char_col
  - Output: char_code, attribute byte

Stage 2 (clock cycle N+1):
  - Font ROM read: addr = {char_code, glyph_row}
  - Output: 8-bit glyph row (one bit per pixel)
  - Pixel select: glyph_row[7 - pixel_x_within_cell] selects fg or bg color
  - Color lookup: attribute byte -> RGB values
  - Output: R[7:0], G[7:0], B[7:0]
```

**Note**: The 2-stage pipeline means the display is shifted left by 2 pixels. Compensate by adjusting the horizontal start position (H_BACK + 2).

---

## 7. Memory-Mapped Registers

### Proposed Address Map (Base: 0xC100_0000)

The HDMI text framebuffer occupies a new address range in the SoC memory map. It does not conflict with the existing ATOMiK registers at `0xC000_0000`.

| Address Range | Name | Access | Description |
|---------------|------|--------|-------------|
| `0xC100_0000 - 0xC100_12BF` | CHAR_BUF | W | Character buffer (2,400 x 2 = 4,800 bytes) |
| `0xC100_2000` | CURSOR_POS | RW | Cursor position: {row[15:8], col[7:0]} |
| `0xC100_2004` | CURSOR_CFG | RW | Cursor config: {blink_rate[7:4], style[3:0]} |
| `0xC100_2008` | SCREEN_CFG | RW | Screen config: {bg_color[7:4], border[3:0]} |
| `0xC100_200C` | STATUS | R | Status: {vsync_count[15:0], ...} |
| `0xC100_2010` | SCROLL_OFF | RW | Scroll offset (row): enables hardware scroll |

### Character Buffer Write Protocol

Each character cell is written as a 16-bit half-word (or two byte-writes):

```c
// C header definitions
#define HDMI_FB_BASE     0xC1000000
#define HDMI_CURSOR_POS  (*(volatile uint32_t*)(HDMI_FB_BASE + 0x2000))
#define HDMI_CURSOR_CFG  (*(volatile uint32_t*)(HDMI_FB_BASE + 0x2004))
#define HDMI_SCREEN_CFG  (*(volatile uint32_t*)(HDMI_FB_BASE + 0x2008))
#define HDMI_STATUS      (*(volatile uint32_t*)(HDMI_FB_BASE + 0x200C))
#define HDMI_SCROLL_OFF  (*(volatile uint32_t*)(HDMI_FB_BASE + 0x2010))

// Write character + attribute to position (col, row)
static inline void fb_putchar(int col, int row, char ch, uint8_t attr) {
    uint32_t addr = HDMI_FB_BASE + (row * 80 + col) * 2;
    *(volatile uint16_t*)addr = ((uint16_t)attr << 8) | (uint8_t)ch;
}

// Simplified: write character with default white-on-black
static inline void fb_putchar_simple(int col, int row, char ch) {
    fb_putchar(col, row, ch, 0x07);  // White on black
}

// Set cursor position
static inline void fb_set_cursor(int col, int row) {
    HDMI_CURSOR_POS = ((row & 0xFF) << 8) | (col & 0xFF);
}
```

### Hardware Scroll

Instead of copying the entire framebuffer to scroll (2,400 words = expensive for a 25.2 MHz CPU), the hardware scroll register shifts the start row:

```c
// Scroll up by one line (zero-copy)
static inline void fb_scroll_up(void) {
    uint32_t current = HDMI_SCROLL_OFF;
    HDMI_SCROLL_OFF = (current + 1) % 30;
    // Clear the new bottom line
    int bottom_row = (current + 29) % 30;
    for (int col = 0; col < 80; col++)
        fb_putchar(col, bottom_row, ' ', 0x07);
}
```

The video scan logic applies the scroll offset:

```verilog
// Adjusted character row with scroll offset
wire [4:0] char_row_adj = (char_row + scroll_offset >= 30)
                        ? (char_row + scroll_offset - 30)
                        : (char_row + scroll_offset);
wire [11:0] char_addr = char_row_adj * 80 + char_col;
```

---

## 8. Verilog Module Skeleton

### Top-Level HDMI Text Module

```verilog
// =============================================================================
// HDMI Text-Mode Display Controller
//
// 80x30 character text mode over DVI-D (HDMI connector)
// 640x480 @ 60 Hz, 25.2 MHz pixel clock, 126 MHz serializer
//
// Resources: ~500-800 LUT, 4-5 BSRAM (font ROM + char buffer)
// =============================================================================

module hdmi_text #(
    parameter H_VISIBLE = 640,
    parameter H_FRONT   = 16,
    parameter H_SYNC    = 96,
    parameter H_BACK    = 48,
    parameter V_VISIBLE = 480,
    parameter V_FRONT   = 10,
    parameter V_SYNC    = 2,
    parameter V_BACK    = 33,
    parameter COLS      = 80,
    parameter ROWS      = 30,
    parameter CHAR_W    = 8,
    parameter CHAR_H    = 16
)(
    // Clocks
    input  wire        clk_pixel,      // 25.2 MHz pixel clock
    input  wire        clk_5x,         // 126 MHz serializer clock
    input  wire        rst_n,

    // CPU Bus Interface (active on clk_pixel, same domain as CPU)
    input  wire        bus_valid,
    input  wire [31:0] bus_addr,
    input  wire [31:0] bus_wdata,
    input  wire [3:0]  bus_wstrb,
    output wire        bus_ready,
    output wire [31:0] bus_rdata,

    // TMDS outputs
    output wire        tmds_clk_p,
    output wire        tmds_clk_n,
    output wire        tmds_d0_p,
    output wire        tmds_d0_n,
    output wire        tmds_d1_p,
    output wire        tmds_d1_n,
    output wire        tmds_d2_p,
    output wire        tmds_d2_n
);

    // =========================================================================
    // Timing Generator
    // =========================================================================

    localparam H_TOTAL = H_VISIBLE + H_FRONT + H_SYNC + H_BACK;  // 800
    localparam V_TOTAL = V_VISIBLE + V_FRONT + V_SYNC + V_BACK;   // 525

    reg [9:0]  h_count;   // 0..799
    reg [9:0]  v_count;   // 0..524
    wire       h_active, v_active, data_enable;
    wire       hsync, vsync;

    always @(posedge clk_pixel or negedge rst_n) begin
        if (!rst_n) begin
            h_count <= 10'd0;
            v_count <= 10'd0;
        end else begin
            if (h_count == H_TOTAL - 1) begin
                h_count <= 10'd0;
                if (v_count == V_TOTAL - 1)
                    v_count <= 10'd0;
                else
                    v_count <= v_count + 1'b1;
            end else begin
                h_count <= h_count + 1'b1;
            end
        end
    end

    // Active video region
    assign h_active    = (h_count < H_VISIBLE);
    assign v_active    = (v_count < V_VISIBLE);
    assign data_enable = h_active & v_active;

    // Sync signals (active-low for 640x480)
    assign hsync = ~((h_count >= H_VISIBLE + H_FRONT) &&
                     (h_count <  H_VISIBLE + H_FRONT + H_SYNC));
    assign vsync = ~((v_count >= V_VISIBLE + V_FRONT) &&
                     (v_count <  V_VISIBLE + V_FRONT + V_SYNC));

    // =========================================================================
    // Character Address Generation
    // =========================================================================

    wire [6:0] char_col  = h_count[9:3];           // pixel_x / 8
    wire [4:0] char_row  = v_count[9:4];           // pixel_y / 16
    wire [2:0] glyph_x   = h_count[2:0];           // pixel_x % 8
    wire [3:0] glyph_y   = v_count[3:0];           // pixel_y % 16

    // Scroll offset register
    reg [4:0] scroll_offset;

    // Adjusted row with scroll
    wire [4:0] adj_row = (char_row + scroll_offset >= ROWS)
                       ? (char_row + scroll_offset - ROWS[4:0])
                       : (char_row + scroll_offset);

    wire [11:0] char_addr = adj_row * COLS + char_col;  // max 2399

    // =========================================================================
    // Character Buffer (Dual-Port BSRAM)
    // =========================================================================

    // Port A: CPU write
    // Port B: Video read (one clock ahead for pipeline)

    wire [7:0]  char_code;
    wire [7:0]  char_attr;

    // Bus write interface
    wire        buf_we    = bus_valid & (|bus_wstrb) & (bus_addr[13] == 1'b0);
    wire [11:0] buf_waddr = bus_addr[12:1];
    wire [15:0] buf_wdata_16 = bus_wdata[15:0];

    // Dual-port character + attribute buffer
    // Implementation: Use Gowin BSRAM or inferred dual-port RAM
    reg [15:0] char_buffer [0:2399];  // 80x30 x 16-bit (char + attr)

    // Port A: CPU write
    always @(posedge clk_pixel) begin
        if (buf_we)
            char_buffer[buf_waddr] <= buf_wdata_16;
    end

    // Port B: Video read
    reg [15:0] char_data_pipe;
    always @(posedge clk_pixel) begin
        char_data_pipe <= char_buffer[char_addr];
    end

    assign char_code = char_data_pipe[7:0];
    assign char_attr = char_data_pipe[15:8];

    // =========================================================================
    // Font ROM (BSRAM, initialized at synthesis)
    // =========================================================================

    wire [11:0] font_addr = {char_code, glyph_y};  // 256 chars x 16 rows
    reg  [7:0]  font_data;

    // Font ROM: 4 KB, initialized from .mi file or hex
    (* ram_style = "block" *)
    reg [7:0] font_rom [0:4095];

    initial begin
        $readmemh("font_8x16.hex", font_rom);
    end

    // Pipeline stage: font ROM read
    reg [7:0] font_row_pipe;
    always @(posedge clk_pixel) begin
        font_row_pipe <= font_rom[font_addr];
    end

    // =========================================================================
    // Pixel Generation
    // =========================================================================

    // Delayed signals to match pipeline
    reg       data_enable_d1, data_enable_d2;
    reg       hsync_d1, hsync_d2;
    reg       vsync_d1, vsync_d2;
    reg [2:0] glyph_x_d1, glyph_x_d2;
    reg [7:0] char_attr_d1;

    always @(posedge clk_pixel or negedge rst_n) begin
        if (!rst_n) begin
            data_enable_d1 <= 1'b0;
            data_enable_d2 <= 1'b0;
            hsync_d1 <= 1'b1; hsync_d2 <= 1'b1;
            vsync_d1 <= 1'b1; vsync_d2 <= 1'b1;
        end else begin
            data_enable_d1 <= data_enable;
            data_enable_d2 <= data_enable_d1;
            hsync_d1       <= hsync;
            hsync_d2       <= hsync_d1;
            vsync_d1       <= vsync;
            vsync_d2       <= vsync_d1;
            glyph_x_d1    <= glyph_x;
            glyph_x_d2    <= glyph_x_d1;
            char_attr_d1   <= char_attr;
        end
    end

    // Select pixel from font row (MSB first)
    wire pixel_on = font_row_pipe[7 - glyph_x_d2];

    // Color lookup from attribute byte
    wire [2:0] fg_color = char_attr_d1[2:0];
    wire       fg_bright = char_attr_d1[3];
    wire [2:0] bg_color = char_attr_d1[6:4];

    // Convert 3-bit color + bright to 8-bit RGB
    // Simple CGA palette
    function [23:0] color_to_rgb;
        input [2:0] color;
        input       bright;
        begin
            case ({bright, color})
                4'b0_000: color_to_rgb = 24'h000000;  // Black
                4'b0_001: color_to_rgb = 24'h0000AA;  // Blue
                4'b0_010: color_to_rgb = 24'h00AA00;  // Green
                4'b0_011: color_to_rgb = 24'h00AAAA;  // Cyan
                4'b0_100: color_to_rgb = 24'hAA0000;  // Red
                4'b0_101: color_to_rgb = 24'hAA00AA;  // Magenta
                4'b0_110: color_to_rgb = 24'hAA5500;  // Brown
                4'b0_111: color_to_rgb = 24'hAAAAAA;  // Light Gray
                4'b1_000: color_to_rgb = 24'h555555;  // Dark Gray
                4'b1_001: color_to_rgb = 24'h5555FF;  // Light Blue
                4'b1_010: color_to_rgb = 24'h55FF55;  // Light Green
                4'b1_011: color_to_rgb = 24'h55FFFF;  // Light Cyan
                4'b1_100: color_to_rgb = 24'hFF5555;  // Light Red
                4'b1_101: color_to_rgb = 24'hFF55FF;  // Light Magenta
                4'b1_110: color_to_rgb = 24'hFFFF55;  // Yellow
                4'b1_111: color_to_rgb = 24'hFFFFFF;  // White
            endcase
        end
    endfunction

    wire [23:0] fg_rgb = color_to_rgb(fg_color, fg_bright);
    wire [23:0] bg_rgb = color_to_rgb(bg_color, 1'b0);

    wire [7:0] pixel_r = pixel_on ? fg_rgb[23:16] : bg_rgb[23:16];
    wire [7:0] pixel_g = pixel_on ? fg_rgb[15:8]  : bg_rgb[15:8];
    wire [7:0] pixel_b = pixel_on ? fg_rgb[7:0]   : bg_rgb[7:0];

    // =========================================================================
    // TMDS Encoding (3 channels)
    // =========================================================================

    wire [9:0] tmds_d0_symbol, tmds_d1_symbol, tmds_d2_symbol;

    // D0: Blue channel + sync signals
    tmds_encoder u_tmds_d0 (
        .clk      (clk_pixel),
        .rst_n    (rst_n),
        .data_in  (pixel_b),
        .ctrl_in  ({vsync_d2, hsync_d2}),
        .data_en  (data_enable_d2),
        .tmds_out (tmds_d0_symbol)
    );

    // D1: Green channel
    tmds_encoder u_tmds_d1 (
        .clk      (clk_pixel),
        .rst_n    (rst_n),
        .data_in  (pixel_g),
        .ctrl_in  (2'b00),
        .data_en  (data_enable_d2),
        .tmds_out (tmds_d1_symbol)
    );

    // D2: Red channel
    tmds_encoder u_tmds_d2 (
        .clk      (clk_pixel),
        .rst_n    (rst_n),
        .data_in  (pixel_r),
        .ctrl_in  (2'b00),
        .data_en  (data_enable_d2),
        .tmds_out (tmds_d2_symbol)
    );

    // =========================================================================
    // Serializers (OSER10 + ELVDS_OBUF)
    // =========================================================================

    tmds_serializer u_ser_d0 (
        .clk_pixel (clk_pixel), .clk_5x (clk_5x), .rst_n (rst_n),
        .tmds_data (tmds_d0_symbol),
        .tmds_p    (tmds_d0_p), .tmds_n (tmds_d0_n)
    );

    tmds_serializer u_ser_d1 (
        .clk_pixel (clk_pixel), .clk_5x (clk_5x), .rst_n (rst_n),
        .tmds_data (tmds_d1_symbol),
        .tmds_p    (tmds_d1_p), .tmds_n (tmds_d1_n)
    );

    tmds_serializer u_ser_d2 (
        .clk_pixel (clk_pixel), .clk_5x (clk_5x), .rst_n (rst_n),
        .tmds_data (tmds_d2_symbol),
        .tmds_p    (tmds_d2_p), .tmds_n (tmds_d2_n)
    );

    // Clock channel
    tmds_serializer u_ser_clk (
        .clk_pixel (clk_pixel), .clk_5x (clk_5x), .rst_n (rst_n),
        .tmds_data (10'b0000011111),
        .tmds_p    (tmds_clk_p), .tmds_n (tmds_clk_n)
    );

    // =========================================================================
    // Bus Interface (CPU access to framebuffer + control registers)
    // =========================================================================

    // Address decoding:
    //   0x0000-0x12BF: Character buffer (4,800 bytes = 2,400 x 16-bit)
    //   0x2000-0x2014: Control registers

    reg        bus_ready_r;
    reg [31:0] bus_rdata_r;

    wire is_ctrl_reg = (bus_addr[13] == 1'b1);  // 0x2000+

    // Cursor registers
    reg [6:0] cursor_col;
    reg [4:0] cursor_row;
    reg [3:0] cursor_blink_rate;
    reg [3:0] cursor_style;

    always @(posedge clk_pixel or negedge rst_n) begin
        if (!rst_n) begin
            bus_ready_r      <= 1'b0;
            bus_rdata_r      <= 32'b0;
            cursor_col       <= 7'd0;
            cursor_row       <= 5'd0;
            cursor_blink_rate <= 4'd8;
            cursor_style     <= 4'd1;
            scroll_offset    <= 5'd0;
        end else begin
            bus_ready_r <= 1'b0;

            if (bus_valid && !bus_ready_r) begin
                bus_ready_r <= 1'b1;

                if (is_ctrl_reg) begin
                    if (|bus_wstrb) begin
                        // Write to control register
                        case (bus_addr[4:2])
                            3'd0: begin  // 0x2000: CURSOR_POS
                                cursor_col <= bus_wdata[6:0];
                                cursor_row <= bus_wdata[12:8];
                            end
                            3'd1: begin  // 0x2004: CURSOR_CFG
                                cursor_blink_rate <= bus_wdata[7:4];
                                cursor_style      <= bus_wdata[3:0];
                            end
                            3'd4: begin  // 0x2010: SCROLL_OFF
                                scroll_offset <= bus_wdata[4:0];
                            end
                        endcase
                    end else begin
                        // Read control register
                        case (bus_addr[4:2])
                            3'd0: bus_rdata_r <= {19'b0, cursor_row, cursor_col};
                            3'd1: bus_rdata_r <= {24'b0, cursor_blink_rate, cursor_style};
                            3'd3: bus_rdata_r <= {16'b0, v_count, 6'b0};  // STATUS
                            3'd4: bus_rdata_r <= {27'b0, scroll_offset};
                            default: bus_rdata_r <= 32'b0;
                        endcase
                    end
                end
                // Character buffer writes handled by buf_we logic above
            end
        end
    end

    assign bus_ready = bus_ready_r;
    assign bus_rdata = bus_rdata_r;

endmodule
```

---

## 9. SoC Integration

### Updated Memory Map

With the HDMI text framebuffer added, the full SoC memory map becomes:

| Address Range | Device | Access | Notes |
|---------------|--------|--------|-------|
| `0x0000_0000 - 0x3FFF_FFFF` | SPI Flash XIP | R | Instruction fetch |
| `0x4000_0000 - 0x4000_1FFF` | SRAM 8 KB | RW | Data memory |
| `0x8000_0000 - 0x8000_1FFF` | Boot ROM | R | ISP flasher |
| `0x8100_0000 - 0x8100_001F` | SPI Flash Config | RW | |
| `0x8200_0000 - 0x8200_0007` | GPIO | RW | |
| `0x8300_0000 - 0x8300_0007` | UART | RW | |
| `0xC000_0000 - 0xC000_001F` | ATOMiK | RW | Delta accumulator |
| **`0xC100_0000 - 0xC100_2FFF`** | **HDMI Framebuffer** | **RW** | **NEW** |

### Bus Routing

The existing picotiny SoC uses a 1:4 address mux based on `mem_addr[31:30]`:

| `mem_addr[31:30]` | Peripheral |
|---------------------|------------|
| `2'b00` | SPI Flash XIP |
| `2'b01` | SRAM |
| `2'b10` | Peripherals (ROM, SPI config, GPIO, UART) |
| `2'b11` | ATOMiK (`0xC000_xxxx`) |

The HDMI framebuffer at `0xC100_0000` also falls in the `2'b11` range. We need secondary address decoding within this range:

```verilog
// Secondary decode for 0xC0000000-0xCFFFFFFF range
wire sel_atomik = (mem_addr[31:24] == 8'hC0);  // 0xC000_xxxx
wire sel_hdmi   = (mem_addr[31:24] == 8'hC1);  // 0xC100_xxxx

// Mux ready/rdata based on secondary decode
assign wbp_ready = sel_atomik ? atomik_ready :
                   sel_hdmi   ? hdmi_ready   : 1'b1;
assign wbp_rdata = sel_atomik ? atomik_rdata :
                   sel_hdmi   ? hdmi_rdata   : 32'b0;
```

### Clock Connections

No new PLLs are required. The HDMI text module uses existing clocks:

```verilog
hdmi_text u_hdmi_text (
    .clk_pixel (clk_p),           // 25.2 MHz (from HDMI PLL CLKOUTD)
    .clk_5x    (clk_hdmi_ser),    // 126 MHz (from HDMI PLL CLKOUT)
    .rst_n     (resetn),

    // Bus interface (directly from PicoRV32)
    .bus_valid (mem_valid & sel_hdmi),
    .bus_addr  (mem_addr),
    .bus_wdata (mem_wdata),
    .bus_wstrb (mem_wstrb),
    .bus_ready (hdmi_ready),
    .bus_rdata (hdmi_rdata),

    // TMDS pins
    .tmds_clk_p (tmds_clk_p),
    .tmds_clk_n (tmds_clk_n),
    .tmds_d0_p  (tmds_d0_p),
    .tmds_d0_n  (tmds_d0_n),
    .tmds_d1_p  (tmds_d1_p),
    .tmds_d1_n  (tmds_d1_n),
    .tmds_d2_p  (tmds_d2_p),
    .tmds_d2_n  (tmds_d2_n)
);
```

### SDC Timing Constraints

Add the following to the SDC file:

```tcl
# HDMI pixel clock (already defined if sharing with CPU clk_p)
# create_clock -name clk_pixel -period 39.682 [get_pins u_hdmi_pll/CLKOUTD]

# HDMI serializer clock
# create_clock -name clk_5x -period 7.936 [get_pins u_hdmi_pll/CLKOUT]

# Async clock groups: ATOMiK domain vs HDMI/CPU domain
set_clock_groups -asynchronous \
    -group [get_clocks clk_atomik] \
    -group [get_clocks {clk_pixel clk_5x}]

# TMDS outputs: false path (serializer handles timing internally)
set_false_path -to [get_ports tmds_*]
```

---

## 10. Resource Budget

### Estimated Resources for HDMI Text Module

| Component | LUTs | FFs | BSRAM | Notes |
|-----------|------|-----|-------|-------|
| Timing generator | ~30 | ~25 | 0 | H/V counters, sync gen |
| Character address gen | ~40 | ~15 | 0 | Division by 80, scroll |
| Character buffer | ~20 | ~10 | 2 | 2,400 x 16-bit dual-port |
| Font ROM | ~10 | ~5 | 2 | 4,096 x 8-bit single-port |
| Pixel pipeline | ~60 | ~40 | 0 | Color lookup, mux |
| TMDS encoders (x3) | ~180 | ~90 | 0 | ~60 LUT each |
| TMDS serializers (x4) | ~20 | ~20 | 0 | OSER10 primitives |
| Bus interface | ~80 | ~50 | 0 | Address decode, registers |
| Cursor logic | ~30 | ~20 | 0 | Blink counter, overlay |
| **Total HDMI text** | **~470-700** | **~275** | **4** | |

### Updated SoC Resource Budget

| Component | LUTs | BSRAM | Notes |
|-----------|------|-------|-------|
| PicoRV32 + peripherals (baseline) | 3,608 | 12 | Current production |
| ATOMiK single-bank + CDC | +230 | 0 | Current production |
| **HDMI text module** | **+500-700** | **+4** | **New** |
| **Total** | **~4,340-4,540** | **16** | |
| **Available** | **8,640** | **26** | GW1NR-9 |
| **Utilization** | **50-53%** | **62%** | |
| **Remaining** | **~4,100-4,300** | **10** | |

This leaves comfortable headroom for the command shell (firmware only, no LUT cost), future multi-bank ATOMiK scaling, and other peripherals.

### BSRAM Usage Detail

| Block | Usage | Size | Blocks |
|-------|-------|------|--------|
| PicoRV32 boot ROM | 2 KB | 2 KB | 1 |
| PicoRV32 SRAM | 8 KB | 8 KB | 4 |
| SPI flash buffer | varies | varies | ~7 |
| **Character buffer** | **4,800 B** | **8 KB** | **2** (each BSRAM is 2 KB in 16-bit mode) |
| **Font ROM** | **4,096 B** | **4 KB** | **2** |
| **Total** | | | **~16 of 26** |

---

## Appendix A: Font Data

### Generating the Font ROM

The 8x16 font ROM must be initialized with glyph bitmaps. Standard CP437 (IBM PC) fonts are available as binary or hex files.

**To generate `font_8x16.hex`:**

```python
#!/usr/bin/env python3
"""Generate font_8x16.hex for Gowin BSRAM initialization."""

# Option 1: Load from a standard .psf or .bin font file
# Option 2: Use the classic VGA 8x16 font (public domain, widely available)

import struct

def generate_font_hex(input_bin, output_hex):
    """Convert raw 8x16 font binary to Verilog $readmemh format."""
    with open(input_bin, 'rb') as f:
        data = f.read()

    with open(output_hex, 'w') as f:
        for i, byte in enumerate(data):
            f.write(f"{byte:02X}\n")

    print(f"Generated {output_hex}: {len(data)} bytes ({len(data)//16} glyphs)")

# Usage: generate_font_hex("vga_8x16.bin", "font_8x16.hex")
```

The classic VGA 8x16 font is 4,096 bytes (256 characters x 16 bytes per glyph). Each byte represents one row of 8 pixels, MSB = leftmost pixel. This font is embedded in every VGA BIOS and is in the public domain.

### Alternative: Gowin `.mi` File Format

Gowin can initialize BSRAM blocks using `.mi` (memory initialization) files. The format is one hex value per line:

```
// font_8x16.mi
// Address 0x000: Character 0x00, row 0
00
00
00
...
// Address 0x100: Character 0x10, row 0
3C
42
...
```

---

## Appendix B: Firmware Integration Example

### Minimal Terminal Driver

```c
// hdmi_fb.h - HDMI framebuffer driver for ATOMiK SoC

#ifndef HDMI_FB_H
#define HDMI_FB_H

#include <stdint.h>

#define FB_BASE      0xC1000000
#define FB_COLS      80
#define FB_ROWS      30

#define FB_CURSOR_POS  (*(volatile uint32_t*)(FB_BASE + 0x2000))
#define FB_CURSOR_CFG  (*(volatile uint32_t*)(FB_BASE + 0x2004))
#define FB_SCROLL_OFF  (*(volatile uint32_t*)(FB_BASE + 0x2010))

// Attribute byte: {blink, bg[2:0], bright, fg[2:0]}
#define ATTR(fg, bg)       (((bg) << 4) | (fg))
#define ATTR_BRIGHT(fg, bg) (((bg) << 4) | 0x08 | (fg))

// Standard colors
#define COLOR_BLACK   0
#define COLOR_BLUE    1
#define COLOR_GREEN   2
#define COLOR_CYAN    3
#define COLOR_RED     4
#define COLOR_MAGENTA 5
#define COLOR_BROWN   6
#define COLOR_WHITE   7

// Default: white on black
#define ATTR_DEFAULT  ATTR(COLOR_WHITE, COLOR_BLACK)
#define ATTR_ERROR    ATTR_BRIGHT(COLOR_RED, COLOR_BLACK)
#define ATTR_SUCCESS  ATTR_BRIGHT(COLOR_GREEN, COLOR_BLACK)
#define ATTR_HEADER   ATTR(COLOR_BLACK, COLOR_CYAN)

// Terminal state
static int term_col = 0;
static int term_row = 0;
static uint8_t term_attr = ATTR_DEFAULT;

static inline void fb_write_char(int col, int row, char ch, uint8_t attr) {
    volatile uint16_t *p = (volatile uint16_t *)(FB_BASE + (row * FB_COLS + col) * 2);
    *p = ((uint16_t)attr << 8) | (uint8_t)ch;
}

static inline void fb_clear(void) {
    for (int i = 0; i < FB_ROWS * FB_COLS; i++) {
        volatile uint16_t *p = (volatile uint16_t *)(FB_BASE + i * 2);
        *p = ((uint16_t)ATTR_DEFAULT << 8) | ' ';
    }
    term_col = 0;
    term_row = 0;
    FB_SCROLL_OFF = 0;
    FB_CURSOR_POS = 0;
}

static inline void fb_scroll_up(void) {
    // Move all rows up by one
    for (int row = 0; row < FB_ROWS - 1; row++) {
        for (int col = 0; col < FB_COLS; col++) {
            volatile uint16_t *src = (volatile uint16_t *)(FB_BASE + ((row + 1) * FB_COLS + col) * 2);
            volatile uint16_t *dst = (volatile uint16_t *)(FB_BASE + (row * FB_COLS + col) * 2);
            *dst = *src;
        }
    }
    // Clear bottom row
    for (int col = 0; col < FB_COLS; col++) {
        fb_write_char(col, FB_ROWS - 1, ' ', ATTR_DEFAULT);
    }
}

static inline void fb_putc(char ch) {
    if (ch == '\n') {
        term_col = 0;
        term_row++;
    } else if (ch == '\r') {
        term_col = 0;
    } else if (ch == '\b') {
        if (term_col > 0) {
            term_col--;
            fb_write_char(term_col, term_row, ' ', term_attr);
        }
    } else {
        fb_write_char(term_col, term_row, ch, term_attr);
        term_col++;
        if (term_col >= FB_COLS) {
            term_col = 0;
            term_row++;
        }
    }

    if (term_row >= FB_ROWS) {
        fb_scroll_up();
        term_row = FB_ROWS - 1;
    }

    FB_CURSOR_POS = ((term_row & 0xFF) << 8) | (term_col & 0xFF);
}

static inline void fb_puts(const char *s) {
    while (*s)
        fb_putc(*s++);
}

static inline void fb_set_attr(uint8_t attr) {
    term_attr = attr;
}

#endif // HDMI_FB_H
```

### Usage in Firmware

```c
#include "hdmi_fb.h"

void main(void) {
    fb_clear();

    // Print boot banner
    fb_set_attr(ATTR_HEADER);
    fb_puts("  ATOMiK SoC v1.0 -- Tang Nano 9K -- 25.2 MHz                                 ");
    fb_set_attr(ATTR_DEFAULT);
    fb_putc('\n');

    fb_puts("ATOMiK delta-state accelerator @ 81 MHz\n");
    fb_puts("PicoRV32 RISC-V CPU @ 25.2 MHz\n");
    fb_putc('\n');

    fb_set_attr(ATTR_SUCCESS);
    fb_puts("[PASS]");
    fb_set_attr(ATTR_DEFAULT);
    fb_puts(" ATOMiK hardware test: 11/11\n");

    fb_set_attr(ATTR_SUCCESS);
    fb_puts("[PASS]");
    fb_set_attr(ATTR_DEFAULT);
    fb_puts(" Runtime integration: 10/10\n");

    fb_putc('\n');
    fb_puts("atomik> ");
}
```

---

## Appendix C: Checklist for Implementation

1. **[ ] Font ROM**: Obtain or generate 8x16 CP437 font data as `.hex` or `.mi` file
2. **[ ] TMDS encoder**: Implement and simulate `tmds_encoder` module
3. **[ ] Serializer**: Instantiate Gowin OSER10 + ELVDS_OBUF, verify with simple test pattern
4. **[ ] Timing generator**: Implement H/V counters, verify sync timing with monitor
5. **[ ] Character buffer**: Implement dual-port BSRAM, verify CPU writes appear on screen
6. **[ ] Font rendering**: Connect character lookup to font ROM to pixel output
7. **[ ] Color support**: Implement attribute byte to RGB color conversion
8. **[ ] Bus integration**: Add address decode for `0xC100_xxxx`, connect to SoC bus
9. **[ ] Firmware driver**: Write `hdmi_fb.h` with putc/puts/clear/scroll functions
10. **[ ] Timing closure**: Run synthesis, verify TMDS serializer paths meet timing
11. **[ ] Hardware test**: Flash bitstream + firmware, verify text appears on HDMI monitor
12. **[ ] Terminal integration**: Route `mini_printf` output to framebuffer in addition to UART

---

## Related Files

| File | Description |
|------|-------------|
| [CLOCK_REFERENCE.md](CLOCK_REFERENCE.md) | PLL configuration and rPLL formulas |
| [TANG_NANO_9K_PINOUT.md](TANG_NANO_9K_PINOUT.md) | Pin assignments including HDMI |
| [GPIO_REFERENCE.md](GPIO_REFERENCE.md) | I/O standards and CST syntax |
| [TIMING_REFERENCE.md](TIMING_REFERENCE.md) | SDC timing constraints |
| `hardware/picorv32/atomik_bus_wrapper.v` | Existing SoC bus wrapper |
| `hardware/picorv32/atomik_cdc_bridge.v` | Existing CDC bridge |
| `hardware/picorv32/memory_map.md` | Current SoC memory map |
| `docs/PRODUCTION_DEPLOYMENT.md` | Production SoC documentation |
| `ROADMAP.md` | Section 5: OS Shell & User Interface |

---

*Reference: Tang Nano 9K Schematics (Tang_nano_9K_3674), Sipeed Pinout, VESA DMT Standard, DVI 1.0 Specification, Gowin UG286-1.9.1E, Gowin UG289-1.9.2E*
