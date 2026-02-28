add_file clkdiv_test.v
add_file ../soc/gowin_ip/gowin_clkdiv/gowin_clkdiv.v
set_device GW1NR-LV9QN88PC6/I5 -name GW1NR-9C
set_option -top_module clkdiv_test
set_option -verilog_std v2001
set_option -use_mspi_as_gpio 1
set_option -use_done_as_gpio 1
add_file clkdiv_test.cst
run all
