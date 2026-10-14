#ifndef BDK_RCC_H
#define BDK_RCC_H

#include <stdint.h>

#include "bdk_status.h"
#include "bdk_gpio.h"
#include "bdk_uart.h"
#include "bdk_i2c.h"
#include "bdk_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PLL clock source (RCC PLLCFGR PLLSRC).
 */
typedef enum {
    BDK_RCC_PLLSRC_HSI = 0,
    BDK_RCC_PLLSRC_HSE,
} bdk_rcc_pllsrc_t;

/**
 * @brief SYSCLK source for @ref bdk_rcc_clock_init. Default (0) = PLL.
 */
typedef enum {
    BDK_RCC_SYSCLK_PLL = 0,
    BDK_RCC_SYSCLK_HSI,
    BDK_RCC_SYSCLK_HSE,
} bdk_rcc_sysclk_src_t;

/**
 * @brief Clock tree for @ref bdk_rcc_clock_init.
 *
 * Set @p sysclk_src to HSI, HSE, or PLL. PLL fields apply when @p sysclk_src is
 * PLL (or for @ref bdk_rcc_pll_enable only). SYSCLK from PLL:
 * (pll_in / pllm) * plln / pllp_div; pll_in = HSI/2 or HSE. pllp: 0 → ÷2 … 3 → ÷8.
 * pllq: 0 → leave PLLQ; else VCO/pllq (USB 48 MHz). Bus divisors: 1, 2, 4, 8, 16
 * (AHB also 64, 128, 256).
 */
typedef struct {
    bdk_rcc_sysclk_src_t sysclk_src;
    bdk_rcc_pllsrc_t     pll_src;
    uint32_t         hse_hz;   /**< HSE frequency when pll_src is HSE; 0 = CMSIS HSE_VALUE */
    uint8_t          pllm;       /**< 2..63 */
    uint16_t         plln;       /**< VCO multiplier (check RM VCO 192..432 MHz) */
    uint8_t          pllp;       /**< 0..3 → SYSCLK divide factor above */
    uint8_t          pllq;       /**< 0 or 2..15 */
    uint16_t         ahb_div;
    uint8_t          apb1_div;
    uint8_t          apb2_div;
} bdk_rcc_clock_config_t;

/**
 * @brief Apply @p config: oscillator, flash latency from HCLK, bus prescalers, SYSCLK mux.
 * @param config Must not be NULL; fields must satisfy RM0090 F407 limits.
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_TIMEOUT.
 */
bdk_status_t bdk_rcc_clock_init(const bdk_rcc_clock_config_t *config);

/**
 * @brief Start the main PLL from @p config without switching SYSCLK (@p sysclk_src ignored).
 */
bdk_status_t bdk_rcc_pll_enable(const bdk_rcc_clock_config_t *config);

/**
 * @brief MCO1 source (PA8). Configure GPIO AF0 before calling.
 */
typedef enum {
    BDK_RCC_MCO1_HSI = 0,
    BDK_RCC_MCO1_LSE,
    BDK_RCC_MCO1_HSE,
    BDK_RCC_MCO1_PLL,
} bdk_rcc_mco1_src_t;

/**
 * @brief Route @p src to MCO1 with output divided by @p pre_div (1..5).
 */
bdk_status_t bdk_rcc_mco1(bdk_rcc_mco1_src_t src, uint8_t pre_div);

/**
 * @brief Enable the AHB1 clock for a GPIO port.
 * @param port One of BDK_GPIO_PORT_A ... BDK_GPIO_PORT_I.
 * @return BDK_OK or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_rcc_gpio_clk_enable(bdk_gpio_port_t port);

/**
 * @brief Output SYSCLK on MCO2 (PC9). GPIO AF0 first; MCO2PRE = ÷1.
 */
bdk_status_t bdk_rcc_mco2_sysclk(void);

/**
 * @brief Enable USART/UART clock on APB1 or APB2 as appropriate.
 * @param id One of BDK_UART_1 ... BDK_UART_6 (enum, not the hardware instance
 *        number).
 * @return BDK_OK or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_rcc_usart_clk_enable(bdk_uart_id_t id);

/**
 * @brief Enable I2C clock on APB1.
 * @param i2c_index @ref BDK_I2C_1 .. @ref BDK_I2C_3 (0 .. 2).
 * @return BDK_OK, @ref BDK_ERR_RANGE, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_rcc_i2c_clk_enable(uint8_t i2c_index);

/**
 * @brief Enable SPI clock on APB1 or APB2 as appropriate.
 * @param id @ref BDK_SPI_1 .. @ref BDK_SPI_3 (0 .. 2). SPI1 is APB2; SPI2/3 are APB1.
 * @return BDK_OK, @ref BDK_ERR_RANGE, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_rcc_spi_clk_enable(bdk_spi_id_t id);

/**
 * @brief Enable TIMx clock.
 * @param tim_index Timer number (1..14 as present on F407).
 * @return BDK_OK, @ref BDK_ERR_RANGE, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_rcc_tim_clk_enable(uint8_t tim_index);

/**
 * @brief SYSCLK frequency in hertz from RCC (PLL, HSE, or HSI).
 */
uint32_t bdk_rcc_get_sysclk_hz(void);

/**
 * @brief AHB (HCLK) frequency in hertz.
 */
uint32_t bdk_rcc_get_hclk_hz(void);

/**
 * @brief APB1 (PCLK1) frequency in hertz (I2C/USART2–5 on this bus; I2C CR2 FREQ = MHz of this clock).
 */
uint32_t bdk_rcc_get_pclk1_hz(void);

/**
 * @brief APB2 (PCLK2) frequency in hertz.
 */
uint32_t bdk_rcc_get_pclk2_hz(void);

#ifdef __cplusplus
}
#endif

#endif /* BDK_RCC_H */
