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
 * @brief SYSCLK = 64 MHz from HSI + PLL (AHB 64 MHz, APB2 64 MHz, APB1 32 MHz).
 * @note Chip-specific. SPI1 @ @ref BDK_SPI_BAUD_DIV4 is then 16 MHz SCK.
 * @return BDK_OK on success, BDK_ERR_TIMEOUT if HSI/PLL does not lock.
 */
bdk_status_t bdk_rcc_sysclk_init(void);

/**
 * @brief Enable the AHB1 clock for a GPIO port.
 * @param port One of BDK_GPIO_PORT_A ... BDK_GPIO_PORT_I.
 * @return BDK_OK or @ref BDK_ERR_RANGE.
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
 * @brief SYSCLK frequency in hertz after bdk_rcc_sysclk_init().
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
