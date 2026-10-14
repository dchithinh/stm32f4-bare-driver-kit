# USB example

## Purpose

Starting point for a **USB device** written in this app, using the kit only for what it already owns.

`bdk_gpio_init` muxes Discovery **PA11** (DM) and **PA12** (DP) to **OTG_FS** (`BDK_GPIO_AF_OTG_FS`). Clock, core, endpoints, and descriptors go in `usb_fs_init()` / `OTG_FS_IRQHandler` in `main.c`. There is no `bdk_usb` driver.

## What it proves

- **Today:** the image links and the FS pins are in AF10. Nothing enumerates.
- **Target:** the board shows up on the USB OTG FS connector after the stack in this example is filled in.

## Hardware

F407 Discovery USB OTG FS micro-AB. Device mode uses PA11/PA12. Leave PA9, PA10, PC0, and PD5 alone until you add host mode.

USB FS needs a **48 MHz** clock from PLLQ. Use `bdk_rcc_clock_init()` with the board preset in `clock_config.h` (168 MHz SYSCLK + 48 MHz USB on Discovery HSE) before turning the OTG core on.

## Debug console

`log_redirect_uart2(115200)` from [`examples/logger`](../logger/) prints `log_*` lines on **USART2** (**PA2** TX → USB–UART RX, GND common). Call it before other init.

## Build

`cmake --build build --target usb_example`
