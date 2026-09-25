#include "bdk_gpio.h"
#include "bdk_uart.h"

static const uint8_t banner[] = "uart_irq_tx: async TX (TXE IRQ)\r\n";

/** Crude delay so Tera Term lines are readable between bursts. */
static void delay_spin(void)
{
    for (volatile int i = 0; i < 2000000; i++) {
    }
}

int main(void)
{
    bdk_gpio_config_t uart_pin_tx = {
        .port  = BDK_GPIO_PORT_A,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_USART2,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
        .pin   = 2,
    };

    bdk_gpio_config_t uart_pin_rx = {
        .port  = BDK_GPIO_PORT_A,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_USART2,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
        .pin   = 3,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&uart_pin_tx));
    BDK_ASSERT_OK(bdk_gpio_init(&uart_pin_rx));

    bdk_uart_config_t uart2_cfg = {
        .id     = BDK_UART_2,
        .baud   = BDK_UART_BAUD_9600,
        .parity = BDK_UART_PARITY_NONE,
        .stop   = BDK_UART_STOP_1,
        .word   = BDK_UART_WORD_8,
    };

    BDK_ASSERT_OK(bdk_uart_init(&uart2_cfg));
    BDK_ASSERT_OK(bdk_uart_irq_enable(BDK_UART_2));

    static const uint8_t tick_msg[] = "ping (TX IRQ)\r\n";

    BDK_ASSERT_OK(bdk_uart_write_async(BDK_UART_2, banner, sizeof(banner) - 1));
    while (bdk_uart_tx_active(BDK_UART_2)) {
    }

    for (;;) {
        if (!bdk_uart_tx_active(BDK_UART_2)) {
            (void)bdk_uart_write_async(BDK_UART_2, tick_msg, sizeof(tick_msg) - 1);
        }
        delay_spin();
    }
}

void USART2_IRQHandler(void)
{
    bdk_uart_irq_handler(BDK_UART_2);
}
