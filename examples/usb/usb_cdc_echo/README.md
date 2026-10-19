# `usb_cdc_echo`

Same Discovery **CN5** CDC ACM gadget as [`usb_cdc`](../usb_cdc/), but this lab
proves **bulk OUT** (host → device): host payload is echoed on bulk IN.

## Purpose

Show `bdk_cdc_poll_in()` draining host data after `bdk_cdc_poll()` services OUT
packets, and `bdk_cdc_write()` returning that data to the host.

## What it proves

- Bulk OUT **0x01** accepts host OUT tokens after `SET_CONFIGURATION`.
- `bdk_cdc_poll_in()` delivers those bytes; the example writes them back on **0x81**.

USART2 @ **115200** is a one-line boot banner only. Do **not** print every RX
byte there: blocking UART stalls USB poll, the RX FIFO overflows, and a
Raspberry Pi USB host can lock up when a terminal opens `/dev/ttyACM0`.

## Wiring

Same as [`usb_cdc`](../usb_cdc/README.md#wiring): CN5, PC0 VBUS, PA11/PA12.

## Build and flash

```bash
cmake --build build --target usb_cdc_echo
# flash build/examples/usb/usb_cdc_echo/usb_cdc_echo.bin
```

Flash only one of `usb_cdc` / `usb_cdc_echo` at a time.

## Host (Raspberry Pi)

Stop anything that auto-opens ACM (this can wedge the Pi's USB host):

```bash
sudo systemctl stop ModemManager
lsusb -d 0483:5740
```

Safer than `picocom` for a first check — two shells:

```bash
# shell 1
stty -F /dev/ttyACM0 raw -echo clocal 115200
cat /dev/ttyACM0

# shell 2
printf 'hello\r\n' > /dev/ttyACM0
```

Shell 1 should print `hello`.

Interactive terminal (after `cat` works):

```bash
picocom /dev/ttyACM0 -b 115200 --noreset --hangup --imap lfcrlf
```

If `picocom` / `screen` hangs the Pi again: unplug CN5, do **not** Ctrl+C a wedged
USB stack blindly — unplug first — then retry the `stty` + `cat` path.

Compare with [`usb_cdc`](../usb_cdc/), which only transmits `BDK CDC` on a timer.
