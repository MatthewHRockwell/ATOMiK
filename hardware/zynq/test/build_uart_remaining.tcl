create_project -in_memory -part xc7z020clg484-2
set SCRIPT_DIR [file dirname [info script]]

create_bd_design "ps_sys"
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7
set_property -dict [list CONFIG.PCW_FPGA_FCLK0_ENABLE {1} CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50} CONFIG.PCW_USE_M_AXI_GP0 {0} CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125}] [get_bd_cells ps7]
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]
create_bd_port -dir O fclk
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_ports fclk]
validate_bd_design
generate_target all [get_files ps_sys.bd]
make_wrapper -files [get_files ps_sys.bd] -top
add_files [glob .gen/sources_1/bd/ps_sys/hdl/*_wrapper.v]

add_files $SCRIPT_DIR/uart_remaining.v

# XDC: 32 header pins + 17 untested Bank35 pins = 49 total
set fp [open "$SCRIPT_DIR/uart_remaining.xdc" w]

# 32 header pins (Bank 34, LVCMOS33)
set header {W12 V9 U9 AB12 Y10 U11 AB9 AB11 W8 AB8 AA8 AB4 AA6 Y5 AA4 AB1 V12 V10 U10 AA12 Y11 U12 AB10 AA11 V8 W11 AB7 AA9 AA7 Y6 Y4 AB2}

# 17 untested pins (Bank 35, try LVCMOS33)
set untested {AA13 AA14 AA16 AA17 AA18 AA19 AA21 AA22 AB14 AB15 AB16 AB17 AB19 AB20 AB21 AB22 W10}

set idx 0
foreach pin [concat $header $untested] {
    puts $fp "set_property PACKAGE_PIN $pin \[get_ports {txpins\[$idx\]}\]"
    puts $fp "set_property IOSTANDARD LVCMOS33 \[get_ports {txpins\[$idx\]}\]"
    incr idx
}

puts $fp {set_property SEVERITY {Warning} [get_drc_checks UCIO-1]}
puts $fp {set_property SEVERITY {Warning} [get_drc_checks NSTD-1]}
close $fp

add_files -fileset constrs_1 $SCRIPT_DIR/uart_remaining.xdc
set_property top uart_remaining [current_fileset]
synth_design -top uart_remaining
opt_design
place_design
route_design
write_bitstream -force $SCRIPT_DIR/uart_remaining.bit
puts "BUILT: uart_remaining.bit ($idx pins)"
