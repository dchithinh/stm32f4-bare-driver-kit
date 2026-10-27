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

/** Cube `stm32f407xx_spi_lcd_2.4inch` (USE_HSI_84MHZ): HSE 8 MHz → PLL → 84 MHz SYSCLK, APB1 42 MHz. */
#define SPI_EXAMPLE_CLOCK_HSE_84MHZ                                            \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_PLL,                                      \
        .pll_src = BDK_RCC_PLLSRC_HSE, .hse_hz = 8000000U, .pllm = 8U,       \
        .plln = 168U, .pllp = 0U, .pllq = 7U, .ahb_div = 1U,                 \
        .apb1_div = 2U, .apb2_div = 1U                                         \
    }

/**
 * F407 max SYSCLK (HSE 8 MHz): 168 MHz, APB1 capped at 42 MHz (÷4).
 * SPI2 SCK max = PCLK1 / 2 = 21 MHz with @ref BDK_SPI_BAUD_DIV2.
 */
#define SPI_EXAMPLE_CLOCK_HSE_168MHZ                                           \
    {                                                                          \
        .sysclk_src = BDK_RCC_SYSCLK_PLL,                                      \
        .pll_src = BDK_RCC_PLLSRC_HSE, .hse_hz = 8000000U, .pllm = 8U,       \
        .plln = 336U, .pllp = 0U, .pllq = 7U, .ahb_div = 1U,                 \
        .apb1_div = 4U, .apb2_div = 2U                                         \
    }

#endif
