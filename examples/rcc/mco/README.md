# RCC MCO1 — PLL 16 MHz on PA8

## Purpose

**MCO1** on **PA8** outputs **PLLCLK ÷ 5** (16 MHz). **SYSCLK** stays on **HSI** 16 MHz.

- `bdk_rcc_clock_init` + `RCC_MCO_SYSCLK_HSI_16MHZ`
- `bdk_rcc_pll_enable` + `RCC_MCO_PLL_FOR_MCO1` — **PLL from HSE** (`RCC_MCO_HSE_HZ` = 8 MHz on F407 Discovery) → **80 MHz PLLCLK**
- `bdk_rcc_mco1(BDK_RCC_MCO1_PLL, 5)` — MCO1 prescaler uses ST F4 bit patterns (see HAL `RCC_MCODIV_5`)

## What it proves

- ~**16 MHz** on **PA8** (~62.5 ns period), not 80 MHz undivided PLLCLK.
- RCC mux + PLL, not GPIO toggling.

If you still see **~80 MHz**, reflash `rcc_mco` after updating `bdk_rcc_mco1` (÷5 must program `RCC_CFGR_MCO1PRE` as `0x07000000`, not `0x04000000`).

## Probe

- **PA8** vs GND; **50–100 ns/div**.
- **USART2** **PA2** TX @ **115200** (USB–UART RX): after flash, paste the `rcc_mco:` log lines for RCC debug.

## Build

`cmake --build build --target rcc_mco`
