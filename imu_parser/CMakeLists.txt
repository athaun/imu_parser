cmake_minimum_required(VERSION 3.10)
project(imu_parser)            

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(imu_parser ${sources})

target_compile_options(imu_parser PUBLIC -lrt -std=c++17 -Wall -Wfloat-conversion)

target_include_directories(imu_parser PUBLIC src)

file(COPY ${data} DESTINATION resources)