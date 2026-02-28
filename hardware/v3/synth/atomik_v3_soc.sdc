//Copyright (C)2014-2021 GOWIN Semiconductor Corporation.
//All rights reserved.
//File Title: Timing Constraints file
//Part Number: GW1NR-LV9QN88PC6/I5

// ============================================================================
// v3 SoC Clock Architecture (restored from v2)
// ============================================================================
// Input: 27 MHz crystal oscillator
// PLL: 27 MHz → 126 MHz (clk_p5, for HDMI 5x serializer)
// CLKDIV: 126 MHz ÷ 5 → 25.2 MHz (clk_p, for CPU + HDMI pixel clock)
// ATOMiK: Direct-wired via custom instructions (same clock domain as CPU)
// ============================================================================

// 27 MHz oscillator input
create_clock -name clk_osc -period 37.037 -waveform {0 18.518} [get_ports {clk}]

// Note: Gowin tools auto-detect PLL and CLKDIV generated clocks.
// Expected: clk_p5 = 126 MHz (period 7.937 ns)
//           clk_p  = 25.2 MHz (period 39.683 ns)
//
// Target Fmax for CPU: 25.2 MHz (well within measured capability of 26.563 MHz)
