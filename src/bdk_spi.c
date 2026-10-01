#include "stm32f4xx.h"

#include "bdk_dma.h"
#include "bdk_rcc.h"
#include "bdk_spi.h"
#include "bdk_util.h"

#define BDK_SPI_POLL_LOOPS 100000U

static SPI_TypeDef *const spi_table[] = {
    [BDK_SPI_1] = SPI1,
    [BDK_SPI_2] = SPI2,
    [BDK_SPI_3] = SPI3,
};

static const IRQn_Type spi_irq_table[] = {
    [BDK_SPI_1] = SPI1_IRQn,
    [BDK_SPI_2] = SPI2_IRQn,
    [BDK_SPI_3] = SPI3_IRQn,
};

typedef struct {
    const uint8_t   *data;
    size_t           len;
    size_t           tx_idx;
    size_t           rx_got;
    volatile uint8_t active;
} spi_irq_tx_t;

static spi_irq_tx_t       spi_irq_tx[BDK_ARRAY_LEN(spi_table)];
static bdk_spi_dma_t      spi_dma_map[BDK_ARRAY_LEN(spi_table)];
static uint8_t            spi_dma_bound[BDK_ARRAY_LEN(spi_table)];
static volatile uint8_t   spi_dma_tx_run[BDK_ARRAY_LEN(spi_table)];
static volatile uint8_t   spi_dma_rx_run[BDK_ARRAY_LEN(spi_table)];
static uint16_t           spi_dma_rx_sink;
static uint16_t           spi_dma_tx_idle = 0xFFFFU;

static SPI_TypeDef *spi_regs(bdk_spi_id_t id)
{
    if ((unsigned)id >= BDK_ARRAY_LEN(spi_table)) {
        return NULL;
    }
    return spi_table[id];
}

static int spi_width16(const SPI_TypeDef *regs)
{
    return (regs->CR1 & SPI_CR1_DFF) != 0U;
}

static uint16_t spi_idle_frame(const SPI_TypeDef *regs)
{
    return spi_width16(regs) ? 0xFFFFU : 0xFFU;
}

static void spi_dr_write(SPI_TypeDef *regs, uint16_t frame)
{
    if (spi_width16(regs)) {
        regs->DR = frame;
    } else {
        *(__IO uint8_t *)&regs->DR = (uint8_t)frame;
    }
}

static uint16_t spi_dr_read(SPI_TypeDef *regs)
{
    if (spi_width16(regs)) {
        return (uint16_t)regs->DR;
    }
    return *(__IO uint8_t *)&regs->DR;
}

static bdk_status_t spi_wait_sr(SPI_TypeDef *regs, uint32_t mask, int want_set)
{
    uint32_t i;

    for (i = 0; i < BDK_SPI_POLL_LOOPS; i++) {
        uint32_t sr = regs->SR & mask;

        if (want_set) {
            if (sr == mask) {
                return BDK_OK;
            }
        } else if (sr == 0U) {
            return BDK_OK;
        }
    }
    return BDK_ERR_TIMEOUT;
}

static int spi_busy(bdk_spi_id_t id)
{
    if (spi_irq_tx[id].active != 0U) {
        return 1;
    }
    if (spi_dma_tx_run[id] != 0U || spi_dma_rx_run[id] != 0U) {
        return 1;
    }
    return 0;
}

static uint16_t spi_buf_load(const void *tx, size_t i, int wide)
{
    if (tx == NULL) {
        return wide ? 0xFFFFU : 0xFFU;
    }
    if (wide) {
        return ((const uint16_t *)tx)[i];
    }
    return ((const uint8_t *)tx)[i];
}

static void spi_buf_store(void *rx, size_t i, uint16_t frame, int wide)
{
    if (rx == NULL) {
        return;
    }
    if (wide) {
        ((uint16_t *)rx)[i] = frame;
    } else {
        ((uint8_t *)rx)[i] = (uint8_t)frame;
    }
}

