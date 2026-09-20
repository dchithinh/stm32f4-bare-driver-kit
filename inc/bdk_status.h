#ifndef BDK_STATUS_H
#define BDK_STATUS_H

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
    BDK_ERR_NACK      /**< I2C slave NACKed. */
} bdk_status_t;

#ifdef __cplusplus
}
#endif

#endif /* BDK_STATUS_H */
