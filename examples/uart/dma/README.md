# UART DMA (stub)

## Purpose

Placeholder for **`bdk_uart_dma_bind`** + **`bdk_uart_write_dma`** (high-level UART TX over DMA).

## What it proves

- **Today:** links and calls the API; `write_dma` returns **`BDK_ERR_NOT_IMPL`** until you implement the UART DMA layer.
- **Target:** one call sends a buffer via bound DMA stream + **DMAT**; completion via DMA TC (see `dma_irq` for raw DMA).

Use [`dma_poll`](../dma_poll/) and [`dma_irq`](../dma_irq/) for working **`bdk_dma_*`** labs.

## Wiring

PA2/PA3, **115200** 8N1 (status string only until `write_dma` works).

## Build

`cmake --build build --target uart_dma`

RM table for USART2 TX: DMA1 stream 6, CHSEL 4 — `docs/notes/dma.md`.
