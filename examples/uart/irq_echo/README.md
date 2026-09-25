# UART IRQ echo

USART2 **PA2** TX, **PA3** RX, 9600 8N1 (same wiring as polling echo).

- **RX:** `bdk_uart_irq_enable` + `bdk_uart_poll_in` (ISR fills ring on RXNE).
- **TX:** polling `bdk_uart_write_byte` in this example; you can switch to
  `bdk_uart_write_async` for full IRQ TX (see `irq_tx`).

The driver does **not** define `USART2_IRQHandler`. The app implements it and
calls `bdk_uart_irq_handler(BDK_UART_2)`.

API overview: `docs/notes/uart.md` and `inc/bdk_uart.h`.
