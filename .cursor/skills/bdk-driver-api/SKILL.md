---
name: bdk-driver-api
description: >-
  Names and shapes bdk_ public APIs, and Doxygen style for inc/bdk_*.h.
  Use when adding UART/I2C/SPI/GPIO/DMA functions, renaming APIs, designing IRQ
  vs polling entry points, or writing/updating header comments. For SDK layering
  and portability, also read bdk-library-design.
---

# Public header Doxygen (`inc/bdk_*.h`)

Describe **what the function does**, not naming philosophy or comparisons to
other libraries.

| Do | Do not |
|----|--------|
| `@brief` states behavior or purpose (length OK if still about *what*, not *why we chose names*) | Vendor/framework names, “style of …”, chatty tone |
| `@param` / `@return` for inputs, outputs, errors | Policy, porting guides, RM tutorials in headers |
| Short preconditions only when behavior depends on them (NULL rules, buffer lifetime, call order) | “Use X instead of Y” unless it is a hard contract |
| `@file` → what this header/module is for | Essays, checklists, extension rules (those live in skills / `docs/notes/`) |

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
- Use **specific** `bdk_status_t` values (see `inc/bdk_status.h`); avoid a
  single catch-all for arguments.

| Code | Use |
|------|-----|
| `BDK_ERR_NULL` | Required pointer was NULL |
| `BDK_ERR_RANGE` | Bad id, pin, stream index, length, baud, enum |
| `BDK_ERR_STATE` | Wrong call order / peripheral state (e.g. DMA config while EN) |
| `BDK_ERR_BUSY` | Async TX, DMA stream already active |
| `BDK_ERR_NODATA` | `poll_in` ring empty |
| `BDK_ERR_NOT_IMPL` | Skeleton not implemented yet |
| `BDK_ERR` | Unspecified hardware failure after init |

- Log strings: `bdk_status_str(status)` — extend the switch when adding enum values.

### Extending `bdk_status_t`

- One enum in `inc/bdk_status.h` for all drivers; no `bdk_uart_err_t` unless a
  module needs many app-visible codes (rare).
- Reuse shared codes (`BDK_ERR_NULL`, `BDK_ERR_RANGE`, `BDK_ERR_BUSY`, …) first.
- Driver-only: append `BDK_ERR_DMA_*`, `BDK_ERR_I2C_*`, etc.; never renumber.
- Optional comment bands in the enum (common / DMA / UART) only if the list grows large.
- `bdk_status.h`: `@file` describes the shared result type; per-value `/**< */` only.
  Extension rules stay in this skill, not in the header.
- **Hardware predicates** (`rx_ready`, `dma_tx_active`): `int` 0 / non-zero.
  Invalid id → 0, not -1.

## Example

```c
uint8_t c;
if (bdk_uart_poll_in(BDK_UART_2, &c) == BDK_OK) {
    bdk_uart_write_byte(BDK_UART_2, c);       /* or write_async for IRQ TX */
}
```

API shape only; do not write register bodies.

## DMA

| Role | Name / pattern |
|------|----------------|
| Stream setup (any peripheral) | `bdk_dma_config`, `bdk_dma_start`, `bdk_dma_stop`, `bdk_dma_busy`, `bdk_dma_irq_enable` (`BDK_DMA_IT_TC` \| `BDK_DMA_IT_TE`), `bdk_dma_irq_handler` |
| USART ↔ DMA mapping (caller supplies RM table) | `bdk_uart_dma_bind`, `bdk_uart_dma_t`, then `bdk_uart_write_dma` / `read_dma` |
| Stream id | `BDK_DMA1_STREAM(n)`, `BDK_DMA2_STREAM(n)`, or `BDK_DMA_STREAM(ctrl, n)` |
| App IRQ | `DMAx_StreamN_IRQHandler` → `bdk_dma_irq_handler(&stream)` |

Do **not** hardcode a single stream/channel inside `bdk_dma` or name the DMA
module after one board. Details: `.cursor/skills/bdk-library-design/SKILL.md`.
