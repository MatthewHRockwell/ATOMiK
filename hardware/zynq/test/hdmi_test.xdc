# HDMI TMDS — CLG484 (mapped from ALINX BANK34 logical pins via Xilinx pkg files)
# CLG400→CLG484: N18→M19, V20→N22, T20→M21, N20→N19, V16→P20
set_property PACKAGE_PIN M19 [get_ports TMDS_clk_p]
set_property IOSTANDARD TMDS_33 [get_ports TMDS_clk_p]
set_property IOSTANDARD TMDS_33 [get_ports TMDS_clk_n]

set_property PACKAGE_PIN N22 [get_ports {TMDS_data_p[0]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[0]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[0]}]

set_property PACKAGE_PIN M21 [get_ports {TMDS_data_p[1]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[1]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[1]}]

set_property PACKAGE_PIN N19 [get_ports {TMDS_data_p[2]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[2]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[2]}]

# HDMI output enable
set_property PACKAGE_PIN P20 [get_ports hdmi_oen]
set_property IOSTANDARD LVCMOS33 [get_ports hdmi_oen]

# PL LEDs (confirmed)
set_property PACKAGE_PIN V15 [get_ports pl_led1]
set_property IOSTANDARD LVCMOS33 [get_ports pl_led1]
set_property PACKAGE_PIN V13 [get_ports pl_led2]
set_property IOSTANDARD LVCMOS33 [get_ports pl_led2]

set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]
