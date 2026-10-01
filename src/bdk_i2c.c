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

/** Spin limit for polling SR1 (tune per bus). */
#define BDK_I2C_POLL_LOOPS 100000U
#define BDK_I2C_WRITE      0x0UL
#define BDK_I2C_READ       0x1UL

/* Master polling helpers (file-local). PE must be set; caller owns transaction order. */

/**
 * @brief Map @p id to the I2C register block.
 * @param id  `BDK_I2C_1` … `BDK_I2C_3`.
 * @return Pointer to `I2C1`…`I2C3`, or NULL if @p id is out of range.
 */
static I2C_TypeDef *i2c_regs(bdk_i2c_id_t id);

/**
 * @brief Poll until selected @c SR1 bits match @p want_set.
 * @param regs      I2C instance (must not be NULL).
 * @param mask      Bit mask of @c I2C_SR1_* flags to watch.
 * @param want_set  Non-zero: wait until @c (SR1 & mask) == mask; zero: wait until @c (SR1 & mask) == 0.
 * @return BDK_OK when the condition is met, BDK_ERR_TIMEOUT after @c BDK_I2C_POLL_LOOPS iterations.
 */
static bdk_status_t i2c_wait_sr1(I2C_TypeDef *regs, uint32_t mask, int want_set);

/**
 * @brief Clear the acknowledge-failure flag after a slave NACK.
 * @param regs  I2C instance (must not be NULL).
 */
static void i2c_clear_af(I2C_TypeDef *regs);

/**
 * @brief Clear @c ADDR after the address byte has been sent or received.
 * @param regs  I2C instance (must not be NULL).
 */
static void i2c_clear_addr(I2C_TypeDef *regs);

/**
 * @brief Generate a START condition and wait until @c SB is set.
 * @param regs  I2C instance (must not be NULL).
 * @return BDK_OK on success, BDK_ERR_TIMEOUT if @c SB does not appear in time.
 */
static bdk_status_t i2c_start(I2C_TypeDef *regs);

/**
 * @brief Generate a STOP condition and wait until the bus is idle (@c BUSY cleared).
 * @param regs  I2C instance (must not be NULL).
 * @return BDK_OK on success, BDK_ERR_TIMEOUT if the bus stays busy.
 */
static bdk_status_t i2c_stop(I2C_TypeDef *regs);

/**
 * @brief Master-transmit the 7-bit slave address (with R/W bit) and wait for @c ADDR.
 * @param regs   I2C instance (must not be NULL).
 * @param addr7  7-bit slave address (0x08 … 0x77); not shifted.
 * @param rw     @c BDK_I2C_WRITE (0) or @c BDK_I2C_READ (1); forms @c (addr7 << 1) | rw.
 * @return BDK_OK if the slave ACKed, BDK_ERR_NACK if @c AF is set, BDK_ERR_TIMEOUT on other waits.
 */
static bdk_status_t i2c_addr_send(I2C_TypeDef *regs, uint8_t addr7, uint8_t rw);

/**
 * @brief Wait for @c TXE, then write one data byte to @c DR.
 * @param regs  I2C instance (must not be NULL).
 * @param byte  Payload byte (after address phase).
 * @return BDK_OK on success, BDK_ERR_TIMEOUT if @c TXE does not appear in time.
 */
static bdk_status_t i2c_tx_byte(I2C_TypeDef *regs, uint8_t byte);

/**
 * @brief Wait for @c BTF after the last transmit byte (required before STOP on master write).
 * @param regs  I2C instance (must not be NULL).
 * @return BDK_OK on success, BDK_ERR_TIMEOUT if @c BTF does not appear in time.
 */
static bdk_status_t i2c_tx_flush(I2C_TypeDef *regs);

/**
 * @brief Receive one byte as master (ACK or NACK per @p last).
 * @param regs  I2C instance (must not be NULL).
 * @param byte  Out: value read from @c DR (must not be NULL).
 * @param last  Non-zero: send NACK after this byte (last byte of a read); zero: ACK (more bytes follow).
 * @return BDK_OK on success, BDK_ERR_TIMEOUT if @c RXNE does not appear in time.
 */
static bdk_status_t i2c_rx_byte(I2C_TypeDef *regs, uint8_t *byte, int last);

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
    I2C_TypeDef *regs = i2c_regs(id);
    if (regs == NULL) {
        return BDK_ERR_NULL;
    }

    if (len > 0 && data == NULL) {
        return BDK_ERR_NULL;
    }

    bdk_status_t st = i2c_start(regs);
    if (st != BDK_OK) {
        return st;
    }

    st = i2c_addr_send(regs, addr, BDK_I2C_WRITE);
    if (st == BDK_ERR_NACK) {
        i2c_clear_af(regs);
        (void)i2c_stop(regs);
        return BDK_ERR_NACK;
    }
    if (st != BDK_OK) {
        (void)i2c_stop(regs);
        return st;
    }

    for (size_t i = 0; i < len; i++) {
        st = i2c_tx_byte(regs, data[i]);
        if (st != BDK_OK) {
            (void)i2c_stop(regs);
            return st;
        }
    }

    /* RM: master transmitter waits BTF before STOP (address-only probe is len == 0). */
    st = i2c_tx_flush(regs);
    if (st != BDK_OK) {
        (void)i2c_stop(regs);
        return st;
    }

    return i2c_stop(regs);
}

