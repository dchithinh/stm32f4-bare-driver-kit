# UART DMA (stub)

Placeholder until `bdk_dma` and `bdk_uart_write_dma` are implemented.

Build: `cmake --build build --target uart_dma`

- Generic DMA: `docs/notes/dma.md` (`bdk_dma_*` on any stream).
- This app: `bdk_uart_dma_bind` with streams/channels from the RM table for **your** USART instance.
- When TC IRQ is used, define the matching `DMAx_StreamN_IRQHandler` in the app and call `bdk_dma_irq_handler` with the same `bdk_dma_stream_t`.
