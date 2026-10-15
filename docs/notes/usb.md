# USB notes (STM32F4 OTG FS)

## Layers

| Layer | In this kit |
|-------|-------------|
| 48 MHz USB clock + SYSCLK | App `bdk_rcc_clock_init()` (e.g. `USB_EXAMPLE_CLOCK_HSE_168_USB`) |
| DM/DP pins | `bdk_gpio_init` + `BDK_GPIO_AF_OTG_FS` |
| OTG FS hardware | **`bdk_usb`** — device mode, PHY, soft connect, IRQ demux |
| Descriptors, EP0, CDC/HID | **App** or third-party stack (TinyUSB, etc.) |

## `bdk_usb` API

- `bdk_usb_init(BDK_USB_OTG_FS, &cfg)` — core reset, FS device mode, FIFO sizing, soft-disconnect.
- `bdk_usb_connect` / `bdk_usb_disconnect` — DCTL SDIS.
- `bdk_usb_irq_enable` + `bdk_usb_irq_handler` from `OTG_FS_IRQHandler`.
- `bdk_usb_set_events` — optional `on_bus_reset` / `on_enum_done` for a stack.

**Preconditions:** PLL **PLLQ → 48 MHz** before `bdk_usb_init`. PA11/PA12 muxed to OTG_FS.

Discovery **CN5** device: drive **PC0** high (OTG_FS_PowerSwitchOn) before `bdk_usb_init`. Example uses `ignore_vbus_sense = 1` (GCCFG NOVBUSSENS). Soft connect is `bdk_usb_connect` (same role as clearing DCTL SDIS in `HAL_PCD_Start`); enumeration still needs EP0 + descriptors in the app.

## Lab examples

| Path | Stack |
|------|--------|
| `examples/usb/usb_bringup/` | `bdk_usb` init — connect, IRQ stats, SETUP discard |
| `examples/usb/usb_example/` | BDK + HAL/USBD — full enumeration (compare) |

Enumeration requires a device stack on top of `bdk_usb` (HAL/USBD in `usb_example`, or future TinyUSB / in-kit EP0).
