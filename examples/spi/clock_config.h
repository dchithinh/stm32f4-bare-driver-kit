#ifndef SPI_EXAMPLE_CLOCK_CONFIG_H
#define SPI_EXAMPLE_CLOCK_CONFIG_H

#include "bdk_rcc.h"

/** F407 SPI labs: HSI PLL → 64 MHz SYSCLK, APB2 64 MHz (16 MHz SCK at DIV4). */
#define SPI_EXAMPLE_CLOCK_HSI_64MHZ                                            \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_PLL,                                      \
        .pll_src = BDK_RCC_PLLSRC_HSI, .hse_hz = 0U, .pllm = 8U,             \
        .plln = 256U, .pllp = 1U, .pllq = 0U, .ahb_div = 1U,                 \
        .apb1_div = 2U, .apb2_div = 1U                                         \
    }

#endif
