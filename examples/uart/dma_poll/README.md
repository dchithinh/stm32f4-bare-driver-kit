# UART DMA poll lab

## Purpose

**Low-level DMA** TX (memory → `USART2->DR`) with **no DMA TC interrupt**.

## What it proves

- Without IRQ, software **must** call **`bdk_dma_busy()`** (poll stream **EN**) to know when the transfer ended.
- Footer line `[poll] DMA complete (detected via bdk_dma_busy)` and a large **`busy_poll_loops`** count show that polling happened.

Compare [`dma_irq`](../dma_irq/): same idea, but completion comes from **TC IRQ**, not `busy`.

## Wiring

PA2 TX → USB–UART RX, GND common. **9600** 8N1.

## Setup

- **1024-byte** buffer (`dma_lab_common`: header + repeating `0123456789abcdef` lines).
- DMA1 stream 6, CHSEL 4; **DMAT** set in app (not in `bdk_dma_*`).
- Status lines after the DMA burst use polling `uart_write_*` on USART2.

## Build

`cmake --build build --target uart_dma_poll`
