# spi_irq_tx

**Purpose:** SPI1 **`write_async`** + `SPI1_IRQHandler` → `bdk_spi_irq_handler`.

**What it proves:** Same 4-byte MOSI pattern as `spi_write` (`00 55 AA FF`), but bytes are clocked from the **TXE/RXNE ISR**, not a poll loop. CS stays low until `bdk_spi_tx_active` is 0.

Pins and mode: same as `spi_write` (PA5/PA7/PA6/PA4, mode 0, **SCK 16 MHz**). PicoScope: decode while CS low; sample interval ≪ 62.5 ns.

SCK may pause slightly between bytes (ISR latency); DMA (`spi_dma`) is the continuous-clock path.
