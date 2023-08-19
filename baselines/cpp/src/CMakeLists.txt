# Set up project and cmake
cmake_minimum_required (VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/")

project (CorrelationBenchmark)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

# Set up compilation options
set (CMAKE_CXX_STANDARD 14)

if (MACHINE_NAME STREQUAL "desktop")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -mavx -march=native -O3") 
elseif(MACHINE_NAME STREQUAL "alveo0" OR MACHINE_NAME STREQUAL "alveo0-membind" OR MACHINE_NAME STREQUAL "alveo0-perf")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -mavx512f -mavx512vl -mavx512bw -mavx512dq -march=native -O3 -fopt-info")
elseif(MACHINE_NAME STREQUAL "r740" OR MACHINE_NAME STREQUAL "r740-membind" OR MACHINE_NAME STREQUAL "r740-perf")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -mavx512f -mavx512vl -mavx512bw -mavx512dq -march=cascadelake -O3")
else ()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -mavx -march=native -O3")
    set (MACHINE_NAME "desktop")
endif()

set (CMAKE_C_STANDARD 11)

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -pthread -mavx")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pthread -mavx")

find_package(Boost COMPONENTS program_options REQUIRED)

# Set up target for main executable
add_executable(pcc_sums
    pcc_sums.cpp
    )
target_link_libraries(pcc_sums
    ${Boost_LIBRARIES}
    )

add_executable(pcc_dev
    pcc_dev.cpp
    )
target_link_libraries(pcc_dev
    ${Boost_LIBRARIES}
    )

add_executable(amnes8
    amnes_bench.cpp
    )
target_compile_definitions(amnes8 PRIVATE DATA=8)
target_link_libraries(amnes8
    ${Boost_LIBRARIES}
    )

add_executable(amnes16
    amnes_bench.cpp
    )
target_compile_definitions(amnes16 PRIVATE DATA=16)
target_link_libraries(amnes16
    ${Boost_LIBRARIES}
    )

add_executable(amnes32
    amnes_bench.cpp
    )
target_link_libraries(amnes32
    ${Boost_LIBRARIES}
    )

if(${DEBUG})
    add_custom_target(amnes_throughput COMMAND bash ../scripts/amnes_throughput.sh -d 1 -m ${MACHINE_NAME})
else()
	add_custom_target(amnes_throughput COMMAND bash ../scripts/amnes_throughput.sh -d 0 -m ${MACHINE_NAME} -t ${TESTOPT})
endif()
