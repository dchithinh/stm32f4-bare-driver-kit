# Vendored USB stack (trimmed)

Only files referenced by `../CMakeLists.txt` are kept (~62 files). Not the full STM32Cube HAL tree.

| Path | Role |
|------|------|
| `HAL/Src/*.c` | PCD + RCC/flash/pwr/gpio/cortex + `ll_usb` (11 sources) |
| `HAL/Inc/` | Headers for those modules + `Legacy/` |
| `HAL/stm32f4xx_hal_conf.h` | Modules: GPIO, RCC, FLASH, PWR, PCD, DMA, CORTEX (no SPI/I2C/I2S) |
| `Middlewares/.../Core` | USBD core (`usbd_core`, `ctlreq`, `ioreq`) |
| `Middlewares/.../Class/CDC` | CDC class only |
| `USB_DEVICE/App` | `usb_device`, `usbd_conf`, `usbd_desc`, `usbd_cdc_if` |
| `stm32f4xx_hal_msp.c`, `stm32f4xx_it.c` | OTG FS MSP + SysTick (from Cube `usb_test`) |

Regenerate: copy from Cube `usb_test` / F4 HAL v1.28.x, then re-run the trim (drop `*_template.*`, duplicate `Class/Inc`, unused HAL headers).
