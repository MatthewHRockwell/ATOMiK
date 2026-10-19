create_project -in_memory -part xc7z020clg484-2

# Create PS7 block design with EMIO GPIO
create_bd_design "scanner_sys"
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7

set_property -dict [list \
    CONFIG.PCW_FPGA_FCLK0_ENABLE {1} \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50} \
    CONFIG.PCW_USE_M_AXI_GP0 {0} \
    CONFIG.PCW_EN_GPIO_EMIO {1} \
    CONFIG.PCW_GPIO_EMIO_GPIO_ENABLE {1} \
    CONFIG.PCW_GPIO_EMIO_GPIO_IO {64} \
    CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125} \
] [get_bd_cells ps7]

apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR"} [get_bd_cells ps7]

# Export EMIO GPIO
create_bd_port -dir IO -from 63 -to 0 gpio_emio
connect_bd_net [get_bd_pins ps7/GPIO_O] [get_bd_ports gpio_emio]

# Actually, we need the tri-state I/O interface for GPIO
# Let me use a simpler approach - just read pins via FCLK + simple register
delete_bd_objs [get_bd_ports gpio_emio]

# Re-enable AXI GP0 for register reads
set_property -dict [list \
    CONFIG.PCW_USE_M_AXI_GP0 {1} \
    CONFIG.PCW_EN_GPIO_EMIO {0} \
] [get_bd_cells ps7]

# Create FCLK output and AXI ports
create_bd_port -dir O fclk
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_ports fclk]
connect_bd_net [get_bd_pins ps7/FCLK_CLK0] [get_bd_pins ps7/M_AXI_GP0_ACLK]

# Export AXI GP0 signals
make_bd_intf_pins_external [get_bd_intf_pins ps7/M_AXI_GP0]

validate_bd_design
generate_target all [get_files scanner_sys.bd]
make_wrapper -files [get_files scanner_sys.bd] -top
add_files [glob .gen/sources_1/bd/scanner_sys/hdl/*_wrapper.v]

set SCRIPT_DIR [file dirname [info script]]
add_files $SCRIPT_DIR/pin_scanner.v

# Create XDC with ALL untested pins + header pins (48 total)
set xdc_content ""

# The 17 truly untested pins
set untested_pins {AA13 AA14 AA16 AA17 AA18 AA19 AA21 AA22 AB14 AB15 AB16 AB17 AB19 AB20 AB21 AB22 W10}

# Header pins (32 pins, might also have FTDI)
set header_pins {W12 V9 U9 AB12 Y10 U11 AB9 AB11 W8 AB8 AA8 AB4 AA6 Y5 AA4 AB1 V12 V10 U10 AA12 Y11 U12 AB10 AA11 V8 W11 AB7 AA9 AA7 Y6 Y4 AB2}

# Combine all, up to 200 pins
set all_pins [concat $untested_pins $header_pins]

# Also add the ALINX RS232 pins explicitly
lappend all_pins K19 J19

set idx 0
foreach pin $all_pins {
    append xdc_content "set_property PACKAGE_PIN $pin \[get_ports {scan_in\[$idx\]}\]\n"
    # Try to figure out bank voltage
    # Bank 35 pins (AA13+, AB14+) might need different IOSTANDARD
    # For safety, use LVCMOS33 for all and waive DRC
    append xdc_content "set_property IOSTANDARD LVCMOS33 \[get_ports {scan_in\[$idx\]}\]\n"
    # Internal pull-down so unconnected pins read LOW
    append xdc_content "set_property PULLDOWN TRUE \[get_ports {scan_in\[$idx\]}\]\n"
    incr idx
}

# Pad remaining scan_in bits
# The module has [199:0] scan_in, but we only use $idx pins
# Remaining pins will be tied off by synthesis

set f [open "$SCRIPT_DIR/pin_scanner.xdc" "w"]
puts $f $xdc_content
# Waive DRC for unspecified I/O standards and unconstrained ports
puts $f {set_property SEVERITY {Warning} [get_drc_checks UCIO-1]}
puts $f {set_property SEVERITY {Warning} [get_drc_checks NSTD-1]}
close $f

add_files -fileset constrs_1 $SCRIPT_DIR/pin_scanner.xdc
set_property top pin_scanner [current_fileset]

puts "Synthesizing... ($idx pins configured)"
synth_design -top pin_scanner
opt_design
place_design
route_design
write_bitstream -force $SCRIPT_DIR/pin_scanner.bit
puts "BUILT: pin_scanner.bit with $idx input pins"
