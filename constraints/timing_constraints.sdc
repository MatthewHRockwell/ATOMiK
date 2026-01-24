# =============================================================================
# ATOMiK Timing Constraints (PLL-correct, Gowin-friendly)
#
# Reference clock: 27 MHz  (sys_clk port)
# Fabric clock:    81 MHz (PLL CLKOUT -> atomik_clk)
#
# Notes:
#  - sys_clk and atomik_clk are intentionally asynchronous domains in this design
#    (POR counter runs on sys_clk; logic runs on PLL clock).
#  - We explicitly declare them asynchronous to prevent bogus cross-domain timing.
# =============================================================================

# -----------------------------------------------------------------------------
# 1) Primary reference clock (PLL input) @ 27 MHz
# -----------------------------------------------------------------------------
    create_clock -name sys_clk -period 37.037 -waveform {0 18.518} [get_ports {sys_clk}]

# -----------------------------------------------------------------------------
# 2) Fabric clock (PLL output) @ 81 MHz
#    Use the exact pin path reported in your Clock Summary "Objects" column.
# -----------------------------------------------------------------------------
    create_clock -name atomik_clk -period 12.346 -waveform {0 6.173} [get_pins {*/CLKOUT}]

# -----------------------------------------------------------------------------
# 3) Declare the two clock domains asynchronous
#    This removes false cross-domain timing paths (e.g., POR -> reset sync).
# -----------------------------------------------------------------------------
    set_clock_groups -asynchronous -group [get_clocks {sys_clk}] -group [get_clocks {atomik_clk}]

# -----------------------------------------------------------------------------
# 4) UART I/O delays (used in fabric clock domain)
#    Conservative 2ns margins are fine for bring-up.
# -----------------------------------------------------------------------------
    set_input_delay  -clock atomik_clk -max 2.0 [get_ports {uart_rx}]
    set_output_delay -clock atomik_clk -max 2.0 [get_ports {uart_tx}]

# -----------------------------------------------------------------------------
# 5) Non-timing-critical paths
# -----------------------------------------------------------------------------
    set_false_path -from [get_ports {sys_rst_n}]
    set_false_path -to   [get_ports {led[*]}]
