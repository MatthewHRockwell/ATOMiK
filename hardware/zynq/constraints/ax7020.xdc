# ==============================================================================
# ATOMiK Zynq Constraints — ALINX AX7020
# Target: XC7Z020-2CLG400I
#
# Phase 1: Minimal timing constraints only.
# Pin assignments are handled by the PS7 block design — the AXI interface
# connects internally through the PS-PL bridge, no PL I/O pins needed.
#
# Date: March 2026
# ==============================================================================

# ------------------------------------------------------------------------------
# PS Fabric Clock (FCLK_CLK0)
# Phase 1: 100 MHz, single clock domain for both AXI and ATOMiK core.
# The PS7 IP generates this clock; this constraint ensures timing analysis.
# ------------------------------------------------------------------------------
create_clock -period 10.000 -name fclk_clk0 [get_pins -hier -filter {NAME =~ */FCLK_CLK0}]

# ------------------------------------------------------------------------------
# Phase 2: MMCM-generated ATOMiK clock (uncomment when MMCM is added)
# ------------------------------------------------------------------------------
# create_generated_clock -name atomik_clk \
#     -source [get_pins u_clk/u_mmcm/CLKIN1] \
#     -master_clock fclk_clk0 \
#     [get_pins u_clk/u_mmcm/CLKOUT0]
#
# # CDC false paths between AXI and ATOMiK domains
# set_false_path -from [get_clocks fclk_clk0] -to [get_clocks atomik_clk]
# set_false_path -from [get_clocks atomik_clk] -to [get_clocks fclk_clk0]
# # (Replace false_path with set_max_delay for tighter CDC control)

# ------------------------------------------------------------------------------
# Phase 3: PL I/O constraints (uncomment when PL LEDs/buttons are used)
# ------------------------------------------------------------------------------
# # PL LED 0 (active low)
# set_property PACKAGE_PIN M14 [get_ports {pl_led[0]}]
# set_property IOSTANDARD LVCMOS33 [get_ports {pl_led[0]}]
