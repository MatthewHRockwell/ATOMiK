set SCRIPT_DIR [file dirname [info script]]
set ALINX_HDMI "$SCRIPT_DIR/../alinx_reference/course_s1_fpga/13_hdmi_out/auto_create_project"

create_project -in_memory -part xc7z020clg484-2

# Add Digilent rgb2dvi IP repo
set_property ip_repo_paths [list "$ALINX_HDMI/my_ip"] [current_project]
update_ip_catalog

# Create PS7 block design
create_bd_design "ps_sys"
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7
set_property -dict [list CONFIG.PCW_FPGA_FCLK0_ENABLE {1} CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100} CONFIG.PCW_USE_M_AXI_GP0 {0} CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125}] [get_bd_cells ps7]
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]
create_bd_port -dir O fclk
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_ports fclk]
validate_bd_design
generate_target all [get_files ps_sys.bd]
make_wrapper -files [get_files ps_sys.bd] -top
add_files [glob .gen/sources_1/bd/ps_sys/hdl/*_wrapper.v]

# Add HDL sources
add_files $SCRIPT_DIR/hdmi_test_top.v
add_files $ALINX_HDMI/src/design/color_bar.v
add_files $ALINX_HDMI/src/design/video_define.v

# Add rgb2dvi VHDL sources directly (avoid IP catalog issues)
add_files [glob $ALINX_HDMI/my_ip/rgb2dvi_v1_3/src/*.vhd]

# Constraints
add_files -fileset constrs_1 $SCRIPT_DIR/hdmi_test_bank33.xdc

set_property top hdmi_test_top [current_fileset]
set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]

synth_design -top hdmi_test_top -verilog_define HDMI_EXACT_74250
opt_design
place_design
route_design
write_bitstream -force $SCRIPT_DIR/hdmi_test_bank33.bit
puts "BUILT: hdmi_test_bank33.bit"

# Generate .bin for Linux fpga_manager (PCAP byte-swapped)
write_cfgmem -format bin -interface smapx32 -disablebitswap -loadbit "up 0 $SCRIPT_DIR/hdmi_test_bank33.bit" -file $SCRIPT_DIR/hdmi_test_bank33.bin -force
puts "BUILT: hdmi_test_bank33.bin (for fpga_manager)"
