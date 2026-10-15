#ifndef BDK_USB_H
#define BDK_USB_H

/**
 * @file bdk_usb.h
 * @brief USB OTG FS device-mode hardware (STM32F4 internal FS PHY).
 *
 * Programs the OTG FS peripheral only. USB descriptors, EP0, and class drivers
 * belong in the application or a separate device stack.
 */

#include <stdint.h>

#include "bdk_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB OTG instance (F407 has one FS OTG).
 */
typedef enum {
    BDK_USB_OTG_FS = 0,
} bdk_usb_id_t;

/**
 * @brief Device-mode bring-up options.
 */
typedef struct {
    /**
     * When non-zero, sets @c USB_OTG_GCCFG_NOVBUSSENS: OTG does not use the VBUS
     * sense pin for session valid (Discovery device on PA11/PA12 only).
     */
    uint8_t ignore_vbus_sense;
} bdk_usb_config_t;

/**
 * @brief Optional IRQ callbacks for a device stack (may be NULL).
 */
typedef struct {
    void (*on_bus_reset)(void);
    void (*on_enum_done)(void);
    /** Optional: standard control SETUP accepted (not STALL). */
    void (*on_ep0_setup)(void);
} bdk_usb_events_t;

/** Standard USB setup packet (8 bytes). */
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} bdk_usb_setup_t;

/**
 * @brief Handle a control SETUP request (EP0).
 * @param setup  Parsed setup packet.
 * @param tx     On success, set to descriptor/response bytes (may stay NULL for ZLP).
 * @param tx_len Bytes to send on the IN data stage (0 for status-only).
 * @return BDK_OK to accept, BDK_ERR to STALL.
 */
typedef bdk_status_t (*bdk_usb_setup_fn)(const bdk_usb_setup_t *setup,
                                         const uint8_t **tx,
                                         uint16_t *tx_len);

/**
 * @brief Reset and configure OTG FS for full-speed device mode.
 * @pre App has configured 48 MHz USB clock (PLLQ) via @ref bdk_rcc_clock_init.
 * @pre DM/DP pins are muxed to OTG_FS (e.g. @ref bdk_gpio_init).
 */
bdk_status_t bdk_usb_init(bdk_usb_id_t id, const bdk_usb_config_t *config);

/**
 * @brief Register stack callbacks (optional; copy stored until replaced).
 */
void bdk_usb_set_events(bdk_usb_id_t id, const bdk_usb_events_t *events);

/**
 * @brief Register EP0 SETUP handler (optional). When set, @ref bdk_usb_irq_handler
 *        answers standard control transfers; when NULL, RX traffic is discarded.
 */
void bdk_usb_set_setup_handler(bdk_usb_id_t id, bdk_usb_setup_fn fn);

/**
 * @brief Apply soft connect (clear DCTL SDIS) so the host can enumerate.
 */
bdk_status_t bdk_usb_connect(bdk_usb_id_t id);

/**
 * @brief Soft disconnect (set DCTL SDIS).
 */
bdk_status_t bdk_usb_disconnect(bdk_usb_id_t id);

/**
 * @brief Enable OTG FS global interrupt in NVIC.
 */
bdk_status_t bdk_usb_irq_enable(bdk_usb_id_t id);

/**
 * @brief Service OTG FS global interrupt; call from app @c OTG_FS_IRQHandler.
 */
void bdk_usb_irq_handler(bdk_usb_id_t id);

/**
 * @brief Run deferred EP0 SETUP handling (call from main loop when using
 *        @ref bdk_usb_set_setup_handler). SETUP bytes are captured in the IRQ;
 *        the IN/OUT response is started here to avoid doing that inside the ISR.
 */
void bdk_usb_ep0_poll(bdk_usb_id_t id);

/** Counters since boot (debug); updated only inside @ref bdk_usb_irq_handler. */
typedef struct {
    uint32_t handler_calls;
    uint32_t usbrst;
    uint32_t enumdne;
    uint32_t rxflvl_batches;
    uint32_t fifo_bytes_discarded;
    uint32_t ep0_stup;
} bdk_usb_irq_stats_t;

/** Snapshot of @ref bdk_usb_irq_stats_t (IRQ-side activity, not NVIC entry count). */
void bdk_usb_irq_stats_get(bdk_usb_id_t id, bdk_usb_irq_stats_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BDK_USB_H */
