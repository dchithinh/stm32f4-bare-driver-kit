/**
 * @file main.c
 * @brief 3× 8x8Y — MikroSDK v2 scroll (matches Cube spi_8x8).
 */

#include "bdk_gpio.h"
#include "bdk_rcc.h"
#include "bdk_spi.h"
#include "bdk_uart.h"

#include "c8x8y.h"
#include "c8x8y_board.h"
#include "clock_config.h"
#include "log.h"

#include <stdint.h>

#define LAB_SPI    BDK_SPI_1
#define LAB_SPI_AF BDK_GPIO_AF_SPI1

static const char k_scroll_msg[] = " HELLO WORLD! ";

/** Busy-wait ms @ 16 MHz HSI (tune if delays feel long/short). */
#define C8X8Y_MS_LOOPS 2000U

void c8x8y_example_delay_ms(unsigned ms)
{
    uint32_t ticks = C8X8Y_MS_LOOPS * (uint32_t)ms;

    for (volatile uint32_t i = 0; i < ticks; i++) {
    }
}

static void lab_spi1_gpio_init(void)
{
    bdk_gpio_config_t sck = {
        .port  = BDK_GPIO_PORT_A,
        .pin   = 5,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = LAB_SPI_AF,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
    };
    bdk_gpio_config_t miso = sck;
    bdk_gpio_config_t mosi = sck;

    miso.pin = 6;
    mosi.pin = 7;

    BDK_ASSERT_OK(bdk_gpio_init(&sck));
    BDK_ASSERT_OK(bdk_gpio_init(&miso));
    BDK_ASSERT_OK(bdk_gpio_init(&mosi));
}

int main(void)
{
    bdk_rcc_clock_config_t clk = SPI_EXAMPLE_CLOCK_HSI_16MHZ;
    c8x8y_t                dev = {0};

    BDK_ASSERT_OK(bdk_rcc_clock_init(&clk));
    log_redirect_uart2(C8X8Y_DEBUG_UART_BAUD);
    if (!log_uart2_ready()) {
        for (;;) {
            /* USART2 init failed — fix PA2/PA3 / clock before SPI debug. */
        }
    }
    log_uart2_puts("8x8: boot (raw)\r\n");
    log_info("8x8: boot sysclk=%lu usart2_ker=%lu",
             (unsigned long)bdk_rcc_get_sysclk_hz(),
             (unsigned long)bdk_rcc_usart_ker_clk_hz(BDK_UART_2));

    lab_spi1_gpio_init();
    log_info("8x8: SPI1 GPIO PA5/6/7 ok");

    bdk_spi_config_t cfg = {
        .id    = LAB_SPI,
        .baud  = BDK_SPI_BAUD_DIV16,
        .cpol  = BDK_SPI_CPOL_LOW,
        .cpha  = BDK_SPI_CPHA_1EDGE,
        .width = BDK_SPI_WIDTH_8,
    };
    BDK_ASSERT_OK(bdk_spi_init(&cfg));
    log_info("8x8: SPI1 init div16 (1 MHz @ 16 MHz HSI)");

    c8x8y_bind_spi(&dev, LAB_SPI);
    c8x8y_set_wiring(&dev, C8X8Y_WIRING_PARALLEL_CS);

    (void)c8x8y_cs_add(&dev, C8X8Y_CS0_PORT, C8X8Y_CS0_PIN);
    log_info("8x8: CS0 PA4");
    (void)c8x8y_cs_add(&dev, C8X8Y_CS1_PORT, C8X8Y_CS1_PIN);
    log_info("8x8: CS1 PB0");
    (void)c8x8y_cs_add(&dev, C8X8Y_CS2_PORT, C8X8Y_CS2_PIN);
    log_info("8x8: CS2 PB1 cs_count=%u", (unsigned)dev.cs_count);

    for (uint8_t p = 0U; p < C8X8Y_MODULE_COUNT; p++) {
        log_info("8x8: panel %u init", (unsigned)p);
        c8x8y_default_cfg_panel(&dev, p);
        c8x8y_write_cmd_panel(&dev, p, C8X8Y_INTENSITY_REG, C8X8Y_INTENSITY_LEVEL);
    }
    c8x8y_display_refresh(&dev);
    log_info("8x8: all panels blank");
    c8x8y_example_delay_ms(300U);

    for (uint8_t p = 0U; p < C8X8Y_MODULE_COUNT; p++) {
        c8x8y_display_byte_panel(&dev, p, (char)('1' + p));
        log_info("8x8: panel %u show '%c'", (unsigned)p, (char)('1' + p));
        c8x8y_example_delay_ms(C8X8Y_BOOT_DIGIT_MS);
    }

    c8x8y_set_speed_scroll(&dev, C8X8Y_SPEED_SLOW);
    c8x8y_display_refresh(&dev);
    c8x8y_scroll_prepare(&dev, k_scroll_msg);
    log_info("8x8: scroll start (24-col wide, slow, len=%u)",
             (unsigned)sizeof(k_scroll_msg) - 1U);

    for (;;) {
        if (c8x8y_scroll_tick(&dev) != 0) {
            c8x8y_scroll_delay(&dev);
        }
    }
}
