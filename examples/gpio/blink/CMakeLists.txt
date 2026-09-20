cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(gpio_blink C ASM)

add_executable(gpio_blink main.c)
target_link_libraries(gpio_blink bdk_stm32f4)

bdk_add_extra_outputs(gpio_blink)
