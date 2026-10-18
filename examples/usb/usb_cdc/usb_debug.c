#include "usb_debug.h"

#include "bdk_cdc.h"
#include "bdk_usb.h"
#include "bdk_usbd.h"

#include "log.h"

#include <stdio.h>

#define USB_DBG_HEARTBEAT_LOOPS 80000U

static const char *setup_result_str(uint8_t r)
{
    switch (r) {
    case BDK_USBD_SETUP_STD_DEV_DESC:
        return "std_dev_desc";
    case BDK_USBD_SETUP_STD_DEV_SET_ADDR:
        return "std_set_addr";
    case BDK_USBD_SETUP_STD_DEV_SET_CONFIG:
        return "std_set_config";
    case BDK_USBD_SETUP_STD_DEV_GET_CONFIG:
        return "std_get_config";
    case BDK_USBD_SETUP_STD_DEV_GET_STATUS:
        return "std_dev_status";
    case BDK_USBD_SETUP_STD_IF_GET:
        return "std_if_get";
    case BDK_USBD_SETUP_STD_IF_SET:
        return "std_if_set";
    case BDK_USBD_SETUP_STD_IF_STATUS:
        return "std_if_status";
    case BDK_USBD_SETUP_STD_IF_CLEAR_FEATURE:
        return "std_if_clr_feat";
    case BDK_USBD_SETUP_CLASS_IN:
        return "class_in";
    case BDK_USBD_SETUP_CLASS_OUT_ARMED:
        return "class_out_arm";
    case BDK_USBD_SETUP_CLASS_STATUS_IN:
        return "class_zlp_in";
    case BDK_USBD_SETUP_STALL:
        return "STALL";
    default:
        return "?";
    }
}

static void log_setup_line(bdk_usb_id_t id, const bdk_usbd_trace_t *tr)
{
    const uint8_t *p = tr->last_setup;
    uint16_t         wval = (uint16_t)(p[2] | (p[3] << 8));
    uint16_t         widx = (uint16_t)(p[4] | (p[5] << 8));
    uint16_t         wlen = (uint16_t)(p[6] | (p[7] << 8));
    bdk_usb_irq_stats_t irq;

    bdk_usbd_irq_stats_get(id, &irq);
    log_info("SETUP #%lu bm=0x%02x req=0x%02x val=%04x idx=%04x len=%u -> %s "
             "stg=%u in_xfrc=%lu st=%lu stalls=%lu",
             (unsigned long)tr->setup_count, p[0], p[1], wval, widx, wlen,
             setup_result_str(tr->last_result), (unsigned)bdk_usb_ep0_stage_get(id),
             (unsigned long)irq.ep0_in_xfrc, (unsigned long)irq.ep0_status_done,
             (unsigned long)tr->stall_count);
}

static void log_status_line(bdk_usb_id_t id)
{
    bdk_usb_irq_stats_t irq;
    bdk_cdc_debug_t     cdc;
    uint8_t             cfg;
    int                 ready;

    bdk_usbd_irq_stats_get(id, &irq);
    bdk_cdc_debug_get(id, &cdc);
    cfg   = bdk_usbd_config_value(id);
    ready = bdk_cdc_write_ready(id);

    log_info("STAT cfg=%u ep0_stage=%u ready=%d ep_open=%lu tx_ok=%lu "
             "ep0_in_xfrc=%lu ep0_status=%lu usbrst=%lu dispatch=%lu setup_updt=%lu "
             "bulk_in=%lu/%lu bulk_out=%lu stalls=%lu",
             (unsigned)cfg, (unsigned)bdk_usb_ep0_stage_get(id), ready,
             (unsigned long)cdc.config_opens, (unsigned long)cdc.tx_ok,
             (unsigned long)irq.ep0_in_xfrc, (unsigned long)irq.ep0_status_done,
             (unsigned long)irq.usbrst, (unsigned long)irq.ep0_setup_dispatch,
             (unsigned long)irq.ep0_setup_updt, (unsigned long)irq.ep1_bulk_in_start,
             (unsigned long)irq.ep1_bulk_in_xfrc, (unsigned long)irq.ep1_bulk_out_xfrc,
             (unsigned long)irq.ep0_stall);
}

void usb_debug_poll(bdk_usb_id_t id)
{
    static uint32_t last_setup_count;
    static uint32_t last_dispatch;
    static uint32_t last_usbrst;
    static uint8_t  last_cfg;
    static uint32_t heartbeat;
    static uint8_t  boot_stat;

    bdk_usbd_trace_t    tr;
    bdk_usb_irq_stats_t irq;

    bdk_usbd_irq_stats_get(id, &irq);
    bdk_usbd_trace_get(id, &tr);

    if (irq.usbrst != last_usbrst) {
        log_info("USB bus reset (usbrst=%lu)", (unsigned long)irq.usbrst);
        last_usbrst = irq.usbrst;
    }

    if (irq.ep0_setup_dispatch != last_dispatch) {
        uint32_t delta = irq.ep0_setup_dispatch - last_dispatch;

        if (delta > 1U) {
            log_info("SETUP burst +%lu (dispatch now %lu, trace #%lu)",
                     (unsigned long)delta, (unsigned long)irq.ep0_setup_dispatch,
                     (unsigned long)tr.setup_count);
        }
        log_setup_line(id, &tr);
        last_dispatch = irq.ep0_setup_dispatch;
        last_setup_count = tr.setup_count;
    } else if (tr.setup_count != last_setup_count) {
        log_setup_line(id, &tr);
        last_setup_count = tr.setup_count;
    }

    {
        uint8_t cfg = bdk_usbd_config_value(id);

        if (cfg != last_cfg) {
            log_info("CONFIG cfg=%u (was %u)", (unsigned)cfg, (unsigned)last_cfg);
            last_cfg = cfg;
            log_status_line(id);
        }
    }

    if (boot_stat == 0U) {
        boot_stat = 1U;
        log_status_line(id);
    }

    heartbeat++;
    if (heartbeat >= USB_DBG_HEARTBEAT_LOOPS) {
        heartbeat = 0U;
        log_status_line(id);
    }
}
