# I2C scanner

## Purpose

Poll the bus for **7-bit addresses** that ACK a master **write** probe (no data bytes).

App wires **I2C1** on the F407 Discovery (**PB6** SCL, **PB9** SDA), calls **`bdk_i2c_init`**, then **`bdk_i2c_write(id, addr, NULL, 0)`** for each address **0x08..0x77**. Results print on **USART2**.

## What it proves

- GPIO (AF, open-drain, pull-up) + I2C master timing in **`bdk_i2c`**.
- Serial lists every responding address; logic analyzer shows **START**, addr+W, **ACK**, **STOP** on hits and **NACK** on misses.

## Driver contract (you implement in `src/bdk_i2c.c`)

| API | Scanner use |
|-----|-------------|
| **`bdk_i2c_init`** | 100 kHz master on **I2C1** |
| **`bdk_i2c_write(..., NULL, 0)`** | Address probe only — **BDK_OK** if slave ACKs the address byte |

Until those return **`BDK_OK`**, the app will assert-fail on init or print **`(no ACK)`** for every address.

## Wiring

**On-board bus (Discovery codec / MEMS):** uses **PB6/PB9** already.

**External module** (3.3 V sensor breakout):

| Module | MCU |
|--------|-----|
| SCL | **PB6** |
| SDA | **PB9** |
| GND | GND |
| VCC | 3.3 V (not 5 V unless module is 3.3 V I/O) |

Status / log: **PA2** TX → USB–UART RX, GND common, **115200** 8N1.

Change pins at the top of `main.c` (`LAB_I2C_*`) if your hardware differs.

## Build

`cmake --build build --target i2c_scanner`

## Expected output (example)

```text
i2c_scanner: PB6=SCL PB9=SDA I2C1
I2C scan 0x08..0x77 @ 100kHz
  0x1E
  0x68
done.
```

Addresses depend on what is on the bus.

## Probe

Logic analyzer on **PB6** (SCL) and **PB9** (SDA); decode I2C or inspect timing manually. See `docs/notes/i2c.md`.
