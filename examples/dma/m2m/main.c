#include <stdint.h>
#include <string.h>

#include "stm32f4xx.h"

#include "bdk_dma.h"
#include "bdk_gpio.h"
#include "bdk_uart.h"
#include "dma_lab_common.h"

#define M2M_LEN 64U

static const bdk_dma_stream_t m2m_stream = BDK_DMA2_STREAM(0);

static volatile uint8_t dma_m2m_done;

void DMA2_Stream0_IRQHandler(void)
{
    bdk_dma_irq_handler(&m2m_stream);
    if (bdk_dma_busy(&m2m_stream) == 0) {
        dma_m2m_done = 1;
    }
}

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

static void uart_write_hex8(bdk_uart_id_t id, uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";

    bdk_uart_write_byte(id, (uint8_t)hex[(v >> 4) & 0x0FU]);
    bdk_uart_write_byte(id, (uint8_t)hex[v & 0x0FU]);
}

static void uart_dump_mem(bdk_uart_id_t id, const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uart_write_hex8(id, buf[i]);
        if ((i & 0x0FU) == 0x0FU) {
            uart_write_str(id, "\r\n");
        } else if (i + 1U < len) {
            bdk_uart_write_byte(id, (uint8_t)' ');
        }
    }
    if ((len & 0x0FU) != 0U) {
        uart_write_str(id, "\r\n");
    }
}

int main(void)
{
    static uint8_t src[M2M_LEN];
    static uint8_t dst[M2M_LEN];
    uint32_t       mismatches = 0;

    uart2_gpio_init();
    uart2_cfg_init();

    for (uint32_t i = 0; i < M2M_LEN; i++) {
        src[i] = (uint8_t)(0xA0U + (i & 0x0FU));
    }
    memset(dst, 0, sizeof(dst));

    uart_write_str(BDK_UART_2,
                   "\r\n[m2m] DMA mem-to-mem copy (DMA2 stream 0, TC IRQ)\r\n");
    uart_write_str(BDK_UART_2, "[m2m] dst before:\r\n");
    uart_dump_mem(BDK_UART_2, dst, M2M_LEN);

    static const bdk_dma_config_t m2m_cfg = {
        .stream       = m2m_stream,
        .channel      = 0,
        .direction    = BDK_DMA_MEM_TO_MEM,
        .periph_addr  = src,
        .periph_inc   = 1,
        .mem_inc      = 1,
        .periph_width = BDK_DMA_WIDTH_BYTE,
        .mem_width    = BDK_DMA_WIDTH_BYTE,
        .circular     = 0,
    };
    BDK_ASSERT_OK(bdk_dma_config(&m2m_cfg));
    BDK_ASSERT_OK(bdk_dma_irq_enable(&m2m_stream, BDK_DMA_IT_TC));

    dma_m2m_done = 0;
    BDK_ASSERT_OK(bdk_dma_start(&m2m_stream, dst, M2M_LEN));

    while (dma_m2m_done == 0) {
    }

    uart_write_str(BDK_UART_2, "[m2m] dst after (main waited on dma_m2m_done, not busy):\r\n");
    uart_dump_mem(BDK_UART_2, dst, M2M_LEN);

    for (uint32_t i = 0; i < M2M_LEN; i++) {
        if (dst[i] != src[i]) {
            mismatches++;
        }
    }
    uart_write_str(BDK_UART_2, "[m2m] mismatches=");
    uart_write_u32(BDK_UART_2, mismatches);
    uart_write_str(BDK_UART_2, " (expect 0)\r\n");

    for (;;) {
    }
}
