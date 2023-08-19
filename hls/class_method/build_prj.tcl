# vitis_hls -f ../build_prj.tcl
open_project amnes
set_top design_user_hls_c0_0

add_files "../coefficients.hpp ../correlation.hpp ../correlation_parameters.hpp"
add_files ../correlation.cpp -cflags "-std=c++14"

add_files -tb ../correlation_tb.cpp -cflags "-std=c++14 -Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"

open_solution "solution1" -flow_target vivado

set_part {xcu55c-fsvh2892-2L-e}

create_clock -period 4 -name default
csim_design
csynth_design

export_design -format ip_catalog

exit
