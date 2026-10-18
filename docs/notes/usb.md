# USB notes (STM32F4 OTG FS)

Sources: `inc/bdk_usb/`, `src/bdk_usb/` (`bdk_usb.c`, `bdk_usbd.c`, `cdc/bdk_cdc*.c`).
Layer diagram: [`src/bdk_usb/README.md`](../../src/bdk_usb/README.md).

## Layers

| Layer | Module |
|-------|--------|
| 48 MHz USB clock + SYSCLK | App `bdk_rcc_clock_init()` |
| DM/DP pins | `bdk_gpio_init` + `BDK_GPIO_AF_OTG_FS` |
| OTG FS hardware | **`bdk_usb`** — PHY, FIFO, IRQ, EP0 transfer primitives |
| Chapter 9 + EP0 policy | **`bdk_usbd`** — GET_DESCRIPTOR, SET_ADDRESS, SET_CONFIGURATION, class hook |
| CDC ACM | **`bdk_cdc`** (`inc/bdk_usb/cdc/`) — class EP0 + EP open on config |
| HID / MSC | Future class modules or TinyUSB |

## `bdk_usb`

- `bdk_usb_init` — core reset, FS device mode, FIFO sizing, soft-disconnect.
- `bdk_usb_connect` / `disconnect` — DCTL SDIS.
- `bdk_usb_irq_enable` + `bdk_usb_irq_handler` from `OTG_FS_IRQHandler`.
- `bdk_usb_set_ep0_stack` — used by `bdk_usbd` (SETUP handled in `bdk_usbd_poll`).

## `bdk_usbd`

- `bdk_usbd_init(id, hw_cfg, &descriptors)` — calls `bdk_usb_init`, registers EP0 stack.
- `bdk_usbd_poll` — deferred SETUP (main loop, not ISR).
- `bdk_usbd_set_class_setup_handler` — class/vendor EP0 requests.
- `bdk_usbd_connect`, `bdk_usbd_irq_handler`, `bdk_usbd_irq_enable`.

**Descriptors** are app-owned (`bdk_usbd_descriptors_t`: device, config, optional strings).

## Lab examples

| Path | Stack |
|------|--------|
| `examples/usb/usb_bringup/` | `bdk_usb` only — no `bdk_usbd` |
| `examples/usb/usb_cdc/` | `bdk_cdc` (thin board `main.c`) |
| `examples/usb/usb_example/` | BDK clock/log + HAL/USBD CDC (reference) |

Discovery **CN5**: PC0 high before init; `ignore_vbus_sense = 1` for PA11/PA12 device port.
