/**
 * Discovery CN5 OTG FS — @ref bdk_cdc (default ST-style CDC ACM descriptors).
 * Bulk data path (IN/OUT transfers) not implemented yet — see README.
 */
#include "bdk_gpio.h"
#include "bdk_rcc.h"
#include "bdk_cdc.h"
#include "bdk_usb.h"
#include "bdk_usbd.h"

#include "clock_config.h"

#include "../../logger/log.h"

static volatile uint32_t usb_bus_reset_count;
static volatile uint32_t usb_enum_done_count;

static void board_usb_init(void)
{
    bdk_rcc_clock_config_t clk = USB_CDC_CLOCK_HSE_168_USB(USB_CDC_HSE_HZ);

    BDK_ASSERT_OK(bdk_rcc_clock_init(&clk));

    bdk_gpio_config_t pwr = {
        .port  = BDK_GPIO_PORT_C,
        .pin   = 0,
        .mode  = BDK_GPIO_MODE_OUTPUT,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_LOW,
    };
    bdk_gpio_config_t dm = {
        .port = BDK_GPIO_PORT_A, .pin = 11, .mode = BDK_GPIO_MODE_AF,
        .otype = BDK_GPIO_OTYPE_PP, .pull = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH, .af = BDK_GPIO_AF_OTG_FS,
    };
    bdk_gpio_config_t dp = {
        .port = BDK_GPIO_PORT_A, .pin = 12, .mode = BDK_GPIO_MODE_AF,
        .otype = BDK_GPIO_OTYPE_PP, .pull = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH, .af = BDK_GPIO_AF_OTG_FS,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&pwr));
    bdk_gpio_write_pin(BDK_GPIO_PORT_C, 0, BDK_GPIO_HIGH);
    BDK_ASSERT_OK(bdk_gpio_init(&dm));
    BDK_ASSERT_OK(bdk_gpio_init(&dp));
}

static void on_usb_bus_reset(void)
{
    usb_bus_reset_count++;
}

static void on_usb_enum_done(void)
{
    usb_enum_done_count++;
}

static const char *setup_request_name(uint8_t bRequest)
{
    switch (bRequest) {
    case 0U:
        return "GET_STATUS";
    case 5U:
        return "SET_ADDRESS";
    case 6U:
        return "GET_DESCRIPTOR";
    case 8U:
        return "GET_CONFIGURATION";
    case 9U:
        return "SET_CONFIGURATION";
    default:
        return "?";
    }
}

static void log_setup_packet(uint32_t n, const uint8_t pkt[8])
{
    uint16_t wValue  = (uint16_t)(pkt[2] | (pkt[3] << 8));
    uint16_t wIndex  = (uint16_t)(pkt[4] | (pkt[5] << 8));
    uint16_t wLength = (uint16_t)(pkt[6] | (pkt[7] << 8));
    uint8_t  desc_type;

    log_info("usb_cdc: SETUP #%lu bmReq=0x%02x bReq=%u (%s) wVal=0x%04x wIdx=0x%04x wLen=%u",
             (unsigned long)n, (unsigned)pkt[0], (unsigned)pkt[1],
             setup_request_name(pkt[1]), (unsigned)wValue, (unsigned)wIndex,
             (unsigned)wLength);
    log_info("usb_cdc:   raw %02x %02x %02x %02x %02x %02x %02x %02x",
             pkt[0], pkt[1], pkt[2], pkt[3], pkt[4], pkt[5], pkt[6], pkt[7]);

    if (pkt[1] == 6U) {
        desc_type = (uint8_t)(wValue >> 8);
        log_info("usb_cdc:   GET_DESCRIPTOR type=%u index=%u",
                 (unsigned)desc_type, (unsigned)(wValue & 0xFFU));
    }
}

