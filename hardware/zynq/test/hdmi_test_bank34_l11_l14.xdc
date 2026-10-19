# HDMI TMDS — CLG484 Bank 34 candidate window: L11-L14
set_property PACKAGE_PIN K19 [get_ports TMDS_clk_p]
set_property IOSTANDARD TMDS_33 [get_ports TMDS_clk_p]
set_property IOSTANDARD TMDS_33 [get_ports TMDS_clk_n]

set_property PACKAGE_PIN L18 [get_ports {TMDS_data_p[0]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[0]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[0]}]

set_property PACKAGE_PIN M19 [get_ports {TMDS_data_p[1]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[1]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[1]}]

set_property PACKAGE_PIN N19 [get_ports {TMDS_data_p[2]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[2]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[2]}]

# Keep the existing enable guess while the TMDS quartet is being searched.
set_property PACKAGE_PIN P20 [get_ports hdmi_oen]
set_property IOSTANDARD LVCMOS33 [get_ports hdmi_oen]

set_property PACKAGE_PIN V15 [get_ports pl_led1]
set_property IOSTANDARD LVCMOS33 [get_ports pl_led1]
set_property PACKAGE_PIN V13 [get_ports pl_led2]
set_property IOSTANDARD LVCMOS33 [get_ports pl_led2]

set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]
