# RCC MCO2 — SYSCLK on PC9

## Purpose

Show that **clock output** comes from **RCC**, not from toggling GPIO.

You implement `bdk_rcc_mco2_sysclk()` (RM0090 `RCC_CFGR`: MCO2 = SYSCLK, prescaler ÷1). `main.c` only sets up PC9 alternate function for MCO2.

## What it proves

- With default **HSI**, scope on **PC9** shows ~**16 MHz**, ~3.3 V square (period ~62.5 ns).
- Confirms SYSCLK frequency before you change the PLL in `bdk_rcc`.

MCO1 is **PA8** and cannot select SYSCLK on F407.

## Probe

- **PC9** vs GND; timebase **50–100 ns/div** (not seconds — this is not blink).

## Build

`cmake --build build --target rcc_mco`

## Captures

```markdown
![MCO2 16 MHz](captures/mco2_16mhz.png)
```
