# =============================================================================
# ATOMiK Timing Constraints (SDC)
#
# Target: Gowin GW1NR-9 (Tang Nano 9K)
# Tool:   Gowin EDA V1.9.11.03 Education
#
# Clock Configuration:
#   - Reference clock: 27 MHz (sys_clk port, on-board crystal)
#   - Fabric clock:    94.5 MHz (PLL CLKOUT -> atomik_clk)
#
# Version: 2.2 (Gowin-compatible SDC subset)
# Date:    January 25, 2026
# =============================================================================

# -----------------------------------------------------------------------------
# 1) Primary Reference Clock (PLL Input) @ 27 MHz
# -----------------------------------------------------------------------------
create_clock -name sys_clk -period 37.037 -waveform {0 18.518} [get_ports {sys_clk}]

# -----------------------------------------------------------------------------
# 2) Fabric Clock (PLL Output) @ 94.5 MHz
# -----------------------------------------------------------------------------
create_clock -name atomik_clk -period 10.582 -waveform {0 5.291} [get_pins {gen_pll.u_pll/rpll_inst/CLKOUT}]

# -----------------------------------------------------------------------------
# 3) Clock Domain Relationships (Asynchronous)
# -----------------------------------------------------------------------------
set_clock_groups -asynchronous -group [get_clocks {sys_clk}] -group [get_clocks {atomik_clk}]

# -----------------------------------------------------------------------------
# 4) Input/Output Constraints
# -----------------------------------------------------------------------------
set_input_delay -clock atomik_clk -max 2.0 [get_ports {uart_rx}]
set_output_delay -clock atomik_clk -max 2.0 [get_ports {uart_tx}]

# -----------------------------------------------------------------------------
# 5) False Paths (Non-Timing-Critical)
# -----------------------------------------------------------------------------
set_false_path -from [get_ports {sys_rst_n}]
set_false_path -to [get_ports {led[*]}]
