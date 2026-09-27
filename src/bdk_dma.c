#include "stm32f4xx.h"
#include "bdk_dma.h"

typedef struct {
    uint32_t tc_isr;    /* mask in LISR or HISR — test with READ_BIT */
    uint32_t tc_ifcr;   /* mask for LIFCR or HIFCR — write to clear TCIF */
} dma_tc_flag_t;

typedef struct {
    uint32_t te_isr;
    uint32_t te_ifcr;
} dma_te_flag_t;

typedef struct {
    DMA_Stream_TypeDef *stream;
    IRQn_Type             irqn;
} dma_hw_t;

static const dma_hw_t dma_hw[BDK_DMA_STREAM_COUNT * 2U] = {
    { DMA1_Stream0, DMA1_Stream0_IRQn },
    { DMA1_Stream1, DMA1_Stream1_IRQn },
    { DMA1_Stream2, DMA1_Stream2_IRQn },
    { DMA1_Stream3, DMA1_Stream3_IRQn },
    { DMA1_Stream4, DMA1_Stream4_IRQn },
    { DMA1_Stream5, DMA1_Stream5_IRQn },
    { DMA1_Stream6, DMA1_Stream6_IRQn },
    { DMA1_Stream7, DMA1_Stream7_IRQn },
    { DMA2_Stream0, DMA2_Stream0_IRQn },
    { DMA2_Stream1, DMA2_Stream1_IRQn },
    { DMA2_Stream2, DMA2_Stream2_IRQn },
    { DMA2_Stream3, DMA2_Stream3_IRQn },
    { DMA2_Stream4, DMA2_Stream4_IRQn },
    { DMA2_Stream5, DMA2_Stream5_IRQn },
    { DMA2_Stream6, DMA2_Stream6_IRQn },
    { DMA2_Stream7, DMA2_Stream7_IRQn },
};

static const dma_tc_flag_t tc_flag_tbl[BDK_DMA_STREAM_COUNT] = {
    {DMA_LISR_TCIF0, DMA_LIFCR_CTCIF0},
    {DMA_LISR_TCIF1, DMA_LIFCR_CTCIF1},
    {DMA_LISR_TCIF2, DMA_LIFCR_CTCIF2},
    {DMA_LISR_TCIF3, DMA_LIFCR_CTCIF3},
    {DMA_HISR_TCIF4, DMA_HIFCR_CTCIF4},
    {DMA_HISR_TCIF5, DMA_HIFCR_CTCIF5},
    {DMA_HISR_TCIF6, DMA_HIFCR_CTCIF6},
    {DMA_HISR_TCIF7, DMA_HIFCR_CTCIF7}
};

static const dma_te_flag_t te_flag_tbl[BDK_DMA_STREAM_COUNT] = {
    {DMA_LISR_TEIF0, DMA_LIFCR_CTEIF0},
    {DMA_LISR_TEIF1, DMA_LIFCR_CTEIF1},
    {DMA_LISR_TEIF2, DMA_LIFCR_CTEIF2},
    {DMA_LISR_TEIF3, DMA_LIFCR_CTEIF3},
    {DMA_HISR_TEIF4, DMA_HIFCR_CTEIF4},
    {DMA_HISR_TEIF5, DMA_HIFCR_CTEIF5},
    {DMA_HISR_TEIF6, DMA_HIFCR_CTEIF6},
    {DMA_HISR_TEIF7, DMA_HIFCR_CTEIF7}
};

/** OR of CTCIF | CHTIF | CTEIF | CDMEIF | CFEIF for each stream index (RM IFCR clear). */
static const uint32_t dma_ifcr_clr_all[BDK_DMA_STREAM_COUNT] = {
    DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 |
        DMA_LIFCR_CFEIF0,
    DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 |
        DMA_LIFCR_CFEIF1,
    DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2 | DMA_LIFCR_CDMEIF2 |
        DMA_LIFCR_CFEIF2,
    DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 |
        DMA_LIFCR_CFEIF3,
    DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 |
        DMA_HIFCR_CFEIF4,
    DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 |
        DMA_HIFCR_CFEIF5,
    DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 |
        DMA_HIFCR_CFEIF6,
    DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7 |
        DMA_HIFCR_CFEIF7,
};

bdk_status_t bdk_dma_stream_valid(const bdk_dma_stream_t *s)
{
    if (s == NULL) {
        return BDK_ERR_NULL;
    }
    if (s->controller != BDK_DMA1 && s->controller != BDK_DMA2) {
        return BDK_ERR_RANGE;
    }
    if (s->stream >= BDK_DMA_STREAM_COUNT) {
        return BDK_ERR_RANGE;
    }
    return BDK_OK;
}

