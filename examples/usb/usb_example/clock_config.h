#ifndef USB_EXAMPLE_CLOCK_CONFIG_H
#define USB_EXAMPLE_CLOCK_CONFIG_H

#include "bdk_rcc.h"

/** STM32F407 Discovery HSE (PH0/PH1). */
#define USB_EXAMPLE_HSE_HZ  8000000U

/*
 * F407 Discovery USB FS clock tree (RM0090):
 *   PLL in = HSE / PLLM = 8 MHz / 8 = 1 MHz
 *   VCO    = 1 MHz × 336 = 336 MHz
 *   SYSCLK = VCO / 2 = 168 MHz  (pllp = 0 → ÷2)
 *   USB    = VCO / 7 = 48 MHz   (pllq = 7)
 */
#define USB_EXAMPLE_CLOCK_HSE_168_USB(hse_hz_val)                              \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_PLL,                                      \
        .pll_src = BDK_RCC_PLLSRC_HSE, .hse_hz = (hse_hz_val), .pllm = 8U,   \
        .plln = 336U, .pllp = 0U, .pllq = 7U, .ahb_div = 1U,                 \
        .apb1_div = 4U, .apb2_div = 2U                                         \
    }

#endif
