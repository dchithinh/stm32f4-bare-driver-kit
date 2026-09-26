cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(uart_ring_drop C ASM)

add_executable(uart_ring_drop main.c)
target_link_libraries(uart_ring_drop bdk_stm32f4)

bdk_add_extra_outputs(uart_ring_drop)
