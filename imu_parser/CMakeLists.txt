cmake_minimum_required(VERSION 3.10)
project(imu_parser)

# Source files
file(GLOB_RECURSE sources src/*.cpp src/*.h)

# Main executable
add_executable(imu_parser ${sources})
target_compile_options(imu_parser PUBLIC -lrt -std=c++17 -Wall -Wfloat-conversion)
target_include_directories(imu_parser PUBLIC src)

# Copy data if needed
file(COPY ${data} DESTINATION resources)

# Test sources
file(GLOB_RECURSE test_sources tests/*.cpp)

# Reuse src files except entry.cpp (which has main())
set(test_sources_all ${test_sources} ${sources})
get_filename_component(ENTRY_CPP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/entry.cpp" ABSOLUTE)
list(REMOVE_ITEM test_sources_all "${ENTRY_CPP_PATH}")

# Add test target
add_executable(run_tests ${test_sources_all})
target_link_libraries(run_tests gtest gtest_main pthread)
target_include_directories(run_tests PUBLIC src)

enable_testing()
add_test(NAME run_tests COMMAND run_tests)
