cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(spi_lcd_ili9341_dma C ASM)

add_executable(spi_lcd_ili9341_dma
    main.c
    ili9341.c
    ili9341_board_dma.c
)
target_include_directories(spi_lcd_ili9341_dma PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
target_link_libraries(spi_lcd_ili9341_dma bdk_stm32f4)

bdk_add_extra_outputs(spi_lcd_ili9341_dma)
