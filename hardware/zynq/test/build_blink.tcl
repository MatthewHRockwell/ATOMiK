set SCRIPT_DIR [file dirname [info script]]

create_project -in_memory -part xc7z020clg400-2
add_files $SCRIPT_DIR/blink_test.v
add_files -fileset constrs_1 $SCRIPT_DIR/blink_test.xdc

synth_design -top blink_test
opt_design
place_design
route_design

write_bitstream -force $SCRIPT_DIR/blink_test.bit
puts "Blink test bitstream generated."
