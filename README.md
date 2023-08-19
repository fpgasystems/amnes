# Correlation computation [AMNES]

Design space exploration of correlation computation implementation on FPGA. 

Project branch structure
* main                - it contains stable project versions
* dev_"shortcut_name" - it contains developers branch; it does not have to be stable; only its stable versions merge with main branch  

## Pre-requisites
Vitis 2022.1
Vivado 2022.1

## Repository structure
~~~
├── baselines
│   └── cpp       (sufficient statistics are gathered by the CPU from data stored in the memory)
│   └── outer_lib (extracting the sufficient statistics from the Eigen matrix-matrix multiplication and computing the PCC)
│   └── python    (compute PCC using python's pandas or numpy build in functions) 
├── coyote_integration
│   └── hdl       (RTL *.sv to integrate AMNES as a co-processor kernel into coyote)
│   └── hdl_rdma  (RTL *.sv to integrate AMNES as a network attached kernel into coyote)
│   └── host      (code running )
│   └── host_rdma
│   └── testbench
├── hls
│   └── class_method (high level synthesis code for Pearson Correlation Coefficient)
├── bitstreams (progammable FPGA files)
    └── co-processor (PCIe data source)
    └── rdma         (RDMA network stack data source)
~~~
