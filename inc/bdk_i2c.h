#ifndef BDK_I2C_H
#define BDK_I2C_H

#include <stddef.h>
#include <stdint.h>

#include "bdk_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bdk_i2c.h
 * @brief Polling I2C master. GPIO for SCL/SDA is configured in the app before @ref bdk_i2c_init.
 */

/**
 * @brief I2C instance on the F407.
 */
typedef enum {
    BDK_I2C_1 = 0,
    BDK_I2C_2,
    BDK_I2C_3
} bdk_i2c_id_t;

/** Common bus rates in Hz (not register fields). Any @c uint32_t is still valid if the driver supports it. */
#define BDK_I2C_SPEED_100KHZ 100000u /**< I2C standard mode */
#define BDK_I2C_SPEED_400KHZ 400000u /**< I2C fast mode */

/**
 * @brief I2C master configuration (polling).
 */
typedef struct {
    bdk_i2c_id_t id;
    uint32_t     speed_hz; /**< @ref BDK_I2C_SPEED_100KHZ, @ref BDK_I2C_SPEED_400KHZ, or other Hz */
} bdk_i2c_config_t;

/**
 * @brief Configure I2C as a polling master (clocks, timing, PE).
 *
 * @p config fields: @c id (which I2C block), @c speed_hz (@ref BDK_I2C_SPEED_100KHZ / @ref BDK_I2C_SPEED_400KHZ).
 * Does not select pins — enable @ref bdk_rcc_i2c_clk_enable via this call and
 * program CCR/TRISE from the current APB1 clock inside the driver.
 *
 * @param config I2C configuration; must not be NULL.
 * @return BDK_OK, @ref BDK_ERR_NULL, or @ref BDK_ERR_NOT_IMPL.
 */
bdk_status_t bdk_i2c_init(const bdk_i2c_config_t *config);

/**
 * @brief Write @p len bytes to 7-bit slave @p addr (START, data, STOP).
 * @param id   I2C instance.
 * @param addr 7-bit slave address (unshifted).
 * @param data Bytes to write; may be NULL when @p len is 0 (address probe only).
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
