# GPIO driver — implementation steps

Work through these in order. Fill in `src/bdk_rcc.c` / `src/bdk_gpio.c` /
`examples/gpio/blink/main.c` yourself from RM0090. This page is a checklist,
not a register recipe.

API already exists: `inc/bdk_gpio.h`, `inc/bdk_rcc.h`.

Skip for this pass: alternate function (`af`), analog, EXTI, full PLL clock
tree (`bdk_rcc_sysclk_init`). Reset clock is HSI 16 MHz — enough to toggle a
pin. Capture what you learn in the “Notes after implementing” section at the
bottom.

---

## 0. Read first (before any writes)

| Doc | What to extract |
|-----|-----------------|
| RM0090 **ch. 8 GPIO** (functional description) | Four modes (input / output / AF / analog). Per-pin config, not per-port. |
| RM0090 **§8.4 GPIO registers** | One table per register: `MODER`, `OTYPER`, `OSPEEDR`, `PUPDR`, `IDR`, `ODR`, `BSRR`, `AFRL`/`AFRH`. Note reset values and bit width per pin (2 bits vs 1 bit). |
| RM0090 **ch. 7 RCC**, `RCC_AHB1ENR` | Which bit enables GPIOA..GPIOI. GPIO is on **AHB1**. |
| CMSIS `stm32f407xx.h` | `GPIO_TypeDef`, `GPIOA`…`GPIOI` base pointers. Use these; do not invent addresses. |
| Board user manual (Discovery: UM1472) | Which port+pin is the LED you will probe. |

Question to answer in your own words before coding: **why must the GPIO
port clock be on before you touch `GPIOx->*`?**

---

## 1. Map `bdk_gpio_port_t` → `GPIO_TypeDef *`

Private helper in `bdk_gpio.c` (and the same idea inside
`bdk_rcc_gpio_clk_enable`). One port enum → one CMSIS port pointer.

Treat `pin > 15` as invalid (ignore or early-return). Do not configure
the whole port when the API is one pin.

---

## 2. Clock enable — `bdk_rcc_gpio_clk_enable` only

File: `src/bdk_rcc.c`. Leave `bdk_rcc_sysclk_init` alone.

- Enable the **AHB1** clock for that port (`RCC_AHB1ENR` in RM0090).
- Call this **before** `bdk_gpio_init` / `bdk_gpio_set`.
- Do not enable every GPIO port “just in case.”

---

## 3. `bdk_gpio_init` — configure one pin

Goal: apply `bdk_gpio_config_t` to **that pin only**; leave the other 15
pins on the port unchanged (read-modify-write the fields that are shared
in one register).

For **blink / output**, program these (see the matching RM0090 table):

| Config field | Register | Why |
|--------------|----------|-----|
| `mode` | `MODER` | Must be output. Reset is mostly analog on F4 — a common first bug. |
| `otype` | `OTYPER` | Push-pull vs open-drain. LED = push-pull. |
| `speed` | `OSPEEDR` | Edge rate. Start with low or medium; raise if the scope looks sluggish. |
| `pull` | `PUPDR` | Output LED: usually none. |

Leave `AFR` alone unless `mode` is AF (not this pass).

Suggested order inside init: mode last or first — the RM does not mandate
one, but think about whether the pin glitches if it becomes an output
before type/speed/pull are set.

---

## 4. Drive the pin — `bdk_gpio_set` / `bdk_gpio_clear` / `bdk_gpio_write`

Use **`BSRR`**, not a read-modify-write of `ODR`.

Read the `BSRR` table: one half sets, the other half resets, write-1-to-affect.
Why this register exists (interrupt safety) is worth a sentence in the notes
below once it clicks.

`bdk_gpio_write` should only choose set vs clear.

---

## 5. `bdk_gpio_toggle`

Needs the current output level (`ODR`), then invert that bit.

Decide: BSRR after reading ODR, or a direct ODR toggle. Note the race if an
ISR also touches the same port.

---

## 6. `bdk_gpio_read`

Input data is **`IDR`**, not `ODR`. `ODR` is what you asked the output to be;
`IDR` is the actual pin. For a push-pull output they usually match — confirm
you know when they would not (open-drain, another driver on the net).

---

## 7. `examples/gpio/blink/main.c`

Sequence in `main`:

1. `bdk_rcc_gpio_clk_enable` for the LED port.
2. `bdk_gpio_init` — output, push-pull, no pull, pin from the board manual.
3. Loop: `bdk_gpio_toggle` (or set/clear).

Two useful loops (do both at some point):

- **Tight toggle** — no delay. Probe the pin. This is the real check.
- **Slow blink** — crude software delay is fine; SysTick comes later.

Do not call `bdk_rcc_sysclk_init` yet.

---

## 8. Verify on hardware

Oscilloscope on the LED pad / header, ground on board GND:

- [ ] Pin actually toggles (if flat: clock enable, `MODER` still analog, wrong port/pin).
- [ ] High ≈ 3.3 V, low ≈ 0 V.
- [ ] Tight-loop frequency: record it. Roughly related to HSI and how many
      instructions the toggle path is — not a magic HAL number.
- [ ] Change `speed` and see if the edge shape changes.

---

## Notes after implementing

Fill this in yourself (the point of the file):

**Clock**

- Register/bit used for the port clock:
- What happened if you skipped it:

**Init**

- `MODER` reset value that bit you:
- Fields you programmed vs left at reset:

**Output**

- Why `BSRR` instead of `ODR` for set/clear:
- Toggle method you chose and why:

**Measurement**

- Board / pin:
- Tight-loop frequency:
- `OSPEEDR` vs edge (what you saw):
- Anything that confused you in the RM tables:

**Later (not this pass)**

- AF: `AFRL` pins 0–7, `AFRH` pins 8–15; AF number is in the **datasheet**
  alternate-function table, not the GPIO chapter.
- `bdk_rcc_sysclk_init` once you care about a known HCLK, not just “it toggles.”
