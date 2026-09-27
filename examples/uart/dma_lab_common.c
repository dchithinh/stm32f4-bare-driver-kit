#include "dma_lab_common.h"

#include "bdk_uart.h"

void lab_fill_dma_tx_buffer(uint8_t *buf, uint32_t len)
{
    static const char hdr[] = "DMA lab TX @ 9600 (phase 1, memory -> USART2 via DMA)\r\n";
    static const char line[] = "0123456789abcdef\r\n";
    uint32_t        off      = 0;

    for (uint32_t i = 0; hdr[i] != '\0' && off < len; i++) {
        buf[off++] = (uint8_t)hdr[i];
    }
    while (off + sizeof(line) - 1U <= len) {
        for (uint32_t i = 0; line[i] != '\0'; i++) {
            buf[off++] = (uint8_t)line[i];
        }
    }
    while (off < len) {
        buf[off++] = (uint8_t)'.';
    }
}

void uart_write_str(bdk_uart_id_t id, const char *s)
{
    while (*s != '\0') {
        bdk_uart_write_byte(id, (uint8_t)*s);
        s++;
    }
}

void uart_write_u32(bdk_uart_id_t id, uint32_t v)
{
    char    buf[10];
    int     i = 0;
    uint8_t ch;

    if (v == 0U) {
        bdk_uart_write_byte(id, (uint8_t)'0');
        return;
    }
    while (v > 0U) {
        buf[i++] = (char)('0' + (v % 10U));
        v /= 10U;
    }
    while (i > 0) {
        ch = (uint8_t)buf[--i];
        bdk_uart_write_byte(id, ch);
    }
}
