//Copyright (C)2014-2021 GOWIN Semiconductor Corporation.
//All rights reserved.
//File Title: Timing Constraints file
//Part Number: GW1NR-LV9QN88PC6/I5

// 27 MHz oscillator
create_clock -name clk_osc -period 37.037 -waveform {0 18.518} [get_ports {clk}]

// v3 SoC: Single clock domain (no CDC needed)
// PLL: 27 MHz → 126 MHz, CLKDIV ÷5 → 25.2 MHz
// ATOMiK is direct-wired via custom instructions (same clock as CPU)
// No false paths needed — no cross-domain crossing
