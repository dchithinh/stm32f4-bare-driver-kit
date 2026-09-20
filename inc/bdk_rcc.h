#ifndef BDK_RCC_H
#define BDK_RCC_H

#include <stdint.h>

#include "bdk_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status codes used by clock setup.
 */
typedef enum {
    BDK_RCC_OK = 0,
    BDK_RCC_ERR,
    BDK_RCC_ERR_TIMEOUT
} bdk_rcc_status_t;

/**
 * @brief Configure the F407 system clock tree (HSE/HSI/PLL, AHB/APB prescalers).
 * @note Chip-specific. Do not assume this is valid on other F4 parts.
 * @return BDK_RCC_OK on success.
 */
bdk_rcc_status_t bdk_rcc_sysclk_init(void);

/**
 * @brief Enable the AHB1 clock for a GPIO port.
 * @param port GPIO port whose clock to enable.
 */
void bdk_rcc_gpio_clk_enable(bdk_gpio_port_t port);

/**
 * @brief Enable USART/UART clock on APB1 or APB2 as appropriate.
 * @param usart_index 1 for USART1, 2 for USART2, ... 6 for USART6.
 */
void bdk_rcc_usart_clk_enable(uint8_t usart_index);

/**
 * @brief Enable I2C clock on APB1.
 * @param i2c_index 1, 2, or 3.
 */
void bdk_rcc_i2c_clk_enable(uint8_t i2c_index);

/**
 * @brief Enable SPI clock on APB1 or APB2 as appropriate.
 * @param spi_index 1, 2, or 3.
 */
void bdk_rcc_spi_clk_enable(uint8_t spi_index);

/**
 * @brief Enable TIMx clock.
 * @param tim_index Timer number (1..14 as present on F407).
 */
void bdk_rcc_tim_clk_enable(uint8_t tim_index);

/**
 * @brief SYSCLK frequency in hertz after bdk_rcc_sysclk_init().
 */
uint32_t bdk_rcc_get_sysclk_hz(void);

/**
 * @brief AHB (HCLK) frequency in hertz.
 */
uint32_t bdk_rcc_get_hclk_hz(void);

/**
 * @brief APB1 (PCLK1) frequency in hertz.
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
