# BDK `bdk_usb` / `bdk_cdc` vs HAL `usb_example` — root cause analysis

This note is **evidence from the reverted baseline** (no bulk TX API in `bdk_usb.h`). HAL `usb_example` is the known-good reference on Discovery CN5 + Pi.

## Symptom map

| Symptom | Layer | Evidence type |
|---------|--------|----------------|
| Slow / failed enumeration | EP0 / Chapter 9 | Pi `dmesg`, USART `bdk_usb_irq_stats_t`, stuck `config_value=0` |
| `Timed out waiting for FSM NP transfer…` (Pi dwc2) | Host waited on device | Control or bulk transfer never completed on device |
| No `hello` on `ttyACM0` | CDC bulk IN | **No `bdk_cdc_write` / `bdk_usb_ep_write_in` in reverted tree** — not a mystery TX bug |

---

## 1. Confirmed defect: SETUP handled while EP0 is busy

**Location:** `usb_ep0_run_pending()` in `src/bdk_usb/bdk_usb.c`.

`usb_ep0_stage` is set non-zero for EP0 IN data (`1`) and status OUT (`2`). The dispatcher **does not** check it:

```c
static void usb_ep0_run_pending(void)
{
    if (usb_setup_pending == 0U || usb_ep0_stack.on_setup == NULL) {
        return;
    }
    usb_setup_pending = 0U;
    usb_irq_stats.ep0_setup_dispatch++;
    usb_ep0_stack.on_setup(usb_setup_pkt);  /* runs even if usb_ep0_stage != 0 */
}
```

**Dispatch is triggered from OTG ISR** on:

- RX FIFO `SETUP_COMP`
- `DOEPINT` `STUP`
- `DOEPINT` `STPKTRX`

So `usbd_on_setup()` → `bdk_usb_ll_ep0_in()` can run **inside `OTG_FS_IRQHandler`** while a previous control transfer is still in `usb_ep0_stage` 1 or 2. That restarts EP0 and matches host behaviour: **repeat `GET_DESCRIPTOR`, long enum, dwc2 NP timeout**.

**Contrast HAL:** `HAL_PCD_IRQHandler` → `USBD_LL_SetupStage` / `USBD_LL_DataInStage` only when the **USBD core state machine** allows the next stage (`pdev->ep0_state`).

**Contrast this repo’s own README** (`src/bdk_usb/README.md`): says SETUP is **not** handled in the ISR and `bdk_usbd_poll()` drives EP0. **Implementation does the opposite:** `bdk_usb_ll_ep0_poll()` is empty; all dispatch is in IRQ.

**How to verify (one plug, USART2):**

- If `ep0_setup_dispatch` increments but `ep0_status_done` does not keep pace → transfers started but not finished.
- If `setup_count` (trace) stalls at 1–2 while Pi retries → classic incomplete first control transfer.
- **Proof harness (recommended):** add `ep0_dispatch_rejected_stage++` when `usb_setup_pending && usb_ep0_stage != 0` **before** any fix; flash once — non-zero proves this defect on hardware.

---

## 2. Confirmed gap: Chapter 9 / CDC vs HAL after `SET_CONFIGURATION`

| Request / behaviour | HAL `USBD_CDC_Setup` + core | BDK reverted baseline |
|---------------------|------------------------------|------------------------|
| `SET_LINE_CODING` (EP0 OUT, 7 B) | `USBD_CtlPrepareRx` → buffer | `cdc_class_setup` → **`BDK_ERR` → STALL** (`bdk_cdc.c`) |
| `GET_INTERFACE` / `SET_INTERFACE` | Handled when configured | **`usbd_handle_standard_setup` → default → STALL** |
| Bulk OUT after config | `USBD_CDC_ReceivePacket` | EP opened, **no OUT prepare** |
| Bulk IN to host | `USBD_CDC_TransmitPacket` | **Not implemented** (no LL API) |

These do **not** explain the **first** `GET_DESCRIPTOR` (standard path). They explain **ACM / `ttyACM0` useless or flaky after partial enum**.

---

## 3. Confirmed amplifier: USART log masks IRQs

`log_redirect_uart2()` uses `uart_log_irq_lock` → **`__disable_irq()`** for the whole `log_info()` (`examples/logger/log.c`).

Verbose `usb_cdc` debug from the main loop **extends** time where `OTG_FS_IRQHandler` cannot run → same external symptoms as (1). HAL example may “feel fine” with less logging.

**Verify:** `log_set_lock(NULL, NULL)` after redirect; compare enum time with logging unchanged.

---

## 4. What is *not* the root cause (reverted baseline)

- **FIFO word layout** — BDK `128 / 64 / 128` matches HAL FS `SetRxFiFo(0x80)`, `SetTxFiFo(0,0x40)`, `SetTxFiFo(1,0x80)` in `usbd_conf.c`.
- **Descriptors** — same class layout as ST CDC example (`bdk_cdc_desc.c`).
- **Missing “hello” in app** — reverted `main.c` states bulk path not implemented; there is nothing to send on `0x81`.

---

## 5. Fix order (only after verification)

Do **not** change multiple layers at once.

1. **EP0 discipline (DCD):** gate dispatch on `usb_ep0_stage == 0`; queue SETUP until idle; run Chapter 9 from **main** via `bdk_usb_ll_ep0_poll()` as README describes; single dispatch source (avoid SETUP_COMP + STUP + STPKTRX triple call).
2. **Prove enum:** Pi `lsusb 0483:5740`, `config_value=1`, `dispatch` ≫ 2, `status_done` tracks `in_xfrc`.
3. **CDC class parity:** `SET_LINE_CODING` OUT, interface requests, bulk OUT prepare (match HAL).
4. **Bulk IN:** `bdk_usb_ep_write_in` + `bdk_cdc_write_*`; test only after step 2 is stable.

---

## 6. Reference captures

When reporting bugs, paste:

1. One USART block: `ep0_setup_updt`, `dispatch`, `in_xfrc`, `status_arm`, `status_done`, `config_value`.
2. Pi: `dmesg | tail -30` after plug (include dwc2 lines).
3. Whether `log_set_lock(NULL, NULL)` was used.

Optional: `usbmon` on Pi for a single failed enum (shows SETUP retries).
