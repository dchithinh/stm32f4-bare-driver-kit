#ifndef BDK_I2C_H
#define BDK_I2C_H

#include <stddef.h>
#include <stdint.h>

#include "bdk_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C instance on the F407.
 */
typedef enum {
    BDK_I2C_1 = 1,
    BDK_I2C_2,
    BDK_I2C_3
} bdk_i2c_id_t;

/**
 * @brief I2C master configuration (polling).
 */
typedef struct {
    bdk_i2c_id_t id;
    uint32_t     speed_hz; /**< e.g. 100000 or 400000 */
} bdk_i2c_config_t;

/**
 * @brief Configure I2C as a polling master.
 * @param config I2C configuration; must not be NULL.
 * @return BDK_OK, @ref BDK_ERR_NULL, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_i2c_init(const bdk_i2c_config_t *config);

/**
 * @brief Write @p len bytes to 7-bit slave @p addr (START, data, STOP).
 * @param id   I2C instance.
 * @param addr 7-bit slave address (unshifted).
 * @param data Bytes to write.
 * @param len  Number of bytes.
 * @return BDK_OK on ACK of the whole transaction.
 */
bdk_status_t bdk_i2c_write(bdk_i2c_id_t id, uint8_t addr,
                           const uint8_t *data, size_t len);

/**
 * @brief Read @p len bytes from 7-bit slave @p addr (START, data, STOP).
 * @param id   I2C instance.
 * @param addr 7-bit slave address (unshifted).
 * @param data Destination buffer.
 * @param len  Number of bytes to read.
 * @return BDK_OK on success.
 */
bdk_status_t bdk_i2c_read(bdk_i2c_id_t id, uint8_t addr,
                          uint8_t *data, size_t len);

/**
 * @brief Write then repeated-START read (typical register access).
 * @param id      I2C instance.
 * @param addr    7-bit slave address (unshifted).
 * @param tx      Bytes to write before the restart.
 * @param tx_len  Write length.
 * @param rx      Destination for the read phase.
 * @param rx_len  Read length.
 * @return BDK_OK on success.
 */
bdk_status_t bdk_i2c_write_read(bdk_i2c_id_t id, uint8_t addr,
                                const uint8_t *tx, size_t tx_len,
                                uint8_t *rx, size_t rx_len);

#ifdef __cplusplus
}
#endif

#endif /* BDK_I2C_H */