bdk_status_t bdk_spi_init(const bdk_spi_config_t *config)
{
    SPI_TypeDef *regs;
    bdk_status_t st;
    uint32_t cr1;

    if (config == NULL) {
        return BDK_ERR_NULL;
    }
    if ((unsigned)config->baud > (unsigned)BDK_SPI_BAUD_DIV256) {
        return BDK_ERR_RANGE;
    }
    if (config->cpol > BDK_SPI_CPOL_HIGH || config->cpha > BDK_SPI_CPHA_2EDGE) {
        return BDK_ERR_RANGE;
    }
    if (config->width > BDK_SPI_WIDTH_16) {
        return BDK_ERR_RANGE;
    }

    regs = spi_regs(config->id);
    if (regs == NULL) {
        return BDK_ERR_RANGE;
    }

    st = bdk_rcc_spi_clk_enable(config->id);
    if (st != BDK_OK) {
        return st;
    }

    CLEAR_BIT(regs->CR1, SPI_CR1_SPE);

    cr1 = 0U;
    cr1 |= SPI_CR1_MSTR;
    cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;
    cr1 |= ((uint32_t)config->baud << SPI_CR1_BR_Pos) & SPI_CR1_BR;
    if (config->cpol == BDK_SPI_CPOL_HIGH) {
        cr1 |= SPI_CR1_CPOL;
    }
    if (config->cpha == BDK_SPI_CPHA_2EDGE) {
        cr1 |= SPI_CR1_CPHA;
    }
    if (config->width == BDK_SPI_WIDTH_16) {
        cr1 |= SPI_CR1_DFF;
    }
    regs->CR1 = cr1;
    regs->CR2 = 0U;

    SET_BIT(regs->CR1, SPI_CR1_SPE);
    return BDK_OK;
}

bdk_status_t bdk_spi_set_width(bdk_spi_id_t id, bdk_spi_width_t width)
{
    SPI_TypeDef *regs = spi_regs(id);

    if (regs == NULL) {
        return BDK_ERR_RANGE;
    }
    if (width > BDK_SPI_WIDTH_16) {
        return BDK_ERR_RANGE;
    }
    if (spi_busy(id)) {
        return BDK_ERR_BUSY;
    }

    (void)spi_wait_sr(regs, SPI_SR_BSY, 0);
    CLEAR_BIT(regs->CR1, SPI_CR1_SPE);
    if (width == BDK_SPI_WIDTH_16) {
        SET_BIT(regs->CR1, SPI_CR1_DFF);
    } else {
        CLEAR_BIT(regs->CR1, SPI_CR1_DFF);
    }
    SET_BIT(regs->CR1, SPI_CR1_SPE);
    return BDK_OK;
}

bdk_status_t bdk_spi_transfer(bdk_spi_id_t id, uint16_t tx, uint16_t *rx)
{
    SPI_TypeDef *regs = spi_regs(id);
    bdk_status_t st;
    uint16_t frame;

    if (regs == NULL) {
        return BDK_ERR_RANGE;
    }

    st = spi_wait_sr(regs, SPI_SR_TXE, 1);
    if (st != BDK_OK) {
        return st;
    }
    spi_dr_write(regs, tx);

    st = spi_wait_sr(regs, SPI_SR_RXNE, 1);
    if (st != BDK_OK) {
        return st;
    }
    frame = spi_dr_read(regs);
    if (rx != NULL) {
        *rx = frame;
    }
    return BDK_OK;
}

bdk_status_t bdk_spi_transfer_buf(bdk_spi_id_t id, const void *tx, void *rx,
                                  size_t len)
{
    SPI_TypeDef *regs = spi_regs(id);
    size_t i;
    int wide;

    if (regs == NULL) {
        return BDK_ERR_RANGE;
    }
    if (len == 0U) {
        return BDK_OK;
    }

    wide = spi_width16(regs);
    for (i = 0; i < len; i++) {
        uint16_t got;
        uint16_t out = spi_buf_load(tx, i, wide);
        bdk_status_t st = bdk_spi_transfer(id, out, &got);

        if (st != BDK_OK) {
            return st;
        }
        spi_buf_store(rx, i, got, wide);
    }
    return BDK_OK;
}

bdk_status_t bdk_spi_write(bdk_spi_id_t id, const void *data, size_t len)
{
    if (len > 0U && data == NULL) {
        return BDK_ERR_NULL;
    }
    return bdk_spi_transfer_buf(id, data, NULL, len);
}