static unsigned dma_hw_index(const bdk_dma_stream_t *stream)
{
    return (unsigned)stream->controller * BDK_DMA_STREAM_COUNT + stream->stream;
}

static const dma_hw_t *dma_hw_get(const bdk_dma_stream_t *stream)
{
    if (bdk_dma_stream_valid(stream) != BDK_OK) {
        return NULL;
    }
    return &dma_hw[dma_hw_index(stream)];
}

static DMA_TypeDef *dma_ctrl(const bdk_dma_stream_t *stream)
{
    if (bdk_dma_stream_valid(stream) != BDK_OK) {
        return NULL;
    }
    return (stream->controller == BDK_DMA1) ? DMA1 : DMA2;
}

static DMA_Stream_TypeDef *dma_stream_regs(const bdk_dma_stream_t *stream)
{
    const dma_hw_t *hw = dma_hw_get(stream);
    if (hw == NULL) {
        return NULL;
    }
    return hw->stream;
}

static int dma_flag_pending(DMA_TypeDef *ctrl, uint8_t st, uint32_t isr_mask)
{
    if (st < 4U) {
        return (READ_BIT(ctrl->LISR, isr_mask) != 0) ? 1 : 0;
    }
    return (READ_BIT(ctrl->HISR, isr_mask) != 0) ? 1 : 0;
}

static void dma_flag_clear(DMA_TypeDef *ctrl, uint8_t st, uint32_t ifcr_mask)
{
    if (st < 4U) {
        SET_BIT(ctrl->LIFCR, ifcr_mask);
    } else {
        SET_BIT(ctrl->HIFCR, ifcr_mask);
    }
}

static void dma_tc_flag_clear(const bdk_dma_stream_t *stream)
{
    DMA_TypeDef *ctrl = dma_ctrl(stream);
    if (ctrl == NULL) {
        return;
    }
    dma_flag_clear(ctrl, stream->stream, tc_flag_tbl[stream->stream].tc_ifcr);
}

static void dma_stream_en_clear(const bdk_dma_stream_t *stream)
{
    DMA_Stream_TypeDef *s = dma_stream_regs(stream);
    if (s == NULL) {
        return;
    }

    CLEAR_BIT(s->CR, DMA_SxCR_EN);
    while (READ_BIT(s->CR, DMA_SxCR_EN) != 0) {
    }
}

static void dma_stream_ifcr_clear_all(const bdk_dma_stream_t *stream)
{
    DMA_TypeDef *ctrl = dma_ctrl(stream);
    if (ctrl == NULL) {
        return;
    }
    dma_flag_clear(ctrl, stream->stream, dma_ifcr_clr_all[stream->stream]);
}

/** Transfer error: disable stream and clear all pending ISR flags for this stream. */
static void dma_stream_te_recover(const bdk_dma_stream_t *stream)
{
    dma_stream_en_clear(stream);
    dma_stream_ifcr_clear_all(stream);
}

bdk_status_t bdk_dma_config(const bdk_dma_config_t *cfg)
{
    if (cfg == NULL) {
        return BDK_ERR_NULL;
    }
    if (cfg->mem_width > BDK_DMA_WIDTH_WORD ||
        cfg->periph_width > BDK_DMA_WIDTH_WORD) {
        return BDK_ERR_RANGE;
    }

    if ((unsigned) cfg->direction > BDK_DMA_MEM_TO_MEM) {
        return BDK_ERR_RANGE;
    }

    if ((unsigned) cfg->circular > 1 ||
        (unsigned) cfg->periph_inc > 1 ||
        (unsigned) cfg->mem_inc > 1) {
        return BDK_ERR_RANGE;
    }

    DMA_Stream_TypeDef *s = dma_stream_regs(&cfg->stream);
    if (s == NULL) {
        return BDK_ERR_RANGE;
    }

    if (cfg->stream.controller == BDK_DMA1) {
        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);
    } else if (cfg->stream.controller == BDK_DMA2) {
        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN);
    }

    if (READ_BIT(s->CR, DMA_SxCR_EN) != 0) {
        return BDK_ERR_BUSY;
    }

    if ((unsigned)cfg->channel > 7) {
        return BDK_ERR_RANGE;
    }
    CLEAR_BIT(s->CR, DMA_SxCR_CHSEL);
    SET_BIT(s->CR, (uint32_t)(cfg->channel << DMA_SxCR_CHSEL_Pos) & DMA_SxCR_CHSEL_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_DIR);
    SET_BIT(s->CR, (uint32_t)(cfg->direction << DMA_SxCR_DIR_Pos) & DMA_SxCR_DIR_Msk);

    if (cfg->direction != BDK_DMA_MEM_TO_MEM) {
        if (cfg->periph_addr == NULL) {
            return BDK_ERR_NULL;
        }
        WRITE_REG(s->PAR, (uint32_t)(uintptr_t)cfg->periph_addr);
    }

    CLEAR_BIT(s->CR, DMA_SxCR_PINC);
    SET_BIT(s->CR, (uint32_t)(cfg->periph_inc << DMA_SxCR_PINC_Pos) & DMA_SxCR_PINC_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_MINC);
    SET_BIT(s->CR, (uint32_t)(cfg->mem_inc << DMA_SxCR_MINC_Pos) & DMA_SxCR_MINC_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_PSIZE);
    SET_BIT(s->CR, (uint32_t)(cfg->periph_width << DMA_SxCR_PSIZE_Pos) & DMA_SxCR_PSIZE_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_MSIZE);
    SET_BIT(s->CR, (uint32_t)(cfg->mem_width << DMA_SxCR_MSIZE_Pos) & DMA_SxCR_MSIZE_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_CIRC);
    SET_BIT(s->CR, (uint32_t)(cfg->circular << DMA_SxCR_CIRC_Pos) & DMA_SxCR_CIRC_Msk);

    return BDK_OK;
}

