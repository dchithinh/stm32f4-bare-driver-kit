cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(spi_lcd_ili9341 C ASM)

add_executable(spi_lcd_ili9341
    main.c
    ili9341.c
)
target_include_directories(spi_lcd_ili9341 PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
target_link_libraries(spi_lcd_ili9341 bdk_stm32f4)

bdk_add_extra_outputs(spi_lcd_ili9341)
