#include "bdk_usbd.h"

#include <stddef.h>

#include "bdk_usb_ll.h"

#define BDK_USBD_REQ_GET_STATUS          0U
#define BDK_USBD_REQ_SET_ADDRESS         5U
#define BDK_USBD_REQ_GET_DESCRIPTOR      6U
#define BDK_USBD_REQ_SET_CONFIGURATION   9U
#define BDK_USBD_REQ_GET_CONFIGURATION   8U

#define BDK_USBD_DESC_DEVICE             1U
#define BDK_USBD_DESC_CONFIGURATION      2U
#define BDK_USBD_DESC_STRING             3U

#define BDK_USBD_REQ_TYPE_STANDARD       0U
#define BDK_USBD_REQ_TYPE_CLASS          0x20U
#define BDK_USBD_REQ_TYPE_VENDOR         0x40U
#define BDK_USBD_REQ_TYPE_MASK           0x60U

#define BDK_USBD_REQ_DIR_IN              0x80U

typedef struct {
    bdk_usb_id_t                 id;
    const bdk_usbd_descriptors_t *desc;
    bdk_usbd_class_setup_fn      class_setup;
    bdk_usbd_config_fn           config_handler;
    uint8_t                      config_value;
} bdk_usbd_ctx_t;

static bdk_usbd_ctx_t usbd_ctx;

static volatile uint32_t usbd_setup_count;
static uint8_t           usbd_last_setup[8];

static void usbd_on_bus_reset(void);
static void usbd_on_setup(const uint8_t setup_pkt[8]);

static const bdk_usb_ep0_stack_t usbd_ep0_stack = {
    .on_bus_reset = usbd_on_bus_reset,
    .on_setup     = usbd_on_setup,
};

static uint16_t usbd_min_u16(uint16_t a, uint16_t b)
{
    return (a < b) ? a : b;
}

static const uint8_t *usbd_get_descriptor_chunk(uint16_t wValue,
                                                 uint16_t wLength,
                                                 uint16_t *out_len)
{
    const bdk_usbd_descriptors_t *d = usbd_ctx.desc;
    uint8_t  type  = (uint8_t)(wValue >> 8);
    uint8_t  index = (uint8_t)(wValue & 0xFFU);
    uint16_t len   = 0U;

    if (d == NULL || out_len == NULL) {
        return NULL;
    }

    *out_len = 0U;

    if (type == BDK_USBD_DESC_DEVICE && d->device != NULL && d->device_len > 0U) {
        len = usbd_min_u16(d->device_len, wLength);
        *out_len = len;
        return d->device;
    }
    if (type == BDK_USBD_DESC_CONFIGURATION && d->config != NULL && d->config_len > 0U) {
        len = usbd_min_u16(d->config_len, wLength);
        *out_len = len;
        return d->config;
    }
    if (type == BDK_USBD_DESC_STRING && d->strings != NULL &&
        index < d->string_count && d->strings[index] != NULL) {
        len = d->strings[index][0];
        if (len == 0U) {
            return NULL;
        }
        len = usbd_min_u16(len, wLength);
        *out_len = len;
        return d->strings[index];
    }

    return (const uint8_t *)NULL;
}

static void usbd_setup_to_struct(const uint8_t pkt[8], bdk_usb_setup_t *out)
{
    out->bmRequestType = pkt[0];
    out->bRequest      = pkt[1];
    out->wValue        = (uint16_t)(pkt[2] | (pkt[3] << 8));
    out->wIndex        = (uint16_t)(pkt[4] | (pkt[5] << 8));
    out->wLength       = (uint16_t)(pkt[6] | (pkt[7] << 8));
}

static void usbd_handle_standard_setup(const bdk_usb_setup_t *setup)
{
    const uint8_t *tx     = NULL;
    uint16_t       tx_len = 0U;
    uint8_t        cfg_byte;

    if ((setup->bmRequestType & BDK_USBD_REQ_TYPE_MASK) != BDK_USBD_REQ_TYPE_STANDARD) {
        bdk_usb_ll_ep0_stall(usbd_ctx.id);
        return;
    }

    switch (setup->bRequest) {
    case BDK_USBD_REQ_GET_DESCRIPTOR:
        tx = usbd_get_descriptor_chunk(setup->wValue, setup->wLength, &tx_len);
        if (tx == NULL || tx_len == 0U) {
            bdk_usb_ll_ep0_stall(usbd_ctx.id);
            return;
        }
        bdk_usb_ll_ep0_in(usbd_ctx.id, tx, tx_len);
        return;

    case BDK_USBD_REQ_SET_ADDRESS:
        if ((setup->bmRequestType & BDK_USBD_REQ_DIR_IN) != 0U || setup->wValue > 127U) {
            bdk_usb_ll_ep0_stall(usbd_ctx.id);
            return;
        }
        bdk_usb_ll_ep0_set_address(usbd_ctx.id, (uint8_t)setup->wValue);
        bdk_usb_ll_ep0_in(usbd_ctx.id, NULL, 0U);
        return;

    case BDK_USBD_REQ_SET_CONFIGURATION: {
        uint8_t cfg = (uint8_t)(setup->wValue & 0xFFU);

        if (usbd_ctx.config_handler != NULL) {
            usbd_ctx.config_handler(usbd_ctx.id, cfg);
        }
        usbd_ctx.config_value = cfg;
        bdk_usb_ll_ep0_in(usbd_ctx.id, NULL, 0U);
        return;
    }

    case BDK_USBD_REQ_GET_CONFIGURATION:
        if (setup->wLength != 1U) {
            bdk_usb_ll_ep0_stall(usbd_ctx.id);
            return;
        }
        cfg_byte = usbd_ctx.config_value;
        bdk_usb_ll_ep0_in(usbd_ctx.id, &cfg_byte, 1U);
        return;

    case BDK_USBD_REQ_GET_STATUS:
        if (setup->wLength == 2U) {
            static const uint8_t z[2] = {0U, 0U};
            bdk_usb_ll_ep0_in(usbd_ctx.id, z, 2U);
            return;
        }
        bdk_usb_ll_ep0_stall(usbd_ctx.id);
        return;

    default:
        bdk_usb_ll_ep0_stall(usbd_ctx.id);
        return;
    }
}

