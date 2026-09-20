#include "bdk_rcc.h"
#include "bdk_gpio.h"

int main(void)
{

    //Leave sysclk now as it is 16MHz by default on reset
    // bdk_rcc_sysclk_init();

    bdk_gpio_config_t red_led_config = {
        .port = BDK_GPIO_PORT_D,
        .pin = 14, //red LED is in pin 14
        .mode = BDK_GPIO_MODE_OUTPUT,
        .pull = BDK_GPIO_PULL_UP,
        .otype = BDK_GPIO_OTYPE_PP,
        .speed = BDK_GPIO_SPEED_LOW
    };

    bdk_gpio_config_t green_led_config = {
        .port = BDK_GPIO_PORT_D,
        .pin = 12, //green LED is in pin 12
        .mode = BDK_GPIO_MODE_OUTPUT,
        .pull = BDK_GPIO_PULL_UP,
        .otype = BDK_GPIO_OTYPE_PP,
        .speed = BDK_GPIO_SPEED_LOW
    };


    if (bdk_gpio_init(&red_led_config) != BDK_OK) {
        while(1);
    }

    if (bdk_gpio_init(&green_led_config) != BDK_OK) {
        while(1);
    }

    for (;;) {
        bdk_gpio_toggle(red_led_config.port, red_led_config.pin);
        bdk_gpio_toggle(green_led_config.port, green_led_config.pin);
        for (int i = 0; i < 1500000 ; i++); //Approximate 1s duty
    }
}
