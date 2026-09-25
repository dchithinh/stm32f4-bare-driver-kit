---
name: bdk-driver-api
description: >-
  Names and shapes bdk_ public APIs, and Doxygen style for inc/bdk_*.h.
  Use when adding UART/I2C/SPI/GPIO functions, renaming APIs, designing IRQ vs
  polling entry points, or writing/updating header comments.
---

# Public header Doxygen (`inc/bdk_*.h`)

Describe **what the function does**, not naming philosophy or comparisons to
other libraries.

| Do | Do not |
|----|--------|
| One-line `@brief` (behavior) | Vendor/framework names, “style of …” |
| `@param` / `@return` for inputs, outputs, errors | Long `@file` essays; put guides in `docs/notes/` |
| Short preconditions only when behavior depends on them (NULL rules, buffer lifetime, call order) | “Use X instead of Y” unless it is a hard contract |
| `@file` → single-line module purpose | Register tutorials in the header (RM tables stay in notes / your learning) |

**Reference:** `inc/bdk_uart.h`.

Example shape:

```c
/**
 * @brief Block until RXNE, then read one byte from DR.
 * @param id   UART instance.
 * @return Received byte; 0 if @p id is invalid.
 */
uint8_t bdk_uart_read_byte(bdk_uart_id_t id);
```

# bdk_ public function names

Follow the names in `inc/bdk_uart.h` and `docs/notes/uart.md`. Do not invent
`try_`, `do_`, `nb_`, `check_` prefixes.

## Read / write (byte)

| Role | Name |
|------|------|
| Block until one byte in | `bdk_uart_read_byte` |
| Block until one byte out | `bdk_uart_write_byte` |
| Buffer in/out | `bdk_uart_read` / `bdk_uart_write` |
| Non-blocking one byte **in** (IRQ ring) | `bdk_uart_poll_in` |
| Non-blocking one byte **out** (future) | `bdk_uart_poll_out` |
| IRQ buffer **TX** | `bdk_uart_write_async` + `bdk_uart_tx_active` |
| Hardware “holding register full?” | `bdk_uart_rx_ready` or `bdk_uart_readable` |

UART IRQ naming details: `docs/notes/uart.md`.

Do **not** use: `try_read`, `try_write`, `nb_read`, `read_nonblock`, `get_char_if_any`.

## IRQ

- Driver: `bdk_uart_irq_enable(id)`, `bdk_uart_irq_handler(id)`.
- App: `USART2_IRQHandler` → `bdk_uart_irq_handler(BDK_UART_2)`.
- Never define `USARTx_IRQHandler` in `src/bdk_*.c`.

## Status

- **`BDK_OK` is 0.** Success is `== BDK_OK`, never `if (fn())`.
- Predicates (`rx_ready`): `int` 0 / non-zero. Invalid id → 0, not -1.

## Example

```c
uint8_t c;
if (bdk_uart_poll_in(BDK_UART_2, &c) == BDK_OK) {
    bdk_uart_write_byte(BDK_UART_2, c);       /* or write_async for IRQ TX */
}
```

API shape only; do not write register bodies.
