#include "bdk_spi.h"

/* Bodies are intentionally empty. Implement from RM0090 SPI (chapter 28). */

void bdk_spi_init(const bdk_spi_config_t *config)
{
    (void)config;
}

uint16_t bdk_spi_transfer(bdk_spi_id_t id, uint16_t data)
{
    (void)id;
    (void)data;
    return 0;
}

void bdk_spi_transfer_buf(bdk_spi_id_t id, const uint8_t *tx, uint8_t *rx,
                          size_t len)
{
    (void)id;
    (void)tx;
    (void)rx;
    (void)len;
}
