# stm32f4-bare-driver-kit

## What this is

A hand-written, register-level driver library for STM32F4 (primary target:
STM32F407), built without ST's HAL or LL libraries. The goal is not
production-grade code — the goal is for me to deeply understand every line by
reading the Reference Manual (RM0090) and CMSIS device headers myself, rather
than calling pre-built abstractions.

Philosophy: thin, readable, close to the hardware, functions named after
what they actually do, no hidden magic.

## Why this project exists (context for the AI assistant)

I already have solid MCU experience (application layer, bootloader,
bare-metal, RTOS, common protocols). I got stuck at a plateau where I could
build things fast but couldn't explain *why* they worked at the register
level, because vendor HALs and AI-assisted coding abstracted too much away.
This project is specifically designed to rebuild that missing depth.

**This changes how I want AI assistance to work on this project — see the
"How I want AI to help" section below. This is the most important part of
this file.**

## Target hardware

- Primary: STM32F407 (Discovery board / custom board)
- Toolchain: arm-none-eabi-gcc, no IDE-generated code (no CubeMX, no HAL)
- Build system: **CMake**. The SDK is a CMake library target, and each
  application (example) is its own small CMake project that imports the SDK
  via a single `.cmake` import file, then just calls `add_executable` +
  `target_link_libraries`. No per-example Makefile hand-editing, no
  toolchain boilerplate duplicated in every example.
- Debug/verification tools available: multimeter, logic analyzer (8ch),
  oscilloscope, USB power meter — used to verify actual hardware behavior,
  not just "it compiles and seems to work"

## Design principles

1. **No HAL, no LL.** Only CMSIS device headers (register struct definitions
   and base addresses) are used as-is. Everything else is written from
   scratch based on RM0090.
2. **Prefix convention:** all public functions/types use the `bdk_` prefix
   (Bare Driver Kit) to avoid symbol collisions with HAL/other libraries and
   to make the code's origin obvious when grepping.
3. **Portable core vs chip-specific code are kept separate.** Peripherals
   with identical register layout across the F4 family (GPIO, USART, basic
   I2C/SPI) go in "core" files. Anything with F407-specific quirks (clock
   tree specifics, peripheral availability) is isolated and clearly marked
   — never assume it works on other F4 chips without verification.
4. **Every driver is verified by measurement**, not just "it looks right."
   GPIO timing/toggle rate checked on oscilloscope, UART framing/baud rate
   checked on logic analyzer, I2C transactions checked for correct
   START/ACK/STOP, etc.
5. **Documented as I learn, not after.** Each peripheral module has a short
   note in `docs/notes/` capturing what I learned reading the RM — which
   bits matter, what confused me initially, what I verified on hardware.

## Planned structure (CMake)

The SDK itself is a CMake library. Each example is a standalone CMake
project that includes `bdk_sdk_import.cmake`, then `add_executable` +
`target_link_libraries(... bdk_stm32f4)`.

```
stm32f4-bare-driver-kit/
├── CMakeLists.txt                  # top-level: defines the bdk_stm32f4 library target
├── bdk_sdk_import.cmake            # the file examples include to pull in the SDK
├── cmake/
│   ├── toolchain-arm-none-eabi.cmake   # cross-compiler toolchain file
│   └── stm32f407.cmake                  # chip-specific flags, linker script, startup source
├── inc/
│   ├── bdk_gpio.h
│   ├── bdk_rcc.h
│   ├── bdk_uart.h
│   ├── bdk_i2c.h
│   ├── bdk_spi.h
│   └── bdk_timer.h
├── src/
│   ├── bdk_gpio.c
│   ├── bdk_rcc.c
│   ├── bdk_uart.c
│   ├── bdk_i2c.c
│   ├── bdk_spi.c
│   └── bdk_timer.c
├── startup/
│   └── startup_stm32f407xx.s       # vector table, reset handler
├── linker/
│   └── stm32f407.ld
├── cmsis/                          # unmodified CMSIS-Core + stm32f407xx.h (not HAL)
├── examples/
│   ├── 01_blink/
│   │   ├── CMakeLists.txt          # ~10 lines: include bdk_sdk_import.cmake, add_executable, link
│   │   └── main.c
│   ├── 02_uart_echo/
│   │   ├── CMakeLists.txt
│   │   └── main.c
│   ├── 03_i2c_scanner/
│   │   ├── CMakeLists.txt
│   │   └── main.c
│   └── 04_pwm_timer/
│       ├── CMakeLists.txt
│       └── main.c
├── docs/
│   └── notes/
│       ├── gpio.md
│       ├── rcc.md
│       ├── uart.md
│       └── i2c.md
└── README.md
```

**How an example's `CMakeLists.txt` should look (minimal):**

