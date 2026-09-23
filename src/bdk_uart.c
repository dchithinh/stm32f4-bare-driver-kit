#include "stm32f4xx.h"
#include "bdk_uart.h"
#include "bdk_rcc.h"


static USART_TypeDef *const uart_table[] = {
    USART1, USART2, USART3, UART4, UART5, USART6
};

static USART_TypeDef *usart_regs(bdk_uart_id_t id)
{
    if ((unsigned) id >= (unsigned) BDK_ARRAY_LEN(uart_table)) {
        return NULL;
    }
    return uart_table[id];
}

static uint32_t get_sysclk()
{
    /*TODO: Update this func when support sysclk configuration*/
    extern uint32_t SystemCoreClock;
    return SystemCoreClock;
}

bdk_status_t bdk_uart_init(const bdk_uart_config_t *config)
{
    if (config == NULL || config->baud == 0) {
        return BDK_ERR_PARAM;
    }

    if ((unsigned) config->id >= BDK_ARRAY_LEN(uart_table)) {
        return BDK_ERR_PARAM;
    }

    if (bdk_rcc_usart_clk_enable(config->id) != BDK_OK) {
        return BDK_ERR_PARAM;
    }

    USART_TypeDef *regs = usart_regs(config->id);
    if (regs == NULL) {
        return BDK_ERR_PARAM;
    }

    /* OVER8 = 0: USARTDIV = f_CK / (16 * baud). BRR stores that in 1/16
     * units (mantissa << 4 | fraction) = round(f_CK / baud).
     * + baud/2 is round-to-nearest before integer divide. */
    uint32_t div = (get_sysclk() + (config->baud / 2)) / config->baud;
    if (div == 0 || div > 0xFFFF) {
        return BDK_ERR_PARAM;
    }
    regs->BRR = (uint16_t) div;

    CLEAR_BIT(regs->CR1, USART_CR1_M);
    SET_BIT(regs->CR1, (uint32_t) (0x1UL & config->word) << USART_CR1_M_Pos);

    if (config->parity != BDK_UART_PARITY_NONE) {
        SET_BIT(regs->CR1, USART_CR1_PCE);
        uint8_t parity = 0;
        if (config->parity == BDK_UART_PARITY_ODD){
            parity = 1;
        } else if (config->parity == BDK_UART_PARITY_EVEN) {
            parity = 0;
        } else {
            return BDK_ERR_PARAM;
        }

        CLEAR_BIT(regs->CR1, USART_CR1_PS);
        SET_BIT(regs->CR1, (uint32_t)(0x1UL & parity) << USART_CR1_PS_Pos);
    } else {
        CLEAR_BIT(regs->CR1, USART_CR1_PCE);
    }

    CLEAR_BIT(regs->CR2, USART_CR2_STOP);
    SET_BIT(regs->CR2, (uint32_t) (0x3UL & config->stop) << USART_CR2_STOP_Pos);

    SET_BIT(regs->CR1, USART_CR1_TE);
    SET_BIT(regs->CR1, USART_CR1_RE);
    SET_BIT(regs->CR1, USART_CR1_UE);

    return BDK_OK;
}

void bdk_uart_write_byte(bdk_uart_id_t id, uint8_t byte)
{
    USART_TypeDef *regs = usart_regs(id);
    if (regs == NULL) {
        return;
    }

    while (READ_BIT(regs->SR, USART_SR_TXE) == 0);
    regs->DR = byte;
}

uint8_t bdk_uart_read_byte(bdk_uart_id_t id)
{
    USART_TypeDef *regs = usart_regs(id);
    if (regs == NULL) {
        return 0;
    }

    while (READ_BIT(regs->SR, USART_SR_RXNE) == 0);
    uint8_t byte = regs->DR;
    return byte;
}

void bdk_uart_write(bdk_uart_id_t id, const uint8_t *data, size_t len)
{
    (void)id;
    (void)data;
    (void)len;
}

int bdk_uart_rx_ready(bdk_uart_id_t id)
{
    USART_TypeDef *regs = usart_regs(id);
    if (regs == NULL) {
        return 0;
    }

    if (READ_BIT(regs->SR, USART_SR_RXNE) != 0) {
        return 1;
    }

    return 0;
}
