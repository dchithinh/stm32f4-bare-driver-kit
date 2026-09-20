#ifndef BDK_UART_H
#define BDK_UART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USART/UART instance on the F407.
 */
typedef enum {
    BDK_UART_1 = 1,
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
 * @brief Configure and enable a USART/UART in polling mode.
 * @param config UART configuration; must not be NULL.
 */
void bdk_uart_init(const bdk_uart_config_t *config);

/**
 * @brief Transmit one byte, blocking until the TX register is empty.
 * @param id   UART instance.
 * @param byte Byte to send.
 */
void bdk_uart_write_byte(bdk_uart_id_t id, uint8_t byte);

/**
 * @brief Receive one byte, blocking until data is available.
 * @param id UART instance.
 * @return The received byte.
 */
uint8_t bdk_uart_read_byte(bdk_uart_id_t id);

/**
 * @brief Transmit @p len bytes from @p data (polling).
 * @param id   UART instance.
 * @param data Buffer to send.
 * @param len  Number of bytes.
 */
void bdk_uart_write(bdk_uart_id_t id, const uint8_t *data, size_t len);

/**
 * @brief True if the receive data register is not empty.
 * @param id UART instance.
 */
int bdk_uart_rx_ready(bdk_uart_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* BDK_UART_H */
