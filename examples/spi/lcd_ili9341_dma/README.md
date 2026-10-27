# spi_lcd_ili9341_dma

Same board, wiring, and demo loop as [`lcd_ili9341`](../lcd_ili9341/README.md). **Difference:** after `0x2C`, each scanline uses **SPI2 TX DMA** via **`ili9341_board_dma_write`** (8-bit SPI, byte DMA, RGB565 as high/low bytes). MISO bytes are **drained in software** while DMA runs (no RX DMA — avoids stuck dual-DMA on this panel). Commands still use polling **`bdk_spi_write`**.

## DMA (F407, SPI2 TX only)

| Direction | Controller | Stream | CHSEL |
|-----------|------------|--------|-------|
| TX | DMA1 | 4 | 0 |

Configured in `ili9341_board_dma.c`. Synchronous wait per scanline (no DMA IRQ).

**Init order:** `ili9341_gpio_init` → `bdk_spi_init` → **`ili9341_board_dma_init`** → `ili9341_init` → demo loop.

## Build

```bash
cmake --build build --target spi_lcd_ili9341_dma
```

Flash `spi_lcd_ili9341_dma.bin`. Visually it should match `spi_lcd_ili9341`; CPU spends less time in the pixel inner loop.

## Toggle

| Macro (`ili9341_board.h`) | Effect |
|---------------------------|--------|
| `ILI9341_PIXEL_SPI_DMA` `0` | Revert pixels to polling (still build DMA init unless removed from `main.c`) |
| `ILI9341_PIXEL_SPI_16BIT` | Keep **`0`** for this target (byte DMA); polling `lcd_ili9341` may use `1` |
