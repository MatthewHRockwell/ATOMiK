# ==============================================================================
# ATOMiK Zynq Constraints — ALINX AX7020
# Target: XC7Z020-2CLG400I
#
# Dual-clock architecture:
#   - fclk_clk0: 100 MHz from PS (FCLK_CLK0)
#   - atomik_clk: 200+ MHz from MMCME2_BASE CLKOUT0
#   - axi_clk: 100 MHz from MMCME2_BASE CLKOUT1 (passthrough)
#
# CDC between domains handled by toggle-handshake (2FF synchronizers).
#
# Date: March 2026
# ==============================================================================

# ------------------------------------------------------------------------------
# PS Fabric Clock (FCLK_CLK0)
# The PS7 IP generates this clock; constraint ensures timing analysis.
# In PL-only builds, this may warn about no matching pins — harmless.
# ------------------------------------------------------------------------------
create_clock -period 10.000 -name fclk_clk0 [get_pins -hier -filter {NAME =~ */FCLK_CLK0}]

# ------------------------------------------------------------------------------
# MMCM-generated clocks
# Vivado auto-derives MMCM output clocks, but explicit constraints ensure
# correct naming for CDC false-path declarations.
# ------------------------------------------------------------------------------
create_generated_clock -name atomik_clk \
    -source [get_pins u_clk/u_mmcm/CLKIN1] \
    -master_clock fclk_clk0 \
    [get_pins u_clk/u_mmcm/CLKOUT0]

create_generated_clock -name axi_clk \
    -source [get_pins u_clk/u_mmcm/CLKIN1] \
    -master_clock fclk_clk0 \
    [get_pins u_clk/u_mmcm/CLKOUT1]

# ------------------------------------------------------------------------------
# CDC False Paths
# The toggle-handshake bridge (atomik_cdc_bridge) uses 2FF synchronizers
# on both req_toggle and ack_toggle crossings. All data is latched stable
# before the toggle edge crosses. No timing relationship needed.
# ------------------------------------------------------------------------------
set_false_path -from [get_clocks axi_clk] -to [get_clocks atomik_clk]
set_false_path -from [get_clocks atomik_clk] -to [get_clocks axi_clk]

# ------------------------------------------------------------------------------
# Phase 3: PL I/O constraints (uncomment when PL LEDs/buttons are used)
# ------------------------------------------------------------------------------
# # PL LED 0 (active low)
# set_property PACKAGE_PIN M14 [get_ports {pl_led[0]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {pl_led[0]}]
