set SCRIPT_DIR [file dirname [info script]]

# PL LED2: {U12 U17 U22 U7 V13} — split 3/2
set led2_x {U12 U17 U22}
set led2_y {U7 V13}

# PL LED1: {U15 U20 U5 V10 V15} — split 3/2
set led1_x {U15 U20 U5}
set led1_y {V10 V15}

foreach {name pins} [list led2_x $led2_x led2_y $led2_y led1_x $led1_x led1_y $led1_y] {
    set n [llength $pins]
    create_project -in_memory -part xc7z020clg484-2
    set fp [open "$SCRIPT_DIR/${name}_high.v" w]
    puts $fp "module ${name}_high(output wire \[[expr {$n-1}]:0\] p);"
    puts $fp "    assign p = {${n}{1'b1}};"
    puts $fp "endmodule"
    close $fp
    set xfp [open "$SCRIPT_DIR/${name}_high.xdc" w]
    set idx 0
    foreach pin $pins {
        puts $xfp "set_property PACKAGE_PIN $pin \[get_ports {p\[$idx\]}\]"
        puts $xfp "set_property IOSTANDARD LVCMOS33 \[get_ports {p\[$idx\]}\]"
        incr idx
    }
    puts $xfp {set_property SEVERITY {Warning} [get_drc_checks UCIO-1]}
    puts $xfp {set_property SEVERITY {Warning} [get_drc_checks NSTD-1]}
    close $xfp
    add_files "$SCRIPT_DIR/${name}_high.v"
    add_files -fileset constrs_1 "$SCRIPT_DIR/${name}_high.xdc"
    set_property top "${name}_high" [current_fileset]
    synth_design -top "${name}_high"
    opt_design; place_design; route_design
    write_bitstream -force "$SCRIPT_DIR/${name}_high.bit"
    puts "BUILT: ${name}_high.bit ($pins)"
    close_project
}
