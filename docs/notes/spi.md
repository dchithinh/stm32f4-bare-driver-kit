# SPI notes

Master only for now. GPIO (SCK, MOSI, MISO, **CS**, LCD **D/C**, **RST**) is configured in the app. The driver does **software NSS** (SSM/SSI); it does not toggle a hardware NSS pin.

RM0090: SPI chapter (F4). Event flags: **TXE**, **RXNE**, **BSY**, **OVR**. DMA: **TXDMAEN** / **RXDMAEN** in CR2.

## Layers (LCD)

| Layer | Owns |
|-------|------|
| `bdk_spi_*` | Instance, mode, frames, IRQ TX, DMA enable + bind |
| `bdk_dma_*` | Stream/channel from the RM request map (app supplies) |
| App / LCD | Pins, CS around each command, D/C, init sequence, pixel format |

Do not put ILI9341/ST7735 command tables in `src/bdk_spi.c`.

## What the app passes to `bdk_spi_init`

| Field | Meaning |
|-------|---------|
| `id` | `BDK_SPI_1` = 0 … `BDK_SPI_3` |
| `baud` | `BDK_SPI_BAUD_DIVn` → PCLK / *n* (SPI1 on APB2, SPI2/3 on APB1) |
| `cpol` / `cpha` | Mode 0 = LOW + 1EDGE (many panels); check the LCD datasheet |
| `width` | 8-bit commands; some panels want 16-bit pixels (`bdk_spi_set_width`) |

Clock: `bdk_rcc_spi_clk_enable(id)` from init. After `bdk_rcc_sysclk_init()` (64 MHz, APB2 64 MHz), SPI1 `BDK_SPI_BAUD_DIV4` is **16 MHz** SCK. Use `bdk_rcc_get_pclk2_hz()` if you change the tree.

## Suggested implement order

1. **Polling:** `init` → `transfer` → `transfer_buf` / `write` / `read`. Prove with a scope (SCK + MOSI) or a loopback (MOSI↔MISO).
2. **IRQ:** `irq_enable`, app `SPIx_IRQHandler` → `bdk_spi_irq_handler(id)`, then `write_async` + `tx_active`.
3. **DMA:** app `bdk_dma_config` + `bdk_spi_dma_bind` (RM stream/channel), then `write_dma` for fills / framebuffers. Wait on `bdk_spi_dma_tx_active` or DMA TC in the app.

## LCD usage (app, not driver)

```
CS low → D/C = cmd → bdk_spi_write(cmd, 1)
       → D/C = data → bdk_spi_write(args, n)   /* or write_dma for pixels */
CS high
```

Hold CS until **BSY** / DMA TC / `tx_active` is idle so the last bits leave the shift register.

TX-only still generates clocks on MISO; drain **RXNE** (or RX DMA) or you will **OVR**.

## IRQ / DMA (app vectors)

- `SPI1_IRQHandler` → `bdk_spi_irq_handler(BDK_SPI_1)` (same for SPI2/3). Never define those names in `src/bdk_*.c`.
- `DMAx_StreamN_IRQHandler` → `bdk_dma_irq_handler(&stream)` after bind.

## Lab vs portable

F407 DMA request rows live in the example, not in `bdk_dma`. Other F4 parts can differ.

Lab `examples/spi/dma/` (SPI1):

| Direction | Controller | Stream | CHSEL |
|-----------|------------|--------|-------|
| TX | DMA2 | 3 | 3 |
| RX | DMA2 | 0 | 3 |
