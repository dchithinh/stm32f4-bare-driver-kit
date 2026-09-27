# UART IRQ echo

## Purpose

**RX in IRQ**, TX still polling: ISR fills the ring on **RXNE**; main uses **`bdk_uart_poll_in`**.

App defines **`USART2_IRQHandler`** → `bdk_uart_irq_handler(BDK_UART_2)` (not in the driver).

## What it proves

- NVIC + **RXNEIE** receive bytes without polling **RXNE** in the main loop.
- Echo behavior matches polling `echo` when you type slowly; under load, ring buffering matters (see `ring_drop`).

## Wiring

Same as [`echo`](../echo/): PA2/PA3, **9600 8N1**.

## Build

`cmake --build build --target uart_irq_echo`
