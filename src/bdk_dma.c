#include "stm32f4xx.h"
#include "bdk_dma.h"

static DMA_TypeDef *dma_ctrl_regs[] = { 
    DMA1, DMA2
};
static DMA_Stream_TypeDef *dma1_stream_regs[] = {
    DMA1_Stream0,
    DMA1_Stream1,
    DMA1_Stream2,
    DMA1_Stream3,
    DMA1_Stream4,
    DMA1_Stream5,
    DMA1_Stream6,
    DMA1_Stream7,
};

static DMA_Stream_TypeDef *dma2_stream_regs[] = {
    DMA2_Stream0,
    DMA2_Stream1,
    DMA2_Stream2,
    DMA2_Stream3,
    DMA2_Stream4,
    DMA2_Stream5,
    DMA2_Stream6,
    DMA2_Stream7,
};

static DMA_TypeDef *dma_get_ctrl_regs(const bdk_dma_stream_t *stream) 
{
    if (bdk_dma_stream_valid(stream) != BDK_OK) {
        return NULL;
    }

    return dma_ctrl_regs[stream->controller];
}

static DMA_Stream_TypeDef *dma_get_stream_regs(const bdk_dma_stream_t *stream)
{
    if (bdk_dma_stream_valid(stream) != BDK_OK) {
        return NULL;
    }

    DMA_Stream_TypeDef *stream_regs = NULL;
    if (stream->controller == BDK_DMA1) {
        stream_regs = dma1_stream_regs[stream->stream];
    } else {
        stream_regs = dma2_stream_regs[stream->stream];
    }

    return stream_regs;
}

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
        (unsigned) cfg->periph_inc > 1||
        (unsigned) cfg->mem_inc > 1 ) {
        return BDK_ERR_RANGE;
    }

    DMA_TypeDef *dma = dma_get_ctrl_regs(&cfg->stream);
    DMA_Stream_TypeDef *s = dma_get_stream_regs(&cfg->stream);
    if (!dma || !s) {
        return BDK_ERR_RANGE;
    }
    
    if (cfg->stream.controller == BDK_DMA1) {
        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);
    } else if (cfg->stream.controller == BDK_DMA2) {
        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN);
    }
    
    /*Stream is still running*/
    if (READ_BIT(s->CR, DMA_SxCR_EN) != 0) {
        return BDK_ERR_BUSY;
    }

    if ((unsigned)cfg->channel > 7 ) {
        return BDK_ERR_RANGE;
    }
    CLEAR_BIT(s->CR, DMA_SxCR_CHSEL);
    SET_BIT(s->CR, (uint32_t)(cfg->channel << DMA_SxCR_CHSEL_Pos) & DMA_SxCR_CHSEL_Msk);
    

    CLEAR_BIT(s->CR, DMA_SxCR_DIR);
    SET_BIT(s->CR, (uint32_t) (cfg->direction << DMA_SxCR_DIR_Pos) & DMA_SxCR_DIR_Msk);

    if (cfg->direction != BDK_DMA_MEM_TO_MEM) {
        if (cfg->periph_addr == NULL) {
            return BDK_ERR_NULL;
        }
        WRITE_REG(s->PAR, (uint32_t)(uintptr_t)cfg->periph_addr);
    }

    CLEAR_BIT(s->CR, DMA_SxCR_PINC);
    SET_BIT(s->CR, (uint32_t) (cfg->periph_inc << DMA_SxCR_PINC_Pos) & DMA_SxCR_PINC_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_MINC);
    SET_BIT(s->CR, (uint32_t) (cfg->mem_inc << DMA_SxCR_MINC_Pos) & DMA_SxCR_MINC_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_PSIZE);
    SET_BIT(s->CR, (uint32_t) (cfg->periph_width << DMA_SxCR_PSIZE_Pos) & DMA_SxCR_PSIZE_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_MSIZE);
    SET_BIT(s->CR, (uint32_t) (cfg->mem_width << DMA_SxCR_MSIZE_Pos) & DMA_SxCR_MSIZE_Msk);

    CLEAR_BIT(s->CR, DMA_SxCR_CIRC);
    SET_BIT(s->CR, (uint32_t) (cfg->circular << DMA_SxCR_CIRC_Pos) & DMA_SxCR_CIRC_Msk);

    return BDK_OK;
}

bdk_status_t bdk_dma_start(bdk_dma_stream_t stream, const void *mem, size_t len)
{
    if (mem == NULL) {
        return BDK_ERR_NULL;
    }
    if (bdk_dma_stream_valid(&stream) != BDK_OK) {
        return BDK_ERR_RANGE;
    }

    if (len == 0U || len > 0xFFFF) {
        return BDK_ERR_RANGE;
    }

    DMA_Stream_TypeDef *s = dma_get_stream_regs(&stream);
    if (!s) {
        return BDK_ERR_RANGE;
    }

    /*Stream is still running*/
    if (READ_BIT(s->CR, DMA_SxCR_EN) != 0) {
        return BDK_ERR_BUSY;
    }

    s->M0AR = (uint32_t)(uintptr_t)mem;
    s->NDTR = len; 

    /*Start DMA*/
    SET_BIT(s->CR, DMA_SxCR_EN);

    return BDK_OK;
}

bdk_status_t bdk_dma_stop(bdk_dma_stream_t stream)
{
    DMA_Stream_TypeDef *s = dma_get_stream_regs(&stream);
    if (!s) {
        return BDK_ERR_RANGE;
    }

    CLEAR_BIT(s->CR, DMA_SxCR_EN);
    while(READ_BIT(s->CR, DMA_SxCR_EN) != 0);

    /*TODO: Clear TC, TE on later usage*/

    return BDK_OK;
}

int bdk_dma_busy(bdk_dma_stream_t stream)
{
    DMA_Stream_TypeDef *s = dma_get_stream_regs(&stream);
    if (!s) {
        return 0;
    }
    
    return (READ_BIT(s->CR, DMA_SxCR_EN) != 0) ? 1 : 0;
}

bdk_status_t bdk_dma_tc_irq_enable(bdk_dma_stream_t stream)
{
    (void)stream;
    return BDK_ERR_NOT_IMPL;
}

void bdk_dma_irq_handler(bdk_dma_stream_t stream)
{
    (void)stream;
}
