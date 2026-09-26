#include "stm32f4xx.h"
#include "bdk_uart.h"
#include "bdk_rcc.h"

#define BDK_UART_RX_BUF_SZ  64UL

typedef struct {
    uint8_t             buf[BDK_UART_RX_BUF_SZ];
    volatile uint16_t   head;    /* ISR writes here (next free slot) */
    volatile uint16_t   tail;    /* App reads here (next byte to deliver) */
    volatile uint32_t   ore_count;
    volatile uint32_t   ring_drop_count;
} bdk_uart_rx_t;

typedef struct {
    const uint8_t      *data;
    size_t              len;
    size_t              idx;
    volatile uint8_t    active;   /* 1 = async TX in progress */
} bdk_uart_tx_t;

static USART_TypeDef *const uart_table[] = {
    USART1, USART2, USART3, UART4, UART5, USART6
};

static const IRQn_Type uart_irq_table[] = {
    [BDK_UART_1] = USART1_IRQn,
    [BDK_UART_2] = USART2_IRQn,
    [BDK_UART_3] = USART3_IRQn,
    [BDK_UART_4] = UART4_IRQn,
    [BDK_UART_5] = UART5_IRQn,
    [BDK_UART_6] = USART6_IRQn
};

static bdk_uart_rx_t uart_rx[BDK_ARRAY_LEN(uart_table)];
static bdk_uart_tx_t uart_tx[BDK_ARRAY_LEN(uart_table)];

static IRQn_Type uart_irqn(bdk_uart_id_t id);
static bdk_uart_rx_t *uart_rx_get(bdk_uart_id_t id);
static void uart_rx_reset(bdk_uart_id_t id);
static int uart_rx_push(bdk_uart_id_t id, uint8_t byte);
static bdk_status_t uart_rx_pop(bdk_uart_id_t id, uint8_t *byte);
static void uart_rx_isr(bdk_uart_id_t id, USART_TypeDef *regs);
static bdk_uart_tx_t *uart_tx_get(bdk_uart_id_t id);
static void uart_tx_reset(bdk_uart_id_t id);
static void uart_tx_isr(bdk_uart_id_t id, USART_TypeDef *regs);
static USART_TypeDef *usart_regs(bdk_uart_id_t id);
static uint32_t get_sysclk(void);

static IRQn_Type uart_irqn(bdk_uart_id_t id)
{
    if ((unsigned)id >= BDK_ARRAY_LEN(uart_irq_table)) {
        return (IRQn_Type) -1;
    }

    return uart_irq_table[id];
}

static bdk_uart_rx_t *uart_rx_get(bdk_uart_id_t id)
{
    if ((unsigned)id >= BDK_ARRAY_LEN(uart_rx)) {
        return NULL;
    }

    return &uart_rx[id];
}
static void uart_rx_reset(bdk_uart_id_t id)
{
    bdk_uart_rx_t *rx = uart_rx_get(id);
    if (rx == NULL) {
        return;
    }

    rx->head = 0;
    rx->tail = 0;
    rx->ore_count = 0;
    rx->ring_drop_count = 0;
}

static int uart_rx_push(bdk_uart_id_t id, uint8_t byte)
{
    uint16_t next;
    bdk_uart_rx_t *rx = uart_rx_get(id);
    if (rx == NULL) {
        return 0;
    }
    next = (rx->head + 1) % BDK_UART_RX_BUF_SZ;
    if (next == rx->tail) {
        rx->ring_drop_count++;
        return 0;
    }

    rx->buf[rx->head] = byte;
    rx->head = next;

    return 1;
}

static bdk_status_t uart_rx_pop(bdk_uart_id_t id, uint8_t *byte)
{
    if (byte == NULL) {
        return BDK_ERR_PARAM;
    }

    bdk_uart_rx_t *rx = uart_rx_get(id);
    if (rx == NULL) {
        return BDK_ERR_PARAM;
    }
    
    if (rx->tail == rx->head) {
        return BDK_ERR_NODATA;
    }

    *byte = rx->buf[rx->tail];
    rx->tail = (rx->tail + 1) % BDK_UART_RX_BUF_SZ;

    return BDK_OK;
}

static void uart_rx_isr(bdk_uart_id_t id, USART_TypeDef *regs)
{
    bdk_uart_rx_t *rx = uart_rx_get(id);
    if (rx == NULL) {
        return;
    }

    while (READ_BIT(regs->SR, USART_SR_RXNE) != 0) {
        uart_rx_push(id, (uint8_t)regs->DR);
    }

    /* If USART_SR_RXNE == 0, and USART_SR_ORE == 1 overrun, 
    *  We need to clear ORE
    */
    if (READ_BIT(regs->SR, USART_SR_ORE) != 0) {
        rx->ore_count++;
        /*Clear ORE*/

        (void)regs->SR;
        (void)regs->DR;
    }

    /*RXNE can be set while still in this IRQ run*/
    while (READ_BIT(regs->SR, USART_SR_RXNE) != 0) {
        uart_rx_push(id, (uint8_t)regs->DR);
    }
}

