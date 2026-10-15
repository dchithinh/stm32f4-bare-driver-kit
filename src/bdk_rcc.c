#include <stdint.h>

#include "bdk_rcc.h"
#include "bdk_util.h"
#include "stm32f4xx.h"

#ifndef HSI_VALUE
#define HSI_VALUE 16000000U
#endif
#ifndef HSE_VALUE
#define HSE_VALUE 8000000U
#endif

extern uint32_t SystemCoreClock;
extern const uint8_t AHBPrescTable[16];
extern const uint8_t APBPrescTable[8];

#define BDK_RCC_PLL_LOOPS 100000U
#define BDK_RCC_VCO_MIN_HZ  192000000U
#define BDK_RCC_VCO_MAX_HZ  432000000U

static uint32_t s_hse_hz = HSE_VALUE;

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

static uint32_t pll_p_divider(uint8_t pllp)
{
    return ((uint32_t)pllp + 1U) * 2U;
}

static uint32_t pll_input_hz_from_cfg(const bdk_rcc_clock_config_t *cfg)
{
    if (cfg->pll_src == BDK_RCC_PLLSRC_HSE) {
        uint32_t hz = cfg->hse_hz;

        if (hz == 0U) {
            hz = HSE_VALUE;
        }
        return hz;
    }
    return HSI_VALUE / 2U;
}

static bdk_status_t encode_hpre(uint16_t div, uint32_t *hpre)
{
    switch (div) {
    case 1U:
        *hpre = RCC_CFGR_HPRE_DIV1;
        return BDK_OK;
    case 2U:
        *hpre = RCC_CFGR_HPRE_DIV2;
        return BDK_OK;
    case 4U:
        *hpre = RCC_CFGR_HPRE_DIV4;
        return BDK_OK;
    case 8U:
        *hpre = RCC_CFGR_HPRE_DIV8;
        return BDK_OK;
    case 16U:
        *hpre = RCC_CFGR_HPRE_DIV16;
        return BDK_OK;
    case 64U:
        *hpre = RCC_CFGR_HPRE_DIV64;
        return BDK_OK;
    case 128U:
        *hpre = RCC_CFGR_HPRE_DIV128;
        return BDK_OK;
    case 256U:
        *hpre = RCC_CFGR_HPRE_DIV256;
        return BDK_OK;
    default:
        return BDK_ERR_RANGE;
    }
}

static bdk_status_t encode_ppre1(uint8_t div, uint32_t *ppre)
{
    switch (div) {
    case 1U:
        *ppre = RCC_CFGR_PPRE1_DIV1;
        return BDK_OK;
    case 2U:
        *ppre = RCC_CFGR_PPRE1_DIV2;
        return BDK_OK;
    case 4U:
        *ppre = RCC_CFGR_PPRE1_DIV4;
        return BDK_OK;
    case 8U:
        *ppre = RCC_CFGR_PPRE1_DIV8;
        return BDK_OK;
    case 16U:
        *ppre = RCC_CFGR_PPRE1_DIV16;
        return BDK_OK;
    default:
        return BDK_ERR_RANGE;
    }
}

static bdk_status_t encode_ppre2(uint8_t div, uint32_t *ppre)
{
    switch (div) {
    case 1U:
        *ppre = RCC_CFGR_PPRE2_DIV1;
        return BDK_OK;
    case 2U:
        *ppre = RCC_CFGR_PPRE2_DIV2;
        return BDK_OK;
    case 4U:
        *ppre = RCC_CFGR_PPRE2_DIV4;
        return BDK_OK;
    case 8U:
        *ppre = RCC_CFGR_PPRE2_DIV8;
        return BDK_OK;
    case 16U:
        *ppre = RCC_CFGR_PPRE2_DIV16;
        return BDK_OK;
    default:
        return BDK_ERR_RANGE;
    }
}

