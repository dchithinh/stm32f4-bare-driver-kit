#ifndef USB_CDC_ECHO_CLOCK_CONFIG_H
#define USB_CDC_ECHO_CLOCK_CONFIG_H

#include "bdk_rcc.h"

#define USB_CDC_ECHO_HSE_HZ  8000000U

#define USB_CDC_ECHO_CLOCK_HSE_168_USB(hse_hz_val)                             \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_PLL,                                      \
        .pll_src = BDK_RCC_PLLSRC_HSE, .hse_hz = (hse_hz_val), .pllm = 8U,   \
        .plln = 336U, .pllp = 0U, .pllq = 7U, .ahb_div = 1U,                 \
        .apb1_div = 4U, .apb2_div = 2U                                         \
    }

#endif
