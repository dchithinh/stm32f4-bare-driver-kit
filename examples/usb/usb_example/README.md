# USB example

## Purpose

Starting point for a **USB device** written in this app, using the kit only for what it already owns.

`bdk_gpio_init` muxes Discovery **PA11** (DM) and **PA12** (DP) to **OTG_FS** (`BDK_GPIO_AF_OTG_FS`). Clock, core, endpoints, and descriptors go in `usb_fs_init()` / `OTG_FS_IRQHandler` in `main.c`. There is no `bdk_usb` driver.

## What it proves

- **Today:** the image links and the FS pins are in AF10. Nothing enumerates.
- **Target:** the board shows up on the USB OTG FS connector after the stack in this example is filled in.

## Hardware

F407 Discovery USB OTG FS micro-AB. Device mode uses PA11/PA12. Leave PA9, PA10, PC0, and PD5 alone until you add host mode.

USB FS needs a **48 MHz** clock. `bdk_rcc_sysclk_init()` sets SYSCLK to 64 MHz from HSI+PLL and does not program PLLQ for 48 MHz — do that here before turning the core on.

## Debug console

`log_redirect_uart2(115200)` from [`examples/logger`](../logger/) prints `log_*` lines on **USART2** (**PA2** TX → USB–UART RX, GND common). Call it before other init.

## Build

`cmake --build build --target usb_example`
