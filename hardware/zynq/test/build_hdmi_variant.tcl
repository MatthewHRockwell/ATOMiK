set SCRIPT_DIR [file dirname [info script]]
set ALINX_HDMI "$SCRIPT_DIR/../alinx_reference/course_s1_fpga/13_hdmi_out/auto_create_project"

if {$argc < 2 || $argc > 4} {
    puts "Usage: vivado -mode batch -source build_hdmi_variant.tcl -tclargs <variant_name> <xdc_file> ?oen_high|oen_low? ?clock_75|clock_74250?"
    exit 1
}

set variant_name [lindex $argv 0]
set xdc_file [lindex $argv 1]
set oen_mode [expr {$argc >= 3 ? [lindex $argv 2] : "oen_high"}]
set clock_mode [expr {$argc >= 4 ? [lindex $argv 3] : "clock_75"}]

if {![file exists $xdc_file]} {
    puts "Missing constraints file: $xdc_file"
    exit 1
}

create_project -in_memory -part xc7z020clg484-2

set_property ip_repo_paths [list "$ALINX_HDMI/my_ip"] [current_project]
update_ip_catalog

create_bd_design "ps_sys"
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7
set_property -dict [list \
    CONFIG.PCW_FPGA_FCLK0_ENABLE {1} \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100} \
    CONFIG.PCW_USE_M_AXI_GP0 {0} \
    CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125} \
] [get_bd_cells ps7]
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]
create_bd_port -dir O fclk
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_ports fclk]
validate_bd_design
generate_target all [get_files ps_sys.bd]
make_wrapper -files [get_files ps_sys.bd] -top
add_files [glob .gen/sources_1/bd/ps_sys/hdl/*_wrapper.v]

add_files $SCRIPT_DIR/hdmi_test_top.v
add_files $ALINX_HDMI/src/design/color_bar.v
add_files $ALINX_HDMI/src/design/video_define.v
add_files [glob $ALINX_HDMI/my_ip/rgb2dvi_v1_3/src/*.vhd]
add_files -fileset constrs_1 $xdc_file

set_property top hdmi_test_top [current_fileset]
set defines {}
if {$oen_mode eq "oen_low"} {
    lappend defines HDMI_OUT_EN_LOW=1
} elseif {$oen_mode ne "oen_high"} {
    puts "Unknown oen_mode: $oen_mode"
    exit 1
}

if {$clock_mode eq "clock_74250"} {
    lappend defines HDMI_EXACT_74250=1
} elseif {$clock_mode ne "clock_75"} {
    puts "Unknown clock_mode: $clock_mode"
    exit 1
}

if {[llength $defines] > 0} {
    set_property verilog_define $defines [current_fileset]
}

set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]

synth_design -top hdmi_test_top
opt_design
place_design
route_design

set bit_file "$SCRIPT_DIR/${variant_name}.bit"
set bin_file "$SCRIPT_DIR/${variant_name}.bit.bin"
write_bitstream -force $bit_file
write_cfgmem -format bin -interface smapx32 -disablebitswap -loadbit "up 0 $bit_file" -file $bin_file -force

puts "BUILT: $bit_file"
puts "BUILT: $bin_file"
