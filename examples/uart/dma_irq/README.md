# UART DMA IRQ lab

## Purpose

Same **low-level DMA** TX as `dma_poll`, plus **`bdk_dma_irq_enable(BDK_DMA_IT_TC)`** and app **`DMA1_Stream6_IRQHandler`**.

## What it proves

- Main waits on **`dma_tx_done`** (set when TC is handled); it does **not** use **`bdk_dma_busy()`** to detect completion.
- Footer: `[irq] DMA complete (TC IRQ set dma_tx_done; main never polled busy)`.

Contrast with **`dma_poll`**: there you must poll **`bdk_dma_busy()`** because no TC IRQ is enabled.

## Wiring

Same as `dma_poll`: PA2, **9600** 8N1.

## Setup

- Same **1024-byte** payload and DMA1 stream 6 / CHSEL 4 as `dma_poll`.
- NVIC: enable **`DMA1_Stream6`** IRQ; handler calls **`bdk_dma_irq_handler`** then sets **`dma_tx_done`** when not busy.

## Build

`cmake --build build --target uart_dma_irq`
