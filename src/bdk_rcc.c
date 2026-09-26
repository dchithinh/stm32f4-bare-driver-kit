#include "bdk_rcc.h"
#include "stm32f4xx.h"

typedef struct {
    volatile uint32_t *enr;
    uint32_t           mask;
} rcc_clk_en_t;

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

static const rcc_clk_en_t usart_clk[] = {
    [BDK_UART_1] = {&RCC->APB2ENR, RCC_APB2ENR_USART1EN },
    [BDK_UART_2] = {&RCC->APB1ENR, RCC_APB1ENR_USART2EN },
    [BDK_UART_3] = {&RCC->APB1ENR, RCC_APB1ENR_USART3EN },
    [BDK_UART_4] = {&RCC->APB1ENR, RCC_APB1ENR_UART4EN  },
    [BDK_UART_5] = {&RCC->APB1ENR, RCC_APB1ENR_UART5EN  },
    [BDK_UART_6] = {&RCC->APB2ENR, RCC_APB2ENR_USART6EN },
    
};

bdk_status_t bdk_rcc_sysclk_init(void)
{
    return BDK_ERR_NOT_IMPL;
}

bdk_status_t bdk_rcc_mco2_sysclk(void)
{
    return BDK_ERR_NOT_IMPL;
}

bdk_status_t bdk_rcc_gpio_clk_enable(bdk_gpio_port_t port)
{
    if ((unsigned)port >= BDK_ARRAY_LEN(gpio_ahb1en)) {
        return BDK_ERR_RANGE;
    }
    SET_BIT(RCC->AHB1ENR, gpio_ahb1en[port]);
    return BDK_OK;
}

bdk_status_t bdk_rcc_usart_clk_enable(bdk_uart_id_t id)
{
    if ((unsigned)id >= BDK_ARRAY_LEN(usart_clk)) {
        return BDK_ERR_RANGE;
    }

    SET_BIT(*usart_clk[id].enr, usart_clk[id].mask);
    return BDK_OK;
}

bdk_status_t bdk_rcc_i2c_clk_enable(uint8_t i2c_index)
{
    (void)i2c_index;
    return BDK_ERR_NOT_IMPL;
}

bdk_status_t bdk_rcc_spi_clk_enable(uint8_t spi_index)
{
    (void)spi_index;
    return BDK_ERR_NOT_IMPL;
}

bdk_status_t bdk_rcc_tim_clk_enable(uint8_t tim_index)
{
    (void)tim_index;
    return BDK_ERR_NOT_IMPL;
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
