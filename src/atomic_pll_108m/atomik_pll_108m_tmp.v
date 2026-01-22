//Copyright (C)2014-2025 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: Template file for instantiation
//Tool Version: V1.9.11.03 Education
//Part Number: GW1NR-LV9QN88PC6/I5
//Device: GW1NR-9
//Device Version: C
//Created Time: Wed Jan 21 23:33:33 2026

//Change the instance name and port connections to the signal names
//--------Copy here to design--------

    atomik_pll_108m your_instance_name(
        .clkout(clkout), //output clkout
        .lock(lock), //output lock
        .reset(reset), //input reset
        .clkin(clkin), //input clkin
        .fbdsel(fbdsel) //input [5:0] fbdsel
    );

//--------Copy end-------------------
