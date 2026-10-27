/**
 * @file ili9341_board_dma.h
 * @brief SPI2 TX DMA for ILI9341 pixel bursts (F407 RM request table).
 */
#ifndef ILI9341_BOARD_DMA_H
#define ILI9341_BOARD_DMA_H

#include "bdk_status.h"
#include "bdk_spi.h"

#include <stddef.h>

/**
 * @brief Configure DMA1 stream for SPI2 TX.
 *
 * F407: SPI2_TX → DMA1 Stream 4 CH0. RX is drained in software (no RX DMA).
 * Call after @ref bdk_spi_init.
 */
void ili9341_board_dma_init(bdk_spi_id_t spi);

/**
 * @brief 8-bit SPI TX burst while CS is held (LCD pixel line).
 * @param len Byte count (e.g. 2 × pixel width for RGB565).
 */
bdk_status_t ili9341_board_dma_write(bdk_spi_id_t spi, const void *data, size_t len);

#endif /* ILI9341_BOARD_DMA_H */
