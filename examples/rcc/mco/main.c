#include "bdk_gpio.h"
#include "bdk_rcc.h"

#include "clock_config.h"
#include "log.h"

#include "stm32f4xx.h"

/** Matches bdk_rcc (CMSIS does not define HSI_VALUE in this tree). */
#define RCC_MCO_DBG_HSI_HZ 16000000U

static uint8_t mco1pre_to_div(uint32_t mco1pre_field)
{
    uint32_t pre = mco1pre_field & RCC_CFGR_MCO1PRE_Msk;

    if (pre == 0U) {
        return 1U;
    }
    if (pre == RCC_CFGR_MCO1PRE_2) {
        return 2U;
    }
    if (pre == (RCC_CFGR_MCO1PRE_0 | RCC_CFGR_MCO1PRE_2)) {
        return 3U;
    }
    if (pre == (RCC_CFGR_MCO1PRE_1 | RCC_CFGR_MCO1PRE_2)) {
        return 4U;
    }
    if (pre == RCC_CFGR_MCO1PRE) {
        return 5U;
    }
    return 0U;
}

static void rcc_dump_mco_debug(const bdk_rcc_clock_config_t *pll_cfg, uint8_t mco_pre_req)
{
    uint32_t pllcfgr = RCC->PLLCFGR;
    uint32_t cfgr    = RCC->CFGR;
    uint32_t cr      = RCC->CR;

    uint32_t pllm = pllcfgr & RCC_PLLCFGR_PLLM;
    uint32_t plln = (pllcfgr & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos;
    uint32_t pllp_bits = (pllcfgr & RCC_PLLCFGR_PLLP) >> RCC_PLLCFGR_PLLP_Pos;
    uint32_t pllp_div  = (pllp_bits + 1U) * 2U;
    bool pll_hse       = (pllcfgr & RCC_PLLCFGR_PLLSRC) != 0U;

    uint32_t pll_in_hz =
        pll_hse ? pll_cfg->hse_hz : (RCC_MCO_DBG_HSI_HZ / 2U);
    uint32_t vco_hz    = (pllm != 0U) ? (pll_in_hz / pllm) * plln : 0U;
    uint32_t pllclk_hz = (pllp_div != 0U) ? vco_hz / pllp_div : 0U;

    uint32_t mco1pre_field = cfgr & RCC_CFGR_MCO1PRE_Msk;
    uint8_t  mco1pre_div   = mco1pre_to_div(mco1pre_field);
    uint32_t mco1_src      = (cfgr & RCC_CFGR_MCO1) >> RCC_CFGR_MCO1_Pos;
    uint32_t sws           = (cfgr & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos;

    uint32_t mco_hz = (mco1pre_div != 0U) ? pllclk_hz / mco1pre_div : 0U;

    log_info("rcc_mco: USART2 PA2/PA3 115200");
    log_info("cfg pllm=%u plln=%u pllp=%u (req MCO1 pre_div=%u)",
             (unsigned)pll_cfg->pllm, (unsigned)pll_cfg->plln, (unsigned)pll_cfg->pllp,
             (unsigned)mco_pre_req);
    log_info("RCC CR PLLON=%u HSION=%u HSEON=%u HSERDY=%u",
             (unsigned)((cr & RCC_CR_PLLON) != 0U), (unsigned)((cr & RCC_CR_HSION) != 0U),
             (unsigned)((cr & RCC_CR_HSEON) != 0U), (unsigned)((cr & RCC_CR_HSERDY) != 0U));
    log_info("PLLCFGR 0x%08lx PLLSRC=%s pllm=%lu plln=%lu PLLP bits=%lu div=%lu",
             (unsigned long)pllcfgr, pll_hse ? "HSE" : "HSI/2",
             (unsigned long)pllm, (unsigned long)plln, (unsigned long)pllp_bits,
             (unsigned long)pllp_div);
    log_info("pll_in=%lu Hz vco=%lu Hz PLLCLK=%lu Hz (HSI=%lu cfg hse_hz=%lu)",
             (unsigned long)pll_in_hz, (unsigned long)vco_hz, (unsigned long)pllclk_hz,
             (unsigned long)RCC_MCO_DBG_HSI_HZ, (unsigned long)pll_cfg->hse_hz);
    log_info("CFGR 0x%08lx SWS=%lu MCO1 src=%lu MCO1PRE field=0x%08lx decoded_div=%u",
             (unsigned long)cfgr, (unsigned long)sws, (unsigned long)mco1_src,
             (unsigned long)mco1pre_field, (unsigned)mco1pre_div);
    log_info("PA8 est f_mco=%lu Hz (PLLCLK/%u)", (unsigned long)mco_hz,
             (unsigned)mco1pre_div);
}

int main(void)
{
    bdk_rcc_clock_config_t sysclk = RCC_MCO_SYSCLK_HSI_16MHZ;
    bdk_rcc_clock_config_t pll   = RCC_MCO_PLL_FOR_MCO1;

    BDK_ASSERT_OK(bdk_rcc_clock_init(&sysclk));
    log_redirect_uart2(BDK_UART_BAUD_115200);

    BDK_ASSERT_OK(bdk_rcc_pll_enable(&pll));

    bdk_gpio_config_t mco1 = {
        .port  = BDK_GPIO_PORT_A,
        .pin   = 8,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_MCO,
        .otype = BDK_GPIO_OTYPE_PP,
        .speed = BDK_GPIO_SPEED_VERY_HIGH,
        .pull  = BDK_GPIO_PULL_NONE,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&mco1));
    BDK_ASSERT_OK(bdk_rcc_mco1(BDK_RCC_MCO1_PLL, RCC_MCO1_PLL_PRE_DIV_5));

    rcc_dump_mco_debug(&pll, RCC_MCO1_PLL_PRE_DIV_5);

    for (;;) {
    }
}
