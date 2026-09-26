# UART notes

USART chapter in the reference manual — capture your own measurements and “aha” moments below.
This section documents the **bdk_uart** API shape (see `inc/bdk_uart.h`).

## API layers

| Layer | Functions | Thread behavior |
|-------|-----------|-----------------|
| **Polling** | `read_byte`, `write_byte`, `read`, `write`, `rx_ready` | Main waits on **SR** (RXNE / TXE) |
| **IRQ setup** | `irq_enable`, app `USARTx_IRQHandler` → `irq_handler` | NVIC + **RXNEIE**; **TXEIE** only during async TX |
| **IRQ RX (app)** | `poll_in` | One byte from **software ring**; no wait; `BDK_ERR_NODATA` if empty |
| **IRQ TX (app)** | `write_async`, `tx_active` | Queue buffer; ISR sends on **TXE**; `BDK_ERR_BUSY` if already active |

`poll_in` is **not** “polling mode.” It **tries once** to read a byte the ISR
already stored in the ring. There is no `poll_out` yet; bulk IRQ TX uses
`write_async` instead of a one-byte non-blocking name.

## Naming: `poll_in` vs `write_async`

- **RX:** hardware keeps arriving → ISR must drain **DR** → ring → app uses
  **`poll_in`** (one byte per call).
- **TX:** app chooses when to send → **`write_async(buf, len)`** hands a buffer
  to the ISR (multi-byte async TX, not per-byte `poll_out`).

Echo with IRQ RX + IRQ TX: `poll_in` + `write_async` (keep buffer alive until
`!tx_active`). IRQ RX + polling TX: `poll_in` + `write_byte` (see `irq_echo`).

## RX overrun (your TODO)

| Counter | Meaning | Where |
|---------|---------|--------|
| `ore_count` | Hardware **ORE** — DR not read before next byte | Clear in `uart_rx_isr` per reference manual |
| `ring_drop_count` | Software ring full (`uart_rx_push`) | Already incremented |

App: `bdk_uart_rx_stats_get` / `bdk_uart_rx_stats_reset`. Counters cleared on
`bdk_uart_irq_enable` (via `uart_rx_reset`).

Implement in `uart_rx_isr`: after the RXNE loop, if **ORE** set → `ore_count++`,
then flag clear sequence (typically read **SR**, read **DR**).

## Bring-up checklist (hardware)

- Baud-rate formula vs PCLK (USART2 on APB1 when you add PLL), OVER8, BRR layout
- **TXE** vs **TC**, **RXNE** vs **ORE**
- Logic analyzer: start bit, 8N1, measured bit time

## Examples

| Target | RX | TX |
|--------|----|----|
| `uart_echo` | polling | polling |
| `uart_irq_echo` | IRQ + `poll_in` | polling `write_byte` |
| `uart_irq_tx` | (unused) | IRQ + `write_async` |
| `uart_ring_drop` | IRQ + paste test | — |
