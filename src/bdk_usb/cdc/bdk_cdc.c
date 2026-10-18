#include "bdk_cdc.h"

#include <stddef.h>

#include "bdk_cdc_desc.h"

#define BDK_CDC_REQ_SET_LINE_CODING        0x20U
#define BDK_CDC_REQ_GET_LINE_CODING        0x21U
#define BDK_CDC_REQ_SET_CONTROL_LINE_STATE 0x22U

static bdk_usb_id_t cdc_id;
static uint8_t      cdc_line_state;
static uint8_t      cdc_line_coding[7] = {
    0x00, 0xC2, 0x01, 0x00,
    0x00,
    0x00,
    0x08,
};

static void cdc_close_eps(bdk_usb_id_t id)
{
    (void)bdk_usb_ep_close(id, BDK_CDC_EP_OUT);
    (void)bdk_usb_ep_close(id, BDK_CDC_EP_IN);
    (void)bdk_usb_ep_close(id, BDK_CDC_EP_CMD);
}

static void cdc_open_eps(bdk_usb_id_t id)
{
    (void)bdk_usb_ep_open(id, BDK_CDC_EP_OUT, BDK_USB_EP_TYPE_BULK, BDK_CDC_BULK_MPS);
    (void)bdk_usb_ep_open(id, BDK_CDC_EP_IN, BDK_USB_EP_TYPE_BULK, BDK_CDC_BULK_MPS);
    (void)bdk_usb_ep_open(id, BDK_CDC_EP_CMD, BDK_USB_EP_TYPE_INTERRUPT, BDK_CDC_CMD_MPS);
}

static void cdc_on_config(bdk_usb_id_t id, uint8_t config_value)
{
    if (id != cdc_id) {
        return;
    }

    cdc_close_eps(id);
    cdc_line_state = 0U;
    if (config_value != 0U) {
        cdc_open_eps(id);
    }
}

static bdk_status_t cdc_class_setup(const bdk_usb_setup_t *setup, const uint8_t **tx,
                                    uint16_t *tx_len, uint8_t **rx, uint16_t *rx_len)
{
    if (setup == NULL || tx == NULL || tx_len == NULL || rx == NULL || rx_len == NULL) {
        return BDK_ERR_NULL;
    }

    *tx     = NULL;
    *tx_len = 0U;
    *rx     = NULL;
    *rx_len = 0U;

    if (setup->bRequest == BDK_CDC_REQ_GET_LINE_CODING) {
        if (setup->wLength < sizeof(cdc_line_coding)) {
            return BDK_ERR;
        }
        *tx     = cdc_line_coding;
        *tx_len = (uint16_t)sizeof(cdc_line_coding);
        return BDK_OK;
    }

    if (setup->bRequest == BDK_CDC_REQ_SET_CONTROL_LINE_STATE) {
        cdc_line_state = (uint8_t)(setup->wValue & 0xFFU);
        return BDK_OK;
    }

    if (setup->bRequest == BDK_CDC_REQ_SET_LINE_CODING) {
        if (setup->wLength < sizeof(cdc_line_coding)) {
            return BDK_ERR;
        }
        *rx     = cdc_line_coding;
        *rx_len = (uint16_t)sizeof(cdc_line_coding);
        return BDK_OK;
    }

    return BDK_ERR;
}

bdk_status_t bdk_cdc_init(bdk_usb_id_t id, const bdk_usb_config_t *hw,
                          const bdk_cdc_config_t *cfg)
{
    const bdk_usbd_descriptors_t *desc;
    bdk_status_t                  st;

    desc = (cfg != NULL && cfg->descriptors != NULL) ? cfg->descriptors
                                                     : bdk_cdc_default_descriptors();

    st = bdk_usbd_init(id, hw, desc);
    if (st != BDK_OK) {
        return st;
    }

    cdc_id = id;
    bdk_usbd_set_class_setup_handler(id, cdc_class_setup);
    bdk_usbd_set_config_handler(id, cdc_on_config);
    return BDK_OK;
}

bdk_status_t bdk_cdc_write(bdk_usb_id_t id, const uint8_t *data, uint16_t len)
{
    if (id != cdc_id) {
        return BDK_ERR_RANGE;
    }
    if (len > 0U && data == NULL) {
        return BDK_ERR_NULL;
    }
    if (len > BDK_CDC_BULK_MPS) {
        return BDK_ERR_RANGE;
    }
    if (bdk_usbd_config_value(id) == 0U) {
        return BDK_ERR_STATE;
    }
    return bdk_usb_ep_in_write(id, BDK_CDC_EP_IN, data, len);
}

uint8_t bdk_cdc_control_line_state(bdk_usb_id_t id)
{
    if (id != cdc_id) {
        return 0U;
    }
    return cdc_line_state;
}

void bdk_cdc_poll(bdk_usb_id_t id)
{
    bdk_usbd_poll(id);
}

bdk_status_t bdk_cdc_connect(bdk_usb_id_t id)
{
    return bdk_usbd_connect(id);
}

bdk_status_t bdk_cdc_irq_enable(bdk_usb_id_t id)
{
    return bdk_usbd_irq_enable(id);
}

void bdk_cdc_irq_handler(bdk_usb_id_t id)
{
    bdk_usbd_irq_handler(id);
}
