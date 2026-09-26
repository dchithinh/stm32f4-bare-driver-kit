#ifndef BDK_UART_H
#define BDK_UART_H

/**
 * @file bdk_uart.h
 * @brief USART driver (polling and interrupt-driven RX/TX).
 */

#include <stddef.h>
#include <stdint.h>

#include "bdk_dma.h"
#include "bdk_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USART/UART instance
 */
typedef enum {
    BDK_UART_1 = 0,
    BDK_UART_2,
    BDK_UART_3,
    BDK_UART_4,
    BDK_UART_5,
    BDK_UART_6
} bdk_uart_id_t;

/**
 * @brief Word length (CR1 M bit).
 */
typedef enum {
    BDK_UART_WORD_8 = 0,
    BDK_UART_WORD_9
} bdk_uart_word_t;

/**
 * @brief Stop bits (CR2 STOP).
 */
typedef enum {
    BDK_UART_STOP_1 = 0,
    BDK_UART_STOP_0_5,
    BDK_UART_STOP_2,
    BDK_UART_STOP_1_5
} bdk_uart_stop_t;

/**
 * @brief Parity (CR1 PCE/PS).
 */
typedef enum {
    BDK_UART_PARITY_NONE = 0,
    BDK_UART_PARITY_EVEN,
    BDK_UART_PARITY_ODD
} bdk_uart_parity_t;

/** Common bit rates in baud (not a register field). Any uint32_t is still valid. */
#define BDK_UART_BAUD_9600     9600u
#define BDK_UART_BAUD_19200    19200u
#define BDK_UART_BAUD_38400    38400u
#define BDK_UART_BAUD_57600    57600u
#define BDK_UART_BAUD_115200   115200u
#define BDK_UART_BAUD_230400   230400u
#define BDK_UART_BAUD_460800   460800u
#define BDK_UART_BAUD_921600   921600u

/**
 * @brief UART configuration.
 */
typedef struct {
    bdk_uart_id_t     id;
    uint32_t          baud;
    bdk_uart_word_t   word;
    bdk_uart_stop_t   stop;
    bdk_uart_parity_t parity;
} bdk_uart_config_t;

/**
 * @brief IRQ RX loss counters (since last @ref bdk_uart_irq_enable or stats reset).
 */
typedef struct {
    uint32_t ore_count;        /**< USART SR ORE events handled in ISR. */
    uint32_t ring_drop_count;  /**< Bytes dropped because the RX ring was full. */
} bdk_uart_rx_stats_t;

/**
 * @brief Apply @p config: clock, BRR, frame format, enable UE/TE/RE.
 * @param config Must not be NULL.
 * @return BDK_OK, @ref BDK_ERR_NULL, or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_uart_init(const bdk_uart_config_t *config);

/**
 * @brief Block until TXE, then write @p byte to DR.
 * @param id UART instance.
 */
void bdk_uart_write_byte(bdk_uart_id_t id, uint8_t byte);

/**
 * @brief Block until RXNE, then read one byte from DR.
 * @param id   UART instance.
 * @return Received byte; 0 if @p id is invalid.
 */
uint8_t bdk_uart_read_byte(bdk_uart_id_t id);

/**
 * @brief Block until @p len bytes are read from DR into @p data.
 * @param id   UART instance.
 * @param data Out buffer; must not be NULL if @p len > 0.
 * @param len  Byte count.
 */
void bdk_uart_read(bdk_uart_id_t id, uint8_t *data, size_t len);

/**
 * @brief Block until @p len bytes from @p data are written to DR.
 * @param id   UART instance.
 * @param data Must not be NULL if @p len > 0.
 * @param len  Byte count.
 */
void bdk_uart_write(bdk_uart_id_t id, const uint8_t *data, size_t len);

/**
 * @brief Report whether RXNE is set in SR.
 * @return 1 if set, 0 if clear or @p id is invalid.
 */
int bdk_uart_rx_ready(bdk_uart_id_t id);