static void usbd_on_setup(const uint8_t setup_pkt[8])
{
    bdk_usb_setup_t setup;
    const uint8_t  *tx     = NULL;
    uint16_t        tx_len = 0U;
    bdk_status_t    st;
    uint8_t         i;

    for (i = 0U; i < 8U; i++) {
        usbd_last_setup[i] = setup_pkt[i];
    }
    usbd_setup_count++;

    usbd_setup_to_struct(setup_pkt, &setup);

    if ((setup.bmRequestType & BDK_USBD_REQ_TYPE_MASK) == BDK_USBD_REQ_TYPE_STANDARD) {
        usbd_handle_standard_setup(&setup);
        return;
    }

    if (usbd_ctx.class_setup != NULL &&
        ((setup.bmRequestType & BDK_USBD_REQ_TYPE_MASK) == BDK_USBD_REQ_TYPE_CLASS ||
         (setup.bmRequestType & BDK_USBD_REQ_TYPE_MASK) == BDK_USBD_REQ_TYPE_VENDOR)) {
        st = usbd_ctx.class_setup(&setup, &tx, &tx_len);
        if (st != BDK_OK || (tx_len > 0U && tx == NULL)) {
            bdk_usb_ll_ep0_stall(usbd_ctx.id);
            return;
        }
        if (tx_len > setup.wLength) {
            tx_len = setup.wLength;
        }
        bdk_usb_ll_ep0_in(usbd_ctx.id, tx, tx_len);
        return;
    }

    bdk_usb_ll_ep0_stall(usbd_ctx.id);
}

static void usbd_on_bus_reset(void)
{
    if (usbd_ctx.config_handler != NULL && usbd_ctx.config_value != 0U) {
        usbd_ctx.config_handler(usbd_ctx.id, 0U);
    }
    usbd_ctx.config_value = 0U;
}

bdk_status_t bdk_usbd_init(bdk_usb_id_t id,
                           const bdk_usb_config_t *hw,
                           const bdk_usbd_descriptors_t *desc)
{
    bdk_status_t st;

    if (desc == NULL || desc->device == NULL || desc->config == NULL) {
        return BDK_ERR_NULL;
    }

    usbd_ctx.id           = id;
    usbd_ctx.desc         = desc;
    usbd_ctx.class_setup     = NULL;
    usbd_ctx.config_handler  = NULL;
    usbd_ctx.config_value    = 0U;

    st = bdk_usb_init(id, hw);
    if (st != BDK_OK) {
        return st;
    }

    bdk_usb_set_ep0_stack(id, &usbd_ep0_stack);
    bdk_usb_set_events(id, NULL);
    return BDK_OK;
}

void bdk_usbd_set_class_setup_handler(bdk_usb_id_t id, bdk_usbd_class_setup_fn fn)
{
    if (id == usbd_ctx.id) {
        usbd_ctx.class_setup = fn;
    }
}

void bdk_usbd_set_config_handler(bdk_usb_id_t id, bdk_usbd_config_fn fn)
{
    if (id == usbd_ctx.id) {
        usbd_ctx.config_handler = fn;
    }
}

bdk_status_t bdk_usbd_connect(bdk_usb_id_t id)
{
    return bdk_usb_connect(id);
}

bdk_status_t bdk_usbd_disconnect(bdk_usb_id_t id)
{
    return bdk_usb_disconnect(id);
}

bdk_status_t bdk_usbd_irq_enable(bdk_usb_id_t id)
{
    return bdk_usb_irq_enable(id);
}

void bdk_usbd_irq_handler(bdk_usb_id_t id)
{
    bdk_usb_irq_handler(id);
}

void bdk_usbd_poll(bdk_usb_id_t id)
{
    if (id == usbd_ctx.id) {
        bdk_usb_ll_ep0_poll(id);
    }
}

void bdk_usbd_irq_stats_get(bdk_usb_id_t id, bdk_usb_irq_stats_t *out)
{
    bdk_usb_irq_stats_get(id, out);
}

uint8_t bdk_usbd_config_value(bdk_usb_id_t id)
{
    if (id != usbd_ctx.id) {
        return 0U;
    }
    return usbd_ctx.config_value;
}

void bdk_usbd_trace_get(bdk_usb_id_t id, bdk_usbd_trace_t *out)
{
    uint8_t i;

    if (out == NULL || id != usbd_ctx.id) {
        return;
    }

    out->setup_count = usbd_setup_count;
    for (i = 0U; i < 8U; i++) {
        out->last_setup[i] = usbd_last_setup[i];
    }
}
