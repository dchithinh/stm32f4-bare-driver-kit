/**
 * @file main.c
 * @brief 2.4" ILI9341 — looping color demo (solids, bars, portrait / landscape).
 */

#include "bdk_rcc.h"
#include "bdk_spi.h"

#include "ili9341.h"
#include "ili9341_board.h"
#include "clock_config.h"

#include <stdint.h>

#define LAB_SPI ILI9341_BOARD_SPI

/** Busy-wait ms @ ~168 MHz SYSCLK (approximate). */
#define ILI9341_MS_LOOPS 42000U

void ili9341_example_delay_ms(unsigned ms)
{
    uint32_t ticks = ILI9341_MS_LOOPS * (uint32_t)ms;

    for (volatile uint32_t i = 0U; i < ticks; i++) {
    }
}

static void lab_clock_init(void)
{
    bdk_rcc_clock_config_t clk = SPI_EXAMPLE_CLOCK_HSE_168MHZ;

    if (bdk_rcc_clock_init(&clk) != BDK_OK) {
        bdk_rcc_clock_config_t fallback = SPI_EXAMPLE_CLOCK_HSE_84MHZ;

        if (bdk_rcc_clock_init(&fallback) != BDK_OK) {
            bdk_rcc_clock_config_t hsi = SPI_EXAMPLE_CLOCK_HSI_64MHZ;

            (void)bdk_rcc_clock_init(&hsi);
        }
    }
}

#define DEMO_SOLID_MS  450U
#define DEMO_HOLD_MS   900U
#define DEMO_FLASH_MS  120U

static void demo_solid_rgb(ili9341_t *lcd)
{
    ili9341_fill_rgb565(lcd, 0xF800U);
    ili9341_example_delay_ms(DEMO_SOLID_MS);
    ili9341_fill_rgb565(lcd, 0x07E0U);
    ili9341_example_delay_ms(DEMO_SOLID_MS);
    ili9341_fill_rgb565(lcd, 0x001FU);
    ili9341_example_delay_ms(DEMO_SOLID_MS);
}

static void demo_phase(ili9341_t *lcd, uint8_t orientation)
{
    ili9341_set_orientation(lcd, orientation);
    demo_solid_rgb(lcd);
    ili9341_draw_color_bars(lcd);
    ili9341_example_delay_ms(DEMO_HOLD_MS);
}

static void demo_flash(ili9341_t *lcd)
{
    ili9341_fill_rgb565(lcd, 0xFFFFU);
    ili9341_example_delay_ms(DEMO_FLASH_MS);
    ili9341_fill_rgb565(lcd, 0x0000U);
    ili9341_example_delay_ms(DEMO_FLASH_MS);
}

static void demo_run_forever(ili9341_t *lcd)
{
    for (;;) {
        demo_phase(lcd, ILI9341_LCD_PORTRAIT);
        demo_phase(lcd, ILI9341_LCD_LANDSCAPE);
        demo_flash(lcd);
    }
}

int main(void)
{
    ili9341_t lcd = {0};

    lab_clock_init();

    ili9341_bind(&lcd, ILI9341_BOARD_SPI,
                 ILI9341_BOARD_CS_PORT, ILI9341_BOARD_CS_PIN,
                 ILI9341_BOARD_DC_PORT, ILI9341_BOARD_DC_PIN,
                 ILI9341_BOARD_RST_PORT, ILI9341_BOARD_RST_PIN);

    ili9341_gpio_init(&lcd);

    bdk_spi_config_t spi_cfg = {
        .id    = LAB_SPI,
        .baud  = ILI9341_BOARD_SPI_BAUD,
        .cpol  = BDK_SPI_CPOL_LOW,
        .cpha  = BDK_SPI_CPHA_1EDGE,
        .width = BDK_SPI_WIDTH_8,
    };
    BDK_ASSERT_OK(bdk_spi_init(&spi_cfg));

    ili9341_init(&lcd);
    demo_run_forever(&lcd);
}
