#ifndef BDK_STATUS_H
#define BDK_STATUS_H

#include "bdk_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bdk_status.h
 * @brief Shared @ref bdk_status_t codes for @c bdk_* APIs and @ref bdk_status_str.
 */

typedef enum {
    BDK_OK = 0,           /**< Success. */
    BDK_ERR,              /**< Unspecified hardware or driver failure. */
    BDK_ERR_NOT_IMPL,     /**< API stub; not implemented from RM yet. */
    BDK_ERR_NULL,         /**< Required pointer argument was NULL. */
    BDK_ERR_RANGE,        /**< Id, pin, stream, length, baud, or enum out of range. */
    BDK_ERR_STATE,        /**< Invalid sequence or peripheral state for this call. */
    BDK_ERR_BUSY,         /**< Resource in use (async TX, DMA stream, etc.). */
    BDK_ERR_TIMEOUT,      /**< Hardware did not become ready in time. */
    BDK_ERR_NACK,         /**< I2C slave NACKed. */
    BDK_ERR_NODATA,       /**< Non-blocking read: no data available. */
} bdk_status_t;

/**
 * @brief Short ASCII label for logging (e.g. with semihosting or a debug UART).
 */
const char *bdk_status_str(bdk_status_t status);

/**
 * @brief Halt if @p expr is false.
 *
 * Do not write `BDK_ASSERT(bdk_gpio_init(&cfg))`: `BDK_OK` is 0, so that
 * treats success as failure. Use `BDK_ASSERT_OK(...)` or
 * `BDK_ASSERT(bdk_gpio_init(&cfg) == BDK_OK)`.
 *
 * Define `BDK_NDEBUG` to compile the check out.
 */
#ifdef BDK_NDEBUG
#define BDK_ASSERT(expr) ((void)0)
#else
#define BDK_ASSERT(expr)                                                       \
    do {                                                                       \
        if (!(expr)) {                                                         \
            for (;;) {                                                         \
            }                                                                  \
        }                                                                      \
    } while (0)
#endif

/**
 * @brief Halt unless @p status_expr returns BDK_OK (PARAM, TIMEOUT, ERR, …).
 *
 * Example: `BDK_ASSERT_OK(bdk_gpio_init(&uart_tx));`
 */
#ifdef BDK_NDEBUG
#define BDK_ASSERT_OK(status_expr) ((void)(status_expr))
#else
#define BDK_ASSERT_OK(status_expr) BDK_ASSERT((status_expr) == BDK_OK)
#endif

#ifdef __cplusplus
}
#endif

#endif /* BDK_STATUS_H */
