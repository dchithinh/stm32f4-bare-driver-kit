#ifndef BDK_USBD_H
#define BDK_USBD_H

/**
 * @file bdk_usbd.h
 * @brief USB device stack (Chapter 9) on @ref bdk_usb.
 *
 * Handles standard control requests and EP0; class drivers extend via
 * @ref bdk_usbd_set_class_setup_handler.
 */

#include <stdint.h>

#include "bdk_status.h"
#include "bdk_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Standard USB setup packet (8 bytes). */
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} bdk_usb_setup_t;

/** Device descriptor and configuration blobs (app-owned, stable through init). */
typedef struct {
    const uint8_t *device;
    uint16_t       device_len;
    const uint8_t *config;
    uint16_t       config_len;
    /**
     * String descriptors by index (UTF-16LE body per USB string descriptor).
     * @p strings[0] is language ID list if provided.
     */
    const uint8_t *const *strings;
    uint8_t               string_count;
} bdk_usbd_descriptors_t;

/**
 * @brief Class or vendor control request on EP0 (optional).
 * @return BDK_OK with @p tx/@p tx_len for IN data; BDK_ERR to STALL.
 */
typedef bdk_status_t (*bdk_usbd_class_setup_fn)(const bdk_usb_setup_t *setup,
                                                const uint8_t **tx,
                                                uint16_t *tx_len);

/**
 * @brief Called when configuration changes (@c SET_CONFIGURATION or bus reset).
 * @param config_value 0 when unconfigured; otherwise host-selected config index.
 */
typedef void (*bdk_usbd_config_fn)(bdk_usb_id_t id, uint8_t config_value);

/**
 * @brief Init OTG FS (@ref bdk_usb_init) and register the device stack on EP0.
 */
bdk_status_t bdk_usbd_init(bdk_usb_id_t id,
                           const bdk_usb_config_t *hw,
                           const bdk_usbd_descriptors_t *desc);

void bdk_usbd_set_class_setup_handler(bdk_usb_id_t id, bdk_usbd_class_setup_fn fn);

void bdk_usbd_set_config_handler(bdk_usb_id_t id, bdk_usbd_config_fn fn);

bdk_status_t bdk_usbd_connect(bdk_usb_id_t id);
bdk_status_t bdk_usbd_disconnect(bdk_usb_id_t id);
bdk_status_t bdk_usbd_irq_enable(bdk_usb_id_t id);

/** @brief Call from @c OTG_FS_IRQHandler (forwards to @ref bdk_usb_irq_handler). */
void bdk_usbd_irq_handler(bdk_usb_id_t id);

/**
 * @brief Run deferred EP0 SETUP handling (main loop).
 */
void bdk_usbd_poll(bdk_usb_id_t id);

/** @copydoc bdk_usb_irq_stats_get */
void bdk_usbd_irq_stats_get(bdk_usb_id_t id, bdk_usb_irq_stats_t *out);

/** Non-zero after successful @c SET_CONFIGURATION. */
uint8_t bdk_usbd_config_value(bdk_usb_id_t id);

/**
 * @brief Last handled SETUP packet (for UART debug from main loop).
 *
 * @p setup_count increments on each @ref bdk_usbd_poll / IRQ-handled SETUP.
 */
typedef struct {
    uint32_t setup_count;
    uint8_t  last_setup[8];
} bdk_usbd_trace_t;

void bdk_usbd_trace_get(bdk_usb_id_t id, bdk_usbd_trace_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BDK_USBD_H */
