# I2C notes

Fill this in while reading RM0090 (I2C chapter). Capture:

- Event sequence for master write / read / repeated START
- ACK vs NACK handling and what the flags actually mean
- Logic-analyzer check: START, address+R/W, ACK, STOP; stretch if it happens

## Lab app

`examples/i2c/scanner/` — **I2C1** **PB6** SCL, **PB9** SDA (Discovery). Probes with
`bdk_i2c_write(id, addr, NULL, 0)`; logs on USART2 **115200**.

## What the app passes to `bdk_i2c_init`

| In `bdk_i2c_config_t` | Meaning |
|------------------------|---------|
| `id` | `BDK_I2C_1` / `_2` / `_3` → `I2C1`… register block |
| `speed_hz` | `BDK_I2C_SPEED_100KHZ` or `BDK_I2C_SPEED_400KHZ` (other Hz if driver allows) |

Not in the struct (app / driver internals):

- **GPIO** — SCL/SDA pins, AF, open-drain, pull-ups (`bdk_gpio_init` in the example).
- **PCLK1 (APB1)** — `bdk_rcc_get_pclk1_hz()` for I2C `CR2.FREQ` and CCR/TRISE.
- **Slave address** — per transaction (`bdk_i2c_write` / `read` / `write_read`), not at init.

Optional features (SMBus, own address, analog filter) are not in the API yet; add fields only if you implement them.

## `bdk_i2c_init` timing checks

- Standard **100 kHz:** `CCR = pclk1 / (2 × f_SCL)`, RM requires **CCR ≥ 4**.
- Fast **400 kHz** (DUTY 0): `CCR = pclk1 / (3 × f_SCL)`, RM requires **CCR ≥ 1** (not the standard-mode minimum of 4).