```cmake
cmake_minimum_required(VERSION 3.20)
include(${CMAKE_CURRENT_LIST_DIR}/../../bdk_sdk_import.cmake)

project(blink C ASM)

add_executable(blink main.c)
target_link_libraries(blink bdk_stm32f4)

bdk_add_extra_outputs(blink)   # generates .bin/.hex, prints size
```

This is the pattern to replicate: the *application* CMakeLists.txt should
be almost trivial — all toolchain setup, chip flags, linker script
selection, and startup file live once in the SDK's own CMake files, not
copy-pasted into every example.

## Roadmap (rough order)

0. CMake skeleton: toolchain file, top-level `CMakeLists.txt` defining the
   `bdk_stm32f4` library target, `bdk_sdk_import.cmake`, and a working
   `01_blink` example project that at least builds an empty `main()` and
   produces a `.bin`/`.hex` — this is boilerplate, fine to get AI help
   setting this up in full (see AI policy below)
1. Minimal boot: startup file, vector table, linker script, clock init
   (RCC) — get to `main()` with known clock config
2. `bdk_gpio` — init, set, clear, toggle, read; verify on oscilloscope
3. SysTick-based blocking delay
4. `bdk_uart` — polling TX/RX first, then interrupt-driven with ring
   buffer; verify framing/baud on logic analyzer
5. `bdk_i2c` — polling master mode; verify transactions on logic analyzer
   using a real sensor (e.g. MPU6050 or BMP280)
6. `bdk_spi` — master mode; verify with SD card or similar
7. `bdk_timer` — PWM output, input capture; verify on oscilloscope
8. DMA basics for UART/ADC
9. A minimal round-robin task switcher (PendSV-based) written from scratch
   — no FreeRTOS — to understand context switching at the register level
10. Applications built on top of the above (e.g. a sensor data logger),
    with matching custom-designed PCB in KiCad

## Longer-term direction (context only, not immediate scope)

- Possibly extend the same `bdk_` API surface to RP2040 and ESP32,
  to compare architectures and separate "universal MCU concepts" from
  "vendor-specific quirks"
- Eventually pair each application with a self-designed schematic (KiCad),
  not just a dev board

---

## How I want AI assistance to work on this project (read this carefully)

**The entire point of this project is for me to build understanding by
struggling with the Reference Manual and hardware measurements myself.** If
AI writes the actual driver implementation for me, the project fails at its
purpose even if the code works. So the division of labor is strict:

### AI (Cursor) SHOULD:
- Generate **function skeletons only**: function signatures, parameter
  names/types, return types, and Doxygen-style comments describing *what*
  a function does (not *how*) — e.g. `@brief`, `@param`, `@return`.
- Suggest what functions a module *should* probably have (e.g. "a GPIO
  driver typically needs init, set, clear, toggle, read, and maybe
  interrupt config") — architecture-level suggestions, not implementation.
- Suggest struct/type organization (e.g. a `bdk_gpio_config_t` struct) —
  still architecture, not register logic.
- Set up boilerplate that has no learning value: the CMake build system
  itself (toolchain file, top-level `CMakeLists.txt`, `bdk_sdk_import.cmake`,
  per-example `CMakeLists.txt` templates), linker script skeleton, folder
  structure, `.gitignore`, README formatting.
- Review code AFTER I've written it, pointing out bugs/edge cases/style
  issues — but not rewriting it for me. Explain the *why* of an issue, let
  me fix it myself.
- Answer conceptual questions about *why* something works a certain way in
  general (e.g. "why does I2C need pull-up resistors", "what is clock
  stretching") — but NOT "here's the code for I2C on STM32F4."

### AI (Cursor) should NOT:
- Write the body/implementation of any driver function (register reads/
  writes, bit manipulation, peripheral configuration sequences). This is
  the part I must derive myself from RM0090.
- Provide "example implementations" or "reference code" for a peripheral
  driver, even when I'm stuck — instead, point me to which section/table
  of RM0090 answers my question, or ask me what I've tried and observed on
  the oscilloscope/logic analyzer so far.
- Auto-complete register-level logic inline. If I'm mid-way through writing
  a register manipulation and pause, don't finish that line for me.

### If I ask something that violates the above:
Push back and remind me of this policy rather than complying — e.g. "This
looks like a request for the actual driver logic — per the project's
ground rules, that part should come from you reading RM0090. Want me to
point you to the relevant register table instead?"

## Current status

CMake skeleton is in place: toolchain file, top-level `bdk_stm32f4` library,
`bdk_sdk_import.cmake`, linker/startup so an image actually links, and an
empty `01_blink` that builds `.bin`/`.hex`. Driver `.c` files are **empty
stubs** (signatures only). Next: `bdk_rcc` + `bdk_gpio` blink implemented
from RM0090 and verified on oscilloscope.