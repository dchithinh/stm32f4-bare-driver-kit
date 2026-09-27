# I2C scanner

## Purpose

**Placeholder** for an address scan once **`bdk_i2c`** is implemented.

## What it proves

- **Today:** empty `main` — build target only.
- **Target:** print responding 7-bit addresses; logic analyzer shows START, address byte, ACK/NACK, STOP.

## Build

`cmake --build build --target i2c_scanner`
