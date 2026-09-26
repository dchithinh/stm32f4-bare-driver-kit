# UART DMA polling lab

`bdk_dma_config` + `bdk_dma_start` + `bdk_dma_busy` only (no `bdk_uart_dma_bind` / `write_dma`).

USART2 TX: DMA1 stream 6, CHSEL 4 — verify in RM. **DMAT** enabled in app (`USART2->CR3`).

Build: `cmake --build build --target uart_dma_poll`
