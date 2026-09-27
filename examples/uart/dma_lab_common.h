#ifndef DMA_LAB_COMMON_H
#define DMA_LAB_COMMON_H

#include <stdint.h>

#include "bdk_uart.h"

#define LAB_DMA_UART_BAUD BDK_UART_BAUD_9600
#define LAB_DMA_TX_BYTES  1024U

void lab_fill_dma_tx_buffer(uint8_t *buf, uint32_t len);

void uart_write_str(bdk_uart_id_t id, const char *s);
void uart_write_u32(bdk_uart_id_t id, uint32_t v);

#endif /* DMA_LAB_COMMON_H */
