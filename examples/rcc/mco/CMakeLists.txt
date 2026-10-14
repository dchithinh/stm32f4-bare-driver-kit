cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(rcc_mco C ASM)

set(BDK_RCC_MCO_LOGGER_DIR "${CMAKE_CURRENT_LIST_DIR}/../../logger")
set(BDK_RCC_MCO_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(rcc_mco
    main.c
    "${BDK_RCC_MCO_LOGGER_DIR}/log.c"
    "${BDK_RCC_MCO_SDK_DIR}/startup/syscalls.c"
)
target_include_directories(rcc_mco PRIVATE "${BDK_RCC_MCO_LOGGER_DIR}")
target_link_libraries(rcc_mco bdk_stm32f4)

bdk_add_extra_outputs(rcc_mco)