static void uart_tx_isr(bdk_uart_id_t id, USART_TypeDef *regs)
{
    bdk_uart_tx_t *tx = uart_tx_get(id);
    if (tx == NULL) {
        return;
    }

    if (tx->active != 0) {
        while (READ_BIT(regs->SR, USART_SR_TXE) != 0 && tx->idx < tx->len) {
            regs->DR = tx->data[tx->idx++];
        }
        if (tx->idx >= tx->len) {
            tx->active = 0;
            CLEAR_BIT(regs->CR1, USART_CR1_TXEIE);
        }
    }
}


static bdk_uart_tx_t *uart_tx_get(bdk_uart_id_t id)
{
    if ((unsigned)id >= BDK_ARRAY_LEN(uart_tx)) {
        return NULL;
    }

    return &uart_tx[id];
}

static void uart_tx_reset(bdk_uart_id_t id)
{
    bdk_uart_tx_t *tx = uart_tx_get(id);
    if (tx == NULL) {
        return;
    }

    tx->data = NULL;
    tx->len  = 0;
    tx->idx  = 0;
    tx->active = 0;
}

static USART_TypeDef *usart_regs(bdk_uart_id_t id)
{
    if ((unsigned) id >= (unsigned) BDK_ARRAY_LEN(uart_table)) {
        return NULL;
    }
    return uart_table[id];
}

static uint32_t get_sysclk(void)
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
    if (data == NULL && len > 0) {
        return;
    }

    USART_TypeDef *regs = usart_regs(id);
    if (regs == NULL) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        bdk_uart_write_byte(id, data[i]);
    }
}

void bdk_uart_read(bdk_uart_id_t id, uint8_t *data, size_t len)
{
    USART_TypeDef *regs = usart_regs(id);
    if (regs == NULL || (len > 0 && data == NULL)) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        data[i] = bdk_uart_read_byte(id);
    }
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

bdk_status_t bdk_uart_irq_enable(bdk_uart_id_t id)
{
    USART_TypeDef *regs = usart_regs(id);
    if (regs == NULL) {
        return BDK_ERR_PARAM;
    }

    IRQn_Type irqn = uart_irqn(id);
    if ((int)irqn < 0) {
        return BDK_ERR_PARAM;
    }

    uart_rx_reset(id);
    uart_tx_reset(id);
    NVIC_SetPriority(irqn, 5);
    NVIC_EnableIRQ(irqn);

    SET_BIT(regs->CR1, USART_CR1_RXNEIE);

    return BDK_OK;
}

bdk_status_t bdk_uart_poll_in(bdk_uart_id_t id, uint8_t *byte)
{
    if (byte == NULL || (unsigned)id >= BDK_ARRAY_LEN(uart_table)) {
        return BDK_ERR_PARAM;
    }

    return uart_rx_pop(id, byte);
}

void bdk_uart_irq_handler(bdk_uart_id_t id)
{
    USART_TypeDef *regs = usart_regs(id);
    if (regs == NULL) {
        return;
    }

    uart_rx_isr(id, regs);
    uart_tx_isr(id, regs);
}

bdk_status_t bdk_uart_write_async(bdk_uart_id_t id, const uint8_t *data,
                                  size_t len)
{
    USART_TypeDef *regs = usart_regs(id);
    bdk_uart_tx_t *tx   = uart_tx_get(id);

    if (regs == NULL || tx == NULL) {
        return BDK_ERR_PARAM;
    }
    if (data == NULL || len == 0) {
        return BDK_ERR_PARAM;
    }
    if (tx->active != 0) {
        return BDK_ERR_BUSY;
    }

    tx->data = data;
    tx->len  = len;
    tx->idx  = 0;
    tx->active = 1;

    SET_BIT(regs->CR1, USART_CR1_TXEIE);
    return BDK_OK;
}

int bdk_uart_tx_active(bdk_uart_id_t id)
{
    bdk_uart_tx_t *tx = uart_tx_get(id);
    if (tx == NULL) {
        return 0;
    }

    return tx->active != 0;
}

bdk_status_t bdk_uart_rx_stats_get(bdk_uart_id_t id, bdk_uart_rx_stats_t *stats)
{
    bdk_uart_rx_t *rx = uart_rx_get(id);

    if (stats == NULL || rx == NULL) {
        return BDK_ERR_PARAM;
    }

    stats->ore_count       = rx->ore_count;
    stats->ring_drop_count = rx->ring_drop_count;

    return BDK_OK;
}

bdk_status_t bdk_uart_rx_stats_reset(bdk_uart_id_t id)
{
    bdk_uart_rx_t *rx = uart_rx_get(id);

    if (rx == NULL) {
        return BDK_ERR_PARAM;
    }

    rx->ore_count       = 0;
    rx->ring_drop_count = 0;

    return BDK_OK;
}
