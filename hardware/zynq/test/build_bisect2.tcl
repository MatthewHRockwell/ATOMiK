set SCRIPT_DIR [file dirname [info script]]

# g0b split (PL LED2 is here)
set g0b_lo {P16 P21 R18 R6 T18 T4 U12 U17 U22 U7 V13}
set g0b_hi {V18 V4 V9 W13 W18 W5 Y10 Y15 Y20 Y6}

# g2b split (PL LED1 is here)
set g2b_lo {P18 R15 R20 T16 T21 U10 U15 U20 U5 V10 V15}
set g2b_hi {V20 V7 W11 W16 W21 W7 Y13 Y18 Y4 Y9}

foreach {name pins} [list g0b_lo $g0b_lo g0b_hi $g0b_hi g2b_lo $g2b_lo g2b_hi $g2b_hi] {
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
    puts "BUILT: ${name}_high.bit ($n pins: [lindex $pins 0]..[lindex $pins end])"
    close_project
}
