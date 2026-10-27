/**
 * @file ili9341_board.h
 * @brief Panel + pin config (`stm32f407xx_spi_lcd_2.4inch` / `lcd.h`, `hw_def.h`).
 */
#ifndef ILI9341_BOARD_H
#define ILI9341_BOARD_H

#include "bdk_gpio.h"
#include "bdk_spi.h"

/* --- Geometry (lcd.h) --- */
#define ILI9341_LCD_PORTRAIT  0U
#define ILI9341_LCD_LANDSCAPE 1U
#define ILI9341_LCD_ORIENTATION ILI9341_LCD_LANDSCAPE

/** Panel memory (portrait); drawing size follows @ref ILI9341_LCD_ORIENTATION. */
#define ILI9341_LCD_NATIVE_WIDTH  240U
#define ILI9341_LCD_NATIVE_HEIGHT 320U

#if (ILI9341_LCD_ORIENTATION == ILI9341_LCD_LANDSCAPE)
#define ILI9341_LCD_DRAW_WIDTH  ILI9341_LCD_NATIVE_HEIGHT
#define ILI9341_LCD_DRAW_HEIGHT ILI9341_LCD_NATIVE_WIDTH
#else
#define ILI9341_LCD_DRAW_WIDTH  ILI9341_LCD_NATIVE_WIDTH
#define ILI9341_LCD_DRAW_HEIGHT ILI9341_LCD_NATIVE_HEIGHT
#endif

/** Max scanline length (pixels) for static line buffers. */
#define ILI9341_LCD_LINE_PIXELS ILI9341_LCD_DRAW_WIDTH

/* Memory access control (lcd.c MADCTL_* , portrait / landscape) */
#define ILI9341_MADCTL_MY   0x80U
#define ILI9341_MADCTL_MX   0x40U
#define ILI9341_MADCTL_MV   0x20U
#define ILI9341_MADCTL_RGB  0x00U
#define ILI9341_MADCTL_BGR  0x08U

#define ILI9341_MADCTL_PORTRAIT  (ILI9341_MADCTL_MY | ILI9341_MADCTL_MX | ILI9341_MADCTL_RGB)
/** MV rotates GRAM vs glass; keep RGB so bar colors match portrait (Cube used BGR here). */
#define ILI9341_MADCTL_LANDSCAPE (ILI9341_MADCTL_MV | ILI9341_MADCTL_MY | ILI9341_MADCTL_RGB)

/* --- SPI2 + GPIO (hw_def.h) --- */
#define ILI9341_BOARD_SPI    BDK_SPI_2
#define ILI9341_BOARD_SPI_AF BDK_GPIO_AF_SPI2

/** Fastest BR on F407 SPI2: PCLK1 (max 42 MHz) / 2 ≈ 21 MHz SCK. */
#define ILI9341_BOARD_SPI_BAUD BDK_SPI_BAUD_DIV2

/**
 * Pixel RAM writes after 0x2C: 16-bit SPI frames (one RGB565 per @ref bdk_spi_write
 * frame; MSB-first on wire). Set to 0 for legacy 8-bit high/low byte per pixel.
 */
/**
 * DMA pixel path uses 8-bit SPI + byte DMA (matches polling byte order).
 * 16-bit SPI + halfword DMA is not used on this lab target.
 */
#define ILI9341_PIXEL_SPI_16BIT 0U

/** Pixel RAM after 0x2C uses @ref bdk_spi_write_dma (commands stay polling). */
#define ILI9341_PIXEL_SPI_DMA 1U

#define ILI9341_BOARD_SCK_PORT  BDK_GPIO_PORT_B
#define ILI9341_BOARD_SCK_PIN   13U
#define ILI9341_BOARD_MOSI_PORT BDK_GPIO_PORT_B
#define ILI9341_BOARD_MOSI_PIN  15U
#define ILI9341_BOARD_MISO_PORT BDK_GPIO_PORT_C
#define ILI9341_BOARD_MISO_PIN  2U

#define ILI9341_BOARD_CS_PORT  BDK_GPIO_PORT_B
#define ILI9341_BOARD_CS_PIN   9U

#define ILI9341_BOARD_DC_PORT  BDK_GPIO_PORT_D
#define ILI9341_BOARD_DC_PIN   9U

#define ILI9341_BOARD_RST_PORT BDK_GPIO_PORT_D
#define ILI9341_BOARD_RST_PIN  10U

#endif /* ILI9341_BOARD_H */
