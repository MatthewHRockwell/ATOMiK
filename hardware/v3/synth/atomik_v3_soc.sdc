//Copyright (C)2014-2021 GOWIN Semiconductor Corporation.
//All rights reserved.
//File Title: Timing Constraints file
//Part Number: GW1NR-LV9QN88PC6/I5

// ============================================================================
// v3 SoC Dual-PLL Clock Architecture
// ============================================================================
// Input: 27 MHz crystal oscillator
//
// PLL1 (CPU): 27 MHz → 108 MHz → CLKDIV ÷5 → 21.6 MHz (clk_p)
//   FBDIV_SEL=3 (×4), IDIV_SEL=0 (÷1), ODIV_SEL=4: 27×4=108, VCO=432 MHz
//   ATOMiK: Direct-wired via custom instructions (same domain as CPU)
//
// PLL2 (HDMI): 27 MHz → 200.57 MHz → CLKDIV2 ÷5 → 40.11 MHz (clk_pixel)
//   FBDIV_SEL=51 (×52), IDIV_SEL=6 (÷7), ODIV_SEL=4: 27×52/7=200.57, VCO=802.3 MHz
//   800×600 @ 60 Hz (40.11 MHz pixel clock, +0.3% from ideal 40 MHz)
//   200.57 MHz used for HDMI 5x serializer
//
// CDC: Toggle-handshake bridge between CPU and pixel domains (disp_mmio_cdc)
// ============================================================================

// 27 MHz oscillator input
create_clock -name clk_osc -period 37.037 -waveform {0 18.518} [get_ports {clk}]

// Note: Gowin tools auto-detect PLL but NOT CLKDIV correctly.
// Must explicitly create the CLKDIV output clock.
// Using create_clock (not create_generated_clock) because get_pins may not resolve
// in Gowin's PNR netlist after hierarchy flattening.

// CPU clock: 21.6 MHz (period 46.296 ns) — PLL1 108 MHz ÷ 5
create_clock -name clk_cpu -period 46.296 -waveform {0 23.148} [get_nets {clk_p}]

// HDMI pixel clock: 40.11 MHz (period 24.931 ns) — PLL2 200.57 MHz ÷ 5
create_clock -name clk_pixel -period 24.931 -waveform {0 12.466} [get_nets {clk_pixel}]

// CDC false paths: toggle-handshake synchronizers handle timing
set_false_path -from [get_clocks {clk_cpu}] -to [get_clocks {clk_pixel}]
set_false_path -from [get_clocks {clk_pixel}] -to [get_clocks {clk_cpu}]
