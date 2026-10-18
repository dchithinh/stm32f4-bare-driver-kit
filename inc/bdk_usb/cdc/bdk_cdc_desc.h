#ifndef BDK_CDC_DESC_H
#define BDK_CDC_DESC_H

/**
 * @file bdk_cdc_desc.h
 * @brief Default CDC ACM descriptor blobs (FS, ST-style EP layout).
 */

#include <stdint.h>

#include "bdk_usbd.h"

/** Bulk OUT (host → device). */
#define BDK_CDC_EP_OUT 0x01U
/** Bulk IN (device → host). */
#define BDK_CDC_EP_IN  0x81U
/** Interrupt IN (CDC management). */
#define BDK_CDC_EP_CMD 0x82U

#define BDK_CDC_BULK_MPS 64U
#define BDK_CDC_CMD_MPS  8U

extern const uint8_t bdk_cdc_device_desc[18];
extern const uint8_t bdk_cdc_config_desc[67];

/** Language, manufacturer, product, serial (indices 0–3). */
const bdk_usbd_descriptors_t *bdk_cdc_default_descriptors(void);

#endif /* BDK_CDC_DESC_H */
