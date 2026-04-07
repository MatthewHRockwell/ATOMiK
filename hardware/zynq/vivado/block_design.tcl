# ==============================================================================
# ATOMiK Zynq Block Design (PS + PL Integration)
#
# Target:  HamGeek RK-ZYNQ7020-F (XC7Z020-2CLG484I)
# NOTE:    PS_CLK = 32.214 MHz (not 33.333 MHz) — affects all PLL frequencies
#          IO PLL = 32.214 × 30 = 966 MHz, FCLK = 966/5/2 ≈ 96.6 MHz
#          (constrained at 100 MHz for conservative timing closure)
# Purpose: Create Zynq PS + ATOMiK PL design via IP Integrator.
#          Phase A: ATOMiK N=16 accelerator via AXI4-Lite + UIO
#          Phase B: RV64I co-processor with DDR3 access + mailbox
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
puts "  Phase A: ATOMiK N=16 accelerator"
puts "  Phase B: RV64I co-processor"
puts "=============================================="
puts ""

file mkdir $ZYNQ_DIR/output

# ------------------------------------------------------------------------------
# Create Project
# ------------------------------------------------------------------------------

create_project atomik_zynq $ZYNQ_DIR/vivado/atomik_zynq -part xc7z020clg484-2 -force

# Add ATOMiK Zynq RTL sources (Phase A: accelerator)
add_files [list \
    $ZYNQ_DIR/rtl/atomik_axi4lite_wrapper.v \
    $ZYNQ_DIR/rtl/atomik_cdc_bridge.v \
    $ZYNQ_DIR/rtl/atomik_core_zynq.v \
    $ZYNQ_DIR/rtl/atomik_core_zynq_parallel.v \
    $ZYNQ_DIR/rtl/atomik_zynq_clk.v \
    $ZYNQ_DIR/rtl/atomik_zynq_top.v \
]

# Add Phase B RTL sources (co-processor)
add_files [list \
    $ZYNQ_DIR/rtl/atomik_v3_cpu_zynq.v \
    $ZYNQ_DIR/rtl/atomik_v3_regfile_zynq.v \
    $ZYNQ_DIR/rtl/atomik_v3_atomik_zynq.v \
    $ZYNQ_DIR/rtl/atomik_v3_bus_to_axi.v \
    $ZYNQ_DIR/rtl/atomik_mailbox.v \
    $ZYNQ_DIR/rtl/atomik_coprocessor_top.v \
]

# Add v3 vendor-independent submodules (shared with Tang Nano)
add_files [list \
    $V3_RTL_DIR/atomik_v3_alu.v \
    $V3_RTL_DIR/atomik_v3_branch.v \
    $V3_RTL_DIR/atomik_v3_control.v \
    $V3_RTL_DIR/atomik_v3_csr.v \
    $V3_RTL_DIR/atomik_v3_decode.v \
    $V3_RTL_DIR/atomik_v3_fetch.v \
    $V3_RTL_DIR/atomik_v3_lsu.v \
]

# Enable automatic XPM detection (required for BRAM inference in parallel core)
auto_detect_xpm

# ------------------------------------------------------------------------------
# Create Block Design
# ------------------------------------------------------------------------------

create_bd_design "atomik_system"

# ------------------------------------------------------------------------------
# Add and Configure Zynq PS
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7

# ===========================================================================
# PS7 Configuration — sourced from ALINX AX7020 reference (ps_config.tcl)
# Validated against: alinx_reference/course_s4_linux/07_opencv_an5642/
# ===========================================================================

# Use the ALINX-validated PS configuration as the base
source $ZYNQ_DIR/alinx_reference/course_s4_linux/07_opencv_an5642/Vivado/auto_create_project/ps_config.tcl
set_ps_config ps7

