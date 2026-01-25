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
# Version: 2.0 (Updated for ATOMiK Core v2 delta architecture)
# Date:    January 25, 2026
# =============================================================================

# -----------------------------------------------------------------------------
# 1) Primary Reference Clock (PLL Input) @ 27 MHz
# -----------------------------------------------------------------------------
# On-board crystal oscillator connected to pin 52
# Period: 1/27MHz = 37.037ns
create_clock -name sys_clk -period 37.037 -waveform {0 18.518} [get_ports {sys_clk}]

# -----------------------------------------------------------------------------
# 2) Fabric Clock (PLL Output) @ 94.5 MHz
# -----------------------------------------------------------------------------
# PLL configuration (from atomik_pll_94p5m.v):
#   FCLKIN = 27 MHz, IDIV_SEL = 1, FBDIV_SEL = 6, ODIV_SEL = 8
#   fCLKOUT = 27 MHz × 7 / 2 = 94.5 MHz
#   Period: 1/94.5MHz = 10.582ns
#
# Use exact pin path from synthesis Clock Summary "Objects" column
create_clock -name atomik_clk -period 10.582 -waveform {0 5.291} [get_pins {gen_pll.u_pll/rpll_inst/CLKOUT}]

# -----------------------------------------------------------------------------
# 3) Clock Domain Relationships
# -----------------------------------------------------------------------------
# sys_clk and atomik_clk are asynchronous domains:
#   - sys_clk: POR counter, reset synchronization
#   - atomik_clk: All fabric logic (core, UART, etc.)
#
# This declaration removes false cross-domain timing paths
set_clock_groups -asynchronous \
    -group [get_clocks {sys_clk}] \
    -group [get_clocks {atomik_clk}]

# -----------------------------------------------------------------------------
# 4) ATOMiK Core v2 Critical Paths
# -----------------------------------------------------------------------------
# The delta architecture has two critical paths:
#
# Path 1: Delta Accumulator Feedback Loop
#   delta_accumulator_reg -> XOR -> delta_accumulator_reg
#   Estimated delay: ~3.9ns (including routing)
#   Margin at 94.5 MHz: 10.582 - 3.9 = 6.68ns ✓
#
# Path 2: State Reconstruction
#   initial_state_reg -> XOR -> data_out_reg
#   Estimated delay: ~3.9ns (including routing)
#   Margin at 94.5 MHz: 10.582 - 3.9 = 6.68ns ✓
#
# No explicit max_delay constraints needed - default clock period is sufficient.
# XOR operations have no carry propagation, so 64-bit width is not a concern.

# -----------------------------------------------------------------------------
# 5) Input/Output Constraints
# -----------------------------------------------------------------------------

# UART Interface (fabric clock domain)
# Conservative 2ns margins for bring-up
set_input_delay  -clock atomik_clk -max 2.0 [get_ports {uart_rx}]
set_input_delay  -clock atomik_clk -min 0.0 [get_ports {uart_rx}]
set_output_delay -clock atomik_clk -max 2.0 [get_ports {uart_tx}]
set_output_delay -clock atomik_clk -min 0.0 [get_ports {uart_tx}]

# -----------------------------------------------------------------------------
# 6) False Paths (Non-Timing-Critical)
# -----------------------------------------------------------------------------

# Reset input - asynchronous, synchronized internally
set_false_path -from [get_ports {sys_rst_n}]

# LED outputs - visual indicators, no timing requirements
set_false_path -to [get_ports {led[*]}]

# Debug outputs (if directly connected to pins)
# set_false_path -to [get_ports {debug_*}]

# -----------------------------------------------------------------------------
# 7) Multicycle Paths (None Required for Core v2)
# -----------------------------------------------------------------------------
# All operations in atomik_core_v2 complete in a single cycle:
#   - LOAD: 1 cycle
#   - ACCUMULATE: 1 cycle  
#   - READ: 1 cycle (combinational XOR + output register)
#
# No multicycle path declarations needed.

# -----------------------------------------------------------------------------
# 8) Clock Uncertainty
# -----------------------------------------------------------------------------
# Account for PLL jitter and clock tree skew
# GW1NR-9 PLL typical jitter: <100ps
# Clock tree skew estimate: <200ps
set_clock_uncertainty -setup 0.3 [get_clocks {atomik_clk}]
set_clock_uncertainty -hold  0.1 [get_clocks {atomik_clk}]

# -----------------------------------------------------------------------------
# 9) Design Rule Constraints
# -----------------------------------------------------------------------------
# Maximum fanout to prevent timing degradation
# Gowin synthesis typically handles this automatically
# set_max_fanout 32 [current_design]

# Maximum transition time for clean edges
# set_max_transition 2.0 [current_design]

# =============================================================================
# Timing Budget Summary (94.5 MHz target)
# =============================================================================
#
# Clock Period:           10.582 ns
# Clock Uncertainty:       0.300 ns (setup)
# Available for Logic:    10.282 ns
#
# Critical Path Estimate:  3.900 ns (64-bit XOR + routing)
# Timing Margin:           6.382 ns (62% margin)
#
# Verdict: Timing closure expected with significant margin
#
# =============================================================================
