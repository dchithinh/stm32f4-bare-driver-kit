---
name: bdk-library-design
description: >-
  Layering and portability for stm32f4-bare-driver-kit as a developer SDK (CMake
  bdk_stm32f4). Use when adding DMA, RCC, drivers, examples, or docs; when APIs
  might bake in a board, USART instance, or F407-only tables; or when splitting
  generic hardware control from app/chip configuration.
---

# BDK as a developer library

Consumers **import** the SDK (`bdk_sdk_import.cmake`), link `bdk_stm32f4`, and
call `bdk_*` from their app. Design for **reuse on other boards and F4 parts**,
not only the F407 Discovery lab wiring.

**Default build target:** STM32F407 (CMSIS define, linker, startup, tested
examples). **Public driver APIs** should stay chip-agnostic where the RM block
is the same; **chip and board facts** belong in app config, bind structs, or
`docs/notes/` — not hidden inside generic modules.

## Layering

| Layer | Owns | Does not own |
|-------|------|----------------|
| **Low-level driver** (`bdk_dma`, core `bdk_gpio` register ops) | Any stream/instance the API exposes; clocks for that block | Which USART uses which DMA stream; Nucleo pin numbers |
| **Peripheral driver** (`bdk_uart`, `bdk_spi`) | Instance id, peripheral registers (DR, CR3 DMAT), optional `*_bind()` for resources the RM maps per instance | `DMA1_Stream6` hardcoded for “our UART” |
| **Application** | `*_dma_bind`, GPIO pins, `USARTx_IRQHandler` / `DMAx_StreamN_IRQHandler`, RM table rows for **their** MCU | — |
| **Examples** | One verified board path (document which) | Must not be the only way to configure the library |

## DMA (required pattern)

- **`bdk_dma_*`:** All DMA1/DMA2 streams 0–7; direction, CHSEL, addresses,
  widths, mem↔mem. No USART/SPI/“Discovery” in `inc/bdk_dma.h` or `bdk_dma.c`.
  **No global `dma_init` that enables both controllers** — set `DMA1EN`/`DMA2EN`
  from `bdk_dma_stream_t` inside `bdk_dma_config` (HAL MSP style).
- **Peripheral DMA:** e.g. `bdk_uart_dma_t` + `bdk_uart_dma_bind()` — caller
  supplies TX/RX stream and channel from the RM **DMA request mapping** table.
- **Docs:** Generic module guide first; board/part tables under “Lab example” in
  `docs/notes/dma.md` (or peripheral note), not in the generic `@file` brief.
- **IRQ:** `bdk_dma_irq_handler(stream)` from app vectors only (same rule as
  `bdk_uart_irq_handler`).

## Naming and headers

- `@file` brief: enough to state what the module is for (not rigidly one line).
- Avoid “(F407)” on generic modules; use “F407” only for RCC/linker/notes that
  are genuinely part-specific.
- RM tutorials, stream/channel tables, and “on Discovery we use…” → `docs/notes/`,
  example `README.md`, or example `main.c` comments — not driver headers.

## Peripheral availability

- Do not assume every F4 has six USARTs or the same DMA map. Prefer explicit
  `id` + bind/config structs over silent tables fixed to one chip in `src/`.
- If a table is unavoidable short-term, mark it chip-specific in the note and
  plan `bind` or compile-time device guards for porters.

## CMake / packaging

- One library target `bdk_stm32f4`; examples stay thin (`add_executable` +
  `target_link_libraries`). Document **F407-first** and porting (CMSIS, linker,
  startup) in `README.md` — do not imply “F4” means zero porting work.

## Checklist before merging API or skeleton changes

- [ ] Could another developer use this API on a different USART/DMA stream without editing `src/bdk_dma.c`?
- [ ] Is board-specific data only in examples or caller-supplied structs?
- [ ] Are IRQ vector names only documented for the **app**, not defined in `src/bdk_*.c`?
- [ ] Does `docs/notes/` separate **generic** vs **lab (F407 / board)** sections?

**References:** `docs/notes/dma.md`, `project.md` (portable core vs chip-specific),
`inc/bdk_dma.h`, `inc/bdk_uart.h` (`bdk_uart_dma_bind`).
