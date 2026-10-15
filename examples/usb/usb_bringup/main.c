/**
 * USB CN5 bring-up using BDK drivers (bdk_usb, no device stack).
 */
#include "bdk_gpio.h"
#include "bdk_rcc.h"
#include "bdk_usb.h"
#include "bdk_uart.h"

#include "clock_config.h"
#include "log.h"

static volatile uint32_t usb_bus_reset_count;
static volatile uint32_t usb_enum_done_count;
static volatile uint32_t usb_otg_irq_count;

static void usb_clock_init(void)
{
    bdk_rcc_clock_config_t clk =
        USB_BRINGUP_CLOCK_HSE_168_USB(USB_BRINGUP_HSE_HZ);

    BDK_ASSERT_OK(bdk_rcc_clock_init(&clk));
}

static void usb_discovery_cn5_power_on(void)
{
    bdk_gpio_config_t pwr = {
        .port  = BDK_GPIO_PORT_C,
        .pin   = 0,
        .mode  = BDK_GPIO_MODE_OUTPUT,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_LOW,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&pwr));
    bdk_gpio_write_pin(BDK_GPIO_PORT_C, 0, BDK_GPIO_HIGH);
}

static void usb_fs_dm_dp_init(void)
{
    bdk_gpio_config_t dm = {
        .port  = BDK_GPIO_PORT_A,
        .pin   = 11,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_OTG_FS,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
    };
    bdk_gpio_config_t dp = {
        .port  = BDK_GPIO_PORT_A,
        .pin   = 12,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_OTG_FS,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
    };

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

void OTG_FS_IRQHandler(void)
{
    usb_otg_irq_count++;
    bdk_usb_irq_handler(BDK_USB_OTG_FS);
}

static void log_usb_irq_stats(uint32_t nvic_count)
{
    bdk_usb_irq_stats_t st;

    bdk_usb_irq_stats_get(BDK_USB_OTG_FS, &st);
    log_info("usb_bringup: OTG_FS_IRQHandler #%lu (bdk handler_calls=%lu)",
             (unsigned long)nvic_count, (unsigned long)st.handler_calls);
    log_info("usb_bringup:   usbrst=%lu enumdne=%lu rxflvl=%lu ep0_stup=%lu "
             "fifo_disc=%lu",
             (unsigned long)st.usbrst, (unsigned long)st.enumdne,
             (unsigned long)st.rxflvl_batches, (unsigned long)st.ep0_stup,
             (unsigned long)st.fifo_bytes_discarded);
}

static void poll_usb_events(uint32_t *last_irq, uint32_t *last_reset,
                           uint32_t *last_enum, uint32_t *last_handler_calls,
                           uint32_t *last_fifo_disc)
{
    uint32_t irqs   = usb_otg_irq_count;
    uint32_t resets = usb_bus_reset_count;
    uint32_t enums  = usb_enum_done_count;
    bdk_usb_irq_stats_t st;

    bdk_usb_irq_stats_get(BDK_USB_OTG_FS, &st);

    if (irqs != *last_irq) {
        *last_irq = irqs;
        log_usb_irq_stats(irqs);
        *last_handler_calls = st.handler_calls;
        *last_fifo_disc     = st.fifo_bytes_discarded;
    } else if (st.handler_calls != *last_handler_calls ||
               st.fifo_bytes_discarded != *last_fifo_disc) {
        /*
         * Host retries often reuse the same NVIC entry count the user watches;
         * handler_calls / fifo_disc still move when the core IRQs again.
         */
        *last_handler_calls = st.handler_calls;
        *last_fifo_disc     = st.fifo_bytes_discarded;
        log_usb_irq_stats(irqs);
    }
    if (resets != *last_reset) {
        *last_reset = resets;
        log_info("usb_bringup: bus reset #%lu", (unsigned long)resets);
    }
    if (enums != *last_enum) {
        *last_enum = enums;
        log_info("usb_bringup: enum done #%lu", (unsigned long)enums);
    }
}

int main(void)
{
    const bdk_usb_config_t usb_cfg = {
        .ignore_vbus_sense = 1U,
    };
    const bdk_usb_events_t usb_ev = {
        .on_bus_reset = on_usb_bus_reset,
        .on_enum_done = on_usb_enum_done,
    };
    uint32_t last_irq            = 0U;
    uint32_t last_reset          = 0U;
    uint32_t last_enum           = 0U;
    uint32_t last_handler_calls  = 0U;
    uint32_t last_fifo_disc      = 0U;

    usb_clock_init();
    log_redirect_uart2(BDK_UART_BAUD_115200);
    log_info("usb_bringup: OTG FS init (bdk_usb, no device stack)");
    log_info("usb_bringup: ker=%lu Hz sysclk=%lu",
             (unsigned long)bdk_rcc_usart_ker_clk_hz(BDK_UART_2),
             (unsigned long)bdk_rcc_get_sysclk_hz());

    usb_discovery_cn5_power_on();
    usb_fs_dm_dp_init();

    BDK_ASSERT_OK(bdk_usb_init(BDK_USB_OTG_FS, &usb_cfg));
    bdk_usb_set_events(BDK_USB_OTG_FS, &usb_ev);
    BDK_ASSERT_OK(bdk_usb_irq_enable(BDK_USB_OTG_FS));
    BDK_ASSERT_OK(bdk_usb_connect(BDK_USB_OTG_FS));

    log_info("usb_bringup: soft connect — plug CN5; watch IRQ stats / host dmesg");

    for (;;) {
        poll_usb_events(&last_irq, &last_reset, &last_enum, &last_handler_calls,
                        &last_fifo_disc);
    }
}
