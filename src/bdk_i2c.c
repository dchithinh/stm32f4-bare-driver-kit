#include "bdk_i2c.h"

/* Bodies are intentionally empty. Implement from RM0090 I2C (chapter 27). */

bdk_status_t bdk_i2c_init(const bdk_i2c_config_t *config)
{
    if (config == NULL) {
        return BDK_ERR_PARAM;
    }
    (void)config;
    return BDK_ERR;
}

bdk_status_t bdk_i2c_write(bdk_i2c_id_t id, uint8_t addr,
                           const uint8_t *data, size_t len)
{
    (void)id;
    (void)addr;
    (void)data;
    (void)len;
    return BDK_ERR;
}

bdk_status_t bdk_i2c_read(bdk_i2c_id_t id, uint8_t addr,
                          uint8_t *data, size_t len)
{
    (void)id;
    (void)addr;
    (void)data;
    (void)len;
    return BDK_ERR;
}

bdk_status_t bdk_i2c_write_read(bdk_i2c_id_t id, uint8_t addr,
                                const uint8_t *tx, size_t tx_len,
                                uint8_t *rx, size_t rx_len)
{
    (void)id;
    (void)addr;
    (void)tx;
    (void)tx_len;
    (void)rx;
    (void)rx_len;
    return BDK_ERR;
}
