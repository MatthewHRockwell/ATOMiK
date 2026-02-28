set_device GW1NR-LV9QN88PC6/I5 -name GW1NR-9C
add_file led_blink.v
add_file led_blink.cst
set_option -top_module led_blink
set_option -verilog_std v2001
set_option -print_all_synthesis_warning 1
run syn
run pnr
