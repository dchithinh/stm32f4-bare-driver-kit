#include <stddef.h>

#include "stm32f4xx.h"
#include "bdk_rcc.h"
#include "bdk_gpio.h"

static GPIO_TypeDef *const gpio_table[] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI};

static GPIO_TypeDef *gpio_regs(bdk_gpio_port_t port)
{
    unsigned n = (unsigned)BDK_ARRAY_LEN(gpio_table);

    if ((unsigned)port >= n) {
        return NULL;
    }
    return gpio_table[port];
}

bdk_status_t bdk_gpio_init(const bdk_gpio_config_t *config)
{
    GPIO_TypeDef *regs = NULL;

    if (config == NULL || config->pin > 15) {
        return BDK_ERR_PARAM;
    }

    if (bdk_rcc_gpio_clk_enable(config->port) != BDK_OK) {
        return BDK_ERR_PARAM;
    }

    regs = gpio_regs(config->port);
    if (regs == NULL) {
        return BDK_ERR_PARAM;
    }

    if (config->mode == BDK_GPIO_MODE_AF) {
        if ((unsigned)config->af > (unsigned)BDK_GPIO_AF_15) {
            return BDK_ERR_PARAM;
        }

        if (config->pin <= 7 ) {
            CLEAR_BIT(regs->AFR[0], 0xFUL << (4 * config->pin));
            SET_BIT(regs->AFR[0], (uint32_t) config->af << (4 * config->pin));
        } else if (config->pin >= 8) {
            uint8_t pin = config->pin % 8;
            CLEAR_BIT(regs->AFR[1], 0xFUL << (4 * pin));
            SET_BIT(regs->AFR[1], (uint32_t) config->af << (4 * pin));
        }
    }

    CLEAR_BIT(regs->MODER, 0x3UL << (2 * config->pin));
    SET_BIT(regs->MODER, (uint32_t)config->mode << (2 * config->pin));

    CLEAR_BIT(regs->OTYPER, 0x1UL << config->pin);
    SET_BIT(regs->OTYPER, (uint32_t)config->otype << config->pin);

    CLEAR_BIT(regs->OSPEEDR, 0x3UL << (2 * config->pin));
    SET_BIT(regs->OSPEEDR, (uint32_t)config->speed << (2 * config->pin));

    CLEAR_BIT(regs->PUPDR, 0x3UL << (2 * config->pin));
    SET_BIT(regs->PUPDR, (uint32_t)config->pull << (2 * config->pin));

    return BDK_OK;
}

void bdk_gpio_set(bdk_gpio_port_t port, uint8_t pin)
{
    GPIO_TypeDef *regs = gpio_regs(port);

    if (regs != NULL && pin < 16) {
        regs->BSRR = 0x1UL << pin;
    }
}

void bdk_gpio_clear(bdk_gpio_port_t port, uint8_t pin)
{
    GPIO_TypeDef *regs = gpio_regs(port);
    if (regs != NULL && pin < 16) {
        regs->BSRR = 0x1UL << ((pin % 16) + 16);
    }
}

void bdk_gpio_toggle(bdk_gpio_port_t port, uint8_t pin)
{
    if (bdk_gpio_read_pin(port, pin) == BDK_GPIO_HIGH) {
        bdk_gpio_clear(port, pin);
    } else {
        bdk_gpio_set(port, pin);
    }
}

void bdk_gpio_write_pin(bdk_gpio_port_t port, uint8_t pin, bdk_gpio_level_t level)
{
    if (level == BDK_GPIO_HIGH) {
        bdk_gpio_set(port, pin);
    } else {
        bdk_gpio_clear(port, pin);
    }
}

bdk_gpio_level_t bdk_gpio_read_pin(bdk_gpio_port_t port, uint8_t pin)
{
    GPIO_TypeDef *regs = gpio_regs(port);

    if (regs == NULL || pin > 15) {
        return BDK_GPIO_LOW;
    }
    if (regs->IDR & (0x1UL << pin)) {
        return BDK_GPIO_HIGH;
    }
    return BDK_GPIO_LOW;
}

void bdk_gpio_write(bdk_gpio_port_t port, uint16_t value)
{
    GPIO_TypeDef *regs = gpio_regs(port);
    if (regs != NULL) {
        regs->ODR = value;
    } 
}

uint16_t bdk_gpio_read(bdk_gpio_port_t port)
{
    uint16_t reg_val = 0;
    GPIO_TypeDef *regs = gpio_regs(port);
    if (regs != NULL) {
        reg_val = (uint16_t) regs->IDR;
    }

    return reg_val;
}
