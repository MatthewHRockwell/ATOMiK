## Minimal LED Blink Synthesis Script
open_project led_blink_minimal.gprj
set_option -top_module led_blink_minimal
set_option -use_mspi_as_gpio 1
set_option -use_done_as_gpio 1
set_option -output_base_name led_blink_minimal
run syn
run pnr
