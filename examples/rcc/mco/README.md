# RCC MCO2 — SYSCLK on PC9

This is not GPIO toggle. The RCC **MCO** (microcontroller clock output) muxes
an internal clock onto a pin. On F407, **SYSCLK → MCO2 → PC9** (datasheet AF0).

At reset SYSCLK = HSI = **16 MHz**, so you should measure ~16 MHz until you
change the clock tree.

## You fill

`bdk_rcc_mco2_sysclk()` in `src/bdk_rcc.c` from RM0090 **RCC_CFGR**:

- **MCO2** source = SYSCLK
- **MCO2PRE** = not divided (div 1)

GPIO for PC9 is already in `main.c`. Do not drive PC9 as a normal output.

MCO1 is **PA8** and **cannot** select SYSCLK (HSI/HSE/LSE/PLL only). Same 16 MHz
at reset if you pick HSI, different pin.

## Probe

- Channel: **PC9** vs GND (Discovery header — check UM1472 / schematic that
  nothing else owns the pin)
- Timebase: **50–100 ns/div** (16 MHz period is 62.5 ns). A 1 s/div blink
  setting will look like a blur or a wall
- 10× probe, short ground
- PicoScope bandwidth and sample rate must be well above 16 MHz (aim ≥50 MS/s)

Expect ~**3.3 V square**, ~**16 MHz**, HSI not crystal-accurate (often 15.9–16.1).

## Captures

```markdown
![MCO2 16 MHz](captures/mco2_16mhz.png)
```