bdk_status_t bdk_spi_read(bdk_spi_id_t id, void *data, size_t len)
{
    if (len > 0U && data == NULL) {
        return BDK_ERR_NULL;
    }
    return bdk_spi_transfer_buf(id, NULL, data, len);
}

bdk_status_t bdk_spi_irq_enable(bdk_spi_id_t id)
{
    SPI_TypeDef *regs = spi_regs(id);

    if (regs == NULL) {
        return BDK_ERR_RANGE;
    }

    spi_irq_tx[id].data    = NULL;
    spi_irq_tx[id].len     = 0;
    spi_irq_tx[id].tx_idx  = 0;
    spi_irq_tx[id].rx_got  = 0;
    spi_irq_tx[id].active  = 0;

    NVIC_SetPriority(spi_irq_table[id], 5);
    NVIC_EnableIRQ(spi_irq_table[id]);
    return BDK_OK;
}

void bdk_spi_irq_handler(bdk_spi_id_t id)
{
    SPI_TypeDef *regs = spi_regs(id);
    spi_irq_tx_t *tx;
    int wide;

    if (regs == NULL) {
        return;
    }
    tx = &spi_irq_tx[id];
    wide = spi_width16(regs);

    if ((regs->SR & SPI_SR_OVR) != 0U) {
        (void)spi_dr_read(regs);
        (void)regs->SR;
    }

    if (tx->active == 0U) {
        if ((regs->SR & SPI_SR_RXNE) != 0U) {
            (void)spi_dr_read(regs);
        }
        return;
    }

    if ((regs->SR & SPI_SR_TXE) != 0U && tx->tx_idx < tx->len) {
        spi_dr_write(regs, spi_buf_load(tx->data, tx->tx_idx, wide));
        tx->tx_idx++;
    }

    if ((regs->SR & SPI_SR_RXNE) != 0U) {
        (void)spi_dr_read(regs);
        tx->rx_got++;
    }

    if (tx->rx_got >= tx->len) {
        CLEAR_BIT(regs->CR2, SPI_CR2_TXEIE | SPI_CR2_RXNEIE);
        tx->active = 0;
    }
}

bdk_status_t bdk_spi_write_async(bdk_spi_id_t id, const void *data, size_t len)
{
    SPI_TypeDef *regs = spi_regs(id);
    spi_irq_tx_t *tx;

    if (regs == NULL) {
        return BDK_ERR_RANGE;
    }
    if (len > 0U && data == NULL) {
        return BDK_ERR_NULL;
    }
    if (len == 0U) {
        return BDK_ERR_RANGE;
    }

    tx = &spi_irq_tx[id];
    if (tx->active != 0U || spi_dma_tx_run[id] != 0U) {
        return BDK_ERR_BUSY;
    }

    tx->data   = (const uint8_t *)data;
    tx->len    = len;
    tx->tx_idx = 0;
    tx->rx_got = 0;
    tx->active = 1;

    SET_BIT(regs->CR2, SPI_CR2_TXEIE | SPI_CR2_RXNEIE);
    return BDK_OK;
}

int bdk_spi_tx_active(bdk_spi_id_t id)
{
    if ((unsigned)id >= BDK_ARRAY_LEN(spi_table)) {
        return 0;
    }
    return spi_irq_tx[id].active != 0U;
}

bdk_status_t bdk_spi_dma_bind(bdk_spi_id_t id, const bdk_spi_dma_t *dma)
{
    if (dma == NULL) {
        return BDK_ERR_NULL;
    }
    if (spi_regs(id) == NULL) {
        return BDK_ERR_RANGE;
    }
    if (bdk_dma_stream_valid(&dma->tx_stream) != BDK_OK ||
        bdk_dma_stream_valid(&dma->rx_stream) != BDK_OK) {
        return BDK_ERR_RANGE;
    }
    if (dma->channel > 7U) {
        return BDK_ERR_RANGE;
    }

    spi_dma_map[id]    = *dma;
    spi_dma_bound[id]  = 1U;
    return BDK_OK;
}

