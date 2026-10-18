# USB examples (Discovery CN5)

| Target | Stack | Purpose |
|--------|--------|---------|
| [`usb_bringup`](usb_bringup/) | `bdk_usb` only | PHY, connect, IRQ, SETUP discard |
| [`usb_cdc`](usb_cdc/) | `bdk_usb` + `bdk_usbd` | CDC descriptors + class EP0 |
| [`usb_example`](usb_example/) | HAL + ST USBD CDC | Reference `ttyACM0` |

Library code: `inc/bdk_usb/`, `src/bdk_usb/`.
