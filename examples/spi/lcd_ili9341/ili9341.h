/**
 * @file ili9341.h
 * @brief ILI9341 over SPI — commands, window, and RGB565 fills (app layer).
 */
#ifndef ILI9341_H
#define ILI9341_H

#include "bdk_gpio.h"
#include "bdk_spi.h"

#include <stdint.h>

typedef struct {
    bdk_spi_id_t      spi;
    bdk_gpio_port_t   cs_port;
    uint8_t           cs_pin;
    bdk_gpio_port_t   dc_port;
    uint8_t           dc_pin;
    bdk_gpio_port_t   rst_port;
    uint8_t           rst_pin;
    uint8_t           orientation; /**< @ref ILI9341_LCD_PORTRAIT or @ref ILI9341_LCD_LANDSCAPE */
} ili9341_t;

void ili9341_bind(ili9341_t *lcd, bdk_spi_id_t spi,
                  bdk_gpio_port_t cs_port, uint8_t cs_pin,
                  bdk_gpio_port_t dc_port, uint8_t dc_pin,
                  bdk_gpio_port_t rst_port, uint8_t rst_pin);

void ili9341_gpio_init(const ili9341_t *lcd);

/** Reset pulse, register init, portrait RGB565. Call after @ref bdk_spi_init. */
void ili9341_init(ili9341_t *lcd);

/** Updates @p lcd->orientation and sends MADCTL (0x36). */
void ili9341_set_orientation(ili9341_t *lcd, uint8_t orientation);

void ili9341_set_window(ili9341_t *lcd, uint16_t x0, uint16_t x1,
                        uint16_t y0, uint16_t y1);

/** Full-panel solid fill for the current @ref ili9341_set_orientation. */
void ili9341_fill_rgb565(ili9341_t *lcd, uint16_t color);

/**
 * Three bands over full panel (y-thirds). Portrait → horizontal stripes;
 * landscape (MV) → vertical columns.
 */
void ili9341_draw_color_bars(ili9341_t *lcd);

static inline uint16_t ili9341_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((((uint16_t)r & 0xF8U) << 8) |
                      (((uint16_t)g & 0xFCU) << 3) | ((uint16_t)b >> 3));
}

#endif /* ILI9341_H */
