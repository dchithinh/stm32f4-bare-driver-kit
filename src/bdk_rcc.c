#include "bdk_rcc.h"
#include "stm32f4xx.h"

static const uint32_t gpio_ahb1en[] = {
    [BDK_GPIO_PORT_A] =  RCC_AHB1ENR_GPIOAEN,
    [BDK_GPIO_PORT_B] =  RCC_AHB1ENR_GPIOBEN,
    [BDK_GPIO_PORT_C] =  RCC_AHB1ENR_GPIOCEN,
    [BDK_GPIO_PORT_D] =  RCC_AHB1ENR_GPIODEN,
    [BDK_GPIO_PORT_E] =  RCC_AHB1ENR_GPIOEEN,
    [BDK_GPIO_PORT_F] =  RCC_AHB1ENR_GPIOFEN,
    [BDK_GPIO_PORT_G] =  RCC_AHB1ENR_GPIOGEN,
    [BDK_GPIO_PORT_H] =  RCC_AHB1ENR_GPIOHEN,
    [BDK_GPIO_PORT_I] =  RCC_AHB1ENR_GPIOIEN
};

bdk_status_t bdk_rcc_sysclk_init(void)
{
    return BDK_ERR;
}

bdk_status_t bdk_rcc_mco2_sysclk(void)
{
    return BDK_ERR;
}

bdk_status_t bdk_rcc_gpio_clk_enable(bdk_gpio_port_t port)
{
    if ((unsigned)port >= (sizeof gpio_ahb1en / sizeof gpio_ahb1en[0])) {
        return BDK_ERR_PARAM;
    }
    SET_BIT(RCC->AHB1ENR, gpio_ahb1en[port]);
    return BDK_OK;
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
