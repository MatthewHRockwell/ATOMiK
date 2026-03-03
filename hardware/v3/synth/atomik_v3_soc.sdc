//Copyright (C)2014-2021 GOWIN Semiconductor Corporation.
//All rights reserved.
//File Title: Timing Constraints file
//Part Number: GW1NR-LV9QN88PC6/I5

// ============================================================================
// v3 SoC Clock Architecture
// ============================================================================
// Input: 27 MHz crystal oscillator
// PLL: 27 MHz → 108 MHz (clk_p5, for HDMI 5x serializer)
//   FBDIV_SEL=3 (×4), IDIV_SEL=0 (÷1), ODIV_SEL=4: 27×4=108, VCO=432 MHz
// CLKDIV: 108 MHz ÷ 5 → 21.6 MHz (clk_p, for CPU + HDMI pixel clock)
// ATOMiK: Direct-wired via custom instructions (same clock domain as CPU)
// ============================================================================
// Previous: 126 MHz / 25.2 MHz — failed timing (40 setup violations, V3-020)
// Changed to: 108 MHz / 21.6 MHz — Fmax 24.745 MHz gives 15% margin
// HDMI: 108 MHz = 5× 21.6 MHz pixel clock (ratio preserved)
// Refresh: 21.6 MHz / (800×525) ≈ 51.4 Hz (within monitor tolerance)
// ============================================================================

// 27 MHz oscillator input
create_clock -name clk_osc -period 37.037 -waveform {0 18.518} [get_ports {clk}]

// Note: Gowin tools auto-detect PLL but NOT CLKDIV correctly.
// Must explicitly create the CLKDIV output clock.
// clk_p5 = 108 MHz (period 9.259 ns) — PLL output (auto-detected)
// clk_p  = 21.6 MHz (period 46.296 ns) — CLKDIV ÷5 output
// Using create_clock (not create_generated_clock) because get_pins may not resolve
// in Gowin's PNR netlist after hierarchy flattening.
create_clock -name clk_cpu -period 46.296 -waveform {0 23.148} [get_nets {clk_p}]
