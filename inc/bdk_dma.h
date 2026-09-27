#ifndef BDK_DMA_H
#define BDK_DMA_H

#include <stddef.h>
#include <stdint.h>

#include "bdk_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bdk_dma.h
 * @brief DMA1/DMA2 stream control (any stream, any peripheral). See docs/notes/dma.md.
 */

/** Streams per controller (RM: 0–7 on DMA1 and on DMA2). */
#define BDK_DMA_STREAM_COUNT 8U

/** @ref bdk_dma_irq_enable interrupt selection (OR together). */
#define BDK_DMA_IT_TC (1u << 0) /**< Transfer complete → **TCIE** */
#define BDK_DMA_IT_TE (1u << 1) /**< Transfer error → **TEIE** */
#define BDK_DMA_IT_MASK (BDK_DMA_IT_TC | BDK_DMA_IT_TE)

typedef enum {
    BDK_DMA1 = 0,
    BDK_DMA2,
} bdk_dma_controller_t;

/**
 * @brief One hardware stream: controller + index 0 .. @ref BDK_DMA_STREAM_COUNT - 1.
 */
typedef struct {
    bdk_dma_controller_t controller;
    uint8_t              stream;
} bdk_dma_stream_t;

/** @p n: stream index 0 .. @ref BDK_DMA_STREAM_COUNT - 1. */
#define BDK_DMA_STREAM(ctrl, n) \
    ((bdk_dma_stream_t){ (bdk_dma_controller_t)(ctrl), (uint8_t)(n) })

#define BDK_DMA1_STREAM(n) BDK_DMA_STREAM(BDK_DMA1, (n))
#define BDK_DMA2_STREAM(n) BDK_DMA_STREAM(BDK_DMA2, (n))

/* The order is important, don't blindly change*/
typedef enum {
    BDK_DMA_PERIPH_TO_MEM = 0,
    BDK_DMA_MEM_TO_PERIPH,
    BDK_DMA_MEM_TO_MEM,
} bdk_dma_dir_t;

typedef enum {
    BDK_DMA_WIDTH_BYTE = 0,
    BDK_DMA_WIDTH_HALF,
    BDK_DMA_WIDTH_WORD,
} bdk_dma_width_t;

/**
 * @brief One DMA stream transfer setup (memory ↔ peripheral or memory ↔ memory).
 */
typedef struct {
    bdk_dma_stream_t stream;
    uint8_t          channel;   /**< CHSEL: 0–7, from RM request mapping table */
    bdk_dma_dir_t    direction;
    volatile void   *periph_addr; /**< Fixed address for periph↔mem; unused for mem↔mem */
    uint8_t          periph_inc;  /**< 0 = fixed (typical DR), 1 = increment */
    uint8_t          mem_inc;     /**< 1 to walk the memory buffer */
    bdk_dma_width_t  periph_width;
    bdk_dma_width_t  mem_width;
    uint8_t          circular;    /**< 1 = CIRC mode (e.g. continuous RX) */
} bdk_dma_config_t;

/**
 * @brief Check controller and stream index (0 .. @ref BDK_DMA_STREAM_COUNT - 1).
 * @return BDK_OK, @ref BDK_ERR_NULL, or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_dma_stream_valid(const bdk_dma_stream_t *s);

/**
 * @brief Configure stream registers; does not start a transfer.
 *
 * Enables the AHB1 clock for @p cfg->stream.controller (**DMA1EN** / **DMA2EN**).
 * @p cfg->channel is CHSEL only.
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, @ref BDK_ERR_BUSY,
 *         @ref BDK_ERR_STATE, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_dma_config(const bdk_dma_config_t *cfg);

/**
 * @brief Start a transfer of @p len items (item size = mem_width in config).
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, @ref BDK_ERR_BUSY,
 *         or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_dma_start(const bdk_dma_stream_t *stream, const void *mem, size_t len);

/**
 * @brief Stop stream and clear enable (abort).
 */
bdk_status_t bdk_dma_stop(const bdk_dma_stream_t *stream);

/**
 * @brief Non-zero while stream enable is set for an active transfer.
 */
int bdk_dma_busy(const bdk_dma_stream_t *stream);

/**
 * @brief Enable DMA stream interrupts selected in @p its (@ref BDK_DMA_IT_TC, @ref BDK_DMA_IT_TE).
 *
 * Sets the matching **TCIE** / **TEIE** bits on `SxCR` and enables the stream NVIC line.
 * Additive: bits already enabled are left set. @p its must be non-zero and only use @ref BDK_DMA_IT_MASK.
 * @return BDK_OK, @ref BDK_ERR_NULL, or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_dma_irq_enable(const bdk_dma_stream_t *stream, uint32_t its);

/**
 * @brief DMA stream IRQ body (TC and TE flags). Call from DMAx_StreamN_IRQHandler in app.
 */
void bdk_dma_irq_handler(const bdk_dma_stream_t *stream);

#ifdef __cplusplus
}
#endif

#endif /* BDK_DMA_H */
