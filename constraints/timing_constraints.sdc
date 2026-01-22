# =============================================================================
# ATOMiK Timing Constraints (PLL-correct)
# Reference clock: 27 MHz
# Fabric clock:    81 MHz (PLL CLKOUT)
# =============================================================================

# 27 MHz reference clock (PLL input)
create_clock -name sys_clk -period 37.037 -waveform {0 18.518} [get_ports {sys_clk}]

//# 81 MHz fabric clock (PLL output)
//create_clock -name atomik_clk -period 12.346 -waveform {0 6.173} [get_pins {gen_pll.u_pll/rpll_inst/CLKOUT}]

# 94.5 MHz fabric clock (PLL output)
create_clock -name atomik_clk -period 10.582 -waveform {0 5.291} [get_pins {gen_pll.u_pll/rpll_inst/CLKOUT}]

//# 108 MHz fabric clock (PLL output)
//create_clock -name atomik_clk -period 9.259 -waveform {0 4.629} [get_pins {gen_pll.u_pll/rpll_inst/CLKOUT}]

//# 120 MHz fabric clock (PLL output)
//create_clock -name atomik_clk -period 8.333 -waveform {0 4.167} [get_pins {gen_pll.u_pll/rpll_inst/CLKOUT}]

# UART timing (UART sampled/used in fabric clock domain)
set_input_delay  -clock atomik_clk -max 2.0 [get_ports {uart_rx}]
set_output_delay -clock atomik_clk -max 2.0 [get_ports {uart_tx}]

# False paths (non-timing-critical)
set_false_path -from [get_ports {sys_rst_n}]
set_false_path -to   [get_ports {led[*]}]
