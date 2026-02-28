## PLL Lock Test Synthesis
open_project pll_lock_test.gprj
set_option -top_module pll_lock_test
set_option -use_mspi_as_gpio 1
set_option -use_done_as_gpio 1
set_option -output_base_name pll_lock_test
run syn
run pnr
