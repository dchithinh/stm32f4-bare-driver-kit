/**
 * Discovery CN5 — CDC ACM echo: host bulk OUT payload is sent back on bulk IN.
 *
 * Do not log each received byte on USART2: a blocking UART dump stalls USB poll,
 * overflows the RX FIFO, and can hang a Raspberry Pi USB host when picocom opens.
 */
#include "bdk_gpio.h"
#include "bdk_rcc.h"
#include "bdk_cdc.h"
#include "bdk_usb.h"
#include "bdk_usbd.h"

#include "clock_config.h"

#include "../../logger/log.h"

#include "bdk_cdc_desc.h"

static uint8_t  echo_tx[BDK_CDC_BULK_MPS];
static uint16_t echo_tx_len;
static uint8_t  echo_tx_submitted;

static void board_usb_init(void)
{
    bdk_rcc_clock_config_t clk = USB_CDC_ECHO_CLOCK_HSE_168_USB(USB_CDC_ECHO_HSE_HZ);

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

static void echo_try_send(void)
{
    bdk_status_t st;

    if (echo_tx_submitted != 0U) {
        if (bdk_cdc_tx_active(BDK_USB_OTG_FS) == 0) {
            echo_tx_submitted = 0U;
            echo_tx_len       = 0U;
        }
        return;
    }
    if (echo_tx_len == 0U) {
        return;
    }
    st = bdk_cdc_write(BDK_USB_OTG_FS, echo_tx, echo_tx_len);
    if (st == BDK_OK) {
        echo_tx_submitted = 1U;
    }
}

static void echo_collect(void)
{
    uint8_t byte;

    if (echo_tx_len != 0U || echo_tx_submitted != 0U) {
        return;
    }
    while (echo_tx_len < (uint16_t)sizeof(echo_tx) &&
           bdk_cdc_poll_in(BDK_USB_OTG_FS, &byte) == BDK_OK) {
        echo_tx[echo_tx_len] = byte;
        echo_tx_len++;
    }
}

int main(void)
{
    const bdk_usb_config_t hw = {.ignore_vbus_sense = 1U};
    const bdk_cdc_config_t cdc_cfg = {.descriptors = NULL};

    board_usb_init();
    log_redirect_uart2(115200);
    log_set_lock(NULL, NULL);
    log_info("usb_cdc_echo: CN5 ACM echo (no per-byte UART dump)");

    BDK_ASSERT_OK(bdk_cdc_init(BDK_USB_OTG_FS, &hw, &cdc_cfg));
    BDK_ASSERT_OK(bdk_cdc_irq_enable(BDK_USB_OTG_FS));
    BDK_ASSERT_OK(bdk_cdc_connect(BDK_USB_OTG_FS));

    for (;;) {
        bdk_cdc_poll(BDK_USB_OTG_FS);
        if (bdk_usbd_config_value(BDK_USB_OTG_FS) == 0U) {
            echo_tx_len         = 0U;
            echo_tx_submitted   = 0U;
            continue;
        }
        echo_collect();
        echo_try_send();
    }
}
