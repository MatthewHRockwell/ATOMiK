# HDMI TMDS — CLG484 Bank 33 (HDMI_HPD=Y18 confirmed on Bank 33)
set_property PACKAGE_PIN W17 [get_ports TMDS_clk_p]
set_property IOSTANDARD TMDS_33 [get_ports TMDS_clk_p]
set_property IOSTANDARD TMDS_33 [get_ports TMDS_clk_n]

set_property PACKAGE_PIN W16 [get_ports {TMDS_data_p[0]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[0]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[0]}]

set_property PACKAGE_PIN U15 [get_ports {TMDS_data_p[1]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[1]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[1]}]

set_property PACKAGE_PIN U17 [get_ports {TMDS_data_p[2]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_p[2]}]
set_property IOSTANDARD TMDS_33 [get_ports {TMDS_data_n[2]}]

# HDMI output enable — try V14 (L19P, single-ended near HDMI area)
set_property PACKAGE_PIN V14 [get_ports hdmi_oen]
set_property IOSTANDARD LVCMOS33 [get_ports hdmi_oen]

# PL LEDs (confirmed on Bank 33!)
# V15 = IO_L19N_T3_VREF_33, V13 = IO_L20P_T3_33
set_property PACKAGE_PIN V15 [get_ports pl_led1]
set_property IOSTANDARD LVCMOS33 [get_ports pl_led1]
set_property PACKAGE_PIN V13 [get_ports pl_led2]
set_property IOSTANDARD LVCMOS33 [get_ports pl_led2]

set_property SEVERITY {Warning} [get_drc_checks UCIO-1]
set_property SEVERITY {Warning} [get_drc_checks NSTD-1]
