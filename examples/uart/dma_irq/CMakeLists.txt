cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(uart_dma_irq C ASM)

add_executable(uart_dma_irq main.c ../dma_lab_common.c)
target_include_directories(uart_dma_irq PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(uart_dma_irq bdk_stm32f4)

bdk_add_extra_outputs(uart_dma_irq)