static void log_usb_irq_stats(void)
{
    bdk_usb_irq_stats_t st;

    bdk_usbd_irq_stats_get(BDK_USB_OTG_FS, &st);
    log_info("usb_cdc: irq handler=%lu usbrst=%lu enumdne=%lu rxflvl=%lu",
             (unsigned long)st.handler_calls, (unsigned long)st.usbrst,
             (unsigned long)st.enumdne, (unsigned long)st.rxflvl_batches);
    log_info("usb_cdc:   ep0 setup_updt=%lu dispatch=%lu stup=%lu stpktrx=%lu "
             "in_start=%lu in_cont=%lu in_xfrc=%lu status_arm=%lu status_done=%lu fifo_disc=%lu",
             (unsigned long)st.ep0_setup_updt, (unsigned long)st.ep0_setup_dispatch,
             (unsigned long)st.ep0_stup, (unsigned long)st.ep0_stpktrx,
             (unsigned long)st.ep0_in_start, (unsigned long)st.ep0_in_cont,
             (unsigned long)st.ep0_in_xfrc, (unsigned long)st.ep0_status_arm,
             (unsigned long)st.ep0_status_done, (unsigned long)st.fifo_bytes_discarded);
    log_info("usb_cdc:   config_value=%u",
             (unsigned)bdk_usbd_config_value(BDK_USB_OTG_FS));
}

static void poll_usb_debug(uint32_t *last_reset, uint32_t *last_enum,
                           uint32_t *last_setup_count, uint32_t *last_handler_calls)
{
    bdk_usb_irq_stats_t st;
    bdk_usbd_trace_t    tr;
    uint32_t            resets = usb_bus_reset_count;
    uint32_t            enums  = usb_enum_done_count;

    bdk_usbd_irq_stats_get(BDK_USB_OTG_FS, &st);
    bdk_usbd_trace_get(BDK_USB_OTG_FS, &tr);

    if (resets != *last_reset) {
        *last_reset = resets;
        log_info("usb_cdc: bus reset #%lu", (unsigned long)resets);
        log_usb_irq_stats();
    }
    if (enums != *last_enum) {
        *last_enum = enums;
        log_info("usb_cdc: enum done #%lu", (unsigned long)enums);
        log_usb_irq_stats();
    }
    if (tr.setup_count != *last_setup_count) {
        log_setup_packet(tr.setup_count, tr.last_setup);
        *last_setup_count = tr.setup_count;
        log_usb_irq_stats();
    } else if (st.handler_calls != *last_handler_calls) {
        *last_handler_calls = st.handler_calls;
        log_usb_irq_stats();
    }
}

int main(void)
{
    const bdk_usb_config_t hw = {.ignore_vbus_sense = 1U};
    const bdk_cdc_config_t cdc_cfg = {.descriptors = NULL};
    const bdk_usb_events_t usb_ev = {
        .on_bus_reset = on_usb_bus_reset,
        .on_enum_done = on_usb_enum_done,
    };
    uint32_t last_reset         = 0U;
    uint32_t last_enum          = 0U;
    uint32_t last_setup_count   = 0U;
    uint32_t last_handler_calls = 0U;

    board_usb_init();
    log_redirect_uart2(115200);
    log_info("usb_cdc: bdk_cdc (USART2 debug log)");
    log_info("usb_cdc: plug CN5 — paste UART log + Pi dmesg");

    BDK_ASSERT_OK(bdk_cdc_init(BDK_USB_OTG_FS, &hw, &cdc_cfg));
    bdk_usb_set_events(BDK_USB_OTG_FS, &usb_ev);
    BDK_ASSERT_OK(bdk_cdc_irq_enable(BDK_USB_OTG_FS));
    BDK_ASSERT_OK(bdk_cdc_connect(BDK_USB_OTG_FS));

    for (;;) {
        bdk_cdc_poll(BDK_USB_OTG_FS);
        poll_usb_debug(&last_reset, &last_enum, &last_setup_count, &last_handler_calls);
    }
}
