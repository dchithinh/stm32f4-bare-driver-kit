#include "bdk_gpio.h"
#include "bdk_uart.h"

#include "log.h"

/*
 * STM32F407 Discovery, USB OTG FS micro-AB (device).
 * Pins come from the board; the USB device stack stays in this example.
 *
 *   PA11  OTG_FS_DM   AF10
 *   PA12  OTG_FS_DP   AF10
 *
 * PA9 (VBUS), PA10 (ID), PC0 (power switch), and PD5 (overcurrent) stay
 * unused until host mode needs them.
 *
 * For USB, call bdk_rcc_clock_init() with a board config (see clock_config.h)
 * before usb_fs_init().
 */

static void usb_pins_init(void)
{
    bdk_gpio_config_t dm = {
        .port  = BDK_GPIO_PORT_A,
        .pin   = 11,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_OTG_FS,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_VERY_HIGH,
    };
    bdk_gpio_config_t dp = {
        .port  = BDK_GPIO_PORT_A,
        .pin   = 12,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_OTG_FS,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_VERY_HIGH,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&dm));
    BDK_ASSERT_OK(bdk_gpio_init(&dp));
}

/* Device stack: 48 MHz clock, RCC OTGFSEN, core, endpoints, enumeration. */
static void usb_fs_init(void)
{
}

void OTG_FS_IRQHandler(void)
{
}

int main(void)
{
    log_redirect_uart2(BDK_UART_BAUD_115200);
    log_info("usb_example: USART2 console on PA2/PA3");

    usb_pins_init();
    log_debug("OTG FS DM/DP (PA11/PA12) configured");
    usb_fs_init();

    for (;;) {
    }
}
