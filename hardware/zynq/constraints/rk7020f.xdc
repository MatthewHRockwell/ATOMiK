# ==============================================================================
# ATOMiK Zynq Constraints — HamGeek RK-ZYNQ7020-F
# Target: XC7Z020-2CLG484I
# PS_CLK: 32.214 MHz (measured 2026-03-30 via UART baud calibration)
#
# Pin assignments discovered empirically — no schematic available.
# ==============================================================================

# ------------------------------------------------------------------------------
# PS Fabric Clock (FCLK_CLK0)
# The PS7 IP generates this clock; constraint ensures timing analysis.
# Actual frequency ~96.6 MHz (IO PLL 966 MHz / 5 / 2), constrained at 100 MHz
# for conservative timing closure.
# ------------------------------------------------------------------------------
create_clock -period 10.000 -name fclk_clk0 [get_pins -hier -filter {NAME =~ */FCLK_CLK0}]

# ------------------------------------------------------------------------------
# MMCM-generated clocks (ATOMiK accelerator dual-clock architecture)
# VCO = fclk × 10.0 ≈ 966 MHz, CLKOUT0 = VCO / ATOMIK_CLK_DIV
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
# Bitstream Configuration
# ------------------------------------------------------------------------------
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]

# ------------------------------------------------------------------------------
# Discovered PL I/O — HamGeek RK-ZYNQ7020-F (CLG484)
# Pins added as they are identified via binary search / sweep
# ------------------------------------------------------------------------------

# --- PL LEDs (active-high, confirmed 2026-03-30) ---
# Both in bank 33 (HR, LVCMOS33)
set_property PACKAGE_PIN V15 [get_ports {pl_led[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {pl_led[0]}]
set_property PACKAGE_PIN V13 [get_ports {pl_led[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {pl_led[1]}]

# --- HDMI TMDS (TBD — needs differential pair sweep) ---
# Likely on MRCC/SRCC pairs in banks 34/35 (CLG484)
# Candidate MRCC pairs (bank 34): L18/L19, M19/M20
# Candidate MRCC pairs (bank 35): D18/C19, B19/B20
# set_property PACKAGE_PIN ?? [get_ports TMDS_clk_p]
# set_property IOSTANDARD TMDS_33 [get_ports TMDS_clk_p]
# set_property PACKAGE_PIN ?? [get_ports {TMDS_data_p[0]}]
# set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[0]}]
# set_property PACKAGE_PIN ?? [get_ports {TMDS_data_p[1]}]
# set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[1]}]
# set_property PACKAGE_PIN ?? [get_ports {TMDS_data_p[2]}]
# set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[2]}]

# --- HDMI OEN (TBD) ---
# set_property PACKAGE_PIN ?? [get_ports hdmi_oen]
# set_property IOSTANDARD LVCMOS33 [get_ports hdmi_oen]

# --- 50 MHz PL Oscillator (TBD — MRCC pin) ---
# CLG484 MRCC candidates: Y6, Y9, W17, Y18, M19, L18, B19, D18
# set_property PACKAGE_PIN ?? [get_ports sys_clk]
# set_property IOSTANDARD LVCMOS33 [get_ports sys_clk]
# create_clock -period 20.000 -name sys_clk [get_ports sys_clk]
