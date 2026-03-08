# ==============================================================================
# ATOMiK Zynq Block Design (PS + PL Integration)
#
# Target:  ALINX AX7020 (XC7Z020-2CLG400I)
# Purpose: Create Zynq PS + ATOMiK PL design via IP Integrator.
#          This is the primary build path — auto-configures PS, AXI
#          interconnect, reset infrastructure, and exports XSA.
#
# Usage:
#   vivado -mode batch -source vivado/block_design.tcl
#
# Run from hardware/zynq/ directory.
#
# Date: March 2026
# ==============================================================================

# ------------------------------------------------------------------------------
# Path Setup
# ------------------------------------------------------------------------------
set SCRIPT_DIR [file dirname [info script]]
set ZYNQ_DIR   [file normalize "$SCRIPT_DIR/.."]
set V3_RTL_DIR [file normalize "$ZYNQ_DIR/../v3/rtl"]

puts "=============================================="
puts " ATOMiK Zynq Block Design — PS + PL"
puts "=============================================="
puts ""

file mkdir $ZYNQ_DIR/output

# ------------------------------------------------------------------------------
# Create Project
# ------------------------------------------------------------------------------

create_project atomik_zynq $ZYNQ_DIR/vivado/atomik_zynq -part xc7z020clg400-2 -force

# Add ATOMiK RTL sources
add_files [list \
    $ZYNQ_DIR/rtl/atomik_axi4lite_wrapper.v \
    $V3_RTL_DIR/atomik_v3_atomik.v \
]

# ------------------------------------------------------------------------------
# Create Block Design
# ------------------------------------------------------------------------------

create_bd_design "atomik_system"

# ------------------------------------------------------------------------------
# Add and Configure Zynq PS
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7

# DDR3: 2x MT41K256M16 (1 GB total, 32-bit bus, 1066 MT/s)
set_property -dict [list \
    CONFIG.PCW_DDR_RAM_HIGHADDR {0x3FFFFFFF} \
    CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125} \
    CONFIG.PCW_UIPARAM_DDR_MEMORY_TYPE {DDR 3} \
    CONFIG.PCW_UIPARAM_DDR_DEVICE_CAPACITY {4096 MBits} \
    CONFIG.PCW_UIPARAM_DDR_BUS_WIDTH {32 Bit} \
    CONFIG.PCW_UIPARAM_DDR_FREQ_MHZ {533.333313} \
] [get_bd_cells ps7]

# UART0 (MIO 46-47, CP2102 USB-UART)
set_property -dict [list \
    CONFIG.PCW_UART0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_UART0_UART0_IO {MIO 46 .. 47} \
] [get_bd_cells ps7]

# Ethernet GEM0 (MIO 16-27, RGMII to KSZ9031)
set_property -dict [list \
    CONFIG.PCW_ENET0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_ENET0_ENET0_IO {MIO 16 .. 27} \
    CONFIG.PCW_ENET0_GRP_MDIO_ENABLE {1} \
    CONFIG.PCW_ENET0_GRP_MDIO_IO {MIO 52 .. 53} \
] [get_bd_cells ps7]

# USB0 (MIO 28-39, ULPI to USB3320)
set_property -dict [list \
    CONFIG.PCW_USB0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_USB0_USB0_IO {MIO 28 .. 39} \
] [get_bd_cells ps7]

# SD0 (MIO 40-45, 4-bit SD card)
set_property -dict [list \
    CONFIG.PCW_SD0_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_SD0_SD0_IO {MIO 40 .. 45} \
] [get_bd_cells ps7]

# QSPI boot flash (MIO 1-6)
set_property -dict [list \
    CONFIG.PCW_QSPI_PERIPHERAL_ENABLE {1} \
    CONFIG.PCW_QSPI_GRP_SINGLE_SS_ENABLE {1} \
] [get_bd_cells ps7]

# GP AXI Master 0 (for ATOMiK peripheral access)
set_property -dict [list \
    CONFIG.PCW_USE_M_AXI_GP0 {1} \
] [get_bd_cells ps7]

# FCLK_CLK0: 100 MHz (Phase 1 — single clock for both AXI and ATOMiK)
set_property -dict [list \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100} \
] [get_bd_cells ps7]

# ------------------------------------------------------------------------------
# Add ATOMiK AXI4-Lite Peripheral
# ------------------------------------------------------------------------------

create_bd_cell -type module -reference atomik_axi4lite_wrapper atomik_0

# ------------------------------------------------------------------------------
# Add AXI Interconnect
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0
set_property CONFIG.NUM_MI {1} [get_bd_cells axi_interconnect_0]

# ------------------------------------------------------------------------------
# Add Processor System Reset
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0

# ------------------------------------------------------------------------------
# Connect Everything
# ------------------------------------------------------------------------------

# PS GP AXI Master → AXI Interconnect → ATOMiK
connect_bd_intf_net [get_bd_intf_pins ps7/M_AXI_GP0] \
                    [get_bd_intf_pins axi_interconnect_0/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_interconnect_0/M00_AXI] \
                    [get_bd_intf_pins atomik_0/S_AXI]

# Clocks
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] \
               [get_bd_pins axi_interconnect_0/ACLK] \
               [get_bd_pins axi_interconnect_0/S00_ACLK] \
               [get_bd_pins axi_interconnect_0/M00_ACLK] \
               [get_bd_pins atomik_0/s_axi_aclk] \
               [get_bd_pins proc_sys_reset_0/slowest_sync_clk]

# Resets
connect_bd_net [get_bd_pins ps7/FCLK_RESET0_N] \
               [get_bd_pins proc_sys_reset_0/ext_reset_in]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] \
               [get_bd_pins axi_interconnect_0/ARESETN] \
               [get_bd_pins axi_interconnect_0/S00_ARESETN] \
               [get_bd_pins axi_interconnect_0/M00_ARESETN] \
               [get_bd_pins atomik_0/s_axi_aresetn]

# PS FIXED_IO and DDR (mandatory external connections)
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]

# ------------------------------------------------------------------------------
# Address Assignment
# ------------------------------------------------------------------------------

# ATOMiK at 0x43C0_0000 (4 KB range)
assign_bd_address -target_address_space /ps7/Data \
    [get_bd_addr_segs atomik_0/S_AXI/reg0] \
    -range 4K -offset 0x43C00000

# ------------------------------------------------------------------------------
# Generate and Build
# ------------------------------------------------------------------------------

# Validate block design
validate_bd_design

# Generate output products
generate_target all [get_files atomik_system.bd]

# Create HDL wrapper (Vivado-managed)
make_wrapper -files [get_files atomik_system.bd] -top
add_files -norecurse [glob $ZYNQ_DIR/vivado/atomik_zynq/atomik_zynq.gen/sources_1/bd/atomik_system/hdl/*_wrapper.v]

# Add constraints
add_files -fileset constrs_1 $ZYNQ_DIR/constraints/ax7020.xdc

# Run synthesis + implementation + bitstream
launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

# Export hardware definition (for PetaLinux / Vitis)
write_hw_platform -fixed -include_bit -force $ZYNQ_DIR/output/atomik_zynq.xsa

puts ""
puts "=============================================="
puts " Block design build complete."
puts "  XSA file: output/atomik_zynq.xsa"
puts "=============================================="