/**
 * @brief Enable NVIC for this USART and set RXNEIE; reset driver RX/TX state.
 *
 * The application must call @ref bdk_uart_irq_handler from the matching
 * USARTx_IRQHandler vector.
 *
 * @param id USART instance.
 * @return BDK_OK or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_uart_irq_enable(bdk_uart_id_t id);

/**
 * @brief Service RX (RXNE ring + ORE), then TXE async drain.
 * @param id USART instance for this vector.
 */
void bdk_uart_irq_handler(bdk_uart_id_t id);

/**
 * @brief Queue @p data for transmit in the USART ISR (sets TXEIE until done).
 *
 * @p data must stay valid until @ref bdk_uart_tx_active is false. Only one
 * outstanding transfer per @p id.
 *
 * @param id   USART instance.
 * @param data Must not be NULL if @p len > 0.
 * @param len  Byte count.
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_BUSY.
 */
bdk_status_t bdk_uart_write_async(bdk_uart_id_t id, const uint8_t *data,
                                  size_t len);

/**
 * @brief Report whether an async TX started by @ref bdk_uart_write_async is running.
 * @return 1 if active, 0 if idle or @p id is invalid.
 */
int bdk_uart_tx_active(bdk_uart_id_t id);

/**
 * @brief Copy one byte from the internal RX ring into @p byte if available.
 * @param id   USART instance.
 * @param byte Out; must not be NULL.
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_NODATA.
 */
bdk_status_t bdk_uart_poll_in(bdk_uart_id_t id, uint8_t *byte);

/**
 * @brief Copy IRQ RX loss counters into @p stats.
 * @param id    USART instance.
 * @param stats Out; must not be NULL.
 * @return BDK_OK, @ref BDK_ERR_NULL, or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_uart_rx_stats_get(bdk_uart_id_t id, bdk_uart_rx_stats_t *stats);

/**
 * @brief Zero @ref bdk_uart_rx_stats_t counters for @p id; does not flush the ring.
 * @param id USART instance.
 * @return BDK_OK or @ref BDK_ERR_RANGE.
 */
bdk_status_t bdk_uart_rx_stats_reset(bdk_uart_id_t id);

/**
 * @brief DMA stream/channel mapping for one USART (from RM request table).
 */
typedef struct {
    bdk_dma_stream_t tx_stream;
    bdk_dma_stream_t rx_stream;
    uint8_t          channel; /**< CHSEL for both directions of this USART */
} bdk_uart_dma_t;

/**
 * @brief Remember TX/RX DMA streams for @p id (call before @ref bdk_uart_write_dma).
 * @return BDK_OK, @ref BDK_ERR_NULL, @ref BDK_ERR_RANGE, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_uart_dma_bind(bdk_uart_id_t id, const bdk_uart_dma_t *dma);

/**
 * @brief Start DMA transmit of @p len bytes from @p data.
 *
 * Requires @ref bdk_uart_dma_bind, @ref bdk_dma_config on the TX stream, and
 * USART CR3 DMAT. Completion via DMA TC IRQ
 * (not USART TXE). @p data valid until @ref bdk_uart_dma_tx_active is false.
 *
 * @return BDK_OK, @ref BDK_ERR_BUSY, or other @ref bdk_status_t codes.
 */
bdk_status_t bdk_uart_write_dma(bdk_uart_id_t id, const uint8_t *data, size_t len);

/**
 * @brief Start DMA receive of @p len bytes into @p data (fixed count).
 *
 * Requires USART CR3 DMAR. On complete, DMA TC IRQ; check @ref bdk_uart_dma_rx_active.
 */
bdk_status_t bdk_uart_read_dma(bdk_uart_id_t id, uint8_t *data, size_t len);

/**
 * @brief Non-zero while @ref bdk_uart_write_dma is in progress.
 */
int bdk_uart_dma_tx_active(bdk_uart_id_t id);

/**
 * @brief Non-zero while @ref bdk_uart_read_dma is in progress.
 */
int bdk_uart_dma_rx_active(bdk_uart_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* BDK_UART_H */
