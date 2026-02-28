set_device GW1NR-LV9QN88PC6/I5 -name GW1NR-9C
add_file uart_direct_test.v
add_file ../soc/simpleuart.v
add_file ../soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.v
add_file uart_direct_test.cst
set_option -top_module uart_direct_test
set_option -verilog_std v2001
run syn
run pnr
