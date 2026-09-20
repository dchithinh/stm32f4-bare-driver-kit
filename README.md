# stm32f4-bare-driver-kit

Hand-written, register-level drivers for **STM32F407**, without ST HAL or LL.
CMSIS device headers are used as-is; everything else is derived from RM0090.

The SDK is a CMake library (`bdk_stm32f4`). Each example is a tiny CMake
project that includes `bdk_sdk_import.cmake`, then `add_executable` and
`target_link_libraries`.

Public APIs use the `bdk_` prefix. Driver **bodies** are empty on purpose —
fill them in from the Reference Manual, then verify on a scope or logic analyzer.
See `project.md` for goals, roadmap, and how AI assistance is supposed to work.

## Requirements

- CMake >= 3.20
- `arm-none-eabi-gcc` (objcopy, size)

## Build all examples from the SDK root

```sh
cmake -S . -B build
cmake --build build
```

Outputs land in `build/examples/<driver>/<app>/` (`*.elf`, `*.bin`, `*.hex`,
`*.map`).

## Build one example on its own

```sh
cmake -S examples/gpio/blink -B build-blink
cmake --build build-blink
```

That CMakeLists is intentionally small: import the SDK, `add_executable`,
`target_link_libraries(... bdk_stm32f4)`, `bdk_add_extra_outputs`.

Examples are grouped by driver (`examples/gpio/`, `examples/i2c/`, …). Each
app can store oscilloscope or analyzer shots in `captures/` — see
`examples/README.md`.

## Layout

| Path | What |
|------|------|
| `inc/`, `src/` | `bdk_*` headers (API) and empty driver sources |
| `cmake/` | Toolchain, F407 flags, `bdk_add_extra_outputs` |
| `bdk_sdk_import.cmake` | The one file examples include |
| `startup/`, `linker/` | Reset/vector table and F407VG memory map |
| `cmsis/` | Unmodified CMSIS-Core + `stm32f407xx.h` (vendor headers, not HAL) |
| `examples/<driver>/<app>/` | Apps; start with `examples/gpio/blink` |
| `docs/notes/` | Per-peripheral notes written while reading the RM |

## Next

1. `bdk_rcc` clock init so `main()` runs at a known SYSCLK (RM0090 RCC).
2. `bdk_gpio` blink, verified on an oscilloscope.