bdk_status_t bdk_spi_write_dma(bdk_spi_id_t id, const void *data, size_t len)
{
    SPI_TypeDef *regs = spi_regs(id);
    bdk_status_t st;

    if (regs == NULL) {
        return BDK_ERR_RANGE;
    }
    if (len > 0U && data == NULL) {
        return BDK_ERR_NULL;
    }
    if (len == 0U) {
        return BDK_ERR_RANGE;
    }
    if (spi_dma_bound[id] == 0U) {
        return BDK_ERR_STATE;
    }
    if (spi_busy(id) || bdk_dma_busy(&spi_dma_map[id].tx_stream) ||
        bdk_dma_busy(&spi_dma_map[id].rx_stream)) {
        return BDK_ERR_BUSY;
    }

    CLEAR_BIT(regs->CR2, SPI_CR2_TXEIE | SPI_CR2_RXNEIE);
    SET_BIT(regs->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

    st = bdk_dma_start(&spi_dma_map[id].rx_stream, &spi_dma_rx_sink, len);
    if (st != BDK_OK) {
        CLEAR_BIT(regs->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
        return st;
    }
    st = bdk_dma_start(&spi_dma_map[id].tx_stream, data, len);
    if (st != BDK_OK) {
        (void)bdk_dma_stop(&spi_dma_map[id].rx_stream);
        CLEAR_BIT(regs->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
        return st;
    }

    spi_dma_tx_run[id] = 1U;
    return BDK_OK;
}

bdk_status_t bdk_spi_read_dma(bdk_spi_id_t id, void *data, size_t len)
{
    SPI_TypeDef *regs = spi_regs(id);
    bdk_status_t st;

    if (regs == NULL) {
        return BDK_ERR_RANGE;
    }
    if (len > 0U && data == NULL) {
        return BDK_ERR_NULL;
    }
    if (len == 0U) {
        return BDK_ERR_RANGE;
    }
    if (spi_dma_bound[id] == 0U) {
        return BDK_ERR_STATE;
    }
    if (spi_busy(id) || bdk_dma_busy(&spi_dma_map[id].tx_stream) ||
        bdk_dma_busy(&spi_dma_map[id].rx_stream)) {
        return BDK_ERR_BUSY;
    }

    CLEAR_BIT(regs->CR2, SPI_CR2_TXEIE | SPI_CR2_RXNEIE);
    SET_BIT(regs->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

    spi_dma_tx_idle = spi_idle_frame(regs);

    st = bdk_dma_start(&spi_dma_map[id].rx_stream, data, len);
    if (st != BDK_OK) {
        CLEAR_BIT(regs->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
        return st;
    }
    st = bdk_dma_start(&spi_dma_map[id].tx_stream, &spi_dma_tx_idle, len);
    if (st != BDK_OK) {
        (void)bdk_dma_stop(&spi_dma_map[id].rx_stream);
        CLEAR_BIT(regs->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
        return st;
    }

    spi_dma_rx_run[id] = 1U;
    return BDK_OK;
}

int bdk_spi_dma_tx_active(bdk_spi_id_t id)
{
    SPI_TypeDef *regs = spi_regs(id);

    if (regs == NULL || spi_dma_tx_run[id] == 0U) {
        return 0;
    }
    if (bdk_dma_busy(&spi_dma_map[id].tx_stream) ||
        bdk_dma_busy(&spi_dma_map[id].rx_stream) ||
        (regs->SR & SPI_SR_BSY) != 0U) {
        return 1;
    }
    spi_dma_tx_run[id] = 0U;
    return 0;
}

int bdk_spi_dma_rx_active(bdk_spi_id_t id)
{
    SPI_TypeDef *regs = spi_regs(id);

    if (regs == NULL || spi_dma_rx_run[id] == 0U) {
        return 0;
    }
    if (bdk_dma_busy(&spi_dma_map[id].tx_stream) ||
        bdk_dma_busy(&spi_dma_map[id].rx_stream) ||
        (regs->SR & SPI_SR_BSY) != 0U) {
        return 1;
    }
    spi_dma_rx_run[id] = 0U;
    return 0;
}