bdk_status_t bdk_dma_start(const bdk_dma_stream_t *stream, const void *mem, size_t len)
{
    if (stream == NULL || mem == NULL) {
        return BDK_ERR_NULL;
    }
    if (bdk_dma_stream_valid(stream) != BDK_OK) {
        return BDK_ERR_RANGE;
    }

    if (len == 0U || len > 0xFFFF) {
        return BDK_ERR_RANGE;
    }

    DMA_Stream_TypeDef *s = dma_stream_regs(stream);
    if (s == NULL) {
        return BDK_ERR_RANGE;
    }

    if (READ_BIT(s->CR, DMA_SxCR_EN) != 0) {
        return BDK_ERR_BUSY;
    }

    s->M0AR = (uint32_t)(uintptr_t)mem;
    s->NDTR = len;

    SET_BIT(s->CR, DMA_SxCR_EN);

    return BDK_OK;
}

bdk_status_t bdk_dma_stop(const bdk_dma_stream_t *stream)
{
    if (stream == NULL) {
        return BDK_ERR_NULL;
    }

    DMA_Stream_TypeDef *s = dma_stream_regs(stream);
    if (s == NULL) {
        return BDK_ERR_RANGE;
    }

    dma_stream_en_clear(stream);
    dma_stream_ifcr_clear_all(stream);

    return BDK_OK;
}

int bdk_dma_busy(const bdk_dma_stream_t *stream)
{
    if (stream == NULL) {
        return 0;
    }

    DMA_Stream_TypeDef *s = dma_stream_regs(stream);
    if (s == NULL) {
        return 0;
    }

    return (READ_BIT(s->CR, DMA_SxCR_EN) != 0) ? 1 : 0;
}

bdk_status_t bdk_dma_irq_enable(const bdk_dma_stream_t *stream, uint32_t its)
{
    if (stream == NULL) {
        return BDK_ERR_NULL;
    }
    if (its == 0U || (its & ~BDK_DMA_IT_MASK) != 0U) {
        return BDK_ERR_RANGE;
    }

    const dma_hw_t *hw = dma_hw_get(stream);
    if (hw == NULL) {
        return BDK_ERR_RANGE;
    }

    if ((its & BDK_DMA_IT_TC) != 0U) {
        SET_BIT(hw->stream->CR, DMA_SxCR_TCIE);
    }
    if ((its & BDK_DMA_IT_TE) != 0U) {
        SET_BIT(hw->stream->CR, DMA_SxCR_TEIE);
    }

    NVIC_SetPriority(hw->irqn, 5);
    NVIC_EnableIRQ(hw->irqn);

    return BDK_OK;
}

void bdk_dma_irq_handler(const bdk_dma_stream_t *stream)
{
    DMA_TypeDef *ctrl;

    if (stream == NULL) {
        return;
    }

    ctrl = dma_ctrl(stream);
    if (ctrl == NULL) {
        return;
    }

    uint8_t st = stream->stream;

    if (dma_flag_pending(ctrl, st, te_flag_tbl[st].te_isr) != 0) {
        dma_stream_te_recover(stream);
        return;
    }

    if (dma_flag_pending(ctrl, st, tc_flag_tbl[st].tc_isr) != 0) {
        dma_tc_flag_clear(stream);
    }
}
