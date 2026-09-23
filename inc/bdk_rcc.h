#ifndef BDK_RCC_H
#define BDK_RCC_H

#include <stdint.h>

#include "bdk_status.h"
#include "bdk_gpio.h"
#include "bdk_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the F407 system clock tree (HSE/HSI/PLL, AHB/APB prescalers).
 * @note Chip-specific. Do not assume this is valid on other F4 parts.
 * @return BDK_OK on success, BDK_ERR_TIMEOUT if an oscillator/PLL does not lock.
 */
bdk_status_t bdk_rcc_sysclk_init(void);

/**
 * @brief Enable the AHB1 clock for a GPIO port.
 * @param port One of BDK_GPIO_PORT_A ... BDK_GPIO_PORT_I.
 * @return BDK_OK, or BDK_ERR_PARAM if @p port is out of range.
 */
bdk_status_t bdk_rcc_gpio_clk_enable(bdk_gpio_port_t port);

/**
 * @brief Output SYSCLK on MCO2 (pin PC9).
 *
 * Programs RCC_CFGR only (MCO2 source = SYSCLK, MCO2PRE = not divided).
 * Does not configure GPIO: set PC9 to AF0 (BDK_GPIO_AF_MCO) first.
 * At reset SYSCLK is HSI 16 MHz, so the pin should be 16 MHz until
 * bdk_rcc_sysclk_init() changes the tree.
 *
 * @return BDK_OK, or BDK_ERR until implemented.
 *
 * @see RM0090 RCC register map, RCC_CFGR bits MCO2 and MCO2PRE.
 */
bdk_status_t bdk_rcc_mco2_sysclk(void);

/**
 * @brief Enable USART/UART clock on APB1 or APB2 as appropriate.
 * @param id One of BDK_UART_1 ... BDK_UART_6 (enum, not the hardware instance
 *        number).
 * @return BDK_OK, or BDK_ERR_PARAM if @p id is out of range.
 */
bdk_status_t bdk_rcc_usart_clk_enable(bdk_uart_id_t id);

/**
 * @brief Enable I2C clock on APB1.
 * @param i2c_index 1, 2, or 3.
 * @return BDK_OK, or BDK_ERR_PARAM if @p i2c_index is invalid.
 */
bdk_status_t bdk_rcc_i2c_clk_enable(uint8_t i2c_index);

/**
 * @brief Enable SPI clock on APB1 or APB2 as appropriate.
 * @param spi_index 1, 2, or 3.
 * @return BDK_OK, or BDK_ERR_PARAM if @p spi_index is invalid.
 */
bdk_status_t bdk_rcc_spi_clk_enable(uint8_t spi_index);

/**
 * @brief Enable TIMx clock.
 * @param tim_index Timer number (1..14 as present on F407).
 * @return BDK_OK, or BDK_ERR_PARAM if @p tim_index is invalid.
 */
bdk_status_t bdk_rcc_tim_clk_enable(uint8_t tim_index);

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
