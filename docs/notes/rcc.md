# RCC notes

## Clock tree: `bdk_rcc_clock_init()`

The SDK exposes one SYSCLK setup entry point. The **app** fills **`bdk_rcc_clock_config_t`** (oscillator, PLL M/N/P/Q, AHB/APB divisors) and calls **`bdk_rcc_clock_init()`**.

Set **`sysclk_src`** to **`BDK_RCC_SYSCLK_PLL`** (default 0), **`BDK_RCC_SYSCLK_HSI`**, or **`BDK_RCC_SYSCLK_HSE`**. PLL fields are used when the source is PLL (or for `bdk_rcc_pll_enable` only).

The driver sets **FLASH wait states** from **HCLK**, applies bus prescalers, switches SYSCLK, and updates `SystemCoreClock`.

**`bdk_rcc_*_clk_enable()`** only gates peripheral clocks; it does not configure PLL or prescalers.

Board-specific presets live in **examples** (e.g. `examples/spi/clock_config.h`, `examples/usb/usb_example/clock_config.h`), not in `inc/bdk_rcc.h`.

### HSI → 64 MHz (SPI lab)

See `examples/spi/clock_config.h` (`SPI_EXAMPLE_CLOCK_HSI_64MHZ`).

```c
bdk_rcc_clock_config_t clk = SPI_EXAMPLE_CLOCK_HSI_64MHZ;
BDK_ASSERT_OK(bdk_rcc_clock_init(&clk));
```

### HSE → 168 MHz + 48 MHz USB (Discovery)

See `examples/usb/usb_example/clock_config.h`.

### HSI 16 MHz SYSCLK (MCO lab)

```c
bdk_rcc_clock_config_t sysclk = {
    .sysclk_src = BDK_RCC_SYSCLK_HSI,
    .ahb_div = 1, .apb1_div = 1, .apb2_div = 1,
};
bdk_rcc_clock_init(&sysclk);
bdk_rcc_pll_enable(&pll_cfg);  /* optional: PLL for MCO1 only */
```

### MCO

- **MCO1 (PA8):** `bdk_rcc_mco1(src, pre_div)` — `pre_div` 1..5. Sources: HSI, LSE, HSE, PLL.
- **MCO2 (PC9):** `bdk_rcc_mco2_sysclk()` — SYSCLK, ÷1.

F407 **MCO1 cannot select SYSCLK**; use MCO2 for SYSCLK on the pin.

### Query after init

`bdk_rcc_get_sysclk_hz()`, `bdk_rcc_get_hclk_hz()`, `bdk_rcc_get_pclk1_hz()`, `bdk_rcc_get_pclk2_hz()` read RCC registers (UART baud, I2C CR2 FREQ, etc.).

### F407 PLL reminders

- HSI PLL input = **HSI / 2** (8 MHz when HSI = 16 MHz).
- `pllp`: 0 → ÷2, 1 → ÷4, 2 → ÷6, 3 → ÷8 on SYSCLK.
- `pllq`: 0 = do not change PLLQ; 2..15 for USB/SDIO (48 MHz typical: VCO / Q).
