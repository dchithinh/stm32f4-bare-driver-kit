# stm32f4xx-bare-driver-kit

A small **STM32F407** driver library built with **CMake**. Register-level `bdk_*` APIs, **no ST HAL or LL**. CMSIS device headers only; the rest follows **RM0090**. Lab examples link `bdk_stm32f4`. Not a HAL replacement and not aimed at production.

## Purpose

- **Touch the registers.** Learn clocks, flags, and bus sequences from RM0090 instead of calling vendor HAL/LL and hoping the Cube project is right.
- **Own the stack for personal projects.** When something is slow, large, or wrong, I know which layer to change (prescaler, polling vs IRQ vs DMA, CS timing) instead of digging through HAL.
- **Optimize only where it matters.** Same API can stay polling for a sensor and use DMA for an LCD fill — the cost is visible, not hidden in a generated driver.
- **Prove it on the wire.** Examples are small labs: scope or analyzer first, then reuse the library in a real app (display, I2C sensor, UART console).
- **Keep the API thin.** `bdk_*` is enough to ship a hobby board; the app still owns pins, IRQ vectors, and DMA stream/channel from the RM table.

## Hardware and tools

- **MCU:** STM32F407 (Discovery and similar F407VG boards)
- **Toolchain:** `arm-none-eabi-gcc`, CMake ≥ 3.20 — no CubeMX, no IDE project files
- **Check:** oscilloscope / PicoScope / logic analyzer, USART2 as a console when needed
- **Debug:** OpenOCD + GDB over ST-Link — [docs/notes/debug.md](docs/notes/debug.md)

## What works today

Status is honest: some modules are used on the bench; others are still stubs.

| Module | State | Notes |
|--------|--------|--------|
| GPIO | In use | Init, set/clear/toggle/read; AF / open-drain |
| RCC | Partial | Peripheral clocks, `SYSCLK` via HSI+PLL (64 MHz) when an example calls `bdk_rcc_sysclk_init` |
| UART | In use | Polling + IRQ RX ring / `write_async`; USART DMA helpers still stub |
| DMA | In use | Stream config/start/IRQ; mem↔mem and USART TX labs |
| I2C | In use | Polling master (`write` / `read`; `write_read` not done) |
| SPI | In use | Master polling, `write_async`, `write_dma`; CS / D/C stay in the app |
| Timer | Stub | PWM example is a placeholder |

Examples (blink, UART echo/IRQ, I2C scan, SPI poll/IRQ/DMA, …) live under [`examples/`](examples/README.md). Each app has a short README: purpose and what you should see on the wire.

## Build

```sh
cmake -S . -B build
cmake --build build
```

Or one target, e.g. `cmake --build build --target spi_write`.

Outputs: `build/examples/<driver>/<app>/*.{elf,bin,hex,map}`.

Standalone example (same SDK import):

```sh
cmake -S examples/gpio/blink -B build-blink
cmake --build build-blink
```

Flash with your usual ST-Link / OpenOCD / `st-flash` flow. Default build type is **Debug** (`-g3 -Og`).

## Layout

| Path | Role |
|------|------|
| `inc/`, `src/` | `bdk_*` API and driver sources |
| `cmake/`, `bdk_sdk_import.cmake` | Toolchain, F407 flags, extra `.bin`/`.hex` |
| `startup/`, `linker/` | Reset, vectors, F407VG memory |
| `cmsis/` | Unmodified CMSIS-Core + device header (not HAL) |
| `examples/<driver>/<app>/` | Labs; optional `captures/` for scope shots |
| `docs/notes/` | What I learned from the RM (flags, mistakes, lab wiring) |

## Using the kit in your project

Put this tree **anywhere**. Point CMake at it with `BDK_SDK_PATH` (cache or environment), include `bdk_sdk_import.cmake` **before** `project()`, link `bdk_stm32f4`, and call `bdk_add_extra_outputs`.

```cmake
cmake_minimum_required(VERSION 3.20)
include(${BDK_SDK_PATH}/bdk_sdk_import.cmake)

project(my_app C ASM)

add_executable(my_app main.c)
target_link_libraries(my_app bdk_stm32f4)
bdk_add_extra_outputs(my_app)
```

```sh
cmake -S . -B build -DBDK_SDK_PATH=/path/to/stm32f4xx-bare-driver-kit
cmake --build build
```

Or: `export BDK_SDK_PATH=/path/to/stm32f4xx-bare-driver-kit` and omit `-D`.

Your app still owns GPIO (including **CS / D/C**), `USARTx_IRQHandler` / `SPIx_IRQHandler` / `DMAx_StreamN_IRQHandler` → `bdk_*_irq_handler(...)`, DMA stream/channel from the RM table (`bdk_dma_config` + `bdk_*_dma_bind`), and board code (LCD, sensors). Start from an example under [`examples/`](examples/README.md).

## License

[MIT](LICENSE) — Copyright (c) 2026 Thinh Do.

CMSIS files keep their own ARM / ST licenses under `cmsis/`.

## Disclaimer

This is a **learning log in code form**. Expect gaps, TODOs, and F407-first assumptions. Use it to study registers and buses; do not treat it as certified or portable to every STM32F4 without reading the manual for that part.