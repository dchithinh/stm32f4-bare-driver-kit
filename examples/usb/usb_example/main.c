/**
 * USB enumeration on CN5 — same HAL PCD + ST USBD path as Cube usb_test.
 * BDK: clock, PC0 power, USART2 log. (bdk_usb EP0 is not used here.)
 */
#include "main.h"

#include "bdk_gpio.h"
#include "bdk_rcc.h"
#include "bdk_uart.h"

#include "clock_config.h"
#include "log.h"
#include "usb_device.h"

#include "stm32f4xx_hal.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

static void usb_clock_init(void)
{
    bdk_rcc_clock_config_t clk = USB_EXAMPLE_CLOCK_HSE_168_USB(USB_EXAMPLE_HSE_HZ);

    BDK_ASSERT_OK(bdk_rcc_clock_init(&clk));
}

static void usb_discovery_cn5_power_on(void)
{
    bdk_gpio_config_t pwr = {
        .port  = BDK_GPIO_PORT_C,
        .pin   = 0,
        .mode  = BDK_GPIO_MODE_OUTPUT,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_LOW,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&pwr));
    bdk_gpio_write_pin(BDK_GPIO_PORT_C, 0, BDK_GPIO_HIGH);
}

void Error_Handler(void)
{
    for (;;) {
    }
}

int main(void)
{
    HAL_Init();
    usb_clock_init();
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }

    log_redirect_uart2(BDK_UART_BAUD_115200);
    log_info("usb_example: HAL USBD enumerate (Cube usb_test path)");
    log_info("usb_example: USART2 ker=%lu Hz sysclk=%lu",
             (unsigned long)bdk_rcc_usart_ker_clk_hz(BDK_UART_2),
             (unsigned long)bdk_rcc_get_sysclk_hz());

    usb_discovery_cn5_power_on();
    MX_USB_DEVICE_Init();
    log_info("usb_example: USBD started — plug CN5; Pi dmesg + lsusb 0483:5740");

    for (;;) {
    }
}
