# spi_lcd_ili9341

**Purpose:** Drive a **2.4" ILI9341** (240×320, RGB565) on **SPI2** using `bdk_spi_write` and GPIO for **CS**, **D/C**, and **RST**.

**Hardware reference:** Cube project `stm32f407xx_spi_lcd_2.4inch` (`bsp/lcd/hw_def.h`). Same display SPI pins; **touch (XPT2046 on SPI1) is not used** here.

## Wiring (MCU ↔ panel)

| STM32F407 | `hw_def.h` | ILI9341 module |
|-----------|------------|----------------|
| **PB13** | LCD_SCLK | SCK |
| **PB15** | LCD_MOSI | SDI / MOSI |
| **PC2** | LCD_MISO | SDO (optional read; configured for 2-line SPI) |
| **PB9** | LCD_CS | CS |
| **PD9** | LCD_DCX | D/C |
| **PD10** | LCD_RESX | RESET |
| **3V3** / **5V** | board dependent | VCC |
| **GND** | GND | GND |

**Not in this demo (touch):** SPI1 **PA5/PA6/PA7**, XPT2046 CS on **PA15** — see `bsp/lcd/tsc/XPT2046.c` in the Cube tree.

**Clock / SPI:** HSE **168 MHz** SYSCLK when possible (else 84 MHz, else HSI 64 MHz). **SPI2** uses **`ILI9341_BOARD_SPI_BAUD`** (`BDK_SPI_BAUD_DIV2`) — on F407 this is the **fastest** setting: **SCK = PCLK1 / 2** (PCLK1 is capped at **42 MHz**, so **~21 MHz** SCK). Raising SYSCLK past 84 MHz does not raise SPI2 SCK; only a slower `BDK_SPI_BAUD_DIVn` or moving the LCD to **SPI1** (APB2, up to **~42 MHz** SCK) would change wire speed. All bytes go through **`bdk_spi_write`** (8-bit); CS/D/C/RST stay in `ili9341.c` (see `docs/notes/spi.md`).

**Init order:** `ili9341_gpio_init` → `bdk_spi_init` → `ili9341_init` → demo loop in `main.c`.

**Demo loop:** portrait — full red / green / blue, then RGB bars; landscape — same; brief white/black flash; repeat. Timings: `DEMO_*_MS` in `main.c`.

## Build

```bash
cmake --build build --target spi_lcd_ili9341
```

Flash `spi_lcd_ili9341.bin` (or `.elf`) to the Discovery.

## What you should see

Full-panel color bars: window **(0…W−1, 0…H−1)** (`ILI9341_LCD_DRAW_*`, default **landscape 320×240**). Drawing always uses **one color per scanline** (split on **y** into thirds of **H**). **Portrait** MADCTL → **horizontal** red / green / blue bands; **landscape** MADCTL (**MV**) → same RAM pattern shows as **vertical** columns. Landscape MADCTL uses **RGB** (not BGR) so hues match portrait. One **CS-low** frame after `0x2C`.

## Software layout

| File | Role |
|------|------|
| `ili9341_board.h` | Pin macros (`hw_def.h` mapping) |
| `ili9341.c` | Reset, init table, window, RGB565 fill |
| `main.c` | Clock, SPI2, demo loop |

Command tables and panel geometry stay in the **example**, not in `src/bdk_spi.c` (see `docs/notes/spi.md`).

## Troubleshooting

| Symptom | Check |
|---------|--------|
| White / blank screen | Power/GND; **PD10** reset pulse; try `BDK_SPI_BAUD_DIV8` |
| Wrong colors / mirrored | Tweak **MADCTL** in `ili9341.c` (default `0xC0` = Cube portrait) |
| Garbled pixels | Scope **PB13/PB15**; AF5 **SPI2**, not SPI1 touch pins |
| Works in Cube, not BDK | Confirm **PB9** CS (not HAT **PB8** or lab **PA4**) |
