# spi_8x8_scroll

Three MikroElektronika **8x8Y** (MAX7219) modules on **SPI1**, MikroSDK v2-style scroll driver (`c8x8y.c`). Scroll text: **` HELLO WORLD! `** (edit `k_scroll_msg` in `main.c`).

**Reference:** [mikrosdk_click_v2 `clicks/8x8y`](https://github.com/MikroElektronika/mikrosdk_click_v2/tree/master/clicks/8x8y)

Board: **STM32F407 Discovery**. Cube/HAL twin: `spi_8x8` (parallel CS, same pins).

---

## Pin connections (keep for wiring)

Wiring mode: **parallel CS** (`C8X8Y_WIRING_PARALLEL_CS`) — one **LOAD/CS** per module; **DIN star** from MCU MOSI (do **not** chain SDO→DIN unless you switch to daisy mode in code).

### SPI1 (shared by all modules)

| STM32F407 signal | MCU pin | 8x8Y / mikroBUS | Notes |
|------------------|---------|-----------------|--------|
| SPI1_SCK | **PA5** | **SCK** | |
| SPI1_MOSI | **PA7** | **SDI** (DIN) | Tie **PA7 to SDI on every board** |
| SPI1_MISO | **PA6** | SDO | Not used by this app (no MISO traffic) |
| — | **5V** | **5V** | Power each module |
| — | **GND** | **GND** | Common ground |

### Chip select (LOAD, idle **high**)

| Module | MCU pin | Discovery hint |
|--------|---------|----------------|
| #1 | **PA4** | Default SPI1 NSS; reclaim from I2S3 if Cube used PA4 |
| #2 | **PB0** | Morpho **P1** pin **22** |
| #3 | **PB1** | Morpho **P1** pin **21** |

CS GPIO is configured in `c8x8y_cs_add()`; pin macros live in **`c8x8y_board.h`**.

### Debug UART (optional)

| STM32F407 | Pin | Use |
|-----------|-----|-----|
| USART2_TX | **PA2** | Log TX → ST-LINK **virtual COM** (USB), no extra wire |
| USART2_RX | **PA3** | RX (only if you send data to the MCU) |

Terminal: **`C8X8Y_DEBUG_UART_BAUD`** (default **9600** 8N1). Boot banner includes `ker=16000000 Hz` when clock is 16 MHz HSI.

---

## Runtime behavior

1. **Clock:** 16 MHz **HSI** (`SPI_EXAMPLE_CLOCK_HSI_16MHZ` in `../clock_config.h`).
2. **SPI1:** mode 0, **1 MHz** SCK on PA5 (`BDK_SPI_BAUD_DIV16` in `main.c`).
3. **Boot:** init all three panels → **blank** all → **300 ms** → show **1**, **2**, **3** (one digit per panel, **1 s** each via `C8X8Y_BOOT_DIGIT_MS`).
4. **Scroll:** **24×8** marquee across the row of three modules (`C8X8Y_SCROLL_WIDE_PANELS 1`). Speed **`C8X8Y_SPEED_SLOW`** with delay **`C8X8Y_SPEED_SLOW_MS`** from `c8x8y_board.h`.

---

## Tunables (`c8x8y_board.h`)

| Macro | Default | Meaning |
|-------|---------|---------|
| `C8X8Y_INTENSITY_LEVEL` | **3** | MAX7219 brightness **0…15** |
| `C8X8Y_SPEED_SLOW_MS` | **90** | Ms per scroll step when using `C8X8Y_SPEED_SLOW` |
| `C8X8Y_BOOT_DIGIT_MS` | **1000** | Ms to hold each boot digit **1 / 2 / 3** |
| `C8X8Y_SCROLL_WIDE_PANELS` | **1** | **1** = 24-col strip; **0** = same 8×8 on each panel |
| `C8X8Y_DEBUG_LOG` | **0** | **1** = USART2 logs (slows scroll) |
| `C8X8Y_DEBUG_UART_BAUD` | **9600** | USART2 baud |

Scroll preset in **`main.c`:** `c8x8y_set_speed_scroll(&dev, C8X8Y_SPEED_SLOW)` — switch to `C8X8Y_SPEED_MEDIUM` (~25 ms) or `C8X8Y_SPEED_FAST` (~12 ms) in `c8x8y_scroll_delay()`.

Busy-wait timing: **`C8X8Y_MS_LOOPS`** in `main.c` (default **2000** @ 16 MHz HSI). Increase if delays feel too short; decrease if too long.

SPI SCK: change `cfg.baud` in `main.c` (`BDK_SPI_BAUD_DIV16` → 1 MHz at 16 MHz SYSCLK).

---

## Build & flash

```bash
cmake --build build --target spi_8x8_scroll
# flash build/examples/spi/8x8_scroll/spi_8x8_scroll.bin
```

## Serial debug tips

- Use the **ST-LINK Virtual COM** port on Windows (WSL2 often does not see USB serial).
- Garbled text → terminal baud does not match `C8X8Y_DEBUG_UART_BAUD`.
- Set `C8X8Y_DEBUG_LOG 1` to trace init/scroll in the driver (rebuild required).
