/**
 * @file main.c
 * @brief SPI1 master TX via DMA (LCD-style CS in the app). F407 Discovery lab wiring.
 *
 * SPI1 TX: DMA2 Stream3 CH3. SPI1 RX: DMA2 Stream0 CH3 (dummy drain, mem_inc 0).
 * Change the bind if your RM row differs.
 */

#include "stm32f4xx.h"

#include "bdk_dma.h"
#include "bdk_gpio.h"
#include "bdk_rcc.h"
#include "bdk_spi.h"

#define LAB_SPI     BDK_SPI_1
#define LAB_SPI_AF  BDK_GPIO_AF_SPI1
#define LAB_CS_PORT BDK_GPIO_PORT_A
#define LAB_CS_PIN  4U

static const bdk_dma_stream_t lab_spi1_tx = BDK_DMA2_STREAM(3);
static const bdk_dma_stream_t lab_spi1_rx = BDK_DMA2_STREAM(0);

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

static void lab_spi1_dma_init(void)
{
    bdk_dma_config_t dma_tx = {
        .stream       = lab_spi1_tx,
        .channel      = 3,
        .direction    = BDK_DMA_MEM_TO_PERIPH,
        .periph_addr  = &SPI1->DR,
        .periph_inc   = 0,
        .mem_inc      = 1,
        .periph_width = BDK_DMA_WIDTH_BYTE,
        .mem_width    = BDK_DMA_WIDTH_BYTE,
        .circular     = 0,
    };
    bdk_dma_config_t dma_rx = {
        .stream       = lab_spi1_rx,
        .channel      = 3,
        .direction    = BDK_DMA_PERIPH_TO_MEM,
        .periph_addr  = &SPI1->DR,
        .periph_inc   = 0,
        .mem_inc      = 0,
        .periph_width = BDK_DMA_WIDTH_BYTE,
        .mem_width    = BDK_DMA_WIDTH_BYTE,
        .circular     = 0,
    };
    bdk_spi_dma_t bind = {
        .tx_stream = lab_spi1_tx,
        .rx_stream = lab_spi1_rx,
        .channel   = 3,
    };

    BDK_ASSERT_OK(bdk_dma_config(&dma_tx));
    BDK_ASSERT_OK(bdk_dma_config(&dma_rx));
    BDK_ASSERT_OK(bdk_spi_dma_bind(LAB_SPI, &bind));
}

int main(void)
{
    static const uint8_t burst[] = {
        0x00, 0x55, 0xAA, 0xFF,
        0x00, 0x55, 0xAA, 0xFF,
        0x00, 0x55, 0xAA, 0xFF,
        0x00, 0x55, 0xAA, 0xFF,
    };

    lab_cs_init();
    lab_spi1_gpio_init();

    bdk_spi_config_t cfg = {
        .id    = LAB_SPI,
        .baud  = BDK_SPI_BAUD_DIV4,
        .cpol  = BDK_SPI_CPOL_LOW,
        .cpha  = BDK_SPI_CPHA_1EDGE,
        .width = BDK_SPI_WIDTH_8,
    };
    BDK_ASSERT_OK(bdk_rcc_sysclk_init());
    BDK_ASSERT_OK(bdk_spi_init(&cfg));
    lab_spi1_dma_init();

    for (;;) {
        bdk_gpio_clear(LAB_CS_PORT, LAB_CS_PIN);
        BDK_ASSERT_OK(bdk_spi_write_dma(LAB_SPI, burst, sizeof(burst)));
        while (bdk_spi_dma_tx_active(LAB_SPI) != 0) {
        }
        bdk_gpio_set(LAB_CS_PORT, LAB_CS_PIN);

        for (volatile int i = 0; i < 800000; i++) {
        }
    }
}
