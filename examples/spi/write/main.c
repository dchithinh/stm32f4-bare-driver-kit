/**
 * @file main.c
 * @brief Polling SPI1 master write (LCD-style: CS in the app). Implement src/bdk_spi.c from RM0090.
 *
 * Lab default: SPI1 PA5=SCK, PA7=MOSI, PA6=MISO (optional), PA4=CS GPIO.
 * Change pins for your panel. D/C and RST are not used here.
 */

#include "bdk_gpio.h"
#include "bdk_spi.h"

#define LAB_SPI      BDK_SPI_1
#define LAB_SPI_AF   BDK_GPIO_AF_SPI1
#define LAB_CS_PORT  BDK_GPIO_PORT_A
#define LAB_CS_PIN   4U

static void lab_cs_init(void)
{
    bdk_gpio_config_t cs = {
        .port  = LAB_CS_PORT,
        .pin   = LAB_CS_PIN,
        .mode  = BDK_GPIO_MODE_OUTPUT,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&cs));
    bdk_gpio_set(LAB_CS_PORT, LAB_CS_PIN);
}

static void lab_spi1_gpio_init(void)
{
    bdk_gpio_config_t sck = {
        .port  = BDK_GPIO_PORT_A,
        .pin   = 5,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = LAB_SPI_AF,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
    };
    bdk_gpio_config_t miso = sck;
    bdk_gpio_config_t mosi = sck;

    miso.pin = 6;
    mosi.pin = 7;

    BDK_ASSERT_OK(bdk_gpio_init(&sck));
    BDK_ASSERT_OK(bdk_gpio_init(&miso));
    BDK_ASSERT_OK(bdk_gpio_init(&mosi));
}

int main(void)
{
    static const uint8_t burst[] = {0x00, 0x55, 0xAA, 0xFF};

    lab_cs_init();
    lab_spi1_gpio_init();

    bdk_spi_config_t cfg = {
        .id    = LAB_SPI,
        .baud  = BDK_SPI_BAUD_DIV16,
        .cpol  = BDK_SPI_CPOL_LOW,
        .cpha  = BDK_SPI_CPHA_1EDGE,
        .width = BDK_SPI_WIDTH_8,
    };
    BDK_ASSERT_OK(bdk_spi_init(&cfg));

    for (;;) {
        bdk_gpio_clear(LAB_CS_PORT, LAB_CS_PIN);
        (void)bdk_spi_write(LAB_SPI, burst, sizeof(burst));
        bdk_gpio_set(LAB_CS_PORT, LAB_CS_PIN);

        for (volatile int i = 0; i < 800000; i++) {
        }
    }
}
