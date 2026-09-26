cmake_minimum_required(VERSION 3.20)

get_filename_component(BDK_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if (NOT CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "${BDK_SDK_PATH}/cmake/toolchain-arm-none-eabi.cmake"
            CACHE FILEPATH "ARM GCC toolchain file")
    endif()
    project(bdk_stm32f4_sdk C ASM)
    set(BDK_IS_TOP_LEVEL TRUE)
else()
    set(BDK_IS_TOP_LEVEL FALSE)
endif()

include("${BDK_SDK_PATH}/cmake/stm32f407.cmake")

add_library(bdk_stm32f4 STATIC
    src/bdk_status.c
    src/bdk_gpio.c
    src/bdk_rcc.c
    src/bdk_uart.c
    src/bdk_dma.c
    src/bdk_i2c.c
    src/bdk_spi.c
    src/bdk_timer.c
    startup/startup_stm32f407xx.s
    startup/system_stm32f4xx.c
    startup/syscalls.c
)

set_source_files_properties(
    startup/startup_stm32f407xx.s
    PROPERTIES LANGUAGE ASM
)

target_include_directories(bdk_stm32f4 PUBLIC
    "${BDK_SDK_PATH}/inc"
    "${BDK_SDK_PATH}/cmsis/cmsis_core/Include"
    "${BDK_SDK_PATH}/cmsis/cmsis_device_f4/Include"
)

target_compile_definitions(bdk_stm32f4 PUBLIC
    STM32F407xx
)

target_compile_options(bdk_stm32f4 PUBLIC
    ${BDK_CHIP_CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    $<$<COMPILE_LANGUAGE:C>:-std=c11>
)

target_link_options(bdk_stm32f4 PUBLIC
    ${BDK_CHIP_CPU_FLAGS}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    "-T${BDK_LINKER_SCRIPT}"
)

set_target_properties(bdk_stm32f4 PROPERTIES
    OUTPUT_NAME bdk_stm32f4
)

if (BDK_IS_TOP_LEVEL)
    option(BDK_BUILD_EXAMPLES "Build example applications" ON)
    if (BDK_BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()
endif()
