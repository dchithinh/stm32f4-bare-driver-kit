#ifndef SPI_EXAMPLE_CLOCK_CONFIG_H
#define SPI_EXAMPLE_CLOCK_CONFIG_H

#include "bdk_rcc.h"

/** Reset-style HSI: 16 MHz SYSCLK, APB1/APB2 16 MHz (matches `uart_tx` bring-up). */
#define SPI_EXAMPLE_CLOCK_HSI_16MHZ                                            \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_HSI, .pll_src = BDK_RCC_PLLSRC_HSI,       \
        .hse_hz = 0U, .pllm = 8U, .plln = 256U, .pllp = 1U, .pllq = 0U,       \
        .ahb_div = 1U, .apb1_div = 1U, .apb2_div = 1U                          \
    }

/** F407 SPI labs: HSI PLL → 64 MHz SYSCLK, APB2 64 MHz (16 MHz SCK at DIV4). */
#define SPI_EXAMPLE_CLOCK_HSI_64MHZ                                            \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_PLL,                                      \
        .pll_src = BDK_RCC_PLLSRC_HSI, .hse_hz = 0U, .pllm = 8U,             \
        .plln = 256U, .pllp = 1U, .pllq = 0U, .ahb_div = 1U,                 \
        .apb1_div = 2U, .apb2_div = 1U                                         \
    }

#endif
