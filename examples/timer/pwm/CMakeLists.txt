cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(timer_pwm C ASM)

add_executable(timer_pwm main.c)
target_link_libraries(timer_pwm bdk_stm32f4)

bdk_add_extra_outputs(timer_pwm)
