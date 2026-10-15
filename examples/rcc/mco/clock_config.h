#ifndef RCC_MCO_CLOCK_CONFIG_H
#define RCC_MCO_CLOCK_CONFIG_H

#include "bdk_rcc.h"

/** STM32F407 Discovery external crystal (PH0/PH1). */
#define RCC_MCO_HSE_HZ  8000000U

/*
 * SYSCLK on HSI (16 MHz). PLL for MCO1 only (HSE input).
 * PLLCLK = (HSE / 8) * 320 / 4 = 80 MHz → MCO1 PLL ÷5 → 16 MHz on PA8.
 */
#define RCC_MCO_SYSCLK_HSI_16MHZ                                               \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_HSI, .ahb_div = 1U, .apb1_div = 1U,       \
        .apb2_div = 1U                                                         \
    }

#define RCC_MCO_PLL_FOR_MCO1                                                     \
    {                                                                          \
        .pll_src = BDK_RCC_PLLSRC_HSE, .hse_hz = RCC_MCO_HSE_HZ, .pllm = 8U,   \
        .plln = 320U, .pllp = 1U, .pllq = 0U, .ahb_div = 1U,                 \
        .apb1_div = 1U, .apb2_div = 1U                                         \
    }

#define RCC_MCO1_PLL_PRE_DIV_5  5U

#endif
