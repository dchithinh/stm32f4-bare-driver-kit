#include "bdk_gpio.h"
#include "bdk_uart.h"

int main(void)
{
    bdk_gpio_config_t uart_pin_tx = {
        .port =     BDK_GPIO_PORT_A,
        .mode =     BDK_GPIO_MODE_AF,
        .af =       BDK_GPIO_AF_USART2,
        .otype =    BDK_GPIO_OTYPE_PP,
        .pull =     BDK_GPIO_PULL_NONE,
        .speed =    BDK_GPIO_SPEED_HIGH,
        .pin =      2,
    };
    
    bdk_gpio_config_t uart_pin_rx = {
        .port =     BDK_GPIO_PORT_A,
        .mode =     BDK_GPIO_MODE_AF,
        .af =       BDK_GPIO_AF_USART2,
        .otype =    BDK_GPIO_OTYPE_PP,
        .pull =     BDK_GPIO_PULL_NONE,
        .speed =    BDK_GPIO_SPEED_HIGH,
        .pin =      3,
    };
    
    BDK_ASSERT_OK(bdk_gpio_init(&uart_pin_tx));
    BDK_ASSERT_OK(bdk_gpio_init(&uart_pin_rx));

    bdk_uart_config_t uart2_cfg = {
        .id     = BDK_UART_2,
        .baud   = BDK_UART_BAUD_9600,
        .parity = BDK_UART_PARITY_NONE,
        .stop   = BDK_UART_STOP_1,
        .word   = BDK_UART_WORD_8
    };

    BDK_ASSERT_OK(bdk_uart_init(&uart2_cfg));

    for (;;) {
        bdk_uart_write_byte(BDK_UART_2, 'H');
        bdk_uart_write_byte(BDK_UART_2, 'E');
        bdk_uart_write_byte(BDK_UART_2, 'L');
        bdk_uart_write_byte(BDK_UART_2, 'L');
        bdk_uart_write_byte(BDK_UART_2, 'O');
        bdk_uart_write_byte(BDK_UART_2, '!');
        bdk_uart_write_byte(BDK_UART_2, '\n');
        for (int i = 0; i < 1500000; i++);
    }
}
