# spi_dma

**Purpose:** SPI1 master **write via DMA** + GPIO CS (same pins as `spi_write`).

**What it proves:** One CS-low burst of 16 bytes (`00 55 AA FF` × 4) with **continuous 16 MHz SCK** (~8 µs). PicoScope: mode 0, CS low; interval ≪ 62.5 ns.

| Signal | Pin |
|--------|-----|
| SCK / MOSI / MISO / CS | PA5 / PA7 / PA6 / PA4 |

F407 **DMA2** (RM request map — change if your part differs):

| SPI1 | Stream | CHSEL |
|------|--------|-------|
| TX | 3 | 3 |
| RX | 0 | 3 (dummy drain, `mem_inc` 0) |

No DMA IRQ: `main` waits on `bdk_spi_dma_tx_active` (includes **BSY** before CS rises).
