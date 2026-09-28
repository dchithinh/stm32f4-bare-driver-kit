#include "stm32f4xx.h"
#include "bdk_i2c.h"
#include "bdk_rcc.h"
#include "bdk_util.h"

static I2C_TypeDef *const i2c_table[] = {
    [BDK_I2C_1] = I2C1,
    [BDK_I2C_2] = I2C2,
    [BDK_I2C_3] = I2C3,
};

/** RM0090: standard-mode CCR minimum is 4; fast-mode minimum is 1 (12-bit field). */
#define BDK_I2C_CCR_SM_MIN 4U
#define BDK_I2C_CCR_FM_MIN 1U
#define BDK_I2C_CCR_MAX    0xFFFU

bdk_status_t bdk_i2c_init(const bdk_i2c_config_t *config)
{
    if (config == NULL) {
        return BDK_ERR_NULL;
    }

    if ((unsigned)config->id >= BDK_ARRAY_LEN(i2c_table)) {
        return BDK_ERR_RANGE;
    }

    I2C_TypeDef *regs = i2c_table[config->id];
    if (regs == NULL) {
        return BDK_ERR_NULL;
    }

    bdk_status_t st = bdk_rcc_i2c_clk_enable(config->id);
    if (st != BDK_OK) {
        return st;
    }

    /* Disable I2C peripheral before doing any configuration */
    CLEAR_BIT(regs->CR1, I2C_CR1_PE);

    uint32_t pclk1 = bdk_rcc_get_pclk1_hz();
    if (pclk1 == 0U) {
        return BDK_ERR_RANGE;
    }

    uint32_t freq_mhz = pclk1 / 1000000U;
    /* RM: I2C input clock 2 MHz .. 42 MHz */
    if (freq_mhz < 2U || freq_mhz > 42U) {
        return BDK_ERR_RANGE;
    }

    CLEAR_BIT(regs->CR2, I2C_CR2_FREQ);
    SET_BIT(regs->CR2, (freq_mhz << I2C_CR2_FREQ_Pos) & I2C_CR2_FREQ_Msk);

    uint32_t ccr;
    uint32_t trise;

    if (config->speed_hz == BDK_I2C_SPEED_100KHZ) {
        ccr = pclk1 / (2U * config->speed_hz);
        if (ccr < BDK_I2C_CCR_SM_MIN || ccr > BDK_I2C_CCR_MAX) {
            return BDK_ERR_RANGE;
        }
        CLEAR_BIT(regs->CCR, I2C_CCR_FS);
        CLEAR_BIT(regs->CCR, I2C_CCR_DUTY);
        /* Standard mode: max rise 1000 ns -> TRISE = FREQ(MHz) + 1 */
        trise = freq_mhz + 1U;
    } else if (config->speed_hz == BDK_I2C_SPEED_400KHZ) {
        /* Fast mode, duty 2:1 (DUTY = 0) only */
        ccr = pclk1 / (3U * config->speed_hz);
        if (ccr < BDK_I2C_CCR_FM_MIN || ccr > BDK_I2C_CCR_MAX) {
            return BDK_ERR_RANGE;
        }
        SET_BIT(regs->CCR, I2C_CCR_FS);
        CLEAR_BIT(regs->CCR, I2C_CCR_DUTY);
        /* Fast mode: max rise 300 ns */
        trise = (freq_mhz * 300U) / 1000U + 1U;
    } else {
        return BDK_ERR_RANGE;
    }

    CLEAR_BIT(regs->TRISE, I2C_TRISE_TRISE_Msk);
    SET_BIT(regs->TRISE, trise & I2C_TRISE_TRISE_Msk);

    CLEAR_BIT(regs->CCR, I2C_CCR_CCR_Msk);
    SET_BIT(regs->CCR, (ccr << I2C_CCR_CCR_Pos) & I2C_CCR_CCR_Msk);

    SET_BIT(regs->CR1, I2C_CR1_PE);

    return BDK_OK;
}

bdk_status_t bdk_i2c_write(bdk_i2c_id_t id, uint8_t addr,
                           const uint8_t *data, size_t len)
{
    (void)id;
    (void)addr;
    (void)data;
    (void)len;
    return BDK_ERR_NOT_IMPL;
}

bdk_status_t bdk_i2c_read(bdk_i2c_id_t id, uint8_t addr,
                          uint8_t *data, size_t len)
{
    (void)id;
    (void)addr;
    (void)data;
    (void)len;
    return BDK_ERR_NOT_IMPL;
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
    return BDK_ERR_NOT_IMPL;
}
