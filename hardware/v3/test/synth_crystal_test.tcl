## Crystal Test Synthesis
open_project crystal_test.gprj
set_option -top_module crystal_test
set_option -use_mspi_as_gpio 1
set_option -use_done_as_gpio 1
set_option -output_base_name crystal_test
run syn
run pnr
