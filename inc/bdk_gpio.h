#ifndef BDK_GPIO_H
#define BDK_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO port identifiers (F4 GPIOA..GPIOI).
 */
typedef enum {
    BDK_GPIO_PORT_A = 0,
    BDK_GPIO_PORT_B,
    BDK_GPIO_PORT_C,
    BDK_GPIO_PORT_D,
    BDK_GPIO_PORT_E,
    BDK_GPIO_PORT_F,
    BDK_GPIO_PORT_G,
    BDK_GPIO_PORT_H,
    BDK_GPIO_PORT_I
} bdk_gpio_port_t;

/**
 * @brief Pin function (MODER).
 */
typedef enum {
    BDK_GPIO_MODE_INPUT = 0,
    BDK_GPIO_MODE_OUTPUT,
    BDK_GPIO_MODE_AF,
    BDK_GPIO_MODE_ANALOG
} bdk_gpio_mode_t;

/**
 * @brief Output type (OTYPER).
 */
typedef enum {
    BDK_GPIO_OTYPE_PP = 0,
    BDK_GPIO_OTYPE_OD
} bdk_gpio_otype_t;

/**
 * @brief Output speed (OSPEEDR).
 */
typedef enum {
    BDK_GPIO_SPEED_LOW = 0,
    BDK_GPIO_SPEED_MEDIUM,
    BDK_GPIO_SPEED_HIGH,
    BDK_GPIO_SPEED_VERY_HIGH
} bdk_gpio_speed_t;

/**
 * @brief Pull-up / pull-down (PUPDR).
 */
typedef enum {
    BDK_GPIO_PULL_NONE = 0,
    BDK_GPIO_PULL_UP,
    BDK_GPIO_PULL_DOWN
} bdk_gpio_pull_t;

/**
 * @brief Configuration for a single pin.
 */
typedef struct {
    bdk_gpio_port_t  port;
    uint8_t          pin;      /**< 0..15 */
    bdk_gpio_mode_t  mode;
    bdk_gpio_otype_t otype;
    bdk_gpio_speed_t speed;
    bdk_gpio_pull_t  pull;
    uint8_t          af;       /**< Alternate function number, 0..15 */
} bdk_gpio_config_t;

/**
 * @brief Apply @p config to one pin (clock enable is a bdk_rcc concern).
 * @param config Pin configuration; must not be NULL.
 */
void bdk_gpio_init(const bdk_gpio_config_t *config);

/**
 * @brief Drive the pin high (BSRR).
 * @param port GPIO port.
 * @param pin  Pin number 0..15.
 */
void bdk_gpio_set(bdk_gpio_port_t port, uint8_t pin);

/**
 * @brief Drive the pin low (BSRR reset half).
 * @param port GPIO port.
 * @param pin  Pin number 0..15.
 */
void bdk_gpio_clear(bdk_gpio_port_t port, uint8_t pin);

/**
 * @brief Toggle the pin (ODR).
 * @param port GPIO port.
 * @param pin  Pin number 0..15.
 */
void bdk_gpio_toggle(bdk_gpio_port_t port, uint8_t pin);

/**
 * @brief Write a logic level to the pin.
 * @param port  GPIO port.
 * @param pin   Pin number 0..15.
 * @param value true = high, false = low.
 */
void bdk_gpio_write(bdk_gpio_port_t port, uint8_t pin, bool value);

/**
 * @brief Read the pin input level (IDR).
 * @param port GPIO port.
 * @param pin  Pin number 0..15.
 * @return true if the pin reads high.
 */
bool bdk_gpio_read(bdk_gpio_port_t port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* BDK_GPIO_H */
