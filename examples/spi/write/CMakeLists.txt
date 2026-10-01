cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(spi_write C ASM)

add_executable(spi_write main.c)
target_link_libraries(spi_write bdk_stm32f4)

bdk_add_extra_outputs(spi_write)
