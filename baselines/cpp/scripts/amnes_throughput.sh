#!/bin/bash

echo "Amnes Throughput Analysis."
# Usage: ./amnes <num_items> <num_streams> <databits> <num_threads> <repetitions>

while getopts d:m:t: flag
do 
	case "${flag}" in
	d) debug=${OPTARG};;
        m) machine=${OPTARG};;
	t) testop=${OPTARG};;
	esac
done

if [ $debug -eq 1 ]; then

	echo "DEBUG"
	for th in 1; do
		items=10
		while [ $items -le 100000000 ]; do
			if [ $machine = "desktop" ]; then
				./amnes8  $items 64  8 $th 1
				./amnes16 $items 32 16 $th 1 
				./amnes32 $items 16 32 $th 1
			fi

			if [ $machine = "alveo0" ]; then
				numactl --cpunodebind=0 --membind=0 ./amnes8  $items 64  8 $th 1
				numactl --cpunodebind=0 --membind=0 ./amnes16 $items 32 16 $th 1 
				numactl --cpunodebind=0 --membind=0 ./amnes32 $items 16 32 $th 1
			fi

			((items *= 10)) 
		done
	done
else

	echo "NO DEBUG"
	echo "Machine: "$machine
	
	declare -a allThreads

	case $testop in
		1) allThreads=(24 16 12 11 10 9 8 7 6 4 2 1)
			echo "24 threads";;
		2) allThreads=(48 40 32 24 16 12 10 8 6 4 2 1);;
		3) allThreads=(96 88 80 72 70 64 56 48 40 32 24 16 12 10 8 6 4 2 1);;
		4) allThreads=(1024 512 256 128 120 96 88 80 72 64 56 48 40 32 24 16 12 10 8 6 4 2 1);;
		5) allThreads=(1024 512);;
		*) allThreads=(8 4 2 1);;
        esac

	for th in ${allThreads[@]}; do
		items=1048576
		while [ $items -le 10000000 ]; do
			if [ $machine = "desktop" ]; then
				./amnes8  $items 64  8 $th 100
				./amnes16 $items 32 16 $th 100
				./amnes32 $items 16 32 $th 100
			fi

			if [ $machine = "alveo0" ] || [ $machine = "r740" ]; then

				./amnes8  $items 64  8 $th 100
				./amnes16 $items 32 16 $th 100
				./amnes32 $items 16 32 $th 100
			fi

			if [ $machine = "alveo0-membind" ] || [ $machine = "r740-membind" ]; then
				echo "Numactl Membind"
                                numactl --membind=1 ./amnes8  $items 64  8 $th 100
                                numactl --membind=1 ./amnes16 $items 32 16 $th 100
                                numactl --membind=1 ./amnes32 $items 16 32 $th 100
                        fi
			
			if [ $machine = "alveo0-perf" ] || [ $machine = "r740-perf" ]; then
				echo "Perf Stat"
				repeat=1
				while [ $repeat -le 100 ]; do
					perf stat -o perfout8_${th}_${items}_${repeat}.txt  -e cpu-cycles -e instructions -e cache-references -e cache-misses ./amnes8  $items 64  8 $th $repeat
					perf stat -o perfout16_${th}_${items}_${repeat}.txt -e cpu-cycles -e instructions -e cache-references -e cache-misses ./amnes16 $items 32 16 $th $repeat
					perf stat -o perfout32_${th}_${items}_${repeat}.txt -e cpu-cycles -e instructions -e cache-references -e cache-misses ./amnes32 $items 16 32 $th $repeat
                        		((repeat *=10))
				done
			fi


			((items *= 2)) 
		done
	done
fi
