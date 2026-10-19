# Bisect group0 (PL LED2) and group2 (PL LED1) — drive halves HIGH
set SCRIPT_DIR [file dirname [info script]]

# Group 0 pins (42 total) — contains PL LED2
set g0_pins {A16 A21 B17 B22 C19 D16 D21 E18 F16 F21 G17 G22 H19 J16 J21 K18 L16 L21 M17 M22 N19 P16 P21 R18 R6 T18 T4 U12 U17 U22 U7 V13 V18 V4 V9 W13 W18 W5 Y10 Y15 Y20 Y6}

# Group 2 pins (42 total) — contains PL LED1
set g2_pins {A18 B15 B20 C17 C22 D18 E15 E20 F18 G15 G20 H17 H22 J18 K15 K20 L18 M15 M20 N17 N22 P18 R15 R20 T16 T21 U10 U15 U20 U5 V10 V15 V20 V7 W11 W16 W21 W7 Y13 Y18 Y4 Y9}

foreach {name pins} [list g0a [lrange $g0_pins 0 20] g0b [lrange $g0_pins 21 41] g2a [lrange $g2_pins 0 20] g2b [lrange $g2_pins 21 41]] {
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
    opt_design
    place_design
    route_design
    write_bitstream -force "$SCRIPT_DIR/${name}_high.bit"
    puts "BUILT: ${name}_high.bit ($n pins: [lindex $pins 0]..[lindex $pins end])"
    close_project
}
