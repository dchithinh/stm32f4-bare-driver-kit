cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(uart_irq_echo C ASM)

add_executable(uart_irq_echo main.c)
target_link_libraries(uart_irq_echo bdk_stm32f4)

bdk_add_extra_outputs(uart_irq_echo)
