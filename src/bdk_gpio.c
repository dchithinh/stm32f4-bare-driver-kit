#include "bdk_gpio.h"

/* Bodies are intentionally empty. Implement from RM0090 GPIO (chapter 8). */

void bdk_gpio_init(const bdk_gpio_config_t *config)
{
    (void)config;
}

void bdk_gpio_set(bdk_gpio_port_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
}

void bdk_gpio_clear(bdk_gpio_port_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
}

void bdk_gpio_toggle(bdk_gpio_port_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
}

void bdk_gpio_write(bdk_gpio_port_t port, uint8_t pin, bool value)
{
    (void)port;
    (void)pin;
    (void)value;
}

bool bdk_gpio_read(bdk_gpio_port_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
    return false;
}
