add_file pin_test.v
set_device GW1NR-LV9QN88PC6/I5 -name GW1NR-9C
set_option -top_module pin_test
set_option -use_mspi_as_gpio 1
set_option -use_done_as_gpio 1
add_file pin_test.cst
run all