bdk_status_t bdk_i2c_read(bdk_i2c_id_t id, uint8_t addr,
                          uint8_t *data, size_t len)
{
    I2C_TypeDef *regs = i2c_regs(id);
    if (regs == NULL || data == NULL) {
        return BDK_ERR_NULL;
    }

    if (len == 0) {
        return BDK_ERR_RANGE;
    }

    bdk_status_t st = i2c_start(regs);
    if (st != BDK_OK) {
        return st;
    }

    st = i2c_addr_send(regs, addr, BDK_I2C_READ);
    if (st == BDK_ERR_NACK) {
        i2c_clear_af(regs);
        (void)i2c_stop(regs);
        return BDK_ERR_NACK;
    }
    if (st != BDK_OK) {
        (void)i2c_stop(regs);
        return st;
    }

    SET_BIT(regs->CR1, I2C_CR1_ACK);

    for (size_t i = 0; i < len; i++) {
        int last = (i == len - 1U) ? 1 : 0;
        st = i2c_rx_byte(regs, &data[i], last);
        if (st != BDK_OK) {
            (void)i2c_stop(regs);
            return st;
        }
    }

    return i2c_stop(regs);
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

static I2C_TypeDef *i2c_regs(bdk_i2c_id_t id)
{
    if ((unsigned)id >= BDK_ARRAY_LEN(i2c_table)) {
        return NULL;
    }
    return i2c_table[id];
}

static bdk_status_t i2c_wait_sr1(I2C_TypeDef *regs, uint32_t mask, int want_set)
{
    for (uint32_t i = 0; i < BDK_I2C_POLL_LOOPS; i++) {
        uint32_t sr1 = regs->SR1 & mask;

        if (want_set) {
            if (sr1 == mask) {
                return BDK_OK;
            }
        } else if (sr1 == 0U) {
            return BDK_OK;
        }
    }

    return BDK_ERR_TIMEOUT;
}

static void i2c_clear_af(I2C_TypeDef *regs)
{
    (void)regs->SR1;
    (void)regs->SR2;
}

static void i2c_clear_addr(I2C_TypeDef *regs)
{
    (void)regs->SR1;
    (void)regs->SR2;
}

static bdk_status_t i2c_start(I2C_TypeDef *regs)
{
    for (uint32_t i = 0; i < BDK_I2C_POLL_LOOPS; i++) {
        if ((regs->SR2 & I2C_SR2_BUSY) == 0U) {
            break;
        }
        if (i == BDK_I2C_POLL_LOOPS - 1U) {
            return BDK_ERR_TIMEOUT;
        }
    }

    SET_BIT(regs->CR1, I2C_CR1_START);

    return i2c_wait_sr1(regs, I2C_SR1_SB, 1);
}

static bdk_status_t i2c_stop(I2C_TypeDef *regs)
{
    SET_BIT(regs->CR1, I2C_CR1_STOP);

    for (uint32_t i = 0; i < BDK_I2C_POLL_LOOPS; i++) {
        if ((regs->SR2 & I2C_SR2_BUSY) == 0U) {
            return BDK_OK;
        }
    }

    return BDK_ERR_TIMEOUT;
}

static bdk_status_t i2c_addr_send(I2C_TypeDef *regs, uint8_t addr7, uint8_t rw)
{
    regs->DR = (uint8_t)((addr7 << 1) | (rw & 1U));

    for (uint32_t i = 0; i < BDK_I2C_POLL_LOOPS; i++) {
        uint32_t sr1 = regs->SR1;

        if ((sr1 & I2C_SR1_AF) != 0U) {
            return BDK_ERR_NACK;
        }
        if ((sr1 & I2C_SR1_ADDR) != 0U) {
            i2c_clear_addr(regs);
            return BDK_OK;
        }
    }

    return BDK_ERR_TIMEOUT;
}

static bdk_status_t i2c_tx_byte(I2C_TypeDef *regs, uint8_t byte)
{
    bdk_status_t st = i2c_wait_sr1(regs, I2C_SR1_TXE, 1);
    if (st != BDK_OK) {
        return st;
    }

    regs->DR = byte;

    return BDK_OK;
}

static bdk_status_t i2c_tx_flush(I2C_TypeDef *regs)
{
    return i2c_wait_sr1(regs, I2C_SR1_BTF, 1);
}

static bdk_status_t i2c_rx_byte(I2C_TypeDef *regs, uint8_t *byte, int last)
{
    if (last) {
        CLEAR_BIT(regs->CR1, I2C_CR1_ACK);
    }

    bdk_status_t st = i2c_wait_sr1(regs, I2C_SR1_RXNE, 1);
    if (st != BDK_OK) {
        if (last) {
            SET_BIT(regs->CR1, I2C_CR1_ACK);
        }
        return st;
    }

    *byte = (uint8_t)regs->DR;

    if (last) {
        SET_BIT(regs->CR1, I2C_CR1_ACK);
    }

    return BDK_OK;
}
