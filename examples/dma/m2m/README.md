# DMA memory-to-memory

## Purpose

Copy a block **RAM → RAM** with **`BDK_DMA_MEM_TO_MEM`** (no USART/SPI peripheral request).

Uses **`bdk_dma_config`** (source in **`periph_addr`**, both **PINC** and **MINC**), **`bdk_dma_irq_enable(BDK_DMA_IT_TC)`**, app **`DMA2_Stream0_IRQHandler`** → **`bdk_dma_irq_handler`**, then **`bdk_dma_start`** (destination buffer).

Main waits on **`dma_m2m_done`** (set in the IRQ when TC is handled); it does **not** call **`bdk_dma_busy()`**.

USART2 is only for **printing** `dst` before/after the transfer.

## What it proves

- **`dst` before** is all `00` (zeroed).
- **`dst` after** matches the **`src`** pattern (`A0`..`AF` repeating) as a hex dump.
- **`mismatches=0`** confirms byte-for-byte copy via DMA.
- Completion is detected via **TC IRQ**, same pattern as [`uart/dma_irq`](../../uart/dma_irq/).

## Wiring

PA2 TX → USB–UART RX, GND common. **115200** 8N1.

## Build

`cmake --build build --target dma_m2m`

## Stream choice

**DMA2 stream 0** — avoids DMA1 stream 6 used by USART2 TX labs. CHSEL is unused for M2M (set to 0).
