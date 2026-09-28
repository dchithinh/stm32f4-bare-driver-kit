#include "bdk_rcc.h"
#include "stm32f4xx.h"

#ifndef HSI_VALUE
#define HSI_VALUE 16000000U
#endif
#ifndef HSE_VALUE
#define HSE_VALUE 8000000U
#endif

extern const uint8_t AHBPrescTable[16];
extern const uint8_t APBPrescTable[8];

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

static const uint32_t i2c_apb1en[] = {
    [BDK_I2C_1] =  RCC_APB1ENR_I2C1EN,
    [BDK_I2C_2] =  RCC_APB1ENR_I2C2EN,
    [BDK_I2C_3] =  RCC_APB1ENR_I2C3EN
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
    if ((unsigned)i2c_index >= BDK_ARRAY_LEN(i2c_apb1en)) {
        return BDK_ERR_RANGE;
    }
    SET_BIT(RCC->APB1ENR, i2c_apb1en[i2c_index]);
    return BDK_OK;
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

static uint32_t rcc_pll_input_hz(void)
{
    if ((RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) != 0U) {
        return HSE_VALUE;
    }
    return HSI_VALUE / 2U;
}

static uint32_t rcc_sysclk_hz(void)
{
    uint32_t sysclk;
    uint32_t tmp;
    uint32_t pllm;
    uint32_t pllvco;
    uint32_t pllp;

    switch (RCC->CFGR & RCC_CFGR_SWS) {
    case RCC_CFGR_SWS_HSE:
        sysclk = HSE_VALUE;
        break;
    case RCC_CFGR_SWS_PLL:
        pllm   = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
        if (pllm == 0U) {
            return 0U;
        }
        tmp    = (RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos;
        pllvco = (rcc_pll_input_hz() / pllm) * tmp;
        pllp   = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> RCC_PLLCFGR_PLLP_Pos) + 1U) * 2U;
        sysclk = pllvco / pllp;
        break;
    case RCC_CFGR_SWS_HSI:
    default:
        sysclk = HSI_VALUE;
        break;
    }

    return sysclk;
}

uint32_t bdk_rcc_get_sysclk_hz(void)
{
    return rcc_sysclk_hz();
}

uint32_t bdk_rcc_get_hclk_hz(void)
{
    uint32_t sysclk = rcc_sysclk_hz();
    uint32_t tmp    = (RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos;

    if (tmp >= 16U) {
        return 0U;
    }
    return sysclk >> AHBPrescTable[tmp];
}

uint32_t bdk_rcc_get_pclk1_hz(void)
{
    uint32_t hclk = bdk_rcc_get_hclk_hz();
    uint32_t tmp  = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;

    if (tmp >= 8U) {
        return 0U;
    }
    return hclk >> APBPrescTable[tmp];
}

uint32_t bdk_rcc_get_pclk2_hz(void)
{
    uint32_t hclk = bdk_rcc_get_hclk_hz();
    uint32_t tmp  = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;

    if (tmp >= 8U) {
        return 0U;
    }
    return hclk >> APBPrescTable[tmp];
}
