#ifndef BDK_GPIO_H
#define BDK_GPIO_H

#include <stdint.h>

#include "bdk_status.h"

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
 * @brief Logic level on one pin (output latch or pad).
 */
typedef enum {
    BDK_GPIO_LOW = 0,
    BDK_GPIO_HIGH = 1
} bdk_gpio_level_t;

/**
 * @brief Alternate function index (AFR, 4 bits). Pin-to-function mapping is
 *        in the STM32F407xx datasheet, not here.
 */
typedef enum {
    BDK_GPIO_AF_0 =  0x0,
    BDK_GPIO_AF_1 =  0x1,
    BDK_GPIO_AF_2 =  0x2,
    BDK_GPIO_AF_3 =  0x3,
    BDK_GPIO_AF_4 =  0x4,
    BDK_GPIO_AF_5 =  0x5,
    BDK_GPIO_AF_6 =  0x6,
    BDK_GPIO_AF_7 =  0x7,
    BDK_GPIO_AF_8 =  0x8,
    BDK_GPIO_AF_9 =  0x9,
    BDK_GPIO_AF_10 = 0xA,
    BDK_GPIO_AF_11 = 0xB,
    BDK_GPIO_AF_12 = 0xC,
    BDK_GPIO_AF_13 = 0xD,
    BDK_GPIO_AF_14 = 0xE,
    BDK_GPIO_AF_15 = 0xF
} bdk_gpio_af_t;

/* Peripheral names → AF index (STM32F407xx datasheet AF map). Same index
 * can name several peripherals; which pin carries which function is still
 * the datasheet table. */
#define BDK_GPIO_AF_RTC_50HZ   BDK_GPIO_AF_0
#define BDK_GPIO_AF_MCO        BDK_GPIO_AF_0
#define BDK_GPIO_AF_TAMPER     BDK_GPIO_AF_0
#define BDK_GPIO_AF_SWJ        BDK_GPIO_AF_0
#define BDK_GPIO_AF_TRACE      BDK_GPIO_AF_0

#define BDK_GPIO_AF_TIM1       BDK_GPIO_AF_1
#define BDK_GPIO_AF_TIM2       BDK_GPIO_AF_1

#define BDK_GPIO_AF_TIM3       BDK_GPIO_AF_2
#define BDK_GPIO_AF_TIM4       BDK_GPIO_AF_2
#define BDK_GPIO_AF_TIM5       BDK_GPIO_AF_2

#define BDK_GPIO_AF_TIM8       BDK_GPIO_AF_3
#define BDK_GPIO_AF_TIM9       BDK_GPIO_AF_3
#define BDK_GPIO_AF_TIM10      BDK_GPIO_AF_3
#define BDK_GPIO_AF_TIM11      BDK_GPIO_AF_3

#define BDK_GPIO_AF_I2C1       BDK_GPIO_AF_4
#define BDK_GPIO_AF_I2C2       BDK_GPIO_AF_4
#define BDK_GPIO_AF_I2C3       BDK_GPIO_AF_4

#define BDK_GPIO_AF_SPI1       BDK_GPIO_AF_5
#define BDK_GPIO_AF_SPI2       BDK_GPIO_AF_5
#define BDK_GPIO_AF_I2S2       BDK_GPIO_AF_5
#define BDK_GPIO_AF_I2S3EXT    BDK_GPIO_AF_5

#define BDK_GPIO_AF_SPI3       BDK_GPIO_AF_6
#define BDK_GPIO_AF_I2S3       BDK_GPIO_AF_6
#define BDK_GPIO_AF_I2S2EXT    BDK_GPIO_AF_6

#define BDK_GPIO_AF_USART1     BDK_GPIO_AF_7
#define BDK_GPIO_AF_USART2     BDK_GPIO_AF_7
#define BDK_GPIO_AF_USART3     BDK_GPIO_AF_7

#define BDK_GPIO_AF_UART4      BDK_GPIO_AF_8
#define BDK_GPIO_AF_UART5      BDK_GPIO_AF_8
#define BDK_GPIO_AF_USART6     BDK_GPIO_AF_8

#define BDK_GPIO_AF_CAN1       BDK_GPIO_AF_9
#define BDK_GPIO_AF_CAN2       BDK_GPIO_AF_9
#define BDK_GPIO_AF_TIM12      BDK_GPIO_AF_9
#define BDK_GPIO_AF_TIM13      BDK_GPIO_AF_9
#define BDK_GPIO_AF_TIM14      BDK_GPIO_AF_9

#define BDK_GPIO_AF_OTG_FS     BDK_GPIO_AF_10
#define BDK_GPIO_AF_OTG_HS     BDK_GPIO_AF_10

#define BDK_GPIO_AF_ETH        BDK_GPIO_AF_11

#define BDK_GPIO_AF_FSMC       BDK_GPIO_AF_12
#define BDK_GPIO_AF_SDIO       BDK_GPIO_AF_12
#define BDK_GPIO_AF_OTG_HS_FS  BDK_GPIO_AF_12

#define BDK_GPIO_AF_DCMI       BDK_GPIO_AF_13

#define BDK_GPIO_AF_EVENTOUT   BDK_GPIO_AF_15

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
    bdk_gpio_af_t    af;       /**< Used when mode is BDK_GPIO_MODE_AF */
} bdk_gpio_config_t;

/**
 * @brief Apply @p config to one pin (enables the port clock via bdk_rcc).
 * @param config Pin configuration; must not be NULL. pin must be 0..15.
 * @return BDK_OK, or BDK_ERR_PARAM if config/port/pin is invalid.
 */
bdk_status_t bdk_gpio_init(const bdk_gpio_config_t *config);

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
 * @brief Toggle the pin (read latch, then BSRR set or reset).
 * @param port GPIO port.
 * @param pin  Pin number 0..15.
 */
void bdk_gpio_toggle(bdk_gpio_port_t port, uint8_t pin);

/**
 * @brief Drive one pin high or low (BSRR set or reset).
 * @param port  GPIO port.
 * @param pin   Pin number 0..15.
 * @param level BDK_GPIO_HIGH or BDK_GPIO_LOW.
 */
void bdk_gpio_write_pin(bdk_gpio_port_t port, uint8_t pin, bdk_gpio_level_t level);

/**
 * @brief Read one pin from IDR (pad level).
 * @param port GPIO port.
 * @param pin  Pin number 0..15.
 * @return BDK_GPIO_HIGH or BDK_GPIO_LOW. LOW if port/pin is invalid.
 */
bdk_gpio_level_t bdk_gpio_read_pin(bdk_gpio_port_t port, uint8_t pin);

/**
 * @brief Write all 16 output bits on a port (ODR).
 * @param port  GPIO port.
 * @param value Bit n = pin n (0..15).
 */
void bdk_gpio_write(bdk_gpio_port_t port, uint16_t value);

/**
 * @brief Read all 16 input bits on a port (IDR).
 * @param port GPIO port.
 * @return Bit n = pin n (0..15). 0 if the port is invalid.
 */
uint16_t bdk_gpio_read(bdk_gpio_port_t port);

#ifdef __cplusplus
}
#endif

#endif /* BDK_GPIO_H */
