/**
 * @file main.c
 * @brief I2C address scan on USART2 (F407 Discovery lab wiring).
 *
 * Requires a working @ref bdk_i2c_init and @ref bdk_i2c_write with @p len == 0
 * (address + ACK probe, no data bytes). Implement in src/bdk_i2c.c from RM0090.
 */

#include "stm32f4xx.h"

#include "bdk_gpio.h"
#include "bdk_i2c.h"
#include "bdk_status.h"
#include "bdk_uart.h"

/* F407 Discovery: I2C1 on PB6 (SCL), PB9 (SDA). Change if your bus uses other pins. */
#define LAB_I2C         BDK_I2C_1
#define LAB_I2C_AF      BDK_GPIO_AF_I2C1
#define LAB_I2C_PORT    BDK_GPIO_PORT_B
#define LAB_I2C_PIN_SCL 6U
#define LAB_I2C_PIN_SDA 9U
#define LAB_I2C_HZ      BDK_I2C_SPEED_100KHZ

#define I2C_ADDR_MIN 0x08U
#define I2C_ADDR_MAX 0x77U

static void uart_puts(bdk_uart_id_t id, const char *s)
{
    while (*s != '\0') {
        bdk_uart_write_byte(id, (uint8_t)*s++);
    }
}

static void uart_put_hex8(bdk_uart_id_t id, uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";

    bdk_uart_write_byte(id, (uint8_t)hex[(v >> 4) & 0x0FU]);
    bdk_uart_write_byte(id, (uint8_t)hex[v & 0x0FU]);
}

static void uart_put_hex32(bdk_uart_id_t id, uint32_t v)
{
    uart_puts(id, "0x");
    uart_put_hex8(id, (uint8_t)(v >> 24));
    uart_put_hex8(id, (uint8_t)(v >> 16));
    uart_put_hex8(id, (uint8_t)(v >> 8));
    uart_put_hex8(id, (uint8_t)v);
}

static void uart_put_reg(bdk_uart_id_t id, const char *name, uint32_t value)
{
    uart_puts(id, "  ");
    uart_puts(id, name);
    uart_puts(id, " ");
    uart_put_hex32(id, value);
    uart_puts(id, "\r\n");
}

/** Snapshot I2C1 + GPIOB + RCC (why SCL may stay flat). */
static void dump_i2c1_hw(bdk_uart_id_t id)
{
    uart_puts(id, "regs:\r\n");
    uart_put_reg(id, "I2C1 CR1  ", I2C1->CR1);
    uart_put_reg(id, "I2C1 CR2  ", I2C1->CR2);
    uart_put_reg(id, "I2C1 SR1  ", I2C1->SR1);
    uart_put_reg(id, "I2C1 SR2  ", I2C1->SR2);
    uart_put_reg(id, "I2C1 CCR  ", I2C1->CCR);
    uart_put_reg(id, "I2C1 TRISE", I2C1->TRISE);
    uart_put_reg(id, "GPIOB MODER ", GPIOB->MODER);
    uart_put_reg(id, "GPIOB OTYPER", GPIOB->OTYPER);
    uart_put_reg(id, "GPIOB PUPDR ", GPIOB->PUPDR);
    uart_put_reg(id, "GPIOB IDR   ", GPIOB->IDR);
    uart_put_reg(id, "GPIOB AFR[0]", GPIOB->AFR[0]);
    uart_put_reg(id, "GPIOB AFR[1]", GPIOB->AFR[1]);
    uart_put_reg(id, "RCC AHB1ENR ", RCC->AHB1ENR);
    uart_put_reg(id, "RCC APB1ENR ", RCC->APB1ENR);
}

