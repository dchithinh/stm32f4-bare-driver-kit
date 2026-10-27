/**
 * @file ili9341_board_dma.c
 * @brief SPI2 TX-only DMA + polled RX drain (ILI9341 pixel lines).
 */

#include "ili9341_board_dma.h"

#include "bdk_dma.h"
#include "bdk_spi.h"

#include "stm32f4xx.h"

#define ILI9341_DMA_WAIT_LOOPS 2000000U

static const bdk_dma_stream_t ili9341_spi2_tx = BDK_DMA1_STREAM(4);

static SPI_TypeDef *spi2_regs(void)
{
    return SPI2;
}

static void spi2_drain_rx(SPI_TypeDef *regs)
{
    while ((regs->SR & SPI_SR_RXNE) != 0U) {
        (void)*(__IO uint8_t *)&regs->DR;
    }
    if ((regs->SR & SPI_SR_OVR) != 0U) {
        (void)*(__IO uint8_t *)&regs->DR;
        (void)regs->SR;
    }
}

void ili9341_board_dma_init(bdk_spi_id_t spi)
{
    SPI_TypeDef *regs;

    if (spi != BDK_SPI_2) {
        return;
    }

    regs = spi2_regs();

    bdk_dma_config_t dma_tx = {
        .stream       = ili9341_spi2_tx,
        .channel      = 0,
        .direction    = BDK_DMA_MEM_TO_PERIPH,
        .periph_addr  = (volatile void *)&regs->DR,
        .periph_inc   = 0,
        .mem_inc      = 1,
        .periph_width = BDK_DMA_WIDTH_BYTE,
        .mem_width    = BDK_DMA_WIDTH_BYTE,
        .circular     = 0,
    };

    BDK_ASSERT_OK(bdk_dma_config(&dma_tx));
}

bdk_status_t ili9341_board_dma_write(bdk_spi_id_t spi, const void *data, size_t len)
{
    SPI_TypeDef *regs;
    bdk_status_t st;
    uint32_t guard;

    if (spi != BDK_SPI_2) {
        return BDK_ERR_RANGE;
    }
    if (len == 0U) {
        return BDK_ERR_RANGE;
    }
    if (data == NULL) {
        return BDK_ERR_NULL;
    }
    if (bdk_dma_busy(&ili9341_spi2_tx) != 0) {
        return BDK_ERR_BUSY;
    }

    regs = spi2_regs();

    guard = 0U;
    while ((regs->SR & SPI_SR_BSY) != 0U) {
        if (++guard > ILI9341_DMA_WAIT_LOOPS) {
            bdk_spi_recover(spi);
            return BDK_ERR_TIMEOUT;
        }
    }
    spi2_drain_rx(regs);

    CLEAR_BIT(regs->CR2, SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_RXDMAEN);
    SET_BIT(regs->CR2, SPI_CR2_TXDMAEN);

    st = bdk_dma_start(&ili9341_spi2_tx, data, len);
    if (st != BDK_OK) {
        CLEAR_BIT(regs->CR2, SPI_CR2_TXDMAEN);
        return st;
    }

    guard = 0U;
    while (bdk_dma_busy(&ili9341_spi2_tx) != 0) {
        spi2_drain_rx(regs);
        if (++guard > ILI9341_DMA_WAIT_LOOPS) {
            (void)bdk_dma_stop(&ili9341_spi2_tx);
            CLEAR_BIT(regs->CR2, SPI_CR2_TXDMAEN);
            bdk_spi_recover(spi);
            return BDK_ERR_TIMEOUT;
        }
    }

    guard = 0U;
    while ((regs->SR & SPI_SR_BSY) != 0U) {
        spi2_drain_rx(regs);
        if (++guard > ILI9341_DMA_WAIT_LOOPS) {
            CLEAR_BIT(regs->CR2, SPI_CR2_TXDMAEN);
            bdk_spi_recover(spi);
            return BDK_ERR_TIMEOUT;
        }
    }

    spi2_drain_rx(regs);
    CLEAR_BIT(regs->CR2, SPI_CR2_TXDMAEN);
    (void)bdk_dma_stop(&ili9341_spi2_tx);

    return BDK_OK;
}
