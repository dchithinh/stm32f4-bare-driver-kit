cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../bdk_sdk_import.cmake)
endif()

project(pwm_timer C ASM)

add_executable(pwm_timer main.c)
target_link_libraries(pwm_timer bdk_stm32f4)

bdk_add_extra_outputs(pwm_timer)
