#include "stm32f4xx.h"

#include "bdk_dma.h"
#include "bdk_gpio.h"
#include "bdk_uart.h"
#include "dma_lab_common.h"

static const bdk_dma_stream_t tx_stream = BDK_DMA1_STREAM(6);

static volatile uint8_t dma_tx_done;

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
        .baud   = LAB_DMA_UART_BAUD,
        .parity = BDK_UART_PARITY_NONE,
        .stop   = BDK_UART_STOP_1,
        .word   = BDK_UART_WORD_8,
    };
    BDK_ASSERT_OK(bdk_uart_init(&uart));
}

void DMA1_Stream6_IRQHandler(void)
{
    bdk_dma_irq_handler(&tx_stream);
    if (bdk_dma_busy(&tx_stream) == 0) {
        dma_tx_done = 1;
    }
}

int main(void)
{
    static uint8_t dma_buf[LAB_DMA_TX_BYTES];

    uart2_gpio_init();
    uart2_cfg_init();
    lab_fill_dma_tx_buffer(dma_buf, LAB_DMA_TX_BYTES);

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
    BDK_ASSERT_OK(bdk_dma_irq_enable(&tx_stream, BDK_DMA_IT_TC));

    uart_write_str(BDK_UART_2,
                   "\r\n[irq] TC IRQ enabled. Wait on dma_tx_done, not bdk_dma_busy().\r\n");

    dma_tx_done = 0;
    BDK_ASSERT_OK(bdk_dma_start(&tx_stream, dma_buf, LAB_DMA_TX_BYTES));

    while (dma_tx_done == 0) {
    }

    uart_write_str(BDK_UART_2, "[irq] DMA complete (TC IRQ set dma_tx_done; main never polled busy)\r\n");

    for (;;) {
    }
}
