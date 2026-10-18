#ifndef BDK_CDC_H
#define BDK_CDC_H

/**
 * @file bdk_cdc.h
 * @brief USB CDC ACM class on @ref bdk_usbd (EP0 class requests + endpoint setup).
 */

#include <stdint.h>

#include "bdk_status.h"
#include "bdk_usb.h"
#include "bdk_usbd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /**
     * USB descriptors; @c NULL uses @ref bdk_cdc_default_descriptors.
     */
    const bdk_usbd_descriptors_t *descriptors;
} bdk_cdc_config_t;

/**
 * @brief Init @ref bdk_usbd, register CDC class and configuration handlers.
 */
bdk_status_t bdk_cdc_init(bdk_usb_id_t id, const bdk_usb_config_t *hw,
                          const bdk_cdc_config_t *cfg);

/** @copydoc bdk_usbd_poll */
void bdk_cdc_poll(bdk_usb_id_t id);

/** @copydoc bdk_usbd_connect */
bdk_status_t bdk_cdc_connect(bdk_usb_id_t id);

/** @copydoc bdk_usbd_irq_enable */
bdk_status_t bdk_cdc_irq_enable(bdk_usb_id_t id);

/** @copydoc bdk_usbd_irq_handler */
void bdk_cdc_irq_handler(bdk_usb_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* BDK_CDC_H */
