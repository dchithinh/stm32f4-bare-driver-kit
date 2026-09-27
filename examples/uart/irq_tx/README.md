# UART IRQ TX

## Purpose

**TX in IRQ**: `bdk_uart_write_async` + **TXEIE**; **`bdk_uart_irq_handler`** drains the queue from **DR**.

Main only starts a new line when **`bdk_uart_tx_active`** is false.

## What it proves

- CPU can return while bytes still leave on TX; ISR handles **TXE**.
- Serial shows `ping (TX IRQ)` about once per second without spinning on **TXE** in `main`.

## Wiring

PA2 TX to USB–UART RX, **9600 8N1** (same as `tx`).

## Build

`cmake --build build --target uart_irq_tx`

## Expected

1. Line: `uart_irq_tx: async TX (TXE IRQ)`
2. Repeated `ping (TX IRQ)` ~1 Hz

`USART2_IRQHandler` must call `bdk_uart_irq_handler(BDK_UART_2)`.
