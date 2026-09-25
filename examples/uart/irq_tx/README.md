# UART IRQ TX

USART2 on **PA2** (TX), **PA3** (RX), 9600 8N1 — same USB–UART wiring as other UART examples.

Exercises **`bdk_uart_write_async`** and **TXE** handling in **`bdk_uart_irq_handler`**. `main` only
queues transfers when **`bdk_uart_tx_active`** is false; bytes leave via the ISR.
Why **`write_async`** and not `poll_out`: `docs/notes/uart.md`.

## Build

From repo `build/`:

```bash
cmake --build . --target uart_irq_tx
```

Flash `uart_irq_tx.bin` / `.hex`.

## Expected

1. One line: `uart_irq_tx: async TX (TXE IRQ)`
2. Then `ping (TX IRQ)` about once per second (spin delay in `main`).

If you see nothing after reset, check **TXEIE** is cleared when idle and that
`USART2_IRQHandler` calls `bdk_uart_irq_handler(BDK_UART_2)`.

## RX + TX IRQ

For echo with async TX, use [`irq_echo`](../irq_echo/) and replace `write_byte` with
`write_async` (keep the byte buffer alive until `!bdk_uart_tx_active`).
