#include "bdk_uart.h"

/* Bodies are intentionally empty. Implement from RM0090 USART (chapter 30). */

bdk_status_t bdk_uart_init(const bdk_uart_config_t *config)
{
    if (config == NULL) {
        return BDK_ERR_PARAM;
    }
    (void)config;
    return BDK_ERR;
}

void bdk_uart_write_byte(bdk_uart_id_t id, uint8_t byte)
{
    (void)id;
    (void)byte;
}

uint8_t bdk_uart_read_byte(bdk_uart_id_t id)
{
    (void)id;
    return 0;
}

void bdk_uart_write(bdk_uart_id_t id, const uint8_t *data, size_t len)
{
    (void)id;
    (void)data;
    (void)len;
}

int bdk_uart_rx_ready(bdk_uart_id_t id)
{
    (void)id;
    return 0;
}