static void uart2_console_init(void)
{
    bdk_gpio_config_t tx = {
        .port  = BDK_GPIO_PORT_A,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_USART2,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
        .pin   = 2,
    };
    bdk_gpio_config_t rx = {
        .port  = BDK_GPIO_PORT_A,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = BDK_GPIO_AF_USART2,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
        .pin   = 3,
    };

    BDK_ASSERT_OK(bdk_gpio_init(&tx));
    BDK_ASSERT_OK(bdk_gpio_init(&rx));

    bdk_uart_config_t uart = {
        .id     = BDK_UART_2,
        .baud   = BDK_UART_BAUD_115200,
        .parity = BDK_UART_PARITY_NONE,
        .stop   = BDK_UART_STOP_1,
        .word   = BDK_UART_WORD_8,
    };
    BDK_ASSERT_OK(bdk_uart_init(&uart));
}

static void lab_i2c1_gpio_init(void)
{
    bdk_gpio_config_t scl = {
        .port  = LAB_I2C_PORT,
        .pin   = LAB_I2C_PIN_SCL,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = LAB_I2C_AF,
        .otype = BDK_GPIO_OTYPE_OD,
        .pull  = BDK_GPIO_PULL_UP,
        .speed = BDK_GPIO_SPEED_HIGH,
    };
    bdk_gpio_config_t sda = scl;

    sda.pin = LAB_I2C_PIN_SDA;

    BDK_ASSERT_OK(bdk_gpio_init(&scl));
    BDK_ASSERT_OK(bdk_gpio_init(&sda));
}

static void lab_i2c_init(void)
{
    bdk_i2c_config_t cfg = {
        .id       = LAB_I2C,
        .speed_hz = LAB_I2C_HZ,
    };

    BDK_ASSERT_OK(bdk_i2c_init(&cfg));
}

/** Zero-length write: START, 7-bit addr (write), ACK/NACK, STOP. */
static int i2c_probe_7bit(bdk_i2c_id_t id, uint8_t addr_7bit)
{
    return bdk_i2c_write(id, addr_7bit, NULL, 0) == BDK_OK ? 1 : 0;
}

static void scan_bus_once(bdk_uart_id_t log, bdk_i2c_id_t bus)
{
    unsigned found = 0;

    uart_puts(log, "I2C scan 0x");
    uart_put_hex8(log, I2C_ADDR_MIN);
    uart_puts(log, "..0x");
    uart_put_hex8(log, I2C_ADDR_MAX);
    uart_puts(log, " @ ");
    if (LAB_I2C_HZ >= 1000000U) {
        uart_puts(log, "1MHz+\r\n");
    } else if (LAB_I2C_HZ >= 400000U) {
        uart_puts(log, "400kHz\r\n");
    } else {
        uart_puts(log, "100kHz\r\n");
    }

    for (uint8_t addr = I2C_ADDR_MIN; addr <= I2C_ADDR_MAX; addr++) {
        if (i2c_probe_7bit(bus, addr) != 0) {
            uart_puts(log, "  0x");
            uart_put_hex8(log, addr);
            uart_puts(log, "\r\n");
            found++;
        }
    }

    if (found == 0U) {
        uart_puts(log, "  (no ACK)\r\n");
    }
    uart_puts(log, "done.\r\n\r\n");
}

int main(void)
{
    uart2_console_init();
    lab_i2c1_gpio_init();
    lab_i2c_init();

    uart_puts(BDK_UART_2, "i2c_scanner: PB6=SCL PB9=SDA I2C1\r\n");
    dump_i2c1_hw(BDK_UART_2);

    {
        bdk_status_t st = bdk_i2c_write(LAB_I2C, 0x40, NULL, 0);
        uart_puts(BDK_UART_2, "probe 0x40 ");
        uart_puts(BDK_UART_2, bdk_status_str(st));
        uart_puts(BDK_UART_2, "\r\n");
        dump_i2c1_hw(BDK_UART_2);
    }

    /*
    for (;;) {
        scan_bus_once(BDK_UART_2, LAB_I2C);
        for (volatile int i = 0; i < 8000000; i++) {
        }
    }
    */
}
