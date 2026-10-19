# Build 4 group bitstreams that drive pins HIGH (for active-high LED discovery)
set SCRIPT_DIR [file dirname [info script]]

foreach grp {0 1 2 3} {
    set xdc_file "$SCRIPT_DIR/group${grp}.xdc"
    set pin_count [llength [regexp -all -inline {PACKAGE_PIN} [read [open $xdc_file]]]]

    create_project -in_memory -part xc7z020clg484-2
    set fp [open "$SCRIPT_DIR/group${grp}_high.v" w]
    puts $fp "module group${grp}_high(output wire \[[expr {$pin_count-1}]:0\] p);"
    puts $fp "    assign p = {${pin_count}{1'b1}};"
    puts $fp "endmodule"
    close $fp

    add_files "$SCRIPT_DIR/group${grp}_high.v"

    # Rewrite XDC pin names from 'p' to 'p' (same port name, compatible)
    add_files -fileset constrs_1 $xdc_file

    set_property top "group${grp}_high" [current_fileset]
    set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
    set_property SEVERITY {Warning} [get_drc_checks NSTD-1]

    synth_design -top "group${grp}_high"
    opt_design
    place_design
    route_design
    write_bitstream -force "$SCRIPT_DIR/group${grp}_high.bit"
    puts "BUILT: group${grp}_high.bit ($pin_count pins driven HIGH)"
    close_project
}
