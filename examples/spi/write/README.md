# spi_write

**Purpose:** Polling SPI1 master + GPIO CS (LCD-style framing).

**What it proves:** After `bdk_spi_init` / `bdk_spi_write` are implemented, SCK/MOSI show a 4-byte burst (`00 55 AA FF`) with CS low, then CS high. Scope: PA5 / PA7 / PA4.

Pins (change in `main.c`): **PA5** SCK, **PA7** MOSI, **PA6** MISO, **PA4** CS. Mode 0, DIV16.

IRQ/DMA come later (`write_async`, `write_dma`); see `docs/notes/spi.md`.
