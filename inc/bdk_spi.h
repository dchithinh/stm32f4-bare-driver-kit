#ifndef BDK_SPI_H
#define BDK_SPI_H

/**
 * @file bdk_spi.h
 * @brief SPI master: polling, IRQ TX, and DMA. GPIO for SCK/MOSI/MISO/CS/DC is in the app.
 */

#include <stddef.h>
#include <stdint.h>

#include "bdk_dma.h"
#include "bdk_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI instance (table index, not the hardware number).
 */
typedef enum {
    BDK_SPI_1 = 0,
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
 * @brief Frame size (DFF).
 */
typedef enum {
    BDK_SPI_WIDTH_8 = 0,
    BDK_SPI_WIDTH_16
} bdk_spi_width_t;

/**
 * @brief Master baud prescaler (CR1 BR, f_PCLK / 2^(n+1)).
 */
typedef enum {
    BDK_SPI_BAUD_DIV2 = 0,
    BDK_SPI_BAUD_DIV4,
    BDK_SPI_BAUD_DIV8,
    BDK_SPI_BAUD_DIV16,
    BDK_SPI_BAUD_DIV32,
    BDK_SPI_BAUD_DIV64,
    BDK_SPI_BAUD_DIV128,
    BDK_SPI_BAUD_DIV256
} bdk_spi_baud_t;

/**
 * @brief SPI master configuration.
 *
 * Software NSS (SSM/SSI). Chip-select, D/C, and reset pins stay in the app.
 */
typedef struct {
    bdk_spi_id_t    id;
    bdk_spi_baud_t  baud;
    bdk_spi_cpol_t  cpol;
    bdk_spi_cpha_t  cpha;
    bdk_spi_width_t width;
} bdk_spi_config_t;

/**
 * @brief Configure SPI as a full-duplex master (clock, mode, SPE).
 * @param config Must not be NULL.
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_spi_init(const bdk_spi_config_t *config);

/**
 * @brief Change frame width without a full re-init (SPE off/on as required).
 * @param id    SPI instance.
 * @param width 8- or 16-bit frames.
 * @return BDK_OK, @ref BDK_ERR_RANGE, @ref BDK_ERR_BUSY, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_spi_set_width(bdk_spi_id_t id, bdk_spi_width_t width);

/**
 * @brief Shift one frame out and wait for the matching frame in.
 * @param id  SPI instance.
 * @param tx  Frame to transmit (8-bit: low byte used).
 * @param rx  Out: received frame; may be NULL to discard.
 * @return BDK_OK, @ref BDK_ERR_RANGE, or @ref BDK_ERR_TIMEOUT.
 */
bdk_status_t bdk_spi_transfer(bdk_spi_id_t id, uint16_t tx, uint16_t *rx);

/**
 * @brief Full-duplex buffer: @p len frames. @p tx NULL sends 0xFF / 0xFFFF; @p rx NULL discards.
 * @param id  SPI instance.
 * @param tx  Transmit buffer, or NULL.
 * @param rx  Receive buffer, or NULL.
 * @param len Frame count (bytes if width is 8; half-words if 16).
 * @return BDK_OK, @ref BDK_ERR_RANGE, or @ref BDK_ERR_TIMEOUT.
 */
bdk_status_t bdk_spi_transfer_buf(bdk_spi_id_t id, const void *tx, void *rx,
                                  size_t len);

/**
 * @brief Transmit @p len frames; discard every received frame (LCD write path).
 * @param id   SPI instance.
 * @param data Must not be NULL if @p len > 0.
 * @param len  Frame count.
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_TIMEOUT.
 */
bdk_status_t bdk_spi_write(bdk_spi_id_t id, const void *data, size_t len);

/**
 * @brief Receive @p len frames while shifting idle bytes (0xFF / 0xFFFF).
 * @param id   SPI instance.
 * @param data Destination; must not be NULL if @p len > 0.
 * @param len  Frame count.
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_TIMEOUT.
 */
bdk_status_t bdk_spi_read(bdk_spi_id_t id, void *data, size_t len);

/**
 * @brief Enable NVIC for this SPI and prepare IRQ TX state.
 *
 * The application must call @ref bdk_spi_irq_handler from the matching
 * SPIx_IRQHandler vector.
 *
 * @param id SPI instance.
 * @return BDK_OK or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_spi_irq_enable(bdk_spi_id_t id);

/**
 * @brief Service TXE (async drain) and RXNE (discard unless a duplex async is added).
 * @param id SPI instance for this vector.
 */
void bdk_spi_irq_handler(bdk_spi_id_t id);

/**
 * @brief Queue @p data for transmit in the SPI ISR (TXEIE until done).
 *
 * @p data must stay valid until @ref bdk_spi_tx_active is false. Only one
 * outstanding transfer per @p id. Received frames are discarded.
 *
 * @param id   SPI instance.
 * @param data Must not be NULL if @p len > 0.
 * @param len  Frame count.
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_BUSY.
 */
bdk_status_t bdk_spi_write_async(bdk_spi_id_t id, const void *data, size_t len);

/**
 * @brief Report whether an async TX started by @ref bdk_spi_write_async is running.
 * @return 1 if active, 0 if idle or @p id is invalid.
 */
int bdk_spi_tx_active(bdk_spi_id_t id);

/**
 * @brief DMA stream/channel mapping for one SPI (from RM request table).
 *
 * TX-only LCD writes still need an RX stream if the driver drains MISO in
 * hardware (bind both; RX may use a dummy byte / no mem increment).
 */
typedef struct {
    bdk_dma_stream_t tx_stream;
    bdk_dma_stream_t rx_stream;
    uint8_t          channel; /**< CHSEL for both directions of this SPI */
} bdk_spi_dma_t;

/**
 * @brief Remember TX/RX DMA streams for @p id (call before @ref bdk_spi_write_dma).
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_spi_dma_bind(bdk_spi_id_t id, const bdk_spi_dma_t *dma);

/**
 * @brief Start DMA transmit of @p len frames from @p data.
 *
 * Requires @ref bdk_spi_dma_bind and @ref bdk_dma_config on the streams.
 * Completion via DMA TC IRQ (not SPI TXE). @p data valid until
 * @ref bdk_spi_dma_tx_active is false. App holds CS (and D/C) around the burst.
 *
 * @return BDK_OK, @ref BDK_ERR_BUSY, or other @ref bdk_status_t codes.
 */
bdk_status_t bdk_spi_write_dma(bdk_spi_id_t id, const void *data, size_t len);

/**
 * @brief Start DMA receive of @p len frames into @p data (idle TX frames).
 */
bdk_status_t bdk_spi_read_dma(bdk_spi_id_t id, void *data, size_t len);

/**
 * @brief Non-zero while @ref bdk_spi_write_dma is in progress.
 */
int bdk_spi_dma_tx_active(bdk_spi_id_t id);

/**
 * @brief Non-zero while @ref bdk_spi_read_dma is in progress.
 */
int bdk_spi_dma_rx_active(bdk_spi_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* BDK_SPI_H */
