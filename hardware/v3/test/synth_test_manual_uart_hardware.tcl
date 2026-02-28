set_device GW1NR-LV9QN88PC6/I5 -name GW1NR-9C
add_file test_manual_uart_hardware.v
add_file ../soc/manual_uart_tx.v
add_file ../soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.v
add_file test_manual_uart_hardware.cst
set_option -top_module test_manual_uart_hardware
set_option -verilog_std v2001
run syn
run pnr
