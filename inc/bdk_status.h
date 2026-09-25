#ifndef BDK_STATUS_H
#define BDK_STATUS_H

#include "bdk_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Common result codes for bdk_ init and transactions.
 */
typedef enum {
    BDK_OK = 0,       /**< Success. */
    BDK_ERR,          /**< Unspecified failure. */
    BDK_ERR_PARAM,    /**< NULL pointer, out-of-range id/pin/port. */
    BDK_ERR_TIMEOUT,  /**< Hardware did not become ready in time. */
    BDK_ERR_NACK,     /**< I2C slave NACKed. */
    BDK_ERR_NODATA,   /**< Non-blocking read: no byte available. */
    BDK_ERR_BUSY      /**< Async TX (or similar) already in progress. */
} bdk_status_t;

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
