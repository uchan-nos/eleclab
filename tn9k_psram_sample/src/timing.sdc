//Copyright (C)2014-2025 GOWIN Semiconductor Corporation.
//All rights reserved.
//File Title: Timing Constraints file
//Tool Version: V1.9.10.01 (64-bit) 
//Created Time: 2025-08-28 15:06:16
create_clock -name ext_clk -period 37.037 -waveform {0 18.518} [get_ports {ext_clk}]
//create_clock -name sys_clk -period 296.296 -waveform {0 148.148} [get_nets {sys_clk}]
create_clock -name mem_clk -period 18.519 -waveform {0 9.259} [get_nets {mem_clk}]
