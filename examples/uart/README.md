# UART examples

USART2 on **PA2** / **PA3** unless noted. API layers: `docs/notes/uart.md`.

| App | What |
|-----|------|
| [`tx`](tx/) | Polling TX (`bdk_uart_write`) |
| [`echo`](echo/) | Polling echo (`read_byte` / `write_byte`) |
| [`irq_echo`](irq_echo/) | RX IRQ (`poll_in`) + polling TX |
| [`irq_tx`](irq_tx/) | TX IRQ (`write_async` + `tx_active`) |
