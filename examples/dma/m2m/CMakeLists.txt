cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(dma_m2m C ASM)

add_executable(dma_m2m main.c ../../uart/dma_lab_common.c)
target_include_directories(dma_m2m PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../uart)
target_link_libraries(dma_m2m bdk_stm32f4)

bdk_add_extra_outputs(dma_m2m)
