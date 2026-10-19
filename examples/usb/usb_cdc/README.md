# `usb_cdc`

STM32F407 Discovery **CN5** (OTG FS) as a USB **CDC ACM** serial gadget using
[`bdk_cdc`](../../../inc/bdk_usb/cdc/bdk_cdc.h). The host should enumerate
**VID:PID `0483:5740`** (ST Virtual COM) and get a TTY (`/dev/ttyACM0` on Linux).

After `SET_CONFIGURATION`, the example sends **`BDK CDC`** on bulk IN (`0x81`)
about **once per second**. For host → device bulk OUT, see
[`usb_cdc_echo`](../usb_cdc_echo/).

## Purpose

Show a full BDK path: OTG FS device + Chapter 9 (`bdk_usbd`) + CDC class
(`SET_LINE_CODING`, `SET_CONTROL_LINE_STATE`) + one bulk IN stream the host can
`cat`.

## What it proves

- CN5 PHY, 48 MHz USB clock, and `bdk_cdc_connect()` enumerate as **0483:5740**.
- EP0 handles standard descriptors and CDC class requests (including 7-byte
  `SET_LINE_CODING`).
- `SET_CONFIGURATION` opens EP **0x01** (bulk OUT), **0x81** (bulk IN), **0x82**
  (interrupt IN).
- Host `cat /dev/ttyACM0` prints a `BDK CDC` line about every second.

USART2 (ST-Link VCP, **115200**) is **debug only**. It logs bus reset, SETUP, and
DTR; it does **not** print each bulk packet.

## Wiring

| Signal | Discovery |
|--------|-----------|
| USB FS | **CN5** (not the ST-Link mini-USB) |
| DM / DP | PA11 / PA12 AF `OTG_FS` |
| CN5 VBUS switch | **PC0** driven high |
| Debug UART | USART2 PA2 TX @ 115200 8N1 (ST-Link) |

Plug a cable from **CN5** to the host (PC or Pi).

## Build and flash

From the SDK root (after a normal `cmake -S . -B build`):

```bash
cmake --build build --target usb_cdc
# flash build/examples/usb/usb_cdc/usb_cdc.bin
```

Reset the board, then plug CN5.

## Host: see the output

On Linux:

```bash
dmesg -w
# expect: idVendor=0483, idProduct=5740  →  ttyACM0  (or ttyACM1)

lsusb -d 0483:5740
ls -l /dev/ttyACM*

# stop ModemManager if it grabs the ACM port:
# sudo systemctl stop ModemManager

cat /dev/ttyACM0
```

Expected on the TTY (one line per second):

```text
BDK CDC
BDK CDC
BDK CDC
...
```

Opening the port asserts DTR. USART2 should show `SET_CONTROL_LINE_STATE=... DTR=1`
and `bulk IN banner queued` (once). Closing `cat` (Ctrl+C) typically sets DTR=0;
the example still transmits while configured (`config_value=1`), on the 1 s timer.

Windows: Device Manager → Ports → **STMicroelectronics Virtual COM Port**, then
a serial terminal on that COMx (any baud; CDC ignores line coding for this demo).

## Custom descriptors

Pass `bdk_cdc_config_t.descriptors`, or `NULL` for
`bdk_cdc_default_descriptors()`. Endpoint addresses must match
`inc/bdk_usb/cdc/bdk_cdc_desc.h` (or update the class).

HAL reference VCP on the same board: [`usb_example`](../usb_example/).
