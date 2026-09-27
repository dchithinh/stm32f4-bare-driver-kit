# DMA examples

Low-level **`bdk_dma_*`** labs (not UART wrapper API).

| App | Purpose | What it proves |
|-----|---------|----------------|
| [`m2m`](m2m/) | Memory-to-memory + TC IRQ | Hex dump of `dst` before/after; `mismatches=0` |

USART2 mem→periph labs live under [`uart/dma_poll`](../uart/dma_poll/) and [`uart/dma_irq`](../uart/dma_irq/).
