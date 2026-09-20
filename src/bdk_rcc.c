#include "bdk_rcc.h"

/* Bodies are intentionally empty. Implement from RM0090 RCC (chapter 7).
 * Sysclk setup is F407-specific (clock tree, PLL limits, USB 48 MHz). */

bdk_rcc_status_t bdk_rcc_sysclk_init(void)
{
    return BDK_RCC_ERR;
}

void bdk_rcc_gpio_clk_enable(bdk_gpio_port_t port)
{
    (void)port;
}

void bdk_rcc_usart_clk_enable(uint8_t usart_index)
{
    (void)usart_index;
}

void bdk_rcc_i2c_clk_enable(uint8_t i2c_index)
{
    (void)i2c_index;
}

void bdk_rcc_spi_clk_enable(uint8_t spi_index)
{
    (void)spi_index;
}

void bdk_rcc_tim_clk_enable(uint8_t tim_index)
{
    (void)tim_index;
}

uint32_t bdk_rcc_get_sysclk_hz(void)
{
    return 0;
}

uint32_t bdk_rcc_get_hclk_hz(void)
{
    return 0;
}

uint32_t bdk_rcc_get_pclk1_hz(void)
{
    return 0;
}

uint32_t bdk_rcc_get_pclk2_hz(void)
{
    return 0;
}
