cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(i2c_scanner C ASM)

add_executable(i2c_scanner main.c)
target_link_libraries(i2c_scanner bdk_stm32f4)

bdk_add_extra_outputs(i2c_scanner)
