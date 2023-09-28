# Correlation computation [AMNES]

Design space exploration of correlation computation implementation on FPGA. 

## Coyote -
Download [https://github.com/fpgasystems/Coyote/tree/e34ce174415732ea43267de51369d28978624c43](Coyote-repository)
```
$ cd hw 
$ mkdir build_rdma && cd build_rdma

$ cmake .. -DFDEV_NAME=u55c -DEXAMPLE=perf_rdma_host
$ make shell
$ make compile
```
After compile is done:
(1) Open vivado
(2) For correlation on top of RDMA, insert into the project
* hld_rdma/design_user_logic_c0_0_rdma.sv
* hls_rdma/user_wrapper_c0_0.sv
* amnes rtl files (obtained after compilation) from ../hls/class_method/build/amnes/solution1/impl/vhdl
* amnes ips from /home/chiosam/projects/correlation/hls/class_method/build/amnes/solution1/impl/ip/tmp.srcs/sources_1/ip
(3) Compile again the project

(4) When compilation is finished, program the FPGA (you will need an acount for the HACC machines at ETH Zurich)
(5) Book two paired machines (example alveo-u55c-07 and alveo-u55c-08)
(6) For each host machine (alveo-u55c-07 and alveo-u55C-08) compile driver and:
* cp coyote_drv.ko /tmp
* sudo insmod /tmp/coyote_drv ip_addr_q0=$FPGA_0_IP_ADDRESS_HEX mac_addr_q0=$FPGA_0_MAC_ADDRESS
(7) For each host machine (alveo-u55c-07 and alveo-u55C-08) rescan PCIe tree and get access to the FPGA
* sudo /opt/cli/program/pci_hot_plug alveo-u55c-X
* sudo /opt/cli/program/fpga_chmod 0
(8) For each host machine (alveo-u55c-07 and alveo-u55C-08) creat a correlation folder into coyote/sw/example and copy the files from coyote_integration/host_rdma
(9) Build host code: /usr/bin/cmake ../ -DTARGET_DIR=examples/correlation
(10) For alveo-u55c-07: 
* for((var=4096; var<2000000; var*=2)); do for((var2=1; var2<17; var2*=2)); do ./main -s 16 -m $var -p 32768 -w 1 -h $var2 -r 10; done sleep 1; done

(11) For alveo-u55-08:
for((var=4096; var<2000000; var*=2)); do for((var2=1; var2<17; var2*=2)); do ./main -s 16 -m $var -p 32768 -w 1 -h $var2 -r 10 -t 10.1.212.177; done sleep1; done



New simplified flow for Coyote to be released by the end of October