static uint32_t flash_latency_ws(uint32_t hclk_hz)
{
    /* RM0090 Table 16, 2.7 V .. 3.6 V, voltage range 1. */
    if (hclk_hz <= 30000000U) {
        return FLASH_ACR_LATENCY_0WS;
    }
    if (hclk_hz <= 60000000U) {
        return FLASH_ACR_LATENCY_1WS;
    }
    if (hclk_hz <= 90000000U) {
        return FLASH_ACR_LATENCY_2WS;
    }
    if (hclk_hz <= 120000000U) {
        return FLASH_ACR_LATENCY_3WS;
    }
    if (hclk_hz <= 150000000U) {
        return FLASH_ACR_LATENCY_4WS;
    }
    if (hclk_hz <= 168000000U) {
        return FLASH_ACR_LATENCY_5WS;
    }
    return UINT32_MAX;
}

static bdk_status_t rcc_wait_cr_ready(uint32_t ready_bit)
{
    uint32_t i;

    for (i = 0; i < BDK_RCC_PLL_LOOPS; i++) {
        if ((RCC->CR & ready_bit) != 0U) {
            return BDK_OK;
        }
    }
    return BDK_ERR_TIMEOUT;
}

static bdk_status_t rcc_wait_sws(uint32_t sws_mask)
{
    uint32_t i;

    for (i = 0; i < BDK_RCC_PLL_LOOPS; i++) {
        if ((RCC->CFGR & RCC_CFGR_SWS) == sws_mask) {
            return BDK_OK;
        }
    }
    return BDK_ERR_TIMEOUT;
}

static bdk_status_t rcc_enable_hsi(void)
{
    SET_BIT(RCC->CR, RCC_CR_HSION);
    return rcc_wait_cr_ready(RCC_CR_HSIRDY);
}

static bdk_status_t rcc_enable_hse(uint32_t hse_hz)
{
    s_hse_hz = hse_hz;
    SET_BIT(RCC->CR, RCC_CR_HSEON);
    return rcc_wait_cr_ready(RCC_CR_HSERDY);
}

static bdk_status_t rcc_validate_pll(const bdk_rcc_clock_config_t *config,
                                     uint32_t *sysclk_hz_out)
{
    uint32_t pll_in;
    uint32_t vco_hz;
    uint32_t sysclk_hz;

    if (config == NULL) {
        return BDK_ERR_NULL;
    }
    if (config->pllm < 2U || config->pllm > 63U) {
        return BDK_ERR_RANGE;
    }
    if (config->plln < 50U || config->plln > 432U) {
        return BDK_ERR_RANGE;
    }
    if (config->pllp > 3U) {
        return BDK_ERR_RANGE;
    }
    if (config->pllq != 0U && (config->pllq < 2U || config->pllq > 15U)) {
        return BDK_ERR_RANGE;
    }

    pll_in = pll_input_hz_from_cfg(config);
    if (pll_in == 0U) {
        return BDK_ERR_RANGE;
    }

    vco_hz = (pll_in / (uint32_t)config->pllm) * (uint32_t)config->plln;
    if (vco_hz < BDK_RCC_VCO_MIN_HZ || vco_hz > BDK_RCC_VCO_MAX_HZ) {
        return BDK_ERR_RANGE;
    }

    sysclk_hz = vco_hz / pll_p_divider(config->pllp);
    if (sysclk_hz == 0U) {
        return BDK_ERR_RANGE;
    }

    if (sysclk_hz_out != NULL) {
        *sysclk_hz_out = sysclk_hz;
    }
    return BDK_OK;
}

static bdk_status_t rcc_osc_enable(const bdk_rcc_clock_config_t *config)
{
    bdk_status_t st;

    if (config->pll_src == BDK_RCC_PLLSRC_HSE) {
        s_hse_hz = config->hse_hz != 0U ? config->hse_hz : HSE_VALUE;
        SET_BIT(RCC->CR, RCC_CR_HSEON);
        st = rcc_wait_cr_ready(RCC_CR_HSERDY);
    } else {
        SET_BIT(RCC->CR, RCC_CR_HSION);
        st = rcc_wait_cr_ready(RCC_CR_HSIRDY);
    }
    return st;
}

