## ATOMiK v3 SoC Synthesis Script for Gowin gw_sh
## Synthesize, Place & Route, and generate bitstream
##
## Usage: gw_sh synth_v3_soc.tcl
## (Run from the synth/ directory)
##
## Target: GW1NR-LV9QN88PC6/I5 (Tang Nano 9K)
## Expected: ~5,000 LUT, 14 BSRAM, Fmax >= 25 MHz, zero TNS

# Open the project
open_project atomik_v3_soc.gprj

# Set top module explicitly
set_option -top_module atomik_v3_soc

# Enable MSPI as regular IO (required for SPI flash pins)
set_option -use_mspi_as_gpio 1

# Enable DONE pin as regular IO (required for Tang Nano 9K to complete configuration)
set_option -use_done_as_gpio 1

# Set output base name
set_option -output_base_name atomik_v3_soc

# Enable plain text timing report for resource extraction
set_option -gen_text_timing_rpt 1

# Run synthesis
run syn

# Run place and route
run pnr

# Close project (note: may produce harmless error in gw_sh)
close_project
