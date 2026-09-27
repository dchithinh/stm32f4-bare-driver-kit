# UART examples

USART2 **PA2** TX / **PA3** RX on F407 Discovery (USB–UART). See `docs/notes/uart.md`.

| App | Purpose | What it proves |
|-----|---------|----------------|
| [`tx`](tx/) | Polling TX | `bdk_uart_write` pushes bytes; PA2 / serial shows `HELLO!` |
| [`echo`](echo/) | Polling RX+TX | `read_byte` / `write_byte`; PC typing echoes back |
| [`irq_echo`](irq_echo/) | RX IRQ + poll TX | `irq_enable`, app `USART2_IRQHandler`, `poll_in` drains ring |
| [`irq_tx`](irq_tx/) | TX IRQ | `write_async`, `tx_active`; CPU queues bytes, ISR sends on TXE |
| [`ring_drop`](ring_drop/) | RX ring limits | Fast paste overflows 64-byte ring; `ring_drop` / `ore` stats |
| [`dma`](dma/) | UART DMA API (stub) | `dma_bind` + `write_dma` target — not implemented yet |
| [`dma_poll`](dma_poll/) | DMA TX, no TC IRQ | Must use **`bdk_dma_busy()`** to know when DMA finished |
| [`dma_irq`](dma_irq/) | DMA TX + TC IRQ | Completion via **`dma_tx_done`**; main does not poll `busy` |

DMA low-level detail: `docs/notes/dma.md`.
