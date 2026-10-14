#ifndef USB_EXAMPLE_CLOCK_CONFIG_H
#define USB_EXAMPLE_CLOCK_CONFIG_H

#include "bdk_rcc.h"

/** F407 Discovery: 8 MHz HSE → 168 MHz SYSCLK, PLLQ 7 → 48 MHz USB. */
#define USB_EXAMPLE_CLOCK_HSE_168_USB(hse_hz)                                  \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_PLL,                                      \
        .pll_src = BDK_RCC_PLLSRC_HSE, .hse_hz = (hse_hz), .pllm = 8U,       \
        .plln = 336U, .pllp = 0U, .pllq = 7U, .ahb_div = 1U,                 \
        .apb1_div = 4U, .apb2_div = 2U                                         \
    }

#endif
