#include "stm32f4xx.h"

#include "bdk_dma.h"
#include "bdk_gpio.h"
#include "bdk_uart.h"

static void uart2_gpio_init(void)
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
}

static void uart2_cfg_init(void)
{
    bdk_uart_config_t uart = {
        .id     = BDK_UART_2,
        .baud   = BDK_UART_BAUD_115200,
        .parity = BDK_UART_PARITY_NONE,
        .stop   = BDK_UART_STOP_1,
        .word   = BDK_UART_WORD_8,
    };
    BDK_ASSERT_OK(bdk_uart_init(&uart));
}

int main(void)
{
    uart2_gpio_init();
    uart2_cfg_init();

    static const bdk_dma_stream_t tx_stream = BDK_DMA1_STREAM(6);

    static const bdk_dma_config_t dma_tx = {
        .stream       = tx_stream,
        .channel      = 4,
        .direction    = BDK_DMA_MEM_TO_PERIPH,
        .periph_addr  = &USART2->DR,
        .periph_inc   = 0,
        .mem_inc      = 1,
        .periph_width = BDK_DMA_WIDTH_BYTE,
        .mem_width    = BDK_DMA_WIDTH_BYTE,
        .circular     = 0,
    };
    BDK_ASSERT_OK(bdk_dma_config(&dma_tx));

    SET_BIT(USART2->CR3, USART_CR3_DMAT);

    static const uint8_t msg[] = "DMA poll TX\r\n";
    BDK_ASSERT_OK(bdk_dma_start(tx_stream, msg, sizeof(msg) - 1));
    while (bdk_dma_busy(tx_stream) != 0) {
    }

    for (;;) {
    }
}
