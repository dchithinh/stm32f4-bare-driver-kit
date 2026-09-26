#ifndef BDK_SPI_H
#define BDK_SPI_H

#include <stddef.h>
#include <stdint.h>

#include "bdk_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI instance on the F407.
 */
typedef enum {
    BDK_SPI_1 = 1,
    BDK_SPI_2,
    BDK_SPI_3
} bdk_spi_id_t;

/**
 * @brief SPI clock polarity (CPOL).
 */
typedef enum {
    BDK_SPI_CPOL_LOW = 0,
    BDK_SPI_CPOL_HIGH
} bdk_spi_cpol_t;

/**
 * @brief SPI clock phase (CPHA).
 */
typedef enum {
    BDK_SPI_CPHA_1EDGE = 0,
    BDK_SPI_CPHA_2EDGE
} bdk_spi_cpha_t;

/**
 * @brief Frame size.
 */
typedef enum {
    BDK_SPI_WIDTH_8 = 0,
    BDK_SPI_WIDTH_16
} bdk_spi_width_t;

/**
 * @brief SPI master configuration (polling).
 */
typedef struct {
    bdk_spi_id_t    id;
    uint32_t        baud_div; /**< SPI_CR1 BR field value, 0..7 */
    bdk_spi_cpol_t  cpol;
    bdk_spi_cpha_t  cpha;
    bdk_spi_width_t width;
} bdk_spi_config_t;

/**
 * @brief Configure SPI as a polling master.
 * @param config SPI configuration; must not be NULL.
 * @return BDK_OK, @ref BDK_ERR_NULL, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_spi_init(const bdk_spi_config_t *config);

/**
 * @brief Shift one frame out and return the frame shifted in.
 * @param id   SPI instance.
 * @param data Frame to transmit.
 * @return Received frame.
 */
uint16_t bdk_spi_transfer(bdk_spi_id_t id, uint16_t data);

/**
 * @brief Shift @p len 8-bit frames. @p rx may be NULL to discard.
 * @param id  SPI instance.
 * @param tx  Bytes to send; NULL sends 0xFF.
 * @param rx  Destination, or NULL.
 * @param len Number of bytes.
 */
void bdk_spi_transfer_buf(bdk_spi_id_t id, const uint8_t *tx, uint8_t *rx,
                          size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BDK_SPI_H */
