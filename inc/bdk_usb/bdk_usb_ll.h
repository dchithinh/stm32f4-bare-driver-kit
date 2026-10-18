#ifndef BDK_USB_LL_H
#define BDK_USB_LL_H

/**
 * @file bdk_usb_ll.h
 * @brief EP0 primitives for @ref bdk_usbd (OTG FS device controller).
 */

#include <stdint.h>

#include "bdk_status.h"
#include "bdk_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start EP0 IN data stage (or ZLP when @p len is 0).
 * @pre Call from main context (@ref bdk_usbd_poll), not from OTG ISR.
 */
void bdk_usb_ll_ep0_in(bdk_usb_id_t id, const uint8_t *data, uint16_t len);

/**
 * @brief Arm EP0 for a host-to-device data stage (control OUT).
 * @pre Call from main context during SETUP handling, not from OTG ISR.
 */
void bdk_usb_ll_ep0_out(bdk_usb_id_t id, uint8_t *data, uint16_t len);

/** @brief STALL both directions of EP0. */
void bdk_usb_ll_ep0_stall(bdk_usb_id_t id);

/**
 * @brief Apply @c SET_ADDRESS in DCFG (same timing as HAL @c HAL_PCD_SetAddress).
 * @param addr 0..127 device address for subsequent transactions.
 */
void bdk_usb_ll_ep0_set_address(bdk_usb_id_t id, uint8_t addr);

/**
 * @brief Apply @c SET_ADDRESS after status stage (legacy; prefer @ref bdk_usb_ll_ep0_set_address).
 */
void bdk_usb_ll_ep0_set_address_deferred(bdk_usb_id_t id, uint8_t addr);

/**
 * @brief Service deferred SETUP (call from @ref bdk_usbd_poll).
 */
void bdk_usb_ll_ep0_poll(bdk_usb_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* BDK_USB_LL_H */
