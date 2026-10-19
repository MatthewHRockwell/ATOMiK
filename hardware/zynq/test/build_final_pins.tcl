set SCRIPT_DIR [file dirname [info script]]

# PL LED2 candidates: U7, V13
# PL LED1 candidates: V10, V15
foreach {name pin} {pin_U7 U7 pin_V13 V13 pin_V10 V10 pin_V15 V15} {
    create_project -in_memory -part xc7z020clg484-2
    set fp [open "$SCRIPT_DIR/${name}_high.v" w]
    puts $fp "module ${name}_high(output wire p);"
    puts $fp "    assign p = 1'b1;"
    puts $fp "endmodule"
    close $fp
    set xfp [open "$SCRIPT_DIR/${name}_high.xdc" w]
    puts $xfp "set_property PACKAGE_PIN $pin \[get_ports p\]"
    puts $xfp "set_property IOSTANDARD LVCMOS33 \[get_ports p\]"
    puts $xfp {set_property SEVERITY {Warning} [get_drc_checks UCIO-1]}
    puts $xfp {set_property SEVERITY {Warning} [get_drc_checks NSTD-1]}
    close $xfp
    add_files "$SCRIPT_DIR/${name}_high.v"
    add_files -fileset constrs_1 "$SCRIPT_DIR/${name}_high.xdc"
    set_property top "${name}_high" [current_fileset]
    synth_design -top "${name}_high"
    opt_design; place_design; route_design
    write_bitstream -force "$SCRIPT_DIR/${name}_high.bit"
    puts "BUILT: ${name}_high.bit (single pin $pin)"
    close_project
}
