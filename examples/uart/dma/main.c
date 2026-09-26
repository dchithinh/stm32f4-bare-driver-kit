#include "bdk_gpio.h"
#include "bdk_uart.h"
#include "bdk_dma.h"

int main(void)
{
    bdk_gpio_config_t tx = {
        .port  = BDK_GPIO_PORT_A,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_USART2,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
        .pin   = 2,
    };
    bdk_gpio_config_t rx = {
        .port  = BDK_GPIO_PORT_A,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_USART2,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
        .pin   = 3,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&tx));
    BDK_ASSERT_OK(bdk_gpio_init(&rx));

    bdk_uart_config_t cfg = {
        .id     = BDK_UART_2,
        .baud   = BDK_UART_BAUD_115200,
        .parity = BDK_UART_PARITY_NONE,
        .stop   = BDK_UART_STOP_1,
        .word   = BDK_UART_WORD_8,
    };

    BDK_ASSERT_OK(bdk_uart_init(&cfg));

    /* RM0090 DMA request table for this chip + USART2 (F407 Discovery lab). */
    static const bdk_uart_dma_t usart2_dma = {
        .tx_stream = BDK_DMA1_STREAM(6),
        .rx_stream = BDK_DMA1_STREAM(5),
        .channel   = 4,
    };
    BDK_ASSERT_OK(bdk_uart_dma_bind(BDK_UART_2, &usart2_dma));

    static const uint8_t msg[] = "DMA TX (stub)\r\n";
    (void)bdk_uart_write_dma(BDK_UART_2, msg, sizeof(msg) - 1);

    for (;;) {
    }
}