static bdk_status_t rcc_pll_apply(const bdk_rcc_clock_config_t *config)
{
    uint32_t pllcfgr;
    bdk_status_t st;

    pllcfgr = RCC->PLLCFGR;
    pllcfgr &= ~(RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP |
                 RCC_PLLCFGR_PLLSRC);
    pllcfgr |= ((uint32_t)config->pllm << RCC_PLLCFGR_PLLM_Pos);
    pllcfgr |= ((uint32_t)config->plln << RCC_PLLCFGR_PLLN_Pos);
    pllcfgr |= ((uint32_t)config->pllp << RCC_PLLCFGR_PLLP_Pos);
    if (config->pll_src == BDK_RCC_PLLSRC_HSE) {
        pllcfgr |= RCC_PLLCFGR_PLLSRC_HSE;
    }
    if (config->pllq != 0U) {
        pllcfgr &= ~RCC_PLLCFGR_PLLQ;
        pllcfgr |= ((uint32_t)config->pllq << RCC_PLLCFGR_PLLQ_Pos);
    }
    RCC->PLLCFGR = pllcfgr;

    SET_BIT(RCC->CR, RCC_CR_PLLON);
    st = rcc_wait_cr_ready(RCC_CR_PLLRDY);
    return st;
}

static bdk_status_t encode_mco1pre(uint8_t pre_div, uint32_t *mco1pre)
{
    /*
     * F4 uses ST HAL / CMSIS bit patterns (not numeric (N-1) in MCO1PRE).
     * See RCC_MCODIV_1..5 in stm32f4xx_hal_rcc.h.
     */
    switch (pre_div) {
    case 1U:
        *mco1pre = 0U;
        return BDK_OK;
    case 2U:
        *mco1pre = RCC_CFGR_MCO1PRE_2;
        return BDK_OK;
    case 3U:
        *mco1pre = RCC_CFGR_MCO1PRE_0 | RCC_CFGR_MCO1PRE_2;
        return BDK_OK;
    case 4U:
        *mco1pre = RCC_CFGR_MCO1PRE_1 | RCC_CFGR_MCO1PRE_2;
        return BDK_OK;
    case 5U:
        *mco1pre = RCC_CFGR_MCO1PRE;
        return BDK_OK;
    default:
        return BDK_ERR_RANGE;
    }
}

static uint32_t mco1_src_bits(bdk_rcc_mco1_src_t src)
{
    switch (src) {
    case BDK_RCC_MCO1_HSI:
        return 0U;
    case BDK_RCC_MCO1_LSE:
        return RCC_CFGR_MCO1_0;
    case BDK_RCC_MCO1_HSE:
        return RCC_CFGR_MCO1_1;
    case BDK_RCC_MCO1_PLL:
        return (RCC_CFGR_MCO1_0 | RCC_CFGR_MCO1_1);
    default:
        return UINT32_MAX;
    }
}

bdk_status_t bdk_rcc_pll_enable(const bdk_rcc_clock_config_t *config)
{
    bdk_status_t st;

    st = rcc_validate_pll(config, NULL);
    if (st != BDK_OK) {
        return st;
    }
    st = rcc_osc_enable(config);
    if (st != BDK_OK) {
        return st;
    }
    return rcc_pll_apply(config);
}

bdk_status_t bdk_rcc_mco1(bdk_rcc_mco1_src_t src, uint8_t pre_div)
{
    uint32_t mco1pre;
    uint32_t mco1;
    bdk_status_t st;

    if ((unsigned)src > (unsigned)BDK_RCC_MCO1_PLL) {
        return BDK_ERR_RANGE;
    }

    st = encode_mco1pre(pre_div, &mco1pre);
    if (st != BDK_OK) {
        return st;
    }

    mco1 = mco1_src_bits(src);
    if (mco1 == UINT32_MAX) {
        return BDK_ERR_RANGE;
    }

    MODIFY_REG(RCC->CFGR, RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE, mco1 | mco1pre);
    return BDK_OK;
}

bdk_status_t bdk_rcc_mco2_sysclk(void)
{
    MODIFY_REG(RCC->CFGR, RCC_CFGR_MCO2 | RCC_CFGR_MCO2PRE, 0U);
    return BDK_OK;
}