# Override/extend for ATOMiK-specific requirements:
# - M_AXI_GP0: ATOMiK accelerator + mailbox (already enabled in ALINX config)
# - S_AXI_HP0: Co-processor DDR3 access (already enabled in ALINX config)
# - S_AXI_HP1: Disable (not needed, ALINX enables it for camera)
# - FCLK_CLK0: Override to 100 MHz (ALINX default is 50 MHz; our MMCM needs ≥60 MHz)
#   NOTE: Actual FCLK at runtime is ~96.6 MHz (set by ps7_init.tcl), not exactly 100 MHz.
#   The 100 MHz setting ensures Vivado timing analysis is conservative.
# - IRQ_F2P: Enabled (already set in ALINX config)
set_property -dict [list \
    CONFIG.PCW_USE_S_AXI_HP1 {0} \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100} \
    CONFIG.PCW_S_AXI_HP0_DATA_WIDTH {32} \
] [get_bd_cells ps7]

# ------------------------------------------------------------------------------
# Phase A: ATOMiK N=16 Accelerator (AXI4-Lite)
# ------------------------------------------------------------------------------

# Add atomik_zynq_top with N_BANKS=16
create_bd_cell -type module -reference atomik_zynq_top atomik_0
set_property -dict [list \
    CONFIG.N_BANKS {16} \
    CONFIG.ATOMIK_CLK_DIV {3.75} \
] [get_bd_cells atomik_0]
# 1000 / 3.75 = 266.7 MHz (N=16 ceiling from characterization)

# Associate fclk_clk0 with the s_axi interface (Vivado can't auto-infer
# because our clock port isn't named 'aclk' or 's_axi_aclk')
set_property CONFIG.ASSOCIATED_BUSIF {s_axi} [get_bd_pins atomik_0/fclk_clk0]
set_property CONFIG.ASSOCIATED_RESET {fclk_reset_n} [get_bd_pins atomik_0/fclk_clk0]

# ------------------------------------------------------------------------------
# Phase B: RV64I Co-Processor
# ------------------------------------------------------------------------------

create_bd_cell -type module -reference atomik_coprocessor_top coprocessor_0

# ------------------------------------------------------------------------------
# Add AXI Interconnect (GP0 → ATOMiK + Mailbox)
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0
set_property CONFIG.NUM_MI {2} [get_bd_cells axi_interconnect_0]
# M00 → ATOMiK accelerator (Phase A)
# M01 → Co-processor mailbox (Phase B)

# ------------------------------------------------------------------------------
# Add AXI Protocol Converter (AXI4 → AXI3)
# The co-processor's M_AXI is AXI4 (8-bit LEN) but PS7 S_AXI_HP0 is AXI3
# (4-bit LEN). The bridge only does single-beat transfers, so this is a
# thin metadata shim — no performance impact.
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_protocol_converter:2.1 axi_proto_conv_0

# ------------------------------------------------------------------------------
# Add Processor System Reset
# ------------------------------------------------------------------------------

create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0

# ------------------------------------------------------------------------------
# Connect Everything
# ------------------------------------------------------------------------------

# PS GP AXI Master → AXI Interconnect
connect_bd_intf_net [get_bd_intf_pins ps7/M_AXI_GP0] \
                    [get_bd_intf_pins axi_interconnect_0/S00_AXI]

# M00 → ATOMiK accelerator (Phase A)
connect_bd_intf_net [get_bd_intf_pins axi_interconnect_0/M00_AXI] \
                    [get_bd_intf_pins atomik_0/S_AXI]

# M01 → Co-processor mailbox (Phase B)
connect_bd_intf_net [get_bd_intf_pins axi_interconnect_0/M01_AXI] \
                    [get_bd_intf_pins coprocessor_0/S_AXI_MBOX]

# Co-processor AXI Master → Protocol Converter → PS HP0 (DDR3 access)
connect_bd_intf_net [get_bd_intf_pins coprocessor_0/M_AXI] \
                    [get_bd_intf_pins axi_proto_conv_0/S_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_proto_conv_0/M_AXI] \
                    [get_bd_intf_pins ps7/S_AXI_HP0]

