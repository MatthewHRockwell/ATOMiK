set_device GW1NR-LV9QN88PC6/I5 -name GW1NR-9C
add_file uart_handshake_test.v
add_file ../soc/simpleuart.v
add_file uart_handshake_test_gpio.cst
set_option -top_module uart_handshake_test
set_option -verilog_std v2001
run syn
run pnr
