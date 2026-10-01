# Examples

Grouped by **driver**, then by **app**. Each app’s `README.md` has **Purpose** and **What it proves** in detail.

```
examples/
  <driver>/
    <app>/
      main.c
      README.md
      captures/
```

Build from `build/`: `cmake --build . --target <app_target>` (target name in each README).

## Index

| Target | App | Purpose | What it proves |
|--------|-----|---------|----------------|
| `gpio_blink` | [`gpio/blink`](gpio/blink/) | GPIO output + toggle | PD12/PD14 LEDs blink (~1 s); GPIO clock and pin config work |
| `rcc_mco` | [`rcc/mco`](rcc/mco/) | Route **SYSCLK** to **MCO2** (PC9) | ~16 MHz on PC9 (HSI); RCC mux, not GPIO toggling |
| `uart_tx` | [`uart/tx`](uart/tx/) | Polling **TX** (`bdk_uart_write`) | `HELLO!` on USART2 @ 9600 |
| `uart_echo` | [`uart/echo`](uart/echo/) | Polling RX + TX echo | Typed characters echo on PA2/PA3 |
| `uart_irq_echo` | [`uart/irq_echo`](uart/irq_echo/) | **RXNE** IRQ + `poll_in` | RX without polling **RXNE** in main; same echo as `echo` at human speed |
| `uart_irq_tx` | [`uart/irq_tx`](uart/irq_tx/) | **TXE** IRQ + `write_async` | `ping` lines ~1 Hz without spinning on **TXE** in main |
| `uart_ring_drop` | [`uart/ring_drop`](uart/ring_drop/) | Stress 64-byte IRQ RX ring | `ring_drop_count` rises on fast paste; `ore_count` for UART overrun |
| `uart_dma` | [`uart/dma`](uart/dma/) | **`bdk_uart_write_dma`** (stub) | API links; `BDK_ERR_NOT_IMPL` until UART DMA layer exists |
| `uart_dma_poll` | [`uart/dma_poll`](uart/dma_poll/) | Raw **DMA** TX, no TC IRQ | Must **`bdk_dma_busy()`**; prints `busy_poll_loops` |
| `uart_dma_irq` | [`uart/dma_irq`](uart/dma_irq/) | Raw **DMA** TX + TC IRQ | Main waits on **`dma_tx_done`**; never polls **`bdk_dma_busy()`** |
| `dma_m2m` | [`dma/m2m`](dma/m2m/) | **Mem→mem** DMA + TC IRQ | Hex dump before/after; **`mismatches=0`**; wait on **`dma_m2m_done`** |
| `i2c_scanner` | [`i2c/scanner`](i2c/scanner/) | 7-bit I2C bus scan | Prints ACKed addrs on UART; needs **`bdk_i2c`** in `src/bdk_i2c.c` |
| `spi_write` | [`spi/write`](spi/write/) | Polling SPI1 + GPIO CS | 4-byte MOSI burst; needs **`bdk_spi`** in `src/bdk_spi.c` |
| `spi_irq_tx` | [`spi/irq_tx`](spi/irq_tx/) | SPI1 **`write_async`** | Same 4 bytes from **TXE IRQ**; CS until `tx_active` |
| `spi_dma` | [`spi/dma`](spi/dma/) | SPI1 **`write_dma`** | 16-byte burst, continuous SCK; DMA2 S3 TX / S0 RX CH3 |
| `timer_pwm` | [`timer/pwm`](timer/pwm/) | PWM (placeholder) | Build only today; future: duty/frequency on scope |

UART wiring (Discovery): USART2 **PA2** TX, **PA3** RX, USB–UART, common GND — see [`uart/README.md`](uart/README.md) and `docs/notes/uart.md`.

DMA stream setup for USART2 TX labs: `docs/notes/dma.md`.