bdk_status_t bdk_rcc_clock_init(const bdk_rcc_clock_config_t *config)
{
    uint32_t sysclk_hz;
    uint32_t hclk_hz;
    uint32_t flash_lat;
    uint32_t hpre;
    uint32_t ppre1;
    uint32_t ppre2;
    bdk_status_t st;

    if (config == NULL) {
        return BDK_ERR_NULL;
    }

    st = encode_hpre(config->ahb_div, &hpre);
    if (st != BDK_OK) {
        return st;
    }
    st = encode_ppre1(config->apb1_div, &ppre1);
    if (st != BDK_OK) {
        return st;
    }
    st = encode_ppre2(config->apb2_div, &ppre2);
    if (st != BDK_OK) {
        return st;
    }

    switch (config->sysclk_src) {
    case BDK_RCC_SYSCLK_HSI:
        sysclk_hz = HSI_VALUE;
        st = rcc_enable_hsi();
        break;
    case BDK_RCC_SYSCLK_HSE:
        sysclk_hz = config->hse_hz != 0U ? config->hse_hz : HSE_VALUE;
        if (sysclk_hz == 0U) {
            return BDK_ERR_RANGE;
        }
        st = rcc_enable_hse(sysclk_hz);
        break;
    case BDK_RCC_SYSCLK_PLL:
    default:
        st = rcc_validate_pll(config, &sysclk_hz);
        if (st != BDK_OK) {
            return st;
        }
        st = rcc_osc_enable(config);
        if (st != BDK_OK) {
            return st;
        }
        st = rcc_pll_apply(config);
        break;
    }

    if (st != BDK_OK) {
        return st;
    }

    hclk_hz = sysclk_hz / (uint32_t)config->ahb_div;
    flash_lat = flash_latency_ws(hclk_hz);
    if (flash_lat == UINT32_MAX) {
        return BDK_ERR_RANGE;
    }

    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, flash_lat);
    SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, hpre);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, ppre2);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, ppre1);

    switch (config->sysclk_src) {
    case BDK_RCC_SYSCLK_HSI:
        MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSI);
        st = rcc_wait_sws(RCC_CFGR_SWS_HSI);
        break;
    case BDK_RCC_SYSCLK_HSE:
        MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSE);
        st = rcc_wait_sws(RCC_CFGR_SWS_HSE);
        break;
    case BDK_RCC_SYSCLK_PLL:
    default:
        MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
        st = rcc_wait_sws(RCC_CFGR_SWS_PLL);
        break;
    }

    if (st != BDK_OK) {
        return st;
    }

    SystemCoreClock = sysclk_hz;
    return BDK_OK;
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

bdk_status_t bdk_rcc_spi_clk_enable(bdk_spi_id_t id)
{
    switch (id) {
    case BDK_SPI_1:
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
        return BDK_OK;
    case BDK_SPI_2:
        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);
        return BDK_OK;
    case BDK_SPI_3:
        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI3EN);
        return BDK_OK;
    default:
        return BDK_ERR_RANGE;
    }
}

bdk_status_t bdk_rcc_tim_clk_enable(uint8_t tim_index)
{
    (void)tim_index;
    return BDK_ERR_NOT_IMPL;
}

bdk_status_t bdk_rcc_usb_otg_fs_clk_enable(void)
{
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_OTGFSEN);
    return BDK_OK;
}

static uint32_t rcc_pll_input_hz(void)
{
    if ((RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) != 0U) {
        return s_hse_hz;
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

uint32_t bdk_rcc_usart_ker_clk_hz(bdk_uart_id_t id)
{
    /*
     * RM0090 §6.2: USART1/6 on APB2, USART2–5 on APB1; f_CK for baud is PCLK2/PCLK1.
     * The APB “×2 when prescaler ≠ 1” rule applies to timers on that bus, not USART.
     */
    switch (id) {
    case BDK_UART_1:
    case BDK_UART_6:
        return bdk_rcc_get_pclk2_hz();
    case BDK_UART_2:
    case BDK_UART_3:
    case BDK_UART_4:
    case BDK_UART_5:
        return bdk_rcc_get_pclk1_hz();
    default:
        return 0U;
    }
}
