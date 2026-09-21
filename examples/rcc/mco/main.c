#include "bdk_rcc.h"
#include "bdk_gpio.h"

int main(void)
{
    /* MCO2 is PC9, AF0. SYSCLK (HSI 16 MHz at reset) is selected in RCC. */
    bdk_gpio_config_t mco2 = {
        .port  = BDK_GPIO_PORT_C,
        .pin   = 9,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_MCO,
        .otype = BDK_GPIO_OTYPE_PP,
        .speed = BDK_GPIO_SPEED_VERY_HIGH,
        .pull  = BDK_GPIO_PULL_NONE
    };

    if (bdk_gpio_init(&mco2) != BDK_OK) {
        while (1) {
        }
    }

    if (bdk_rcc_mco2_sysclk() != BDK_OK) {
        while (1) {
        }
    }

    for (;;) {
    }
}
