# `usb_cdc`

Discovery **CN5** — [`bdk_cdc`](../../../inc/bdk_usb/cdc/bdk_cdc.h) with default ST-style CDC ACM  
descriptors (EP **0x01** / **0x81** / **0x82**, VID:PID **0483:5740**).

```bash
cmake --build build --target usb_cdc
```

| Layer | Status |
|-------|--------|
| EP0 + strings + class EP0 | Yes (`bdk_cdc`) |
| EP open on `SET_CONFIGURATION` | Yes (`bdk_usb_ep_open`) |
| Bulk serial data | **Not yet** — needs RX/TX on non-EP0 in `bdk_usb` |
| `SET_LINE_CODING` EP0 OUT | **Not yet** |

Custom descriptors: pass `bdk_cdc_config_t.descriptors` (must match EP macros in `bdk_cdc_desc.h` or update class).

Compare HAL VCP: [`usb_example`](../usb_example/).
