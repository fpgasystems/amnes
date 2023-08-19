#!/bin/bash

echo "Amnes Throughput Analysis."
# Usage: ./amnes <num_items> <num_streams> <databits> <num_threads> <repetitions>
	
declare -a allThreads

allThreads=(64 32 24 16 8 4 2 1);
# allThreads=(1024 512 256 128 120 96 88 80 72 64 56 48 40 32 24 16 12 10 8 6 4 2 1);;

for th in ${allThreads[@]}; do
	items=1024
	while [ $items -le 10000000 ]; do

		./ss_eigenlib_th_32 $items 16 32 $th 100
		./ss_eigenlib_th_16 $items 32 16 $th 100
		./ss_eigenlib_th_8  $items 64 8  $th 100
		((items *= 2)) 
	done
done