# Clocks — FCLK_CLK0 to all components
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] \
               [get_bd_pins ps7/M_AXI_GP0_ACLK] \
               [get_bd_pins ps7/S_AXI_HP0_ACLK] \
               [get_bd_pins axi_interconnect_0/ACLK] \
               [get_bd_pins axi_interconnect_0/S00_ACLK] \
               [get_bd_pins axi_interconnect_0/M00_ACLK] \
               [get_bd_pins axi_interconnect_0/M01_ACLK] \
               [get_bd_pins atomik_0/fclk_clk0] \
               [get_bd_pins coprocessor_0/clk] \
               [get_bd_pins axi_proto_conv_0/aclk] \
               [get_bd_pins proc_sys_reset_0/slowest_sync_clk]

# Resets
connect_bd_net [get_bd_pins ps7/FCLK_RESET0_N] \
               [get_bd_pins proc_sys_reset_0/ext_reset_in]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] \
               [get_bd_pins axi_interconnect_0/ARESETN] \
               [get_bd_pins axi_interconnect_0/S00_ARESETN] \
               [get_bd_pins axi_interconnect_0/M00_ARESETN] \
               [get_bd_pins axi_interconnect_0/M01_ARESETN] \
               [get_bd_pins atomik_0/fclk_reset_n] \
               [get_bd_pins coprocessor_0/ext_rst_n] \
               [get_bd_pins axi_proto_conv_0/aresetn]

# Interrupt: co-processor mailbox → PS IRQ_F2P
connect_bd_net [get_bd_pins coprocessor_0/irq_to_arm] \
               [get_bd_pins ps7/IRQ_F2P]

# PS FIXED_IO and DDR (mandatory external connections)
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]

# ------------------------------------------------------------------------------
# Address Assignment
# ------------------------------------------------------------------------------

# ATOMiK accelerator at 0x43C0_0000 (4 KB range)
assign_bd_address -target_address_space /ps7/Data \
    [get_bd_addr_segs atomik_0/S_AXI/reg0] \
    -range 4K -offset 0x43C00000

# Co-processor mailbox at 0x43C1_0000 (4 KB range)
assign_bd_address -target_address_space /ps7/Data \
    [get_bd_addr_segs coprocessor_0/S_AXI_MBOX/reg0] \
    -range 4K -offset 0x43C10000

# HP0 address range for co-processor DDR3 access (256 MB at 0x0000_0000)
assign_bd_address -target_address_space /coprocessor_0/M_AXI \
    [get_bd_addr_segs ps7/S_AXI_HP0/HP0_DDR_LOWOCM] \
    -range 256M -offset 0x00000000

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

# Set the block design wrapper as top module (not atomik_zynq_top)
set_property top atomik_system_wrapper [current_fileset]
update_compile_order -fileset sources_1

# Add constraints (CLG484 board-specific)
add_files -fileset constrs_1 $ZYNQ_DIR/constraints/rk7020f.xdc

# Run synthesis + implementation + bitstream
launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

# Check for build errors
if {[get_property STATUS [get_runs impl_1]] ne "write_bitstream Complete!"} {
    puts "ERROR: Implementation failed: [get_property STATUS [get_runs impl_1]]"
    exit 1
}

# Copy bitstream to output directory
set bit_file [glob $ZYNQ_DIR/vivado/atomik_zynq/atomik_zynq.runs/impl_1/*.bit]
file copy -force $bit_file $ZYNQ_DIR/output/atomik_zynq.bit
puts "Bitstream: output/atomik_zynq.bit"

# Export hardware definition (for PetaLinux / Vitis)
write_hw_platform -fixed -include_bit -force $ZYNQ_DIR/output/atomik_zynq.xsa

puts ""
puts "=============================================="
puts " Block design build complete."
puts "  Bitstream: output/atomik_zynq.bit"
puts "  XSA file:  output/atomik_zynq.xsa"
puts "  Phase A: ATOMiK N=16 @ 0x43C00000"
puts "  Phase B: Mailbox    @ 0x43C10000"
puts "           RV64I DDR3 via S_AXI_HP0"
puts "=============================================="
