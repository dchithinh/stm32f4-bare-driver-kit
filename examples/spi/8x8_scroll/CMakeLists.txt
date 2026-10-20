cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(spi_8x8_scroll C ASM)

set(BDK_8X8_LOGGER_DIR "${CMAKE_CURRENT_LIST_DIR}/../../logger")
set(BDK_8X8_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(spi_8x8_scroll
    main.c
    c8x8y.c
    c8x8y_anim.c
    c8x8y_font.c
    "${BDK_8X8_LOGGER_DIR}/log.c"
    "${BDK_8X8_SDK_DIR}/startup/syscalls.c"
)
target_include_directories(spi_8x8_scroll PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/.."
    "${BDK_8X8_LOGGER_DIR}"
)
target_link_libraries(spi_8x8_scroll bdk_stm32f4)

bdk_add_extra_outputs(spi_8x8_scroll)
