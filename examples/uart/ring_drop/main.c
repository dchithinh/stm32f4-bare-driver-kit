#include "bdk_gpio.h"
#include "bdk_uart.h"

static void delay_ms_approx(unsigned ms)
{
    extern uint32_t SystemCoreClock;
    uint32_t ticks = (SystemCoreClock / 4000U) * ms;

    for (volatile uint32_t i = 0; i < ticks; i++) {
    }
}

static void uart_puts(bdk_uart_id_t id, const char *s)
{
    while (*s != '\0') {
        bdk_uart_write_byte(id, (uint8_t)*s++);
    }
}

static void uart_put_u32(bdk_uart_id_t id, uint32_t v)
{
    char buf[11];
    int i = 10;

    buf[i] = '\0';
    if (v == 0) {
        bdk_uart_write_byte(id, '0');
        return;
    }
    while (v > 0 && i > 0) {
        buf[--i] = (char)('0' + (v % 10U));
        v /= 10U;
    }
    uart_puts(id, &buf[i]);
}

static void uart_print_ring_stats(bdk_uart_id_t id)
{
    bdk_uart_rx_stats_t st;

    uart_puts(id, "ring_drop=");
    if (bdk_uart_rx_stats_get(id, &st) == BDK_OK) {
        uart_put_u32(id, st.ring_drop_count);
    }
    uart_puts(id, "  (ore=");
    if (bdk_uart_rx_stats_get(id, &st) == BDK_OK) {
        uart_put_u32(id, st.ore_count);
    }
    uart_puts(id, ")\r\n");
}

static void usart2_pins_init(void)
{
    bdk_gpio_config_t tx = {
        .port  = BDK_GPIO_PORT_A,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_USART2,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
        .pin   = 2,
    };
    bdk_gpio_config_t rx = { 
        .port = BDK_GPIO_PORT_A, 
        .mode = BDK_GPIO_MODE_AF,
        .af = BDK_GPIO_AF_USART2, 
        .otype = BDK_GPIO_OTYPE_PP,
        .pull = BDK_GPIO_PULL_NONE, 
        .speed = BDK_GPIO_SPEED_HIGH,
        .pin = 3 
    };

    BDK_ASSERT_OK(bdk_gpio_init(&tx));
    BDK_ASSERT_OK(bdk_gpio_init(&rx));
}

int main(void)
{
    usart2_pins_init();

    bdk_uart_config_t cfg = {
        .id     = BDK_UART_2,
        .baud   = BDK_UART_BAUD_115200,
        .parity = BDK_UART_PARITY_NONE,
        .stop   = BDK_UART_STOP_1,
        .word   = BDK_UART_WORD_8,
    };

    BDK_ASSERT_OK(bdk_uart_init(&cfg));
    BDK_ASSERT_OK(bdk_uart_irq_enable(BDK_UART_2));
    BDK_ASSERT_OK(bdk_uart_rx_stats_reset(BDK_UART_2));

    uart_puts(BDK_UART_2, "uart_ring_drop: paste 128+ bytes during countdown\r\n");

    for (int sec = 5; sec > 0; sec--) {
        uart_put_u32(BDK_UART_2, (uint32_t)sec);
        uart_puts(BDK_UART_2, "...\r\n");
        delay_ms_approx(1000);
    }

    uart_print_ring_stats(BDK_UART_2);
    uart_puts(BDK_UART_2, "Done. Expect ring_drop ~ (sent - 63) if you pasted once.\r\n");

    for (;;) {
    }
}

void USART2_IRQHandler(void)
{
    bdk_uart_irq_handler(BDK_UART_2);
}
