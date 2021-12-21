cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 11)

project(zynq-gps-ps)
set(OUTPUT_NAME gps)

include_directories(./lib/spdlog/include)
aux_source_directory(./src DIR_SRCS)
add_executable(${OUTPUT_NAME} ${DIR_SRCS})